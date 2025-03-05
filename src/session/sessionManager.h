#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <unordered_map>
#include "src/session/session.h"

#include "src/common/singleton.h"
using namespace std;
using namespace fz::common;


class SessionManager {

    SINGLETON(SessionManager)

private:
    std::unordered_map<std::string, Session*> sessions; // 会话表
    SessionManager(){};
    ~SessionManager(){};
public:
    // 创建新会话
    std::string addSession(std::string sessionId, Session* session) {
        sessions[sessionId] = session;
        return sessionId;
    }

    // 查找会话
    Session* getSession(std::string sessionId) {
        if (sessions.find(sessionId) != sessions.end()) {
            return sessions[sessionId];
        }
        return nullptr;
    }

    // 删除会话
    void deleteSession(std::string sessionId) {
        if (sessions.find(sessionId) != sessions.end()) {
            delete sessions[sessionId];
            sessions.erase(sessionId);
        }
    }

    // 打印所有会话
    void printAllSessions() {
        for (auto& pair : sessions) {
            pair.second->print();
        }
    }
};
#endif