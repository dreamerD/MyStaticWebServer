#include "httpconn.h"
char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
HttpConn::HttpConn(){};
std::unordered_map<std::string,
                   std::function<void(HttpRequest*, HttpResponse*)>>
    Engine::router;
void HttpConn::Init(int fd, const sockaddr_in& addr) {
  this->fd = fd;
  userCount++;
  this->addr = addr;
  this->running.store(true);
  this->valid.clear();
  // 读buffer初始化
  // 写buffer初始化
  this->writeBuff.Init();
  this->readBuff.Init();
  printf("Client[%d](%s:%d) in, userCount:%d\n", fd, GetIP(), GetPort(),
         (int)userCount);
}

void HttpConn::Close() {
  response.UnmapFile();
  userCount--;
  close(fd);
  LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd, GetIP(), GetPort(),
           (int)userCount);
}
int HttpConn::GetFd() const { return fd; };

struct sockaddr_in HttpConn::GetAddr() const { return addr; }

const char* HttpConn::GetIP() const { return inet_ntoa(addr.sin_addr); }

int HttpConn::GetPort() const { return addr.sin_port; }

// https://zhuanlan.zhihu.com/p/154265925
ssize_t HttpConn::Read(int& saveErrno) {
  ssize_t len = -1;
  do {
    len = readBuff.ReadFd(fd, saveErrno);
    if (len <= 0) {
      break;
    }
  } while (1);
  return len;
}

ssize_t HttpConn::Write(int& saveErrno) {
  ssize_t len = -1;
  do {
    len = writev(fd, iov, iovCnt);
    if (len < 0) {
      saveErrno = errno;
      break;
    }
    if (iov[0].iov_len + iov[1].iov_len == 0) {
      /* 传输结束 */
      break;
    } else if (static_cast<size_t>(len) > iov[0].iov_len) {
      // 传输的内容比iov[0]多
      iov[1].iov_base = (uint8_t*)iov[1].iov_base + (len - iov[0].iov_len);
      iov[1].iov_len -= (len - iov[0].iov_len);
      if (iov[0].iov_len) {
        writeBuff.Init();
        iov[0].iov_len = 0;
      }
    } else {
      // static_cast<size_t>(len) <= iov[0].iov_len
      iov[0].iov_base = (uint8_t*)iov[0].iov_base + len;
      iov[0].iov_len -= len;
      writeBuff.MoveReadPos(len);
    }
  } while (1);  // ET模式，一直写，直到写完
  return len;
}

bool HttpConn::Process() {
  printf("Process()0\n");
  request.Init();
  if (readBuff.writePos - readBuff.readPos <= 0) {
    printf("Process()1\n");
    return false;
  }
  HTTP_CODE res = request.Parse(readBuff);
  if (res == NO_REQUEST) {
    printf("Process()2\n");
    return false;
  } else if (res == BAD_REQUEST) {
    printf("Process()3\n");
    response.Init(srcDir, request.Path(), false, 400);

  } else {
    printf("%s", request.GetMethod().c_str());
    printf("request.Path().c_str()=%s", request.Path().c_str());
    // 执行ServeHTTP(HttpRequest*, HttpResponse*);
    /* 可以当作路由解析器，根据不同的request.Path().c_str()执行不同的func(HttpRequest*,
     * HttpResponse*) 然后在func中写入writebuf，ok，逻辑就是这样
     * 包装(HttpRequest*, HttpResponse*)实现上下文，完成中间件
     */
    response.Init(srcDir, request.Path(), request.IsKeepAlive(), 200);
    Engine::ServeHTTP(&request, &response);
  }
  response.addStateLine(writeBuff);
  response.addHeader(writeBuff);
  response.addContent(writeBuff);

  // response.MakeResponse(writeBuff);
  /* 响应头 */
  iov[0].iov_base = writeBuff.ReadBeginPos();
  iov[0].iov_len = writeBuff.writePos - writeBuff.readPos;
  iovCnt = 1;

  // /* 文件 */
  // if (response.FileLen() > 0 && response.File()) {
  //   iov[1].iov_base = response.File();
  //   iov[1].iov_len = response.FileLen();
  //   iovCnt = 2;
  //   LOG_INFO("filesize:%d, %d  to %d", response.FileLen(), iovCnt,
  //            iov[0].iov_len + iov[1].iov_len);
  // }
  return true;
}

int HttpConn::ToWriteBytes() { return iov[0].iov_len + iov[1].iov_len; }