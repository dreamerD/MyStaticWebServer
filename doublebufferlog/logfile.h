#ifndef LOG_FILE_H
#define LOG_FILE_H
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <string>
class LogFile {
 public:
  LogFile(int flushInterval = 3, long long FileSize = 1024 * 1024 * 1024);
  void Append(const char* msg, size_t len);

 private:
  void writeToFile(const char* msg, size_t len);
  void newLogFile();
  std::string newLogFileName();
  void Flush();

 private:
  int flushInterval;
  int fileId;                      // 当前文件编号
  const long long maxLogFileSize;  // 文件最大字节
  long long writtenBytes;          // 已经写入的字节数
  FILE* logFile;                   // 文件句柄
  int curDay;                      // 当前天数
  long long lastFlush;             // 上次刷新
};
#endif