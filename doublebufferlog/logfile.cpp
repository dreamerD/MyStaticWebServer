#include "logfile.h"
LogFile::LogFile(int flushInterval, long long maxLogFileSize)
    : flushInterval(flushInterval),
      maxLogFileSize(maxLogFileSize),
      writtenBytes(0),
      curDay(0),
      lastFlush(0),
      fileId(0) {
  newLogFile();
}

void LogFile::Append(const char* msg, size_t len) {
  if (writtenBytes + static_cast<long long>(len) <= maxLogFileSize) {
    writeToFile(msg, len);
  } else {
    newLogFile();
    writeToFile(msg, len);
  }
  time_t now;
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  now = tv.tv_sec;
  if (now - lastFlush >= static_cast<long long>(flushInterval)) {
    lastFlush = now;
    Flush();
  }
}

void LogFile::writeToFile(const char* msg, size_t len) {
  size_t written = 0;
  while (written < len) {
    size_t cnt = fwrite(msg + written, sizeof(char), len - written, logFile);
    if (cnt == 0) {
      int err = ferror(logFile);
      if (err) {
        fprintf(stderr,
                "LogFile::Append(const char* msg, size_t len) failed %s \n",
                strerror(err));
      }
      break;
    }
    written += cnt;
  }
  writtenBytes += static_cast<long long>(len);
}
void LogFile::newLogFile() {
  if (logFile) {
    Flush();  // 清空前一个文件的缓冲区
    fclose(logFile);
    logFile = nullptr;
  }
  std::string fileName = newLogFileName();
  fileName += ".log";
  logFile = fopen(fileName.c_str(), "a+");
}

std::string LogFile::newLogFileName() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  time_t tSec = tv.tv_sec;
  struct tm t = *(localtime(&tSec));
  char str[36];
  snprintf(str, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1,
           t.tm_mday);
  if (t.tm_mday > curDay) {
    fileId = 0;
    curDay = t.tm_mday;
  } else {
    fileId++;
  }
  return std::string(str, str + strlen(str)) + "_" + std::to_string(fileId);
}

void LogFile::Flush() { fflush(logFile); }