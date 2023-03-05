#include "epoll.h"

Epoller::Epoller(int maxsize = 1024) : size(maxsize) {
  epollFd = epoll_create(1024);
  events = new epoll_event[size];
}
Epoller::~Epoller() {
  close(epollFd);
  delete[] events;
}
bool Epoller::AddFd(int fd, uint32_t event) {
  if (fd < 0) {
    return false;
  }
  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = event;
  return 0 == epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::DeleteFd(int fd) {
  if (fd < 0) {
    return false;
  }
  epoll_event ev = {0};
  return 0 == epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t event) {
  if (fd < 0) {
    return false;
  }
  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = event;
  // 再次关注
  return 0 == epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}
int Epoller::GetEventFd(int pos) {
  assert(pos < size && pos >= 0);
  return events[static_cast<size_t>(pos)].data.fd;
}

uint32_t Epoller::GetEvent(int pos) {
  assert(pos < size && pos >= 0);
  return events[static_cast<size_t>(pos)].events;
}

int Epoller::Wait(int timeoutMS) {
  return epoll_wait(epollFd, events, size, timeoutMS);
}