#include "server.h"

Server::Server() : ip("0.0.0.0"), port(2000), started(false), clientId(0), socketfd(0), epollfd(0), rsa(256), childpid(0)
{
    init();
}

Server::Server(string ip, int port) : ip(ip), port(port), started(false), clientId(0), socketfd(0), epollfd(0), rsa(256), childpid(0)
{
    init();
}

Server::~Server()
{
    clean();
}

void Server::init()
{
    memset(&servaddr, 0, sizeof(servaddr));
    memset(pipefd, 0, sizeof(pipefd));
    RSAPublicKey publicKey = rsa.getPublicKey();
    e = toBinaryString(publicKey.getE());
    n = toBinaryString(publicKey.getN());
}

void Server::clean()
{
    delEvent(pipefd[0], EPOLLIN);
    while (!clientList.empty())
    {
        unordered_map<int, ClientInfo *>::iterator it = clientList.begin();
        ClientInfo *temp = it->second;
        clientList.erase(temp->fd);
        delEvent(temp->fd, EPOLLIN);
        delEvent(temp->fd, EPOLLOUT);
        close(temp->fd);
        delete temp;
    }
    delEvent(socketfd, EPOLLIN);
    close(pipefd[1]);
    close(pipefd[0]);
    close(epollfd);
    close(socketfd);
    close(0);
    kill(childpid, SIGKILL);
}

void Server::setIPAddress(string ip)
{
    if (started)
    {
        cerr << "The server is started." << endl;
        exit(SET_AFTER_STARTED);
    }
    this->ip = ip;
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
}

void Server::setPort(int port)
{
    if (started)
    {
        cerr << "The server is started." << endl;
        exit(SET_AFTER_STARTED);
    }
    this->port = port;
    servaddr.sin_port = htons(port);
}

void Server::start(bool isBlock)
{
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "create socket error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        exit(CREATE_SOCKET_ERROR);
    }
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    this->isBlock = isBlock;
    if (!isBlock)
    {
        int flags = fcntl(socketfd, F_GETFL, 0);
        fcntl(socketfd, F_SETFL, flags | O_NONBLOCK); // 设置为非阻塞
    }

    // 设置重用地址，防止Address already in use
    int on = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "set reuse addr error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        close(socketfd);
        exit(SET_REUSEADDR_ERROR);
    }

    // 将本地地址绑定到所创建的套接字上
    if (bind(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "bind socket error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        close(socketfd);
        exit(BIND_SOCKET_ERROR);
    }

    // 开始监听是否有客户端连接
    if (listen(socketfd, BACKLOG) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "listen socket error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        close(socketfd);
        exit(LISTEN_ERROR);
    }

    started = true;
    cout << "Listening on <" << ip << ":" << port << ">..." << endl;
}

void Server::doEpoll()
{
    // 创建用于父子进程通信的管道，子进程监控用户输入
    if (pipe(pipefd) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "pipe error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        close(socketfd);
        exit(CREATE_PIPE_ERROR);
    }

    extern volatile bool running;

    pid_t pid = fork();
    if (pid == 0)
    {
        close(pipefd[0]); // 关闭子进程管道读端
        char message[MAX_BUFFER_SIZE];

        while (running)
        {
            memset(message, 0, sizeof(message));
            // fgets(message, MAX_BUFFER_SIZE, stdin);
            string input;
            int id;
            cin >> input;
            if (!running)
                break;
            cout << "Input the client id will send to: ";
            cin >> id;
            if (!running)
                break;
            sprintf(message, "%d %s", id, input.c_str());

            if (write(pipefd[1], message, strlen(message) + 1) < 0)
            {
                memset(errmsg, 0, sizeof(errmsg));
                snprintf(errmsg, sizeof(errmsg), "fork error: %s (errno: %d)\n", strerror(errno), errno);
                cerr << errmsg;
                // exit(-1);
            }
        }
        close(pipefd[1]);
        exit(SUCCESS); // 不让子进程调用析构函数
    }
    else
    {
        childpid = pid;
        close(pipefd[1]); // 关闭父进程管道写端
        if (!isBlock)
        {
            int flags = fcntl(pipefd[0], F_GETFL, 0);
            fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK); // 设置为非阻塞
        }
        struct epoll_event events[MAX_EPOLL_EVENTS];
        int ret;
        char buf[MAX_BUFFER_SIZE] = {0};
        int buflen = 0;
        // 创建一个描述符
        if ((epollfd = epoll_create(FDSIZE)) < 0)
        {
            memset(errmsg, 0, sizeof(errmsg));
            snprintf(errmsg, sizeof(errmsg), "epoll create error: %s (errno: %d)\n", strerror(errno), errno);
            cerr << errmsg;
            clean();
            exit(EPOLL_CREATE_ERROR);
        }
        // 添加监听描述符事件
        addEvent(socketfd, EPOLLIN);
        addEvent(pipefd[0], EPOLLIN);
        while (running)
        {
            // 获取已经准备好的描述符事件
            ret = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
            handleEvents(events, ret, buf, buflen);
        }
        // 等待子进程退出
        int wpid = waitpid(childpid, nullptr, 0);
    }
}

void Server::addEvent(int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET; // 设置边缘触发（ET）
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void Server::delEvent(int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

void Server::modEvent(int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void Server::handleEvents(epoll_event *events, int num, char *buf, int &buflen)
{
    int i;
    int fd;
    // 遍历就绪事件
    for (i = 0; i < num; i++)
    {
        fd = events[i].data.fd;
        // 根据描述符的类型和事件类型进行处理
        if ((fd == socketfd) && (events[i].events & EPOLLIN))
            handleAccept();
        else if (events[i].events & EPOLLIN)
            doRead(fd, buf, buflen);
        else if (events[i].events & EPOLLOUT)
            doWrite(fd, buf, buflen);
        else
            close(fd);
    }
}

bool Server::handleAccept()
{
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    clifd = accept(socketfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    if (clifd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return false;
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "accept client error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        clean();
        exit(ACCEPT_ERROR);
    }
    else
    {
        char msg[128] = {0};
        char *ip = inet_ntoa(cliaddr.sin_addr);
        int port = ntohs(cliaddr.sin_port);
        sprintf(msg, "server: got connection from %s, port %d, id %d\n", ip, port, clientId);
        cout << msg;
        clientList[clifd] = new ClientInfo(string(ip), port, clifd, clientId);
        ClientInfo *currClient = clientList[clifd];
        clientId++;
        sprintf(currClient->writeBuf, "%s %s", e.c_str(), n.c_str());
        addEvent(clifd, EPOLLOUT);
    }
    return true;
}

ClientInfo *Server::findClientById(int id)
{
    unordered_map<int, ClientInfo *>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++)
    {
        if (it->second->id == id)
            return it->second;
    }
    return nullptr;
}

bool Server::doRead(int fd, char *buf, int &buflen)
{
    buflen = read(fd, buf, MAX_BUFFER_SIZE);
    if (buflen == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return false;
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "read error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        clean();
        exit(READ_ERROR);
    }
    else if (buflen == 0)
    {
        if (fd != pipefd[0])
        {
            ClientInfo *currClient = clientList[fd];
            char msg[128] = {0};
            sprintf(msg, "client_%d<%s:%d> quit\n", currClient->id, currClient->ip.c_str(), currClient->port);
            cout << msg;
            clientList.erase(fd);
            delete currClient;
        }
        delEvent(fd, EPOLLIN);
        close(fd);
        return true;
    }
    else
    {
        // 接收来自管道读端的数据，发送给对应的socket
        if (fd == pipefd[0])
        {
            string s;
            int i;
            for (i = 0; i < buflen; i++)
            {
                if (isdigit(buf[i]))
                    s.push_back(buf[i]);
                if (buf[i + 1] == ' ')
                    break;
            }
            int targetid = atoi(s.c_str());
            if (!clientList.size()) {
                cerr << "There's no client." << endl;
                return true;
            }
            ClientInfo *currClient = findClientById(targetid);
            if (!currClient)
            {
                memset(errmsg, 0, sizeof(errmsg));
                snprintf(errmsg, sizeof(errmsg), "No client id is %d\n", targetid);
                cerr << errmsg;
            }
            else if (currClient->state == DES_KEY_RCVD)
            {
                string res = currClient->des->getResult(string(&buf[i + 2]), true);
                memset(currClient->plainBuf, 0, sizeof(currClient->plainBuf));
                memset(currClient->writeBuf, 0, sizeof(currClient->writeBuf));
                strcpy(currClient->plainBuf, &buf[i + 2]);
                strcpy(currClient->writeBuf, res.c_str());
                // 将fd对应的事件由读转为写，准备发送消息
                modEvent(currClient->fd, EPOLLOUT);
            }
            else
            {
                memset(errmsg, 0, sizeof(errmsg));
                snprintf(errmsg, sizeof(errmsg), "The client id %d is not ready.\n", targetid);
                cerr << errmsg;
            }
            return true;
        }
        ClientInfo *currClient = clientList[fd];
        if (!currClient)
        {
            memset(errmsg, 0, sizeof(errmsg));
            snprintf(errmsg, sizeof(errmsg), "No corresopnding client of fd %d\n", fd);
            cerr << errmsg;
            clean();
            exit(NO_CORR_FD_CLIENT_ERROR);
        }
        if (currClient->state == RSA_PUBKEY_SENT)
        {
            currClient->desKey = RSA::decry(string(buf), rsa.getPrivateKey());
            currClient->des = new DES(currClient->desKey);
            currClient->state = DES_KEY_RCVD;
        }
        else if (currClient->state == DES_KEY_RCVD)
        {
            string res = currClient->des->getResult(string(buf), false);
            char msg[MAX_BUFFER_SIZE] = {0};
            sprintf(msg, "Received from client_%d<%s:%d>: %s\n", currClient->id, currClient->ip.c_str(), currClient->port, res.c_str());
            cout << msg;
        }
    }
    return true;
}

bool Server::doWrite(int fd, char *buf, int buflen)
{
    int nwrite;
    ClientInfo *currClient = clientList[fd];
    char msg[MAX_BUFFER_SIZE] = {0};
    if (currClient && currClient->state == SERVER_PREPARED)
    {
        nwrite = write(fd, currClient->writeBuf, strlen(currClient->writeBuf) + 1);
        if (nwrite < 0)
        {
            memset(errmsg, 0, sizeof(errmsg));
            snprintf(errmsg, sizeof(errmsg), "RSA public key send error: %s (errno: %d)\n", strerror(errno), errno);
            cerr << errmsg;
            clean();
            exit(RSA_PUBKEY_SENT_ERROR);
        }
        currClient->state = RSA_PUBKEY_SENT;
        modEvent(fd, EPOLLIN); // 将fd对应的事件由写转为读，准备接收消息
    }
    else if (currClient && currClient->state == DES_KEY_RCVD)
    {
        nwrite = write(fd, currClient->writeBuf, strlen(currClient->writeBuf) + 1);
        if (nwrite < 0)
        {
            memset(errmsg, 0, sizeof(errmsg));
            snprintf(errmsg, sizeof(errmsg), "message send error: %s (errno: %d)\n", strerror(errno), errno);
            cerr << errmsg;
            clean();
            exit(MESSAGE_SENT_ERROR);
        }
        sprintf(msg, "Send to client_%d<%s:%d>: %s\n", currClient->id, currClient->ip.c_str(), currClient->port, currClient->plainBuf);
        cout << msg;
        modEvent(fd, EPOLLIN);
    }
    return true;
}
