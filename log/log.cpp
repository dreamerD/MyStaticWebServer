#include "log.h"
Log::Log() {
  lineCount = 0;
  isAsync = false;
  writeThread = nullptr;
  deque = nullptr;
  toDay = 0;
  fp = nullptr;
}

Log::~Log() {
  if (writeThread && writeThread->joinable()) {
    while (!deque->Empty()) {
      deque->Flush();  // 从双端队列取出log
    }
    deque->Close();

    writeThread->join();
  }
  if (fp) {
    std::lock_guard<std::mutex> locker(mtx);
    fflush(fp);
    fclose(fp);
  }
}

int Log::GetLevel() {
  std::lock_guard<std::mutex> locker(mtx);
  return level;
}

void Log::SetLevel(int level) {
  std::lock_guard<std::mutex> locker(mtx);
  level = level;
}

void Log::Init(int level = 1, const char* path, const char* suffix,
               int maxQueueSize) {
  this->isOpen = true;
  this->level = level;
  this->maxLines = 1000;
  if (maxQueueSize > 0) {
    this->isAsync = true;
    std::unique_ptr<BlockDeque<std::string>> newDeque(
        new BlockDeque<std::string>);
    this->deque = move(newDeque);

    std::unique_ptr<std::thread> NewThread(new std::thread(FlushLogThread));
    NewThread->detach();
    this->writeThread = move(NewThread);
  } else {
    this->isAsync = false;
  }
  this->lineCount = 0;
  time_t timer = time(nullptr);
  tm t = *(localtime(&timer));

  this->path = path;
  this->suffix = suffix;
  char fileName[LOG_NAME_LEN] = {0};
  snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path,
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix);
  this->toDay = t.tm_mday;

  buff.Init();
  fp = fopen(fileName, "a");
  if (fp == nullptr) {
    mkdir(path, 0777);
    fp = fopen(fileName, "a");
  }
  assert(fp != nullptr);
}

Log* Log::Instance() {
  static Log instance;
  return &instance;
}

void Log::asyncWrite() {
  std::string str = "";
  while (deque->Pop(str)) {
    std::lock_guard<std::mutex> locker(mtx);
    fputs(str.c_str(), fp);
  }
}
void Log::FlushLogThread() { Log::Instance()->asyncWrite(); }

void Log::Write(int level, const char* format, ...) {
  std::unique_lock<std::mutex> lockerWrite(mtxWrite);
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tSec = now.tv_sec;
  struct tm t = *(localtime(&tSec));
  va_list vaList;
  if (toDay != t.tm_mday || (lineCount && (lineCount % maxLines == 0))) {
    char newFile[LOG_NAME_LEN];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1,
             t.tm_mday);

    if (toDay != t.tm_mday) {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path, tail, suffix);
      toDay = t.tm_mday;
      lineCount = 0;
    } else {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path, tail,
               (lineCount / MAX_LINES), suffix);
    }
    while (!deque->Empty()) {
      deque->Flush();  // 从双端队列取出log
    }
    std::unique_lock<std::mutex> locker(mtx);
    fflush(fp);
    fclose(fp);
    fp = fopen(newFile, "a");
    locker.unlock();
    assert(fp != nullptr);
  }
  lineCount++;
  int n =
      snprintf(buff.WriteBeginPos(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
               t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
               t.tm_sec, now.tv_usec);

  buff.writePos += n;
  appendLogLevelTitle(level);

  va_start(vaList, format);
  int m =
      vsnprintf(buff.WriteBeginPos(), buff.WriteAbleBytes(), format, vaList);
  va_end(vaList);

  buff.writePos += m;
  std::string s("\n\0");

  buff.Append("\n\0");

  if (isAsync && deque) {
    deque->PushBack(buff.GetAllReadableData());
  } else {
    fputs(buff.ReadBeginPos(), fp);
  }
  buff.Init();
}

void Log::appendLogLevelTitle(int level) {
  switch (level) {
    case 0:
      buff.Append("[debug]: ");
      break;
    case 1:
      buff.Append("[info] : ");
      break;
    case 2:
      buff.Append("[warn] : ");
      break;
    case 3:
      buff.Append("[error]: ");
      break;
    default:
      buff.Append("[info] : ");
      break;
  }
}