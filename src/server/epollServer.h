#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H


#include <stdio.h>
#include <sys/epoll.h>
#include <memory>

#include "src/handler/protocol_handler.h"


#include "src/common/singleton.h"

using namespace fz::common;
#define MAXEVENTS 1024

namespace fz
{
    namespace epollServer
    {
	class EpollServer {

        SINGLETON(EpollServer)

    private:
        EpollServer();
        ~EpollServer();
    public:
        void run();

    private:
        int initserver();
        int init();
        int handleClientRequest(int clientFd);
    private:
        // 用于存放有事件发生的数组
        struct epoll_event events[MAXEVENTS];
        int port_;
        int serverFd_;
        int epollFd_;
};        
    } // namespace 
    
} // namespace fz

#endif