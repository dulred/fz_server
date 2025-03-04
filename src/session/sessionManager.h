class SessionManager {
private:
    std::unordered_map<std::string, Session*> sessions; // 会话表

public:
    // 创建新会话
    std::string createSession(int clientFd, std::string clientIp, int rtpPort, int rtcpPort) {
        std::string sessionId = "sess_" + std::to_string(sessions.size() + 1); // 生成唯一 Session ID
        Session* session = new Session(sessionId, clientFd, clientIp, rtpPort, rtcpPort);
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
