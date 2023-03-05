#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <arpa/inet.h>
#include <epoller/epoll.h>
#include <fcntl.h>
#include <http/httpconn.h>
#include <netinet/in.h>
#include <pool/threadpool.h>
#include <stdint.h>  // uint32_t
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <timer/timer.h>
#include <unistd.h>

#include <memory>
#include <unordered_map>
struct WebServer {
  // member
 private:
  int port;         // 监听端口
  int listenfd;     // 监听套接字
  bool openLinger;  // 是否优雅关闭
  bool isclose;     // 是否关闭
  char* srcDir;     // 文件夹目录
  int timeoutMS;    // 定时器定时
  uint32_t listenEvent;
  uint32_t connEvent;
  std::unique_ptr<Timer> timer;
  std::unique_ptr<ThreadPool> threadpool;
  std::unique_ptr<Epoller> epoller;
  std::unordered_map<int, HttpConn> users;
  // function
 public:
  WebServer(int port, int trigMode, int timeout, bool optLinger, int sqlPort,
            const char* sqlUser, const char* sqlPwd, const char* dbName,
            int connPoolNum, int threadNum, bool openLog, int logLevel,
            int logQueSize);
  void Start();

 private:
  void initEventMode();
  void initListenSocket();
  void setFdNonBlock(int fd);
  void dealListen();
  void sendBusyMsg(int fd, const char* msg);
  void addClient(int fd, sockaddr_in addr);
  void closeConn(int fd);
  void dealRead(int fd);
  void dealWrite(int fd);

  void read(int fd);
  void write(int fd);
  void process(int fd);
};

#endif