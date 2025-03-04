#include "src/handler/rtsp/rtsp_handler.h"
#include "src/common/common.h"
#include <arpa/inet.h>
#include "src/common/RtspConstants.h"
#include "src/handler/rtsp/RtspResponse.h"

using namespace fz::common;

int RtspHandler::handleRequest(int epollfd, struct epoll_event& ev, struct epoll_event events[], int i) {
    
    epollfd_ = epollfd;
    events_ = events;
    index_ = i;
    // 客户端有数据过来或客户端的socket连接被断开。
    char buffer[BUF_MAX_SIZE];
    memset(buffer, 0, sizeof(buffer));

    int isize = 0;

    // recv client message
    if (recv_message(ev,isize, buffer) == -1)
    {
        printf("recv_message error \n");
        return -1;
    }
        
    // parse and send rtsp protocol
    if (interact_rtsp(isize, buffer) == -1)
    {
        printf("interrat_rtsp_error \n");
        return -1;
    }
    
    // excute play pause teardown request
    switch (SessionStaus::NONE)
    {
        case SessionStaus::PLAYING:
            if (sendAacOverUdp() == -1)
            {
                printf("sendAacOverUdp error \n");
                return -1;
            }
            break;
        case SessionStaus::PAUSE:
            if (pause_send() == -1)
            {
                printf("pause_send error \n");
                return -1;
            }
            break;
        case SessionStaus::TEARDOWN:
            if (teardown_send() == -1)
            {
                printf("teardown_send error \n");
                return -1;
            }
            break;
        default:
            break;
    }

    return 0;
}

int RtspHandler::recv_message(struct epoll_event& ev, int& isize, char* buffer)
{

    // 读取客户端的数据。
    isize = recv(events_[index_].data.fd, buffer, BUF_MAX_SIZE ,0);
    // 发生了错误或socket被对方关闭。
    if (isize <= 0)
    {
        printf("client(eventfd=%d) disconnected.\n", events_[index_].data.fd);
        
        memset(&ev, 0, sizeof(struct epoll_event));
        ev.data.fd = events_[index_].data.fd;
        ev.events = EPOLLIN;
        // 从 epollfd 实例中移除对该 fd 的事件监视
        epoll_ctl(epollfd_, EPOLL_CTL_DEL, events_[index_].data.fd, &ev);
        close(events_[index_].data.fd);
        return -1;
    }
}

int RtspHandler::interact_rtsp(const int& isize, char* buffer)
{
    printf("recv isize: %d, data: \n%s\n",isize,buffer);

// parse and extact data

    char method[40];
    char url[100];
    char version[40];
    int CSeq = -1;
    int clientRtpPort = -1, clientRtcpPort =-1;
    int serverRtpSockfd = -1, serverRtcpSockfd = -1;
    const char* delimiter = "\n";
    char* line  = strtok(buffer, delimiter);
    
    while (line)
    {
        if (strstr(line, "OPTIONS") ||
            strstr(line, "DESCRIBE") ||
            strstr(line, "SETUP") ||
            strstr(line, "PLAY") ||
            strstr(line, "PAUSE") ||
            strstr(line, "TEARDOWN") 
            )
        {
            if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
            {
                // error
            }
            
        }
        else if (strstr(line, "CSeq"))
        {
            if (sscanf(line, "CSeq:%d\r\n", &CSeq) != 1)
            {
                // error
            }
        }
        else if (!strncmp(line,"Transport:", strlen("Transport:")))
        {
            if (sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
            &clientRtpPort, &clientRtcpPort) != 2)
            {
                // error
                printf("parse Transport error \n");
            }
        }

        line = strtok(NULL,delimiter);
        
    }
    char* sBuf = (char*)malloc(BUF_MAX_SIZE);

    // build response string  and store message in seesion
    if (build_string(method, sBuf, CSeq, url, 
        clientRtpPort, clientRtcpPort, serverRtpSockfd,
        serverRtcpSockfd) == -1)
    {
        std::string serverError = generateRtspResponse(
            RtspStatusCode::InternalServerError,
            CSeq,
            "Server: " + RTSP_SERVER_NAME +"\r\n",
            ERRPR_NOT_FOUND
        );
        // build error message
        printf("send error: %s \n",serverError.c_str());
        // send 500 
        send(events_[index_].data.fd, serverError.c_str(), serverError.length(),0);
    }else if (build_string(method, sBuf, CSeq, url, 
        clientRtpPort, clientRtcpPort, serverRtpSockfd,
        serverRtcpSockfd) == -2)
    {
        std::string serverError = generateRtspResponse(
            RtspStatusCode::NotImplemented,
            CSeq,
            "Server: MyRtspServer/1.0\r\n",
            "notImplemented error."
        );
        // build error message
        printf("send error: %s \n",serverError.c_str());
        // send 501 
        send(events_[index_].data.fd, serverError.c_str(), serverError.length(),0);
    }
    else 
    {
        printf("send success: %s \n",sBuf);

        send(events_[index_].data.fd, sBuf, strlen(sBuf),0);
    
    }
    

    return 0;
}



int createUdpSocket()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0)
    {
        return -1;
    }

    int opt = 1;
    unsigned int len = sizeof(opt);
    int size = 1024 * 1024; // 1MB buffer size

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
    // setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, len);
    // setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    // setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    
    return sockfd;
    
}

int bindSocketAddr(int sockfd, const char* ip, int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sockfd,(struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
    {
        return -1;
    }

    return 0;
    
}

int handleCmd_OPTIONS(char* result, int CSeq)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN\r\n"
        "\r\n",
        CSeq
    );
    
    return 0;
}

int handleCmd_DESCRIBE(char* result, int CSeq, char* url)
{
    char sdp[500];
    char localIp[100];
    sscanf(url, "rtsp://%[^:]:", localIp);

    sprintf(sdp, "v=0\r\n"
        "o=- 9%ld 1 IN IP4 %s\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=video 0 RTP/AVP 96\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        "a=control:track0\r\n",
        time(0),localIp);
    
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Content-Base: %s\r\n"
        "Content-type: application/sdp\r\n"
        "Content-length: %zu\r\n\r\n"
        "%s",
        CSeq,
        url,
        strlen(sdp),
        sdp
    );
    
    return 0;
}

int handleCmd_SETUP(char* result, int CSeq, int clientRtpPort, int clientRtcpPort)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
        "Session: 66334873\r\n"
        "\r\n",
        CSeq,
        clientRtpPort,
        clientRtcpPort,
        SERVER_RTP_PORT,
        SERVER_RTCP_PORT
    );
    
    return 0;
}

int handleCmd_PLAY(char* result, int CSeq)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Range: npt=0.000-\r\n"
        "Session: 66334873; timeout=10\r\n"
        "\r\n",
        CSeq
    );
    
    return 0;
}

int handleCmd_PAUSE(char* result, int CSeq)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Date: 23 Jan 1997 15:35:06 GMT\r\n"
        "\r\n",
        CSeq
    );
    
    return 0;
}

int handleCmd_TEARDOWN(char* result, int CSeq)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "\r\n",
        CSeq
    );
    
    return 0;
}

int RtspHandler::build_string(char* method, char* sBuf, int CSeq, char* url,
                int clientRtpPort, int clientRtcpPort, int serverRtpSockfd
                , int serverRtcpSockfd)
{
    // if (!strcmp(method, "OPTIONS"))
    // {
    //     if (handleCmd_OPTIONS(sBuf, CSeq))
    //     {
    //         printf("handleCmd_OPTIONS is wrong");
    //         return -1;
    //     }
        
    // }
    // else 
    if(!strcmp(method, "DESCRIBE"))
    {
        if (handleCmd_DESCRIBE(sBuf, CSeq, url))
        {
            printf("handleCmd_DESCRIBE is wrong");
            return -1;
        }
    } 
    else if(!strcmp(method, "SETUP"))
    {
        if (handleCmd_SETUP(sBuf, CSeq, clientRtpPort, clientRtcpPort))
        {
            printf("handleCmd_SETUP is wrong");
            return -1;
        }


        serverRtpSockfd = createUdpSocket();
        serverRtcpSockfd = createUdpSocket();
        if ((serverRtpSockfd < 0) || (serverRtcpSockfd < 0))
        {
            printf("create udp socket faild");
            return -1;
        }
        
       if ((bindSocketAddr(serverRtpSockfd,"0.0.0.0",SERVER_RTP_PORT) == -1) 
       || (bindSocketAddr(serverRtcpSockfd,"0.0.0.0",SERVER_RTCP_PORT) == -1))
       {
            printf("bind udp socket faild");
            return -1;
       }

    }
    else if(!strcmp(method, "PLAY"))
    {

        if (handleCmd_PLAY(sBuf, CSeq))
        {
            printf("handleCmd_PLAY is wrong");
            return -1;
        }
    }
    else if(!strcmp(method, "PAUSE"))
    {
        // is_playing_ = 0;
        if (handleCmd_PAUSE(sBuf, CSeq))
        {
            printf("handleCmd_PAUSE is wrong");
            return -1;
        }
    }
    else if(!strcmp(method, "TEARDOWN"))
    {
        // is_alive_ = 0;
        if (handleCmd_TEARDOWN(sBuf, CSeq))
        {
            printf("handleCmd_PAUSE is wrong");
            return -1;
        }
    }
    else
    {
        printf("unsupported methods \n");
        return -2;
    }

    return 0;
}



int RtspHandler::sendAacOverUdp()
{

    return 0;
}

int RtspHandler::pause_send()
{
    return 0;
}
int RtspHandler::teardown_send()
{

    return 0;
}