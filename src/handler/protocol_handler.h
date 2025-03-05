#ifndef PROTOCOL_HANDER_H
#define PROTOCOL_HANDER_H

class ProtocolHandler {
    public:
        virtual ~ProtocolHandler() = default;
        virtual int handleRequest(int epollfd, int clientFd) = 0;
};

#endif