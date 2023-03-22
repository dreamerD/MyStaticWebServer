#include "dblog.h"

DBLog* DBLog::Instance() {
  static DBLog instance;
  return &instance;
}
void DBLog::Init(int flushInterval) {
  flushInterval = flushInterval;
  isRunning.store(false);
  curBuffer =
      std::make_unique<LogBuffer<Buffer_Size>>(new LogBuffer<Buffer_Size>());
  nextBuffer =
      std::make_unique<LogBuffer<Buffer_Size>>(new LogBuffer<Buffer_Size>());
}

DBLog::~DBLog() { Stop(); }

void DBLog::Stop() {
  if (isRunning) {
    isRunning = false;
    cond.notify_one();
    logThread.join();
  }
}

void DBLog::Start() {
  if (!isRunning) {
    isRunning = true;
    logThread = std::thread(std::bind(&DBLog::threadFunc, this));
  }
}

void DBLog::Write(int level, const char* format, ...) {
  char msg[256];
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tSec = now.tv_sec;
  struct tm t = *(localtime(&tSec));
  int n = snprintf(msg, 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                   t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                   t.tm_min, t.tm_sec, now.tv_usec);

  char* newmsg = appendLogLevelTitle(level, msg + n);
  va_list vaList;
  va_start(vaList, format);
  int m = vsnprintf(newmsg, msg + 254 - newmsg, format, vaList);
  va_end(vaList);
  *(newmsg + m) = '\n';
  Append(msg, newmsg + m + 1 - msg);
}
void DBLog::Append(const char* msg, size_t len) {
  std::unique_lock<std::mutex> locker(mtx);
  // 1.尽量写入currentBuffer
  if (curBuffer->BufferAvail() >= len) {
    curBuffer->Append(msg, len);
  } else {
    // 2.curBuf插入队列
    BufferVec.push_back(std::move(curBuffer));
    // 3. 判断能否写入nextBuffer
    if (!nextBuffer) {
      nextBuffer.reset(new LogBuffer<Buffer_Size>());
    }
    nextBuffer->Append(msg, len);
    curBuffer = std::move(nextBuffer);

    // 4.
    cond.notify_one();
  }
}
void DBLog::threadFunc() {
  BufPtr newLogBuffer1(new LogBuffer<Buffer_Size>());
  BufPtr newLogBuffer2(new LogBuffer<Buffer_Size>());
  std::vector<BufPtr> ToWriteBufferVec;
  while (isRunning) {
    {
      std::unique_lock<std::mutex> locker(mtx);
      if (BufferVec.empty()) {
        cond.wait_for(locker, std::chrono::seconds(flushInterval));
      }
      BufferVec.push_back(std::move(curBuffer));
      ToWriteBufferVec.swap(BufferVec);
      curBuffer = std::move(newLogBuffer1);
      if (!nextBuffer) {
        nextBuffer = std::move(newLogBuffer2);
      }
    }
    for (const auto& ptr : ToWriteBufferVec) {
      // 写入磁盘
      logFile.Append(ptr->Data(), ptr->BufferLen());
    }

    if (!newLogBuffer1) {
      if (!ToWriteBufferVec.empty()) {
        newLogBuffer1 = std::move(ToWriteBufferVec.back());
        ToWriteBufferVec.pop_back();
      } else {
        newLogBuffer1.reset(new LogBuffer<Buffer_Size>());
      }
      newLogBuffer1->Reset();
    }

    if (!newLogBuffer2) {
      if (!ToWriteBufferVec.empty()) {
        newLogBuffer1 = std::move(ToWriteBufferVec.back());
        ToWriteBufferVec.pop_back();
      } else {
        newLogBuffer2.reset(new LogBuffer<Buffer_Size>());
      }
      newLogBuffer2->Reset();
    }
    ToWriteBufferVec.clear();
  }
}

char* DBLog::appendLogLevelTitle(int level, char* msg) {
  switch (level) {
    case 0:
      memcpy(msg, "[debug]: ", 10);
      msg += 9;
      break;
    case 1:
      memcpy(msg, "[info]: ", 9);
      msg += 8;
      break;
    case 2:
      memcpy(msg, "[warn]: ", 9);
      msg += 8;
      break;
    case 3:
      memcpy(msg, "[error]: ", 10);
      msg += 9;
      break;
    default:
      memcpy(msg, "[info]: ", 9);
      msg += 8;
      break;
  }
  return msg;
}