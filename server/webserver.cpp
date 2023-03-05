#include "webserver.h"
// https://blog.csdn.net/mijichui2153/article/details/81004874
// https://xie.infoq.cn/article/8e95ff62ee9d5a2ce97492323
// https://zhuanlan.zhihu.com/p/107995399
WebServer::WebServer(int port, int trigMode, int timeoutMS, bool optLinger,
                     int sqlPort, const char* sqlUser, const char* sqlPwd,
                     const char* dbName, int connPoolNum, int threadNum,
                     bool openLog, int logLevel, int logQueSize)
    : port(port),
      openLinger(optLinger),
      isclose(false),
      timeoutMS(timeoutMS),
      timer(new Timer()),
      threadpool(new ThreadPool()),
      epoller(new Epoller()) {
  if (openLog) {
    // Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
  }
  srcDir = getcwd(nullptr, 256);
  const char* res = "/resources/";
  strncat(srcDir, res, 256);
  //   HttpConn::userCount = 0;
  //   HttpConn::srcDir = srcDir_;
  //   SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd,
  //   dbName,
  //   connPoolNum);

  initEventMode();
  initListenSocket();
}
void WebServer::Start() {
  // 之前的项目中存在线程池线程对资源的释放，存在主线程对资源的释放
  // 修改后的代码后资源的释放应该全部交给主线程
  // 考虑增加字段以决定是否因超时而关闭连接
  int timeMS = -1;
  printf("Server start!\n");
  while (!isclose) {
    if (timeoutMS > 0) {
      // timeMS =timer->GetNextTick();
    }
    int ret = epoller->Wait(timeMS);
    for (int i = 0; i < ret; i++) {
      int fd = epoller->GetEventFd(i);
      uint32_t event = epoller->GetEvent(i);
      if (fd == listenfd) {
        dealListen();
      } else if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        closeConn(fd);
      } else if (event & EPOLLIN) {
        dealRead(fd);
      } else if (event & EPOLLOUT) {
        dealWrite(fd);
      } else {
        printf("unexpected error!\n");
      }
    }
  }
}

void WebServer::dealListen() {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  // listenEvent & EPOLLET
  int fd;
  while (fd = accept(listenfd, (sockaddr*)&addr, &len) > 0) {
    if (1) {  // 超人数
      sendBusyMsg(fd, "server busy!");
      printf("client user is max\n");
    } else {
      addClient(fd, addr);
    }
  }
}
void WebServer::sendBusyMsg(int fd, const char* msg) {
  int ret = send(fd, msg, strlen(msg), 0);
  if (ret == -1) {
    printf("send busy msg to fd[%d] error!\n", fd);
  } else {
    printf("send busy msg to fd[%d] success!\n", fd);
  }
  close(fd);
  return;
}

void WebServer::addClient(int fd, sockaddr_in addr) {
  // 1.添加到users
  // 2.添加timer
  // 应该是向main线程发送信号，然后删除
  timer->Add(fd, timeoutMS, std::bind(WebServer::closeConn, this, fd));
  // 3.
  epoller->AddFd(fd, connEvent | EPOLLIN);
  setFdNonBlock(fd);
  printf("client[%d] add success!\n", fd);
}

void WebServer::closeConn(int fd) {
  HttpConn& client = users[fd];
  //  LOG_INFO("Client[%d] quit!", client->GetFd());
  printf("Client[%d] quit!", fd);
  epoller->DeleteFd(fd);
  client.Close();
}
void WebServer::dealRead(int fd) {
  timer->Adjust(fd, timeoutMS);  // 调整定时器
  threadpool->AddTask(std::bind(&WebServer::read, this, fd));
}
void WebServer::dealWrite(int fd) {
  timer->Adjust(fd, timeoutMS);  // 调整定时器
  threadpool->AddTask(std::bind(&WebServer::write, this, fd));
}

void WebServer::initEventMode() {
  listenEvent = EPOLLRDHUP | EPOLLET;
  connEvent = EPOLLRDHUP | EPOLLONESHOT | EPOLLET;
  //   HttpConn::isET = (connEvent_ & EPOLLET);
}

// 建立监听套接字
void WebServer::initListenSocket() {
  int ret;
  struct sockaddr_in addr;
  if (port > 65535 || port < 1024) {
    printf("port: %d error!", port);
    exit(port);
  }
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    printf("create listenfd %d error!", listenfd);
    exit(listenfd);
  }
  int optval = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    printf("setsocketopt error! ret=%d", ret);
    exit(ret);
  }
  if (openLinger) {
    // 优雅关闭
    struct linger optLinger;
    optLinger.l_linger = 1;
    optLinger.l_onoff = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &optLinger,
                   sizeof(optLinger)) < 0) {
      printf("setsockopt linger error! ret=%d", ret);
      exit(port);
    }
  }
  // listenfd 监听地址
  addr.sin_family = AF_INET;
  // 大小端转换
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  // https://www.cnblogs.com/klxs1996/p/12809044.html
  // bind函数用于将套接字与指定端口相连
  if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    printf("bind error!%d", ret);
    exit(ret);
  }
  // https://blog.csdn.net/seu_lyr/article/details/9427655

  if (listen(listenfd, 5) < 0) {
    printf("bind error!%d", ret);
    close(listenfd);  // 关闭监听套接字
    exit(ret);
  }
  // 设置非阻塞
  setFdNonBlock(listenfd);

  if (epoller->AddFd(listenfd, listenEvent | EPOLLIN) < 0) {
    printf("epoll add listen fd error !%d", ret);
    exit(-1);
  }
  printf("port[%d] init success!\n");
}

void WebServer::setFdNonBlock(int fd) {
  int flag = fcntl(fd, F_GETFD);
  fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

void WebServer::read(int fd) {
  HttpConn& client = users[fd];
  int ret = -1;
  int readErrno = 0;
  ret = client.read(&readErrno);
  if (ret <= 0 && readErrno != EAGAIN) {
    closeConn(fd);
    return;
  }
  process(fd);
}

void WebServer::write(int fd) {
  HttpConn& client = users[fd];
  int ret = -1;
  int writeErrno = 0;
  ret = client.write(&writeErrno);
  if (client.ToWriteBytes() == 0) {
    /* 传输完成 */
    if (client.IsKeepAlive()) {
      process(fd);
      return;
    }
  } else if (ret < 0) {
    if (writeErrno == EAGAIN) {
      /* 继续传输 */
      epoller->ModFd(fd, connEvent | EPOLLOUT);
      return;
    }
  }
  closeConn(fd);
}

void WebServer::process(int fd) {
  HttpConn& client = users[fd];
  if (client.process()) {
    epoller->ModFd(fd, connEvent | EPOLLOUT);
  } else {
    epoller->ModFd(fd, connEvent | EPOLLIN);
  }
}
WebServer::~WebServer() {
  close(listenfd);
  isclose = true;
  free(srcDir);
  // SqlConnPool::Instance()->ClosePool();
}