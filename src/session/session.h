#include <iostream>
#include <string>
#include <unordered_map>

class Session {
public:
    std::string sessionId;          // 会话 ID
    int clientFd;                  // 客户端文件描述符
    std::string clientIp;          // 客户端 IP
    int clientRtpPort;             // 客户端 RTP 端口
    int clientRtcpPort;            // 客户端 RTCP 端口
    int serverRtpPort;             // 服务器 RTP 端口
    int serverRtcpPort;            // 服务器 RTCP 端口
    std::string state;             // 会话状态（SETUP/PLAY/TEARDOWN）

    Session(std::string id, int fd, std::string ip, int rtpPort, int rtcpPort)
        : sessionId(id), clientFd(fd), clientIp(ip), clientRtpPort(rtpPort), clientRtcpPort(rtcpPort), state("INIT") {
        // 初始化服务器端口（假设随机分配）
        serverRtpPort = 6000;  // 实际应用中需要动态分配
        serverRtcpPort = 6001; // 实际应用中需要动态分配
    }

    void print() {
        std::cout << "Session ID: " << sessionId << "\n"
                  << "Client FD: " << clientFd << "\n"
                  << "Client IP: " << clientIp << "\n"
                  << "Client RTP Port: " << clientRtpPort << "\n"
                  << "Client RTCP Port: " << clientRtcpPort << "\n"
                  << "Server RTP Port: " << serverRtpPort << "\n"
                  << "Server RTCP Port: " << serverRtcpPort << "\n"
                  << "State: " << state << "\n";
    }
};
