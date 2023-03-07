#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>  // size_t
#include <sys/uio.h>
#include <unistd.h>  // close()

#include <atomic>

#include "buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
 public:
  HttpConn();
  ~HttpConn();
  void Init(int fd, const sockaddr_in& addr);
  void Close();
  ssize_t Read(int& saveErrno);
  ssize_t Write(int& saveErrno);
  bool Process();
  bool IsKeepAlive() const { return request.IsKeepAlive(); }
  int ToWriteBytes();
  int GetPort() const;
  int HttpConn::GetFd() const;

 private:
  const char* GetIP() const;
  int GetPort() const;
  struct sockaddr_in HttpConn::GetAddr() const;

 public:
  static std::atomic<int> userCount;  // 原子操作
  static char* srcDir;

 private:
  int fd;
  struct sockaddr_in addr;
  int iovCnt;
  struct iovec iov[2];
  Buffer readBuff;
  Buffer writeBuff;
  HttpRequest request;
  HttpResponse response;
};
#endif