
#ifndef RTMP_HANDLER_H
#define RTMP_HANDLER_H

#include "src/handler/protocol_handler.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>

class RtmpHandler : public ProtocolHandler {
    public:
        int handleRequest(int epollfd, int clientFd) override {

            // // 客户端有数据过来或客户端的socket连接被断开。
            // char buffer[1024];
            // memset(buffer, 0, sizeof(buffer));

            // // 读取客户端的数据。
            // ssize_t isize = read(events[i].data.fd, buffer, sizeof(buffer));
            // // 发生了错误或socket被对方关闭。
            // if (isize <= 0)
            // {
            //     printf("client(eventfd=%d) disconnected.\n", events[i].data.fd);
                
            //     memset(&ev, 0, sizeof(struct epoll_event));
            //     ev.data.fd = events[i].data.fd;
            //     ev.events = EPOLLIN;
            //     // 从 epollfd 实例中移除对该 fd 的事件监视
            //     epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
            //     close(events[i].data.fd);
            //     return -1;
            // }

            // printf("recv(eventfd=%d,size=%ld):%s\n", events[i].data.fd, isize, buffer);
            // // 把收到的报文发回给客户端。
            // write(events[i].data.fd, buffer, strlen(buffer));
        }
};
#endif