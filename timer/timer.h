#ifndef TIMER_H
#define TIMER_H
#include <chrono>
#include <functional>

struct TimerNode {
  int fd;  // 对应的套接字
  // https://www.cnblogs.com/zhongpan/p/7490657.html
  typedef std::chrono::high_resolution_clock Clock;
  typedef std::chrono::milliseconds MS;
  Clock::time_point expires;
  std::function<void()> cb;
  bool operator<(const TimerNode& t) { return expires < t.expires; }
};

class Timer {
 public:
  virtual void Adjust(int id, int newExpires) = 0;

  virtual void Add(int id, int timeOut, std::function<void()>&& cb) = 0;

  virtual void DoWork(int id) = 0;

  virtual void Clear() = 0;

  virtual void Tick() = 0;

  virtual void Pop() = 0;

  virtual int GetNextTick() = 0;
};
#endif