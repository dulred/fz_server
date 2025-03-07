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
        #define AAC_BUFFER_SIZE 50000
        #define H264_FILE_BUFFER 650000
        #define SERVER_RTP_PORT 55532
        #define SERVER_RTCP_PORT 55533
        #define AAC_FILE_NAME "data/output.aac"
        #define H264_FILE_NAME "data/test.h264"
        #define RTP_VERSION 2
        #define RTP_HEADER_SIZE 12
        #define RTP_MAX_PKT_SIZE 1400

    } // namespace fz
    
} // namespace fz

#endif