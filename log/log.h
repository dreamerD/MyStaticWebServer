#ifndef LOG_H
#define LOG_H
#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>

#include <memory>
#include <mutex>
#include <thread>

#include "../buffer/buffer.h"
#include "blockqueue.h"
class Log {
 public:
  void Init(int level, const char* path = "./log", const char* suffix = ".log",
            int maxQueueCapacity = 1024);
  static Log* Instance();
  static void FlushLogThread();

  void Write(int level, const char* format, ...);
  void Flush();

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
  std::mutex mtxWrite;
};

#define LOG_BASE(level, format, ...)                 \
  do {                                               \
    Log* log = Log::Instance();                      \
    if (log->IsOpen() && log->GetLevel() <= level) { \
      log->Write(level, format, ##__VA_ARGS__);      \
    }                                                \
  } while (0);

#define LOG_DEBUG(format, ...)         \
  do {                                 \
    LOG_BASE(0, format, ##__VA_ARGS__) \
  } while (0);
#define LOG_INFO(format, ...)          \
  do {                                 \
    LOG_BASE(1, format, ##__VA_ARGS__) \
  } while (0);
#define LOG_WARN(format, ...)          \
  do {                                 \
    LOG_BASE(2, format, ##__VA_ARGS__) \
  } while (0);
#define LOG_ERROR(format, ...)         \
  do {                                 \
    LOG_BASE(3, format, ##__VA_ARGS__) \
  } while (0);
#endif