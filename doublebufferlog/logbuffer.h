#ifndef __LOG_BUFFER_H__
#define __LOG_BUFFER_H__
#define MAX_LOG_BUFFER_SIZE 4096
#include <stdio.h>
#include <string.h>
template <size_t N = MAX_LOG_BUFFER_SIZE>
class LogBuffer {
 public:
  LogBuffer(){offset = 0};
  void Append(const char* msg, size_t len) {
    if (N - offset >= 0) {
      memcpy(buffer + offset, msg, len);
      offset += len;
    }
  }
  void Reset() { offset = 0; }
  char* CurrentPos() { return buffer + offset; }
  size_t BufferLen() const {return offset};
  size_t BufferSize() const {return N};
  size_t BufferAvail() const { return N - offset; }
  char* Data() const {return buffer}

 private:
  char buffer[N];
  size_t offset;
};
#endif