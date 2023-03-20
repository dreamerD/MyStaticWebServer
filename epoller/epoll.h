#ifndef EPOLLER_H
#define EPOLLER_H
#include <assert.h>
#include <stdint.h>  // uint32_t
#include <sys/epoll.h>
#include <unistd.h>
class Epoller {
 public:
  Epoller(int maxsize = 1024);
  ~Epoller();
  int Wait(int timeoutMS);
  bool AddFd(int fd, uint32_t event);
  bool DeleteFd(int fd);
  bool ModFd(int fd, uint32_t event);
  uint32_t GetEvent(int pos);
  int GetEventFd(int pos);

 private:
  int size;
  int epollFd;
  epoll_event *events;
};
#endif