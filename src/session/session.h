#ifndef SESSION_H
#define SESSION_H

#include <iostream>
#include <string>
#include <unordered_map>
#include "src/common/common.h"
class Session {

public:
    Session() = default;
    Session(int clientFd, std::string clientIp,
         int clientRtpPort, int clientRtcpPort, int serverRtpPort, 
         int serverRtcpPort, SessionStaus state,
         int serverRtpSockfd, 
         int serverRtcpSockfd)
    {
        clientFd_ = clientFd;
        clientIp_ = clientIp;
        clientRtpPort_ = clientRtpPort;
        clientRtcpPort_ = clientRtcpPort;
        serverRtpPort_ = serverRtpPort;
        serverRtcpPort_ = serverRtcpPort;
        state = SessionStaus::NONE;
        serverRtpSockfd_ = serverRtpSockfd;
        serverRtcpSockfd_ = serverRtcpSockfd;
    };
    ~Session() = default;

public:
    void print() {
        std::cout << "Client FD: " << clientFd_ << "\n"
                  << "Client IP: " << clientIp_ << "\n"
                  << "Client RTP Port: " << clientRtpPort_ << "\n"
                  << "Client RTCP Port: " << clientRtcpPort_ << "\n"
                  << "Server RTP Port: " << serverRtpPort_ << "\n"
                  << "Server RTCP Port: " << serverRtcpPort_ << "\n"
                  << "State: " << (int)state_ << "\n"
                  << "Server RTP sockfd: " << serverRtpSockfd_ << "\n"
                  << "Server RTCP sockfd: " << serverRtcpSockfd_ << "\n";
                };

    SessionStaus getState()
    {
        return state_;
    };
    int getServerRtpPort()
    {
        return serverRtpPort_ ;
    };
    int getServerRtcpPort()
    {
        return serverRtcpPort_;
    };

    int getServerRtpSockfd()
    {
        return serverRtpSockfd_;
    };
    int getServerRtcpSockfd()
    {
        return serverRtcpSockfd_;
    };

    void setClientFd(int clientFd)
    {
        clientFd_ = clientFd;
    };
    void setClientIp(std::string clientIp)
    {
        clientIp_ = clientIp;
    };
    void setClientRtpPort(int clientRtpPort)
    {
        clientRtpPort_ = clientRtpPort;
    };
    void setClientRtcpPort(int clientRtcpPort)
    {
        clientRtcpPort_ = clientRtcpPort;
    };
    void setServerRtpPort(int serverRtpPort)
    {
        serverRtpPort_ = serverRtpPort;
    };
    void setServerRtcpPort(int serverRtcpPort)
    {
        serverRtcpPort_ = serverRtcpPort;
    };
    void setState(SessionStaus state)
    {
        state_ = state;
    };
private:
    int clientFd_;                  // 客户端文件描述符
    std::string clientIp_;          // 客户端 IP
    int clientRtpPort_;             // 客户端 RTP 端口
    int clientRtcpPort_;            // 客户端 RTCP 端口
    int serverRtpPort_;             // 服务器 RTP 端口
    int serverRtcpPort_;            // 服务器 RTCP 端口
    SessionStaus state_;             // 会话状态（SETUP/PLAY/TEARDOWN）
    int serverRtpSockfd_;
    int serverRtcpSockfd_;

 
};

#endif