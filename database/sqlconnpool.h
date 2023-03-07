/*
 * @Author       : mark
 * @Date         : 2020-06-16
 * @copyleft Apache 2.0
 */
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <semaphore.h>

#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "log/log.h"

class SqlConnPool {
 public:
  static SqlConnPool* Instance();

  MYSQL* GetConn();
  void FreeConn(MYSQL* conn);
  int GetFreeConnCount();

  void Init(const char* host, int port, const char* user, const char* pwd,
            const char* dbName, int connSize);
  void ClosePool();

 private:
  SqlConnPool();
  ~SqlConnPool();

  int maxConn;
  int useCount;
  int freeCount;

  std::queue<MYSQL*> connQue;
  std::mutex mtx;
  sem_t semId;
};

#endif  // SQLCONNPOOL_H