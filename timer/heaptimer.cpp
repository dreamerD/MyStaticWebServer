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
  size_t i = heap.size();
  ref[fd] = i;
  heap.push_back({fd, TimerNode::Clock::now() + TimerNode::MS(timeout),
                  std::forward<std::function<void()>>(cb)});
  upadjust(i);
}

void HeapTimer::DoWork(int id) {}

void HeapTimer::Clear() {}

void HeapTimer::Tick() {}

void HeapTimer::Pop() {}

int HeapTimer::GetNextTick() {}

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

void HeapTimer::swap(int i, int j) {
  // https://blog.csdn.net/zqw_yaomin/article/details/81278948
  // 通过move实现交换
  std::swap(heap[i], heap[j]);  // 思考swap
  ref[heap[i].fd] = j;
  ref[heap[j].fd] = i;
}
