#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "errno.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#define MAXLINE 10

class C {
 private:
  int a;

 public:
  void func() { printf("%d,OK!\n", a); }
};
int main(void) {
  C* ptr = new C;
  delete ptr;
  ptr->func();
}