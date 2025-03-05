#include "src/handler/rtsp/rtsp_handler.h"
#include "src/common/common.h"
#include <arpa/inet.h>
#include "src/common/RtspConstants.h"
#include "src/handler/rtsp/RtspResponse.h"
#include "src/utility/generate_session.h"
#include "src/utility/time.h"


using namespace fz::common;

int print_peer_address(int sockfd,char* ip, int* port)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    memset(&addr, 0, sizeof(addr));

    if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) == 0)
    {
        *port = ntohs(addr.sin_port);
        strcpy(ip, inet_ntoa(addr.sin_addr));
        printf("peer IP: %s, Port: %d \n",ip,*port);
        return 0;
    }
    else
    {
        perror("getpeername \n");
        return  -1;
    }
    
}

int RtspHandler::handleRequest(int epollfd, int clientFd) {

    epollfd_ = epollfd;
    clientFd_ = clientFd;
    print_peer_address(clientFd_,clientIp_,&clientPort_);

    // 客户端有数据过来或客户端的socket连接被断开。
    char buffer[BUF_MAX_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int isize = 0;
    // recv client message
    if (recv_message(isize, buffer) == -1)
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
    if (!session_.empty())
    {
        Session* ss = sessionManager_->getSession(session_);
        switch (ss->getState())
        {
            case SessionStaus::PLAYING:
                if (sendAacOverUdp() == -1)
                {
                    printf("sendAacOverUdp error \n");
                    return -1;
                }
                // to change
                ss->print();
                portManager_->releasePorts(ss->getServerRtpPort(), ss->getServerRtcpPort());
                close(ss->getServerRtpSockfd());
                close(ss->getServerRtcpSockfd());
                
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
    }
    
   

    return 0;
}

int RtspHandler::recv_message(int& isize, char* buffer)
{
    struct epoll_event ev;

    // 读取客户端的数据。
    isize = recv(clientFd_, buffer, BUF_MAX_SIZE ,0);
    // 发生了错误或socket被对方关闭。
    if (isize <= 0)
    {
        printf("client(eventfd=%d) disconnected.\n", clientFd_);
        
        memset(&ev, 0, sizeof(struct epoll_event));
        ev.data.fd = clientFd_;
        ev.events = EPOLLIN;
        // 从 epollfd 实例中移除对该 fd 的事件监视
        epoll_ctl(epollfd_, EPOLL_CTL_DEL, clientFd_, &ev);
        close(clientFd_);
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
    char recv_session[40];
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
        else if (strstr(line, "Session"))
        {
            if (sscanf(line, "Session: %s\r\n", recv_session) != 1)
            {
                // error
            }
            session_ = recv_session;
        }

        line = strtok(NULL,delimiter);

    }
    char* sBuf = (char*)malloc(BUF_MAX_SIZE);

    // build response string  and store message in seesion
    int res = build_string(method, sBuf, CSeq, url, 
        clientRtpPort, clientRtcpPort, serverRtpSockfd,
        serverRtcpSockfd);

    if (res == -1)
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
        send(clientFd_, serverError.c_str(), serverError.length(),0);
    }else if (res == -2)
    {
        std::string serverError = generateRtspResponse(
            RtspStatusCode::NotImplemented,
            CSeq,
            "Server: MyRtspServer/1.0\r\n",
            "notImplemented error."
        );

        error("notImplemented error");
        
        // build error message
        printf("send error: %s \n",serverError.c_str());
        // send 501 
        send(clientFd_, serverError.c_str(), serverError.length(),0);
    }
    else 
    {
        printf("send success: %s \n",sBuf);
        send(clientFd_, sBuf, strlen(sBuf),0);
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

int RtspHandler::handleCmd_OPTIONS(char* result, int CSeq)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN\r\n"
        "\r\n",
        CSeq
    );
    
    return 0;
}

int RtspHandler::handleCmd_DESCRIBE(char* result, int CSeq, char* url)
{
    char sdp[500];
    char localIp[100];
    sscanf(url, "rtsp://%[^:]:", localIp);

    sprintf(sdp, "v=0\r\n"
        "o=- 9%ld 1 IN IP4 %s\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=audio 0 RTP/AVP 97\r\n"
        "a=rtpmap:97 mpeg4-generic/44100/2\r\n"
        "fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;\r\n",
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

int RtspHandler::handleCmd_SETUP(char* result, int CSeq, 
    int clientRtpPort, int clientRtcpPort, int serverRtpPort, int serverRtcpPort)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
        "Session: %s\r\n"
        "\r\n",
        CSeq,
        clientRtpPort,
        clientRtcpPort,
        serverRtpPort,
        serverRtcpPort,
        session_.c_str()
    );
    
    return 0;
}

int RtspHandler::handleCmd_PLAY(char* result, int CSeq)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %s; timeout=10\r\n"
        "\r\n",
        CSeq,
        session_.c_str()
    );
    
    return 0;
}

int RtspHandler::handleCmd_PAUSE(char* result, int CSeq)
{
    
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Date: %s\r\n"
        "\r\n",
        CSeq,
        getGmtTime().c_str()
    );
    
    return 0;
}

int RtspHandler::handleCmd_TEARDOWN(char* result, int CSeq)
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
    if (!strcmp(method, "OPTIONS"))
    {
        if (handleCmd_OPTIONS(sBuf, CSeq))
        {
            printf("handleCmd_OPTIONS is wrong");
            return -1;
        }   
    } 
    else 
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
        // generate sessionId
        session_ = generateSessionId();
        
        auto ports = portManager_->generatePorts(); // 返回 pair
        std::cout << "RTP Port: " << ports.first << ", RTCP Port: " << ports.second << std::endl;

        if (handleCmd_SETUP(sBuf, CSeq, clientRtpPort, clientRtcpPort, ports.first ,ports.second))
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
        
       if ((bindSocketAddr(serverRtpSockfd,"0.0.0.0",ports.first) == -1) 
       || (bindSocketAddr(serverRtcpSockfd,"0.0.0.0",ports.second ) == -1))
       {
            printf("bind udp socket faild");
            return -1;
       }

       // if no any error ,create session
       Session* sess = new Session(clientFd_, clientIp_, 
        clientRtpPort, clientRtcpPort, ports.first, ports.second,
         SessionStaus::NONE ,serverRtpSockfd, serverRtcpSockfd);
       sessionManager_->addSession(session_, sess);

    }
    else if(!strcmp(method, "PLAY"))
    {

        if (handleCmd_PLAY(sBuf, CSeq))
        {
            printf("handleCmd_PLAY is wrong");
            return -1;
        }
        // printf("--------------------- now play--------------");

        Session* ss = sessionManager_->getSession(session_);
        ss->setState(SessionStaus::PLAYING);
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
    // extract aac data
    FILE* fp  = fopen(AAC_FILE_NAME, "rb");
    if (!fp)
    {
        printf("open file error, %s", AAC_FILE_NAME);
        error("open file error, %s", AAC_FILE_NAME);
        return -1;
    }
    int rSize = 0;
    char* frame = (char*)malloc(BUFFER_SIZE);
    while (true)
    {
        rSize = fread(frame, 1, 7, fp);
        if (rSize <= 0)
        {
            printf("read error \n");
            break;
        }

        // extract_aac();
        printf("-----------sendAacOverUdp---------- \n");
        // packing into the rtpPacket
        // packRtpAndSend();
    }


    fclose(fp);
    free(frame);

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