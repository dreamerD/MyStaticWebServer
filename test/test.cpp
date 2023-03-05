#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

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

  pid = fork();

  if (pid == 0) {  // child process
    close(pfd[0]);

    while (1) {
      // aaaa\n
      for (i = 0; i < MAXLINE / 2; i++) {
        buf[i] = ch;
      }
      buf[i - 1] = '\n';
      ch++;
      // bbbb\n
      for (; i < MAXLINE; i++) {
        buf[i] = ch;
      }
      buf[i - 1] = '\n';
      ch++;
      // aaaa\nbbbb\n
      write(pfd[1], buf, sizeof(buf));
      sleep(10);
      break;
    }
    close(pfd[1]);
  } else if (pid > 0) {  // parent process
    struct epoll_event evt;
    struct epoll_event evts[10];
    int res, len;

    close(pfd[1]);
    epfd = epoll_create(10);
    if (epfd < 0) {
      perror("epoll_create error");
    }

    evt.events = EPOLLIN | EPOLLET;  // LT 水平触发 (默认)
    evt.data.fd = pfd[0];
    rval = epoll_ctl(epfd, EPOLL_CTL_ADD, pfd[0], &evt);
    if (rval < 0) {
      perror("epoll_ctl error");
      return 0;
    }

    while (1) {
      memset(buf, 0, sizeof(buf));
      res = epoll_wait(epfd, evts, 10, -1);
      printf("res %d\n", res);
      if (res < 0) {
        perror("epoll_wait error");
        return 0;
      }
      if (evts[0].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        printf("exit!\n");
        break;
      }
      if (evts[0].data.fd == pfd[0]) {
        // sleep(10);
        len = read(pfd[0], buf, MAXLINE / 2);
        if (len <= 0) {
          break;
        }
        printf("%d\n", len);
        write(STDOUT_FILENO, buf, len);
        epoll_event ev = {0};
        ev.data.fd = evts[0].data.fd;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
      }
    }
  }
  close(pfd[0]);
  return 0;
}