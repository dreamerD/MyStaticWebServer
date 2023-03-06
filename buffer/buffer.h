#ifndef BUFFER_H
#define BUFFER_H
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <atomic>
#include <iostream>
#include <vector>
class Buffer {
 public:
  Buffer(size_t size = 1024);
  void Init();
  ssize_t ReadFd(int fd, int& Errno);
  char* ReadBeginPos();
  bool Readable();
  char GetChar(size_t pos);
  void MoveReadPos(const char* end);
  void Buffer::MoveReadPos(size_t len);
  void Append(const std::string& str);
  char* WriteBeginPos();
  size_t WriteAbleBytes();
  std::string GetAllReadableData();

 private:
  void append(const char* pos, size_t len);
  void allocmemory(size_t len);

 private:
  std::vector<char> buffer;

 public:
  size_t readPos;
  size_t writePos;
};
#endif