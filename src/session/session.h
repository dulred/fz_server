#ifndef SESSION_H
#define SESSION_H

#include <iostream>
#include <string>
#include <unordered_map>
#include "src/common/common.h"
#include <unordered_map>


struct rtsp_attr
{
    int clientRtpPort_;             // 客户端 RTP 端口
    int clientRtcpPort_;            // 客户端 RTCP 端口
    int serverRtpPort_;             // 服务器 RTP 端口
    int serverRtcpPort_;            // 服务器 RTCP 端口
    int serverRtpSockfd_;
    int serverRtcpSockfd_;
};

class Session {

public:
    Session() = default;
    Session(int clientFd, std::string clientIp,
          SessionStaus state)
    {
        clientFd_ = clientFd;
        clientIp_ = clientIp;
        state = SessionStaus::NONE;
    };
    ~Session() = default;

public:
    void print() {
        std::cout << "Client FD: " << clientFd_ << "\n"
        << "State: " << (int)state_ << "\n"
                  << "Client IP: " << clientIp_ << "\n"
                  << "video info: " << "\n"
                  << "Client RTP Port: " << info["video"].clientRtpPort_ << "\n"
                  << "Client RTCP Port: " << info["video"].clientRtcpPort_  << "\n"
                  << "Server RTP Port: " << info["video"].serverRtpPort_  << "\n"
                  << "Server RTCP Port: " << info["video"].serverRtcpPort_  << "\n"
                  
                  << "Server RTP sockfd: " << info["video"].serverRtpSockfd_ << "\n"
                  << "Server RTCP sockfd: " << info["video"].serverRtcpSockfd_ << "\n"
                  << "audio info: " << "\n"
                  << "Client RTP Port: " << info["audio"].clientRtpPort_ << "\n"
                  << "Client RTCP Port: " << info["audio"].clientRtcpPort_  << "\n"
                  << "Server RTP Port: " << info["audio"].serverRtpPort_  << "\n"
                  << "Server RTCP Port: " << info["audio"].serverRtcpPort_  << "\n"
                  
                  << "Server RTP sockfd: " << info["audio"].serverRtpSockfd_ << "\n"
                  << "Server RTCP sockfd: " << info["audio"].serverRtcpSockfd_ << "\n";
                }

    SessionStaus getState()
    {
        return state_;
    }
    void setClientFd(int clientFd)
    {
        clientFd_ = clientFd;
    }
    void setClientIp(std::string clientIp)
    {
        clientIp_ = clientIp;
    }
    void setState(SessionStaus state)
    {
        state_ = state;
    }

    void setInfo(std::string str,struct rtsp_attr attr)
    {
        info[str] = attr;
    }

    int getServerAacRtpSockfd()
    {
        return info["audio"].serverRtpSockfd_;
    }

    int getServerH264RtpSockfd()
    {
        return info["video"].serverRtpSockfd_;
    }

    int getClientAacRtpPort()
    {
        return info["audio"].clientRtpPort_;
    }

    int getClientH264RtpPort()
    {
        return info["video"].clientRtpPort_;
    }

    
private:
    int clientFd_;                  // 客户端文件描述符
    std::string clientIp_;          // 客户端 IP
    SessionStaus state_;             // 会话状态（SETUP/PLAY/TEARDOWN）
    std::unordered_map<std::string,struct rtsp_attr> info;
 
};

#endif