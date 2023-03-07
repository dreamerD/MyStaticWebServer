#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "errno.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/types.h"
#define MAXLINE 10

int main(void) {
  pid_t pid;
  int epfd = -1, i, rval;
  int pfd[2];
  char buf[MAXLINE], ch = 'a';

  // 创建一个无名管道
  pipe(pfd);
  close(pfd[0]);
  printf("%d\n", errno);
  write(pfd[0], buf, sizeof(buf));
  printf("%d\n", errno);
  close(pfd[0]);
  printf("%d\n", errno);
  pipe(pfd);
  write(pfd[0], buf, sizeof(buf));
  printf("%d\n", errno);
  return 0;
}