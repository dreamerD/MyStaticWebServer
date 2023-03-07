#include "heaptimer.h"

void HeapTimer::Adjust(int fd, int timeout) {
  // assert检查
  assert(ref.count(fd) != 0 && !heap.empty() && ref[fd] < heap.size());
  heap[ref[fd]].expires = TimerNode::Clock::now() + TimerNode::MS(timeout);
  downadjust(ref[fd]);
}

// https://blog.csdn.net/iuices/article/details/122530402
void HeapTimer::Add(int fd, int timeout, std::function<void()>&& cb) {
  assert(fd >= 0);
  /* 新节点：堆尾插入，调整堆 */
  ref[fd] = heap.size();
  heap.push_back({fd, TimerNode::Clock::now() + TimerNode::MS(timeout),
                  std::forward<std::function<void()>>(cb)});
  upadjust(ref[fd]);
}

void HeapTimer::DoWork(int fd) {
  if (heap.empty() || ref.count(fd) == 0) {
    return;
  }
  del(ref[fd]);
  return;
}

void HeapTimer::Clear() {
  ref.clear();
  heap.clear();
}

void HeapTimer::Tick() {
  /* 清除超时结点 */
  while (!heap.empty()) {
    TimerNode node = heap.front();
    if (std::chrono::duration_cast<TimerNode::MS>(node.expires -
                                                  TimerNode::Clock::now())
            .count() > 0) {
      break;
    }
    node.cb();
    Pop();
  }
}

void HeapTimer::Pop() {
  assert(!heap.empty());
  del(0);
}

int HeapTimer::GetNextTick() {
  Tick();
  size_t res = -1;
  if (!heap.empty()) {
    res = std::chrono::duration_cast<TimerNode::MS>(heap.front().expires -
                                                    TimerNode::Clock::now())
              .count();
    if (res < 0) {
      res = 0;
    }
  }
  return res;
}

void HeapTimer::downadjust(size_t index) {
  size_t i = index;
  while (i < heap.size()) {
    size_t j = i * 2 + 1;
    if (j < heap.size() - 1 && heap[j + 1].expires < heap[j].expires) {
      j++;
    }
    if (heap[i].expires > heap[j].expires) {
      swap(i, j);
      i = j;
    } else {
      break;
    }
  }
}
void HeapTimer::upadjust(size_t index) {
  size_t i = index;
  while (i >= 0) {
    size_t j = (i - 1) / 2;
    if (heap[j].expires < heap[i].expires) {
      break;
    }
    swap(i, j);
    i = j;
  }
}

void HeapTimer::swap(size_t i, size_t j) {
  // https://blog.csdn.net/zqw_yaomin/article/details/81278948
  // 通过move实现交换
  std::swap(heap[i], heap[j]);  // 思考swap
  ref[heap[i].fd] = j;
  ref[heap[j].fd] = i;
}

void HeapTimer::del(size_t index) {
  swap(index, heap.size() - 1);
  ref.erase(heap.back().fd);  // 删除索引
  heap.pop_back();
  downadjust(index);
}