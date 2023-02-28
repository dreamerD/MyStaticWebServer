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
      timeout(timeoutMS),
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
  void WebServer::Start() {}
  void WebServer::initEventMode() {
    listenEvent = EPOLLRDHUP;
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
    int ret =
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret < 0) {
      printf("setsocketopt error! ret=%d", ret);
      exit(ret);
    }
    if (openLinger) {
      // 优雅关闭
      struct linger optLinger;
      optLinger.l_linger = 1;
      optLinger.l_onoff = 1;
      int ret = setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &optLinger,
                           sizeof(optLinger));
      if (ret < 0) {
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
    ret = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
      printf("bind error!%d", ret);
      exit(ret);
    }
    // https://blog.csdn.net/seu_lyr/article/details/9427655
    ret = listen(listenfd, 5);
    if (ret < 0) {
      printf("bind error!%d", ret);
      close(listenfd);  // 关闭监听套接字
      exit(ret);
    }
    // 添加到epoll
    // 。。。

    // 设置非阻塞
    setFdNonBlock(listenfd);
    printf("port[%d] init success!\n");
  }
  void WebServer::setFdNonBlock(int fd) {
    int flag = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
  }
  WebServer::~WebServer() {
    close(listenfd);
    isclose = true;
    free(srcDir);
    // SqlConnPool::Instance()->ClosePool();
  }