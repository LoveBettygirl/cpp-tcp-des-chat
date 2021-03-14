#include "client.h"

Client::Client(string ip, int port) : remoteIP(ip), remotePort(port), state(CLIENT_PREPARED),
                                      started(false), epollfd(0), socketfd(0), des(nullptr)
{
    init();
}

Client::~Client()
{
    clean();
}

void Client::init()
{
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&localaddr, 0, sizeof(localaddr));
    memset(plainBuf, 0, sizeof(plainBuf));
    memset(writeBuf, 0, sizeof(writeBuf));
    srand((unsigned)time(nullptr));
}

void Client::clean()
{
    delete des;
    delEvent(pipefd[0], EPOLLIN);
    delEvent(socketfd, EPOLLIN);
    delEvent(socketfd, EPOLLOUT);
    close(pipefd[0]);
    close(pipefd[1]);
    close(epollfd);
    close(socketfd);
}

void Client::setIPAddress(string ip)
{
    if (started)
    {
        cerr << "The client is started." << endl;
        exit(SET_AFTER_STARTED);
    }
    remoteIP = ip;
    inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr);
}

void Client::setPort(int port)
{
    if (started)
    {
        cerr << "The client is started." << endl;
        exit(SET_AFTER_STARTED);
    }
    remotePort = port;
    servaddr.sin_port = htons(port);
}

void Client::genRandomDESKey()
{
    string s, alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int size = alpha.length();
    for (int i = 0; i < 8; i++)
        s.push_back(alpha[rand() % size]);
    desKey = s;
}

void Client::start(bool isBlock)
{
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "create socket error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        exit(CREATE_SOCKET_ERROR);
    }
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, remoteIP.c_str(), &servaddr.sin_addr);
    servaddr.sin_port = htons(remotePort);
    this->isBlock = isBlock;
    if (!isBlock)
    {
        int flags = fcntl(socketfd, F_GETFL, 0);
        fcntl(socketfd, F_SETFL, flags | O_NONBLOCK); // 设置为非阻塞
    }

    // 开始发起连接
    cout << "Connecting to <" << remoteIP << ":" << remotePort << ">..." << endl;
    if (connect(socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "connect error: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        close(socketfd);
        exit(CONNECT_ERROR);
    }

    // 获取本地socket信息
    socklen_t localaddrlen = sizeof(localaddr);
    if (getsockname(socketfd, (struct sockaddr *)&localaddr, &localaddrlen) < 0)
    {
        memset(errmsg, 0, sizeof(errmsg));
        snprintf(errmsg, sizeof(errmsg), "get socket name failed: %s (errno: %d)\n", strerror(errno), errno);
        cerr << errmsg;
        close(socketfd);
        exit(GET_LOCAL_ADDRESS_ERROR);
    }
    char *ipstr = inet_ntoa(localaddr.sin_addr);
    localPort = ntohs(localaddr.sin_port);
    localIP = string(ipstr);

    started = true;
    cout << "Connect Success!";
    cout << " You get IP " << localIP << " and port " << localPort << endl;
    cout << "Begin to chat..." << endl;
}

int Client::getSocketfd()
{
    return socketfd;
}

void Client::doEpoll()
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

    extern bool running;

    pid_t pid = fork();
    if (pid == 0)
    {
        close(pipefd[0]); // 关闭父进程管道读端
        char message[MAX_BUFFER_SIZE] = {0};

        while (running)
        {
            memset(message, 0, MAX_BUFFER_SIZE);
            // fgets(message, MAX_BUFFER_SIZE, stdin);
            string input;
            cin >> input;
            strcpy(message, input.c_str());
            printf("msg write to pipe: %s\n", message);

            if (write(pipefd[1], message, strlen(message)) < 0)
            {
                memset(errmsg, 0, sizeof(errmsg));
                snprintf(errmsg, sizeof(errmsg), "fork error: %s (errno: %d)\n", strerror(errno), errno);
                cerr << errmsg;
                // exit(-1);
            }
        }
        close(pipefd[1]);
    }
    else
    {
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
        if ((epollfd = epoll_create(FDSIZE)) == -1)
        {
            memset(errmsg, 0, sizeof(errmsg));
            snprintf(errmsg, sizeof(errmsg), "epoll create error: %s (errno: %d)\n", strerror(errno), errno);
            cerr << errmsg;
            close(socketfd);
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
        int wpid = waitpid(pid, nullptr, 0);
    }
}

void Client::addEvent(int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void Client::delEvent(int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

void Client::modEvent(int fd, int state)
{
    struct epoll_event ev;
    ev.events = state | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void Client::handleEvents(epoll_event *events, int num, char *buf, int &buflen)
{
    int i;
    int fd;
    // 遍历就绪事件
    for (i = 0; i < num; i++)
    {
        fd = events[i].data.fd;
        // 根据描述符的类型和事件类型进行处理
        if (events[i].events & EPOLLIN)
            doRead(fd, buf, buflen);
        else if (events[i].events & EPOLLOUT)
            doWrite(fd, buf, buflen);
        else
            close(fd);
    }
}

bool Client::doRead(int fd, char *buf, int &buflen)
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
        // 准备重新连接
        char msg[MAX_BUFFER_SIZE] = {0};
        if (fd == socketfd)
        {
            sprintf(msg, "Connect failed!\nReconnecting to <%s:%d>...\n", remoteIP.c_str(), remotePort);
            cerr << msg;
            int newfd;
            if ((newfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                memset(errmsg, 0, sizeof(errmsg));
                snprintf(errmsg, sizeof(errmsg), "create socket error: %s (errno: %d)\n", strerror(errno), errno);
                cerr << errmsg;
                clean();
                exit(CREATE_SOCKET_ERROR);
            }
            if (!isBlock)
            {
                int flags = fcntl(newfd, F_GETFL, 0);
                fcntl(newfd, F_SETFL, flags | O_NONBLOCK); // 设置为非阻塞
            }
            if (bind(newfd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0)
            {
                memset(errmsg, 0, sizeof(errmsg));
                snprintf(errmsg, sizeof(errmsg), "bind socket error: %s (errno: %d)\n", strerror(errno), errno);
                cerr << errmsg;
                clean();
                exit(BIND_SOCKET_ERROR);
            }
            while (connect(newfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                sleep(1);
            }
            socketfd = newfd;
            addEvent(socketfd, EPOLLIN);
            state = CLIENT_PREPARED;
            cout << "Connect Success!" << endl;
            cout << "Begin to chat..." << endl;
        }
        close(fd);
        delEvent(fd, EPOLLIN);
        return true;
    }
    else
    {
        // 接收来自管道读端的数据，发送给对应的socket
        if (fd == pipefd[0])
        {
            if (state == RSA_PUBKEY_RCVD)
            {
                char msg[MAX_BUFFER_SIZE] = {0};
                string res = des->getResult(string(buf), true);
                memset(plainBuf, 0, sizeof(plainBuf));
                memset(writeBuf, 0, sizeof(writeBuf));
                memcpy(plainBuf, buf, strlen(buf));
                strcpy(writeBuf, res.c_str());
                modEvent(socketfd, EPOLLOUT);
            }
            else
            {
                cerr << "The client is not ready." << endl;
            }
            return true;
        }
        if (state == CLIENT_PREPARED)
        {
            string s(buf);
            int findDelim = s.find(' ');
            string e = s.substr(0, findDelim), n = s.substr(findDelim + 1);
            RSAPublicKey publicKey(string2BigInteger(e), string2BigInteger(n));
            genRandomDESKey();
            string cipher = RSA::encry(desKey, publicKey);
            des = new DES(desKey);
            memset(writeBuf, 0, sizeof(writeBuf));
            strcpy(writeBuf, cipher.c_str());
            modEvent(fd, EPOLLOUT);
        }
        else if (state == RSA_PUBKEY_RCVD)
        {
            string res = des->getResult(string(buf), false);
            char msg[MAX_BUFFER_SIZE] = {0};
            sprintf(msg, "Received from <%s:%d>: %s\n", remoteIP.c_str(), remotePort, res.c_str());
            cout << msg;
        }
    }
    return true;
}

bool Client::doWrite(int fd, char *buf, int buflen)
{
    int nwrite;
    char msg[MAX_BUFFER_SIZE] = {0};
    if (fd == socketfd && state == RSA_PUBKEY_RCVD)
    {
        nwrite = write(fd, writeBuf, strlen(writeBuf));
        if (nwrite < 0)
        {
            memset(errmsg, 0, sizeof(errmsg));
            snprintf(errmsg, sizeof(errmsg), "message send error: %s (errno: %d)\n", strerror(errno), errno);
            cerr << errmsg;
            close(fd);
            delEvent(fd, EPOLLOUT);
            clean();
            exit(MESSAGE_SENT_ERROR);
        }
        sprintf(msg, "Send to <%s:%d>: %s\n", remoteIP.c_str(), remotePort, plainBuf);
        cout << msg;
        modEvent(fd, EPOLLIN);
    }
    else if (fd == socketfd && state == CLIENT_PREPARED)
    {
        nwrite = write(fd, writeBuf, strlen(writeBuf));
        if (nwrite < 0)
        {
            memset(errmsg, 0, sizeof(errmsg));
            snprintf(errmsg, sizeof(errmsg), "DES key send error: %s (errno: %d)\n", strerror(errno), errno);
            cerr << errmsg;
            close(fd);
            delEvent(fd, EPOLLOUT);
            clean();
            exit(DES_KEY_SENT_ERROR);
        }
        state = RSA_PUBKEY_RCVD;
        modEvent(fd, EPOLLIN);
    }
    return true;
}
