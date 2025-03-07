#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <unordered_map>
#include <mutex>
#include "src/session/session.h"

#include "src/common/singleton.h"
using namespace std;
using namespace fz::common;


class SessionManager {

    SINGLETON(SessionManager)

private:
    std::unordered_map<std::string, Session*> sessions; // 会话表
    std::mutex sessionMutex;
    SessionManager(){};
    ~SessionManager(){};
public:
    // 创建新会话
    std::string addSession(std::string sessionId, Session* session) {
        std::lock_guard<mutex> lock(sessionMutex);
        sessions[sessionId] = session;
        return sessionId;
    }

    // 查找会话
    Session* getSession(std::string sessionId) {
        std::lock_guard<mutex> lock(sessionMutex);
        if (sessions.find(sessionId) != sessions.end()) {
            return sessions[sessionId];
        }
        return nullptr;
    }

    // 删除会话
    void deleteSession(std::string sessionId) {
        std::lock_guard<mutex> lock(sessionMutex);
        if (sessions.find(sessionId) != sessions.end()) {
            delete sessions[sessionId];
            sessions.erase(sessionId);
        }
    }

    // 打印所有会话
    void printAllSessions() {
        std::lock_guard<mutex> lock(sessionMutex);
        for (auto& pair : sessions) {
            pair.second->print();
        }
    }
};
#endif