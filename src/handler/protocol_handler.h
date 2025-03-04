#ifndef PROTOCOL_HANDER_H
#define PROTOCOL_HANDER_H

class ProtocolHandler {
    public:
        virtual ~ProtocolHandler() = default;
        virtual int handleRequest(int epollfd, struct epoll_event& ev, struct epoll_event events[], int i) = 0;
};

#endif