#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

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

struct ClientInfo
{
    string ip;
    int port;
    int id;
    int fd;
    int state; // 表示客户端所处的状态，根据状态决定要进行的操作
    char plainBuf[MAX_BUFFER_SIZE]; // 需要发送的消息原文
    char writeBuf[MAX_BUFFER_SIZE]; // 需要往socket发送的数据（RSA/DES密文）
    string desKey;
    DES *des;
    ClientInfo(string ip, int port, int fd, int id) : ip(ip), port(port), fd(fd), id(id),
                                                      state(SERVER_PREPARED), des(nullptr)
    {
        memset(plainBuf, 0, sizeof(plainBuf));
        memset(writeBuf, 0, sizeof(writeBuf));
    }
    ~ClientInfo()
    {
        delete des;
    }
};

class Server : public Epoll
{
public:
    Server();
    Server(string ip, int port);
    ~Server();

private:
    int socketfd;
    string ip;
    int port;
    struct sockaddr_in servaddr;
    int epollfd;
    char errmsg[256];
    unordered_map<int, ClientInfo *> clientList; // 服务端维护的连接客户端列表
    int clientId;
    int pipefd[2];
    bool started;
    bool isBlock; // 是否阻塞
    pid_t childpid;
    string e;
    string n;
    RSA rsa;

private:
    ClientInfo *findClientById(int id);
    void init() override;
    void clean() override;

public:
    void start(bool isBlock = false) override;
    void doEpoll() override;
    void addEvent(int fd, int state) override;
    void delEvent(int fd, int state) override;
    void modEvent(int fd, int state) override;
    void handleEvents(struct epoll_event *events, int num, char *buf, int &buflen) override;
    bool handleAccept();
    bool doRead(int fd, char *buf, int &buflen) override;
    bool doWrite(int fd, char *buf, int buflen) override;
    void setIPAddress(string ip) override;
    void setPort(int port) override;
};

#endif
