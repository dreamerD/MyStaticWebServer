#ifndef LOG_H
#define LOG_H
#include <memory>
#include <mutex>
#include <thread>

#include "blockqueue.h"
#include "buffer/buffer.h"
class Log {
 public:
  void Init(int level, const char* path = "./log", const char* suffix = ".log",
            int maxQueueCapacity = 1024);
  static Log* Instance();
  static void FlushLogThread();

  void write(int level, const char* format, ...);
  void flush();

  int GetLevel();
  void SetLevel(int level);
  bool IsOpen() const { return isOpen; }

 private:
  Log();
  void appendLogLevelTitle(int level);
  virtual ~Log();
  void asyncWrite();

 private:
  static const int LOG_PATH_LEN = 256;
  static const int LOG_NAME_LEN = 256;
  static const int MAX_LINES = 50000;

  const char* path;
  const char* suffix;

  int maxLines;

  int lineCount;
  int toDay;

  bool isOpen;

  Buffer buff;
  int level;
  bool isAsync;

  FILE* fp;
  std::unique_ptr<BlockDeque<std::string>> deque;
  std::unique_ptr<std::thread> writeThread;
  std::mutex mtx;
};
#endif