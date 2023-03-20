#ifndef LOG_H
#define LOG_H
#include <queue>
#include <string>
class LogBuf {
 public:
  LogBuf() : bufferSize(1024) {}
  bool isempty() { return false; }

 private:
  int bufferSize;
  std::queue<std::string> messages;
};
class Log {
 public:
  static Log& Instance();  // 单例模式
  void Init(int level, const char* path, const char* suffix, int maxQueueSize);
  void Write(int level, const char* format, ...);  // 前端
  void FlushLogThread();                           // 后端

 private:
 int fd;//日志文件文件描述符
 
};
#endif