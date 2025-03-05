#include "src/server/epollServer.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "src/handler/protocol_factory.h"
#include <string>
#include "src/utility/ini_file/IniFile.h"
#include "src/utility/logger/Logger.h"
#include "src/session/sessionManager.h"

using namespace fz::epollServer;
using namespace fz::utility;

EpollServer::EpollServer()
    : port_(8554)
    , epollFd_(epoll_create(1)){}

int EpollServer::init()
{

        // 用于监听的 socket
        serverFd_ = initserver();
        if (serverFd_ < 0)
        {
            printf("initserver() failed.\n");
            return -1;
        }
        printf("serverFd_=%d\n", serverFd_);

        /**
         *     typedef union epoll_data {
                   void    *ptr;
                   int      fd;
                   uint32_t u32;
                   uint64_t u64;
               } epoll_data_t;
    
               struct epoll_event {
                   uint32_t     events;    // Epoll events 
                   epoll_data_t data;      // User data variable
               };
         * 
         * */
    
        struct epoll_event ev;
        ev.data.fd = serverFd_;
        // 相关的 fd 有数据可读的情况，包括新客户端的连接、客户端socket有数据可读和客户端socket断开三种情况
        ev.events = EPOLLIN;
    
        /**
         * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
         * 
         * 用于向 epfd 实例注册，修改或移除事件
         * */
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, serverFd_, &ev);
    
        while (1)
        {
    
            /**
             * int epoll_wait(int epfd, struct epoll_event *events,
                          int maxevents, int timeout);
    
             * epoll 将会阻塞，直到：
             * 1. fd 收到事件
             * 2. 调用被信号 interrupt
             * 3. 超时
             * 
             * 返回值：返回有事件发生的 fd 数量，0 表示 timeout 期间都没有事件发生，-1 error
             * */
            int readyfds = epoll_wait(epollFd_, events, MAXEVENTS, -1);
            if (readyfds == -1)
            {
                perror("epoll() failed");
                break;
            }
    
            // 检查有事情发生的socket，包括监听和客户端连接的socket。
            for (int i = 0; i < readyfds; i++)
            {
                if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                    // error case
                    printf("epoll error\n");
                    close(events[i].data.fd);
                    continue;
                }
                // 新的客户端连接
                if (events[i].data.fd == serverFd_)
                {
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
    
                    /**
                     * It extracts the first
           connection request on the queue of pending connections for the
           listening socket, sockfd, creates a new connected socket, and
           returns a new file descriptor referring to that socket.  The
           newly created socket is not in the listening state.  The original
           socket sockfd is unaffected by this call.
                     **/
    
                    // 从 pending 的连接队列中取出第一个给 serverFd_，创建一个新的已连接的 socket，并返回 fd
                    int clientsock = accept(serverFd_, (struct sockaddr *)&client, &len);
    
                    if (clientsock < 0)
                    {
                        printf("accept() failed.\n");
                        continue;
                    }
    
                    printf("client(socket=%d) connected ok.\n", clientsock);
    
                    memset(&ev, 0, sizeof(struct epoll_event));
                    ev.data.fd = clientsock;
                    ev.events = EPOLLIN;
                    epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientsock, &ev);
    
                    continue;    
                }
                else
                {
                    // my business logic is doclient function
                    handleClientRequest(events[i].data.fd);
                }
            }
        }

        return -1;
}
void EpollServer::run() {

    // read default conifg.ini
	IniFile inifile;
    inifile.load("conf/config.ini");

    port_  = inifile.get("server", "port");

    // logger ini
    Logger* log = Singleton<Logger>::instance();
    log->setFilename("log/test.log");
    log->max(10000);
    log->open("log/test.log");

    // server init
    if (init() == -1)
    {
        printf("initServer is wrong \n");
        return;
    }
    
}

int EpollServer::handleClientRequest(int clientFd)
{
    // to do objectPool
    std::unique_ptr<ProtocolHandler> aac_handler = ProtocolFactory::createHandler("RTSP");

    int res =  aac_handler->handleRequest(epollFd_,clientFd);

    if (res == -1)
    {
        printf("handlerequest error \n");
        return -1;
    }
    
    return 0;
}


// 初始化服务端的监听端口。
int EpollServer::initserver()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("socket() failed.\n");
        return -1;
    }

    int opt = 1;
    unsigned int len = sizeof(opt);

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, len);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    // INADDR_ANY is used when you don't need to bind a socket to a specific IP.
    // When you use this value as the address when calling bind(), the socket accepts connections to all the IPs of the machine.
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_);

    // When a socket is created with socket(2), it exists in a namespace (address family) but has no address assigned to it.
    // bind() assigns the address specified by addr to the socket referred to by the file descriptor sockfd.
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("bind() failed.\n");
        close(sock);
        return -1;
    }

    //  listen() marks the socket referred to by sockfd as a passive socket, that is,
    // as a socket that will be used to accept incoming connection requests using accept(2).
    if (listen(sock, 5) != 0)
    {
        printf("listen() failed.\n");
        close(sock);
        return -1;
    }

    return sock;
}
EpollServer::~EpollServer(){
    // 别忘了最后关闭 epollfd
    close(epollFd_); 
}
