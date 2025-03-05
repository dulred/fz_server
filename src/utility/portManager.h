#include <iostream>
#include <unordered_set>
#include <random>
#include <mutex>
#include <system_error>
#include <arpa/inet.h> // 用于 socket 和 bind
#include <unistd.h>
#include "src/common/singleton.h"
using namespace fz::common;

class PortManager {
    SINGLETON(PortManager)
public:
    PortManager() : rng(std::random_device{}()), dist(55000, 65534) {}
    ~PortManager() = default;
    // 生成一对 RTP/RTCP 端口
    std::pair<uint16_t, uint16_t> generatePorts() {
        std::lock_guard<std::mutex> lock(mutex); // 加锁保证线程安全

        while (true) {
            // 生成一个偶数端口
            uint16_t rtpPort = dist(rng) & ~1; // 确保是偶数
            uint16_t rtcpPort = rtpPort + 1;

            // 检查端口是否可用
            if (isPortAvailable(rtpPort) && isPortAvailable(rtcpPort)) {
                // 记录已分配的端口
                allocatedPorts.insert(rtpPort);
                allocatedPorts.insert(rtcpPort);
                return std::make_pair(rtpPort, rtcpPort);
            }
        }
    }

    // 释放端口
    void releasePorts(uint16_t rtpPort, uint16_t rtcpPort) {
        std::lock_guard<std::mutex> lock(mutex); // 加锁保证线程安全
        allocatedPorts.erase(rtpPort);
        allocatedPorts.erase(rtcpPort);
    }

private:
    // 检查端口是否可用
    bool isPortAvailable(uint16_t port) {
        if (allocatedPorts.count(port) > 0) {
            return false; // 端口已分配
        }

        // 尝试绑定端口，检查是否被占用
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to create socket");
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
            close(sock);
            return false; // 端口被占用
        }

        close(sock);
        return true; // 端口可用
    }

    std::unordered_set<uint16_t> allocatedPorts; // 已分配的端口
    std::mt19937 rng;                           // 随机数生成器
    std::uniform_int_distribution<uint16_t> dist; // 端口范围
    std::mutex mutex;                           // 互斥锁
};

// int main() {
    // PortManager portManager;
    // auto ports = portManager->generatePorts(); // 返回 pair
    // std::cout << "RTP Port: " << ports.first << ", RTCP Port: " << ports.second << std::endl;
    // return 0;
// }
