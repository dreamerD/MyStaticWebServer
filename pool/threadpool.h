#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <assert.h>
#include <unistd.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
class ThreadPool {
 private:
  int threadCount;
  std::mutex mtx;
  std::condition_variable conn;
  bool isClose;
  std::queue<std::function<void()>> tasks;

 public:
  ThreadPool(int threadCount) {
    // printf("good\n");
    this->isClose = false;
    this->threadCount = threadCount;
    assert(threadCount > 0);
    for (int i = 0; i < threadCount; i++) {
      std::thread([this_ = this] {
        std::unique_lock<std::mutex> locker(this_->mtx);  // 获得锁
        while (true) {
          if (this_->isClose) {
            break;
          }
          if (!this_->tasks.empty()) {
            auto task = std::move(this_->tasks.front());
            printf("sub thread1!\n");
            this_->tasks.pop();
            locker.unlock();  // 解锁
            task();
            locker.lock();  // 重新获得锁
          } else {
            this_->conn.wait(locker);
            printf("sub thread0!\n");
          }
        }
      }).detach();
    }
  }
  // 万能引用与完美转发
  // https://blog.csdn.net/weixin_44966641/article/details/122381153?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-1-122381153-blog-124264766.pc_relevant_multi_platform_whitelistv3&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-1-122381153-blog-124264766.pc_relevant_multi_platform_whitelistv3&utm_relevant_index=1
  // https://blog.csdn.net/Nie_Quanxin/article/details/81187468
  template <class F>
  void AddTask(F&& task) {
    {  // 一定要加{}
      std::unique_lock<std::mutex> locker(this->mtx);
      this->tasks.emplace(std::forward<F>(task));
    }
    this->conn.notify_one();
  }

  ~ThreadPool() {
    {  // 一定要加{}
      std::unique_lock<std::mutex> locker(this->mtx);
      this->isClose = true;
    }
    conn.notify_all();
  }
};
#endif