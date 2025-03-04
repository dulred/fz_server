#pragma once
#include <string>
#include <fstream>
#include "singleton.h"
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>

using namespace std;

#define g_logger fz::Singleton<fz::utility::Logger>::instance()

namespace fz {
    namespace utility{

#define debug(format, ...) \
    g_logger.log(Logger::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__);

#define info(format, ...) \
    g_logger.log(Logger::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__);

#define warn(format, ...) \
    g_logger.log(Logger::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__);

#define error(format, ...) \
    g_logger.log(Logger::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__);

#define fatal(format, ...) \
    g_logger.log(Logger::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__);

        class Logger
        {
        public:
            friend Singleton<Logger>;
            enum Level
            {
                DEBUG = 0,
                INFO,
                WARN,
                ERROR,
                FATAL,
                LEVEL_COUNT
            };
            void open(const string & filename);
            void close();
            void log(Level level, const char* file, int line, const char* format,...);
            void level(Level level)
            {
                m_level = level;
            }
            void max(int bytes)
            {
                m_max = bytes;
            }
            void setFilename(const string & filename){
                m_filename = filename;
            }
        private:
            Logger();
            ~Logger();
            void rotate();
            void process_logs();  // 后台线程的处理函数
            void stop();
        private:
            string m_filename;
            ofstream m_fout;
            Level m_level;
            int m_max;
            int m_len;
            static const char * s_level[LEVEL_COUNT];
           
            std::queue<std::string> m_log_queue;  // 日志队列
            std::mutex m_log_mutex;    // 保护日志写入队列的锁
            std::mutex m_queue_mutex;
            std::mutex m_cv_mutex;
            std::condition_variable m_condition;
            std::atomic<bool> m_running;
            std::thread m_background_thread;
        };
        

       
    }// namespace utility

} //namespace fz
