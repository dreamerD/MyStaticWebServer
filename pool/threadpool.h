#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <assert.h>

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
  ThreadPool(int threadCount = 8);

  // 万能引用与完美转发
  // https://blog.csdn.net/weixin_44966641/article/details/122381153?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-1-122381153-blog-124264766.pc_relevant_multi_platform_whitelistv3&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-1-122381153-blog-124264766.pc_relevant_multi_platform_whitelistv3&utm_relevant_index=1
  // https://blog.csdn.net/Nie_Quanxin/article/details/81187468
  template <class F>
  void AddTask(F&& task);
  ~ThreadPool();
};
#endif