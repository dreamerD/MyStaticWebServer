#include "webserver.h"
// https://blog.csdn.net/mijichui2153/article/details/81004874
// https://xie.infoq.cn/article/8e95ff62ee9d5a2ce97492323
// https://zhuanlan.zhihu.com/p/107995399
WebServer::WebServer(int port, int trigMode, int timeoutMS, bool optLinger,
                     int threadNum, bool openLog, int logLevel, int logQueSize)
    : port(port),
      openLinger(optLinger),
      isclose(false),
      timeoutMS(timeoutMS),
      timer(new HeapTimer()),
      threadpool(new ThreadPool(8)),
      epoller(new Epoller(1024)) {
  if (true) {
    Log::Instance()->Init(logLevel, "./log", ".log", logQueSize);
  }
  srcDir = getcwd(nullptr, 256);
  const char* res = "/resources/";
  strncat(srcDir, res, 256);
  HttpConn::userCount = 0;
  HttpConn::srcDir = srcDir;

  initEventMode();
  initListenSocket();
}
void WebServer::Start() {
  int timeMS = -1;
  printf("Server start!\n");
  while (!isclose) {
    if (timeoutMS > 0) {
      timeMS = timer->GetNextTick();
    }
    int ret = epoller->Wait(timeMS);
    for (int i = 0; i < ret; i++) {
      int fd = epoller->GetEventFd(i);
      uint32_t event = epoller->GetEvent(i);
      printf("循环,fd=%d", fd);
      if (fd == listenfd) {
        dealListen();
      } else if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        closeConn(&users[fd]);
      } else if (event & EPOLLIN) {
        dealRead(fd);
      } else if (event & EPOLLOUT) {
        dealWrite(fd);
      } else {
        LOG_INFO("unexpected error!\n");
      }
    }
  }
}

void WebServer::dealListen() {
  printf("deal listen\n");
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  // listenEvent & EPOLLET
  int fd;
  while ((fd = accept(listenfd, (sockaddr*)&addr, &len)) > 0) {
    printf("deal listen fd[%d]\n", fd);
    if (HttpConn::userCount >= MAX_FD) {  // 超人数
      sendBusyMsg(fd, "server busy!");
      LOG_INFO("client user is max");
    } else {
      addClient(fd, addr);
    }
  }
}
void WebServer::sendBusyMsg(int fd, const char* msg) {
  int ret = send(fd, msg, strlen(msg), 0);
  if (ret == -1) {
    LOG_INFO("send busy msg to fd[%d] error!\n", fd);
  } else {
    LOG_INFO("send busy msg to fd[%d] success!\n", fd);
  }
  close(fd);
  return;
}

void WebServer::addClient(int fd, sockaddr_in addr) {
  printf("add Client\n");
  // 1.添加到users
  users[fd].Init(fd, addr);
  // 2.添加timer
  timer->Add(fd, timeoutMS,
             std::bind(&WebServer::closeConn, this, &this->users[fd]));
  // 3.
  setFdNonBlock(fd);
  // 4.
  epoller->AddFd(fd, connEvent | EPOLLIN);
  LOG_INFO("client[%d] add success!\n", fd);
}

void WebServer::closeConn(HttpConn* client) {
  printf("end2\n");
  if (client->running.load() == true) {
    printf("end3\n");
    return;
  }
  printf("end4\n");
  if (client->valid.test_and_set()) {
    printf("end5\n");
    return;
  }
  printf("end6\n");
  printf("Client[%d] quit!\n", client->GetFd());
  printf("end7\n");
  client->Close();
  printf("end8\n");
  epoller->DeleteFd(client->GetFd());
  printf("end9\n");
}

void WebServer::dealRead(int fd) {
  timer->Adjust(fd, timeoutMS);  // 调整定时器
  // 避免在子进程中访问全局变量导致race
  threadpool->AddTask(std::bind(&WebServer::read, this, &this->users[fd], fd));
}

void WebServer::dealWrite(int fd) {
  printf("WebServer::dealWrite[%d]", fd);
  timer->Adjust(fd, timeoutMS);  // 调整定时器
  threadpool->AddTask(std::bind(&WebServer::write, this, &this->users[fd], fd));
}

void WebServer::initEventMode() {
  listenEvent = EPOLLRDHUP | EPOLLET;
  connEvent = EPOLLRDHUP | EPOLLONESHOT | EPOLLET;
}

// 建立监听套接字
void WebServer::initListenSocket() {
  int ret;
  struct sockaddr_in addr;
  if (port > 65535 || port < 1024) {
    LOG_ERROR("port: %d error!", port);
    exit(port);
  }
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    LOG_ERROR("create listenfd %d error!", listenfd);
    exit(listenfd);
  }
  int optval = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    LOG_ERROR("setsocketopt error! ret=%d", ret);
    exit(ret);
  }
  if (openLinger) {
    // 优雅关闭
    struct linger optLinger;
    optLinger.l_linger = 1;
    optLinger.l_onoff = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &optLinger,
                   sizeof(optLinger)) < 0) {
      LOG_ERROR("setsockopt linger error! ret=%d", ret);
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
    LOG_ERROR("bind error!");
    exit(-1);
  }
  // https://blog.csdn.net/seu_lyr/article/details/9427655

  if (listen(listenfd, 5) < 0) {
    LOG_ERROR("bind error!");
    close(listenfd);  // 关闭监听套接字
    exit(-1);
  }
  // 设置非阻塞
  setFdNonBlock(listenfd);

  if (!epoller->AddFd(listenfd, listenEvent | EPOLLIN)) {
    LOG_ERROR("epoll add listen fd error !");
    exit(-1);
  }
  LOG_INFO("port[%d] init success!\n");
}

void WebServer::setFdNonBlock(int fd) {
  int flag = fcntl(fd, F_GETFD);
  fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

void WebServer::read(HttpConn* client, int fd) {
  printf("sub thread read\n");
  int ret = -1;
  int readErrno = 0;
  ret = client->Read(readErrno);
  if (ret > 0 || readErrno == EINTR || readErrno == EWOULDBLOCK ||
      readErrno == EAGAIN) {
    process(client, fd);
    printf("sub thread process\n");
  } else {
    printf("sub thread close\n");
    client->running.store(false);
    closeConn(client);
  }
}

void WebServer::write(HttpConn* client, int fd) {
  printf("sub thread write\n");
  int ret = -1;
  int writeErrno = 0;
  ret = client->Write(writeErrno);
  if (client->ToWriteBytes() == 0) {
    /* 传输完成 */
    if (client->IsKeepAlive()) {
      epoller->ModFd(fd, connEvent | EPOLLIN);
      return;
    }
  } else if (ret < 0) {
    if (writeErrno == EINTR || writeErrno == EWOULDBLOCK ||
        writeErrno == EAGAIN) {
      /* 继续传输 */
      epoller->ModFd(fd, connEvent | EPOLLOUT);
      return;
    }
  }
  printf("end0\n");
  client->running.store(false);
  printf("end1\n");
  closeConn(client);
}

void WebServer::process(HttpConn* client, int fd) {
  // HttpConn& client = users[fd];
  if (client->Process()) {  // 分析失败或成功
    epoller->ModFd(fd, connEvent | EPOLLOUT);
    printf("process\n");
  } else {  // 分析不完整
    epoller->ModFd(fd, connEvent | EPOLLIN);
  }
}
WebServer::~WebServer() {
  close(listenfd);
  isclose = true;
  free(srcDir);
}

void ListenAndServe(int port, int trigMode, int timeout, bool optLinger,
                    int threadNum, bool openLog, int logLevel, int logQueSize) {
  WebServer server(port, trigMode, timeout, optLinger, threadNum, openLog,
                   logLevel, logQueSize);
  server.Start();
}