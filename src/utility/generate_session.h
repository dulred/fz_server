#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <array>

// 生成指定长度的随机字节
std::string generateRandomBytes(size_t length) {
    std::random_device rd;  // 用于生成随机种子
    std::uniform_int_distribution<uint8_t> dist(0, 255); // 生成 0-255 的随机数

    std::string randomBytes;
    randomBytes.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        randomBytes.push_back(static_cast<char>(dist(rd)));
    }

    return randomBytes;
}

// 将字节数组转换为十六进制字符串
std::string bytesToHex(const std::string& bytes) {
    std::ostringstream oss;
    for (uint8_t byte : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

// 生成 Session ID
std::string generateSessionId() {
    const size_t sessionIdLength = 16; // Session ID 的长度（字节数）
    std::string randomBytes = generateRandomBytes(sessionIdLength);
    return bytesToHex(randomBytes); // 转换为十六进制字符串
}

// int main() {
//     // 生成并输出 Session ID
//     std::string sessionId = generateSessionId();
//     std::cout << "Generated Session ID: " << sessionId << std::endl;

//     return 0;
// }
