#include "threadpool.h"

ThreadPool::ThreadPool(int threadCount = 8) {
  assert(threadCount > 0);
  for (int i = 0; i < threadCount; i++) {
    std::thread([&] {
      std::unique_lock<std::mutex> locker(mtx);  // 获得锁
      while (true) {
        if (isClose) {
          break;
        }
        if (!tasks.empty()) {
          auto task = tasks.front();
          tasks.pop();
          locker.unlock();  // 解锁
          task();
          locker.lock();  // 重新获得锁
        } else {
          conn.wait(locker);
        }
      }
    });
  }
}

template <class F>
void ThreadPool::AddTask(F&& task) {
  {  // 一定要加{}
    std::unique_lock<std::mutex> locker(mtx);
    tasks.emplace(std::forward<F>(task));
  }
  conn.notify_one();
}

ThreadPool::~ThreadPool() {
  {  // 一定要加{}
    std::unique_lock<std::mutex> locker(mtx);
    isClose = true;
  }
  conn.notify_all();
}
