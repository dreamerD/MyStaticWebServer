#include "httpconn.h"

HttpConn::HttpConn() {
  fd = -1;
  addr = {0};
}

void HttpConn::Init(int fd, const sockaddr_in& addr) {
  this->fd = fd;
  this->addr = addr;
  // 读buffer初始化
  // 写buffer初始化
  printf("Client[%d](%s:%d) in, userCount:%d", fd, GetIP(), GetPort(),
         (int)userCount);
}

void HttpConn::Close() {
  response.UnmapFile();
  userCount--;
  close(fd);
  printf("Client[%d](%s:%d) quit, UserCount:%d", fd, GetIP(), GetPort(),
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
  request.Init();
  if (readBuff.writePos - readBuff.readPos <= 0) {
    return 0;
  }
  HTTP_CODE res = request.Parse(readBuff);
  if (res == NO_REQUEST) {
    return 0;
  } else if (res == BAD_REQUEST) {
    response.Init(srcDir, request.Path(), false, 400);

  } else {
    printf("%s", request.Path().c_str());
    response.Init(srcDir, request.Path(), request.IsKeepAlive(), 200);
  }

  response.MakeResponse(writeBuff);
  /* 响应头 */
  iov[0].iov_base = writeBuff.ReadBeginPos();
  iov[0].iov_len = writeBuff.writePos - writeBuff.readPos;
  iovCnt = 1;

  /* 文件 */
  if (response.FileLen() > 0 && response.File()) {
    iov[1].iov_base = response.File();
    iov[1].iov_len = response.FileLen();
    iovCnt = 2;
  }
  printf("filesize:%d, %d  to %d", response.FileLen(), iovCnt,
         iov[0].iov_len + iov[1].iov_len);
  return 1;
}

int HttpConn::ToWriteBytes() { return iov[0].iov_len + iov[1].iov_len; }