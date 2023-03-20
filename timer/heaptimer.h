#ifndef HEAPTIMER_H
#define HEAPTIMER_H
#include <assert.h>

#include <unordered_map>
#include <vector>

#include "timer.h"
class HeapTimer : public Timer {
 public:
  virtual void Adjust(int id, int newExpires);

  virtual void Add(int id, int timeOut, std::function<void()> cb);

  virtual void Clear();

  virtual void Tick();

  virtual void Pop();

  virtual int GetNextTick();

 private:
  void downadjust(size_t index);
  void upadjust(size_t index);
  void swap(size_t i, size_t j);
  void del(size_t index);

 private:
  std::vector<TimerNode> heap;  // 大顶堆

  std::unordered_map<int, size_t> ref;  // 索引
};
#endif