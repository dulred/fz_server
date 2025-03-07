#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <unordered_map>
#include "src/common/singleton.h"
#include <mutex>
#include <iostream>

using namespace fz::common;

class ClientFdManager
{
    SINGLETON(ClientFdManager)
public:
    void setAvFlags(int clientFd, bool flag)
    {
        std::lock_guard<std::mutex> lock(my_mutex_);

        avFlags_[clientFd] = flag;
        
    };
    bool getAvFlags(int clientFd)
    {
        std::lock_guard<std::mutex> lock(my_mutex_);

        return avFlags_[clientFd];
    };

    void printf()
    {
        for (auto &it : avFlags_)
        {
            std::cout << it.first << ": " << it.second << std::endl;
        }
        
    };

    void setSessionIds(int clientFd, std::string sessionId)
    {
        std::lock_guard<std::mutex> lock(my_mutex_);

        avSessionIds_[clientFd] = sessionId;
        
    };

    std::string getSessionIds(int clientFd)
    {
        std::lock_guard<std::mutex> lock(my_mutex_);

        return avSessionIds_[clientFd];
    };

    ClientFdManager() = default;
    ~ClientFdManager() = default;
    

private:
    std::unordered_map<int,bool> avFlags_;
    std::unordered_map<int,std::string> avSessionIds_;
    std::mutex my_mutex_;
    std::string seesion_;
};

#endif