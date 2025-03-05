#ifndef FZ_TIME_H
#define FZ_TIME_H

#include <iostream>
#include <string>
namespace fz
{
    namespace utility
    {
        // local time
        std::string getLocalTime()
        {
            // 获取当前时间戳
            time_t timestamp = time(0);  // 当前时间的时间戳

            // 将时间戳转换为本地时间
            struct tm *localTime = localtime(&timestamp);

            // 格式化本地时间为字符串
            char buffer[80];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);

            // 输出格式化的时间
            std::cout << "Current local time: " << buffer << std::endl;
            // 返回局部变量，经过拷贝构造
            return std::string(buffer);  // 编译器会自动调用拷贝构造
        }
        // gmt time
        std::string getGmtTime()
        {
                // 获取当前时间戳
            time_t timestamp = time(0);

            // 将时间戳转换为 GMT 时间
            struct tm *gmtTime = gmtime(&timestamp);

            // 格式化为 HTTP 日期格式
            char buffer[80];
            strftime(buffer, sizeof(buffer), "%d %b %Y %H:%M:%S GMT", gmtTime);

            // 输出格式化的时间
            std::cout << "HTTP date: " << buffer << std::endl;
            
            // 返回局部变量，经过拷贝构造
            return std::string(buffer);  // 编译器会自动调用拷贝构造
        }
    } // namespace util
    
} // namespace fz

#endif
