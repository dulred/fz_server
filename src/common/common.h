#ifndef FZ_COMMON_H
#define FZ_COMMON_H
namespace fz
{
    namespace common
    {
        enum class SessionStaus
        {   
            NONE = 0,
            PLAYING = 1,
            PAUSE = 2,
            TEARDOWN = 3,
        }; 


        #define BUF_MAX_SIZE (1024*1024)
        #define BUFFER_SIZE 650000
        #define SERVER_RTP_PORT 55532
        #define SERVER_RTCP_PORT 55533
        #define AAC_FILE_NAME "data/output.aac"
    } // namespace fz
    
} // namespace fz

#endif