#ifndef EPOLL_CLIENT_H
#define EPOLL_CLIENT_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <cctype>
#include "../cipher/RSA.h"
#include "../cipher/DES.h"
#include "../cipher/utils.h"
#include "epoll.h"
#include "../macro.h"
using namespace std;

class Client : public Epoll
{
public:
    Client(string ip, int port);
    ~Client();

private:
    int socketfd;
    string remoteIP;
    int remotePort;
    string localIP;
    int localPort;
    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    int epollfd;
    char errmsg[256];
    char plainBuf[MAX_BUFFER_SIZE]; // 需要发送的消息原文
    char writeBuf[MAX_BUFFER_SIZE]; // 需要往socket发送的数据（RSA/DES密文）
    int pipefd[2];
    bool started;
    int state; // 表示客户端所处的状态，根据状态决定要进行的操作
    bool isBlock; // 是否阻塞
    pid_t childpid;
    string desKey;
    DES *des;

private:
    void genRandomDESKey();
    void init() override;
    void clean() override;

public:
    void start(bool isblock = false) override;
    void doEpoll() override;
    void addEvent(int fd, int state) override;
    void delEvent(int fd, int state) override;
    void modEvent(int fd, int state) override;
    void handleEvents(struct epoll_event *events, int num, char *buf, int &buflen) override;
    bool doRead(int fd, char *buf, int &buflen) override;
    bool doWrite(int fd, char *buf, int buflen) override;
    void setIPAddress(string ip) override;
    void setPort(int port) override;
};

#endif
