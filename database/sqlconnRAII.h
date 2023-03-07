#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnRAII {
 public:
  SqlConnRAII(MYSQL** sql, SqlConnPool* connpool) {
    assert(connpool);
    *sql = connpool->GetConn();
    this->sql = *sql;
    connpool = connpool;
  }

  ~SqlConnRAII() {
    if (sql) {
      connpool->FreeConn(sql);
    }
  }

 private:
  MYSQL* sql;
  SqlConnPool* connpool;
};

#endif  // SQLCONNRAII_H