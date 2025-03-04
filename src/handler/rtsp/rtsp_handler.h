
#ifndef RTSP_HANDLER_H
#define RTSP_HANDLER_H

#include "src/handler/protocol_handler.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>

class RtspHandler : public ProtocolHandler {
    public:
    // handler request
    int handleRequest(int epollfd, struct epoll_event& ev, 
        struct epoll_event events[], int i) override;
    int recv_message(struct epoll_event& ev, int& size, char* buffer);
    int interact_rtsp(const int& isize, char* buffer);
    int sendAacOverUdp();
    int pause_send();
    int teardown_send();
    private:
        int handleCmd_OPTIONS();

        int build_string(char* method, char* sBuf, int CSeq, char* url,
            int clientRtpPort, int clientRtcpPort, int serverRtpSockfd
            , int serverRtcpSockfd);

    private:
        int epollfd_;
        struct epoll_event* events_;
        int index_;
    
};

#endif