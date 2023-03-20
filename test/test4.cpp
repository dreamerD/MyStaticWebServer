#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <utility>

#include "errno.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#define MAXLINE 10

void f(int& x) {
  printf("左值\n");
  x += 10;
}
void f(int&& x) { printf("右值\n"); }
// https://www.cnblogs.com/5iedu/p/11324772.html
int main(void) {
  int x = 5;
  int&& a = 5;
  f(x);
  printf("%d\n", x);
  f(std::move(a));
  printf("%d\n", a);
}