#ifndef EPOLL_H
#define EPOLL_H

#include <string>
using namespace std;

class Epoll
{
public:
    virtual ~Epoll() = 0;

private:
    virtual void init() = 0;
    virtual void clean() = 0;

public:
    virtual void start(bool isBlock = false) = 0;
    virtual void doEpoll() = 0;
    virtual void addEvent(int fd, int state) = 0;
    virtual void delEvent(int fd, int state) = 0;
    virtual void modEvent(int fd, int state) = 0;
    virtual void handleEvents(struct epoll_event *events, int num, char *buf, int &buflen) = 0;
    virtual bool doRead(int fd, char *buf, int &buflen) = 0;
    virtual bool doWrite(int fd, char *buf, int buflen) = 0;
    virtual void setIPAddress(string ip) = 0;
    virtual void setPort(int port) = 0;
};

#endif
