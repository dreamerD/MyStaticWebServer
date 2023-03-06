#include "buffer.h"

Buffer::Buffer(size_t size) : buffer(size), readPos(0), writePos(0) {}
void Buffer::Init() {
  bzero(&buffer[0], buffer.size());
  readPos = 0;
  writePos = 0;
}
ssize_t Buffer::ReadFd(int fd, int& saveErrno) {
  char buff[4096];
  struct iovec iov[2];
  size_t writable = buffer.size() - writePos;
  /* 分散读*/
  iov[0].iov_base = &(*buffer.begin()) + writePos;
  iov[0].iov_len = writable;
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);

  ssize_t len = readv(fd, iov, 2);
  if (len <= 0) {
    saveErrno = errno;
  } else if (static_cast<size_t>(len) <= writable) {
    writePos += len;
  } else {
    append(buff, len);
  }
  return len;
}

void Buffer::Append(const std::string& str) {
  this->append(str.data(), str.size());
}
void Buffer::append(const char* pos, size_t len) {
  if (len > buffer.size() - writePos) {
    allocmemory(len);
  }
  std::copy(pos, pos + len, &(*buffer.begin()) + writePos);
  writePos += len;
}

void Buffer::allocmemory(size_t len) {
  if (readPos + buffer.size() - writePos < len) {
    // 循环buffer剩余区域小于len
    buffer.resize(writePos + len + 1);
  } else {
    std::copy(&(*buffer.begin()) + readPos, &(*buffer.begin()) + writePos,
              &(*buffer.begin()));
    writePos -= readPos;
    readPos = 0;
  }
}

bool Buffer::Readable() { return writePos - readPos > 0; }

char* Buffer::ReadBeginPos() { return &*(readPos + buffer.begin()); }

char* Buffer::WriteBeginPos() { return &*(writePos + buffer.begin()); }

char Buffer::GetChar(size_t pos) { return buffer[pos]; }

void Buffer::MoveReadPos(const char* end) { readPos = end - &*buffer.begin(); }

void Buffer::MoveReadPos(size_t len) { readPos += len; }

size_t Buffer::WriteAbleBytes() { return buffer.size() - writePos; }

std::string Buffer::GetAllReadableData() {
  std::string str(ReadBeginPos(), WriteBeginPos());
  Init();
}