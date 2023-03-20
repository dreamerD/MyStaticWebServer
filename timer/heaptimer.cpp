#include "heaptimer.h"

void HeapTimer::Adjust(int fd, int timeout) {
  // assert检查
  printf("HeapTimer::Adjust[%d]", fd);
  printf("Heap.size()=%d", heap.size());
  assert(ref.count(fd) != 0);
  assert(!heap.empty());
  assert(ref[fd] < heap.size());
  heap[ref[fd]].expires = TimerNode::Clock::now() + TimerNode::MS(timeout);
  downadjust(ref[fd]);
}

// https://blog.csdn.net/iuices/article/details/122530402
void HeapTimer::Add(int fd, int timeout, std::function<void()> cb) {
  assert(fd >= 0);
  printf("插入的fd=%d", fd);
  /* 新节点：堆尾插入，调整堆 */
  if (!ref.count(fd)) {
    ref[fd] = heap.size();
    heap.push_back(
        {fd, TimerNode::Clock::now() + TimerNode::MS(timeout), cb, true});
    upadjust(ref[fd]);
  } else {
    printf("HeapTimer::Add0\n");
    printf("ref[%d]\n", ref[fd]);
    printf("heap.size()=%d\n", heap.size());
    heap[ref[fd]].expires = TimerNode::Clock::now() + TimerNode::MS(timeout);
    if (ref[fd] != heap.size() - 1) {
      swap(ref[fd], heap.size() - 1);
      upadjust(ref[fd]);
      printf("HeapTimer::Add1\n");
    }
  }
}

void HeapTimer::Clear() {
  ref.clear();
  heap.clear();
}

void HeapTimer::Tick() {
  /* 清除超时结点 */
  while (!heap.empty()) {
    TimerNode& node = heap.front();
    // if (!node.valid) {
    //   Pop();
    //   continue;
    // }
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
  if (heap.size() == 0) {
    return;
  }
  size_t i = index;
  while (i < heap.size() - 1) {
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
  while (i > 0) {
    size_t j = (i - 1) / 2;
    if (heap[j].expires < heap[i].expires) {
      break;
    }
    swap(i, j);
    i = j;
  }
}

void HeapTimer::swap(size_t i, size_t j) {
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