
#ifndef RTSP_HANDLER_H
#define RTSP_HANDLER_H

#include "src/handler/protocol_handler.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "src/utility/logger/Logger.h"
#include "src/utility/portManager.h"
#include "src/session/sessionManager.h"
#include <src/common/clientFdManager.h>

using namespace fz::utility;

class RtspHandler : public ProtocolHandler {
    
    public:
        RtspHandler(){
            portManager_ = Singleton<PortManager>::instance();
            sessionManager_ = Singleton<SessionManager>::instance();
            clientFdManager_ = Singleton<ClientFdManager>::instance();
        };
        ~RtspHandler() = default;
    public:
    // handler request udp
    int handleRequest(int epollfd, int clientFd) override;
    int recv_message(int& size, char* buffer);
    int interact_rtsp(const int& isize, char* buffer);
    int sendAacOverUdp();
    int sendH264OverUdp();
    int sendH264AacOverUdp();
    int pause_send();
    int teardown_send();

// tcp
    int handleCmd_DESCRIBE_TCP(char* result, int CSeq, char* url);
    int handleCmd_SETUP_TCP(char* result, int CSeq);
    int build_string_tcp(char* method, char* sBuf, int CSeq, char* url);
    int interact_rtsp_tcp(const int& isize, char* buffer);
    int sendAacOverTcp();
    int sendH264OverTcp();
    int sendH264AacOverTcp();

    private:
        int build_string(char* method, char* sBuf, int CSeq, char* url,
            int clientRtpPort, int clientRtcpPort, int serverRtpSockfd
            , int serverRtcpSockfd);
        int handleCmd_OPTIONS(char* result, int CSeq);
        int handleCmd_DESCRIBE(char* result, int CSeq, char* url);
        int handleCmd_SETUP(char* result, int CSeq, int clientRtpPort, int clientRtcpPort,
            int serverRtpPort, int serverRtcpPort);
        int handleCmd_PLAY(char* result, int CSeq);
        int handleCmd_PAUSE(char* result, int CSeq);
        int handleCmd_TEARDOWN(char* result, int CSeq);
    private:
        int epollfd_;
        int clientFd_;
        std::string session_ = "";
        char clientIp_[20];
        int clientPort_;
        PortManager* portManager_;
        SessionManager* sessionManager_;
        ClientFdManager* clientFdManager_;
    
};

#endif