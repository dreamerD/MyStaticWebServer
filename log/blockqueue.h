#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <sys/time.h>

#include <condition_variable>
#include <deque>
#include <mutex>

template <class T>
class BlockDeque {
 public:
  explicit BlockDeque(size_t maxCapacity = 1000);

  ~BlockDeque();

  void Clear();

  bool Empty();

  bool Full();

  void Close();

  size_t Size();

  size_t Capacity();

  T Front();

  T Back();

  void PushBack(const T &item);

  void PushFront(const T &item);

  bool Pop(T &item);

  bool Pop(T &item, int timeout);

  void Flush();

 private:
  // 双端队列
  std::deque<T> deq;

  size_t capacity;

  std::mutex mtx;

  bool isClose;

  std::condition_variable condConsumer;

  std::condition_variable condProducer;
};

template <class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) : capacity(MaxCapacity) {
  assert(MaxCapacity > 0);
  isClose = false;
}

template <class T>
BlockDeque<T>::~BlockDeque() {
  Close();
};

template <class T>
void BlockDeque<T>::Close() {
  {
    std::lock_guard<std::mutex> locker(mtx);
    deq.clear();
    isClose = true;
  }
  condProducer.notify_all();
  condConsumer.notify_all();
};

template <class T>
void BlockDeque<T>::Flush() {
  condConsumer.notify_one();
};

template <class T>
void BlockDeque<T>::Clear() {
  std::lock_guard<std::mutex> locker(mtx);
  deq.clear();
}

template <class T>
T BlockDeque<T>::Front() {
  std::lock_guard<std::mutex> locker(mtx);
  return deq.front();
}

template <class T>
T BlockDeque<T>::Back() {
  std::lock_guard<std::mutex> locker(mtx);
  return deq.back();
}

template <class T>
size_t BlockDeque<T>::Size() {
  std::lock_guard<std::mutex> locker(mtx);
  return deq.size();
}

template <class T>
size_t BlockDeque<T>::Capacity() {
  std::lock_guard<std::mutex> locker(mtx);
  return capacity;
}

template <class T>
void BlockDeque<T>::PushBack(const T &item) {
  std::unique_lock<std::mutex> locker(mtx);
  while (deq.size() >= capacity && !isClose) {
    condProducer.wait(locker);
  }
  deq.push_back(item);
  condConsumer.notify_one();
}

template <class T>
void BlockDeque<T>::PushFront(const T &item) {
  std::unique_lock<std::mutex> locker(mtx);
  while (deq.size() >= capacity && !isClose) {
    condProducer.wait(locker);
  }
  deq.push_front(item);
  condConsumer.notify_one();
}

template <class T>
bool BlockDeque<T>::Empty() {
  std::lock_guard<std::mutex> locker(mtx);
  return deq.empty();
}

template <class T>
bool BlockDeque<T>::Full() {
  std::lock_guard<std::mutex> locker(mtx);
  return deq.size() >= capacity;
}

template <class T>
bool BlockDeque<T>::Pop(T &item) {
  std::unique_lock<std::mutex> locker(mtx);
  while (deq.empty()) {
    condConsumer.wait(locker);
    if (isClose) {
      return false;
    }
  }
  item = deq.front();
  deq.pop_front();
  condProducer.notify_one();
  return true;
}

template <class T>
bool BlockDeque<T>::Pop(T &item, int timeout) {
  std::unique_lock<std::mutex> locker(mtx);
  while (deq.empty()) {
    if (condConsumer.wait_for(locker, std::chrono::seconds(timeout)) ==
        std::cv_status::timeout) {
      return false;
    }
    if (isClose) {
      return false;
    }
  }
  item = deq.front();
  deq.pop_front();
  condProducer.notify_one();
  return true;
}

#endif  // BLOCKQUEUE_H