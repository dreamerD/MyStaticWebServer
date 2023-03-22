#ifndef __DBLOG_H__
#define __DBLOG_H__
#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "logbuffer.h"
#include "logfile.h"
#define Buffer_Size 4096
class DBLog {
 public:
  static DBLog* Instance();

  void Init(int flushInterval = 3);
  void Start();
  void Stop();
  bool IsRunning() const { return isRunning.load(); }
  void Write(int level, const char* format, ...);

 private:
  DBLog(){};
  virtual ~DBLog();
  void threadFunc();
  void Append(const char* msg, size_t len);
  char* appendLogLevelTitle(int level, char* msg);
  LogFile logFile;
  using BufPtr = std::unique_ptr<LogBuffer<Buffer_Size>>;
  std::atomic<bool> isRunning;
  BufPtr curBuffer;
  BufPtr nextBuffer;
  std::vector<BufPtr> BufferVec;
  std::thread logThread;
  int flushInterval;

  std::mutex mtx;
  std::condition_variable cond;
};

#define LOG_BASE(level, format, ...)            \
  do {                                          \
    DBLog* log = DBLog::Instance();             \
    if (log->IsRunning()) {                     \
      log->Write(level, format, ##__VA_ARGS__); \
    }                                           \
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