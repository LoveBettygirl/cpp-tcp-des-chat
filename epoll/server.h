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
#include "macro.h"
using namespace std;

struct ClientInfo
{
    string ip;
    int port;
    int id;
    int fd;
    int state;
    char plainBuf[MAX_BUFFER_SIZE];
    char writeBuf[MAX_BUFFER_SIZE];
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

class Server
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
    unordered_map<int, ClientInfo *> clientList;
    int clientId;
    int pipefd[2];
    bool started;
    bool isBlock;
    string e;
    string n;
    RSA rsa;

private:
    ClientInfo *findClientById(int id);
    void init();
    void clean();

public:
    void start(bool isBlock = false);
    int getSocketfd();
    void doEpoll();
    void addEvent(int fd, int state);
    void delEvent(int fd, int state);
    void modEvent(int fd, int state);
    void handleEvents(struct epoll_event *events, int num, char *buf, int &buflen);
    bool handleAccept();
    bool doRead(int fd, char *buf, int &buflen);
    bool doWrite(int fd, char *buf, int buflen);
    void setIPAddress(string ip);
    void setPort(int port);
};

#endif
