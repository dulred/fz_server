#include "Logger.h"
#include <string.h>
#include <iostream>
#include <stdarg.h>
#include <sstream>
using namespace fz::utility;

Logger::Logger() : m_level(DEBUG),m_max(0),m_len(0),m_running(true), m_background_thread(&Logger::process_logs, this) 
{
}

Logger::~Logger()
{
    stop();
    close();
}


const char * Logger::s_level[LEVEL_COUNT] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

void Logger::open(const string & filename)
{
        m_fout.open(filename, ios::app);
        if (m_fout.fail())
        {
            throw std::logic_error("open file failed11" + filename);
        }

        m_fout.seekp(0, ios::end);
        m_len = m_fout.tellp();
        
}

void Logger::close()
{
    m_fout.close();
}

void Logger::log(Level level, const char *file, int line, const char *format, ...)
{

    // 锁定互斥量，确保同一时间只有一个线程可以进入此代码块
    if (m_level > level)
    {
        return;
    }
    
    // 获取时间戳操作局部加锁，确保线程安全
    std::ostringstream oss;
    string log_entry;
    {

        std::lock_guard<std::mutex> lock(m_log_mutex);

        //get local format time 
        time_t ticks = time(NULL);
        struct tm * ptm = localtime(&ticks);
        char timestamp[32];
        memset(timestamp, 0, sizeof(timestamp));
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",ptm);

        //
        const char* fmt  = "%s %s %s:%d ";
        int size = snprintf(NULL, 0, fmt, timestamp, s_level[level], file, line);
        if (size > 0)
        {
            char* buffer = new char[size + 1];
            snprintf(buffer, size + 1, fmt, timestamp, s_level[level], file, line);
            buffer[size] = 0;
            // std::cout << buffer << std::endl;
            // m_fout << buffer;
            oss << buffer;
            m_len += size;
            delete[] buffer;
        }

        va_list arg_ptr;
        va_start(arg_ptr, format);
        size = vsnprintf(NULL, 0, format, arg_ptr);
        va_end(arg_ptr);
        if (size > 0)
        {
            char* content = new char[size + 1];
            va_start(arg_ptr, format);
            vsnprintf(content, size + 1, format, arg_ptr);
            va_end(arg_ptr);
            //    std::cout << content << std::endl;
            // m_fout << content;

            oss << content << "\n";
            log_entry = oss.str();
            delete[] content;
        }

            
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_log_queue.push(log_entry);
        }

    }

    

    m_condition.notify_one();  // 通知后台线程处理日志
  
}

void Logger::rotate()
{
    close();
    //get local format time 
    time_t ticks = time(NULL);
    struct tm * ptm = localtime(&ticks);
    char timestamp[32];
    memset(timestamp, 0, sizeof(timestamp));
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H_%M_%S",ptm);


    // 找到 ".log" 后缀的位置
    size_t pos = m_filename.find(".log");
    string filename = m_filename;
    // 如果找到了 ".log" 后缀
    if (pos != std::string::npos) {
        // 将时间戳插入到 ".log" 前面
        filename.insert(pos, timestamp);
    } else {
        std::cout << "No .log suffix found in filename!" << std::endl;
    }

    open(filename);
    
}

void Logger::process_logs()
{
    while (m_running || !m_log_queue.empty()) {
        std::unique_lock<std::mutex> lock(m_cv_mutex);
        m_condition.wait(lock, [this] { return !m_log_queue.empty() || !m_running; });

        while (!m_log_queue.empty()) {
            std::string log_entry;

            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                log_entry = m_log_queue.front();
                m_log_queue.pop();
            }
          
           
            if (m_fout.fail())
            {
                throw std::logic_error("open file failed " + m_filename);
            }
            // 写入文件
            m_fout << log_entry;
            m_fout.flush();
            if (m_len >= m_max && m_max > 0)
            {
                rotate();
            }
        }
    }
}

void Logger::stop()
{
    m_running = false;
    m_condition.notify_all();
    if (m_background_thread.joinable()) {
        m_background_thread.join();
    }
}
