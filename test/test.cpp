#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define PORT 8082

// 设置socket连接为非阻塞模式
void setnonblocking(int sockfd) {
  int opts;
  opts = fcntl(sockfd, F_GETFL);
  if (opts < 0) {
    perror("fcntl(F_GETFL)\n");
    exit(1);
  }
  opts = (opts | O_NONBLOCK);
  if (fcntl(sockfd, F_SETFL, opts) < 0) {
    perror("fcntl(F_SETFL)\n");
    exit(1);
  }
}

int main() {
  struct epoll_event ev,
      events[MAX_EVENTS];  // ev负责添加事件，events接收返回事件
  int addrlen, listenfd, conn_sock, nfds, epfd, fd, i, nread, n;
  struct sockaddr_in local, remote;
  char buf[BUFSIZ];

  // 创建listen socket
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("sockfd\n");
    exit(1);
  }
  setnonblocking(listenfd);  // listenfd设置为非阻塞[1]
  bzero(&local, sizeof(local));
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  ;
  local.sin_port = htons(PORT);
  if (bind(listenfd, (struct sockaddr *)&local, sizeof(local)) < 0) {
    perror("bind\n");
    exit(1);
  }
  listen(listenfd, 20);

  epfd = epoll_create(MAX_EVENTS);
  if (epfd == -1) {
    perror("epoll_create");
    exit(EXIT_FAILURE);
  }

  ev.events = EPOLLIN;
  ev.data.fd = listenfd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {  // 监听listenfd
    perror("epoll_ctl: listen_sock");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_pwait");
      exit(EXIT_FAILURE);
    }

    for (i = 0; i < nfds; ++i) {
      fd = events[i].data.fd;
      if (fd == listenfd) {
        while ((conn_sock = accept(listenfd, (struct sockaddr *)&remote,
                                   (socklen_t *)&addrlen)) > 0) {
          setnonblocking(conn_sock);  // 下面设置ET模式，所以要设置非阻塞
          ev.events = EPOLLIN | EPOLLET;
          ev.data.fd = conn_sock;
          if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {  // 读监听
            perror("epoll_ctl: add");  // 连接套接字
            exit(EXIT_FAILURE);
          }
        }
        if (conn_sock == -1) {
          if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO &&
              errno != EINTR)
            perror("accept");
        }
        continue;
      }
      if (events[i].events & EPOLLIN) {
        n = 0;
        while ((nread = read(fd, buf + n, BUFSIZ - 1)) >
               0) {  // ET下可以读就一直读
          n += nread;
        }
        if (nread == -1 && errno != EAGAIN) {
          perror("read error");
        }
        ev.data.fd = fd;
        ev.events = events[i].events | EPOLLOUT | EPOLLERR;  // MOD OUT
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
          perror("epoll_ctl: mod");
        }
      }
      if (events[i].events & EPOLLERR) {
        break;
      }
      if (events[i].events & EPOLLOUT) {
        sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nHello World",
                11);
        int nwrite, data_size = strlen(buf);
        n = data_size;
        while (n > 0) {
          nwrite = write(fd, buf + data_size - n, n);  // ET下一直将要写数据写完
          if (nwrite < n) {
            if (nwrite == -1 && errno != EAGAIN) {
              perror("write error");
            }
            break;
          }
          n -= nwrite;
        }
        close(fd);
      }
    }
  }
  return 0;
}