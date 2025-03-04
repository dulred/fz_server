/**
 * 相较于 select，poll 的优点
 * 1. 内核中保存一份文件描述符集合，无需用户每次都重新传入，只需告诉内核修改的部分
 * 2. 不再通过轮询的的方式找到就绪的 fd，而是通过异步 IO 事件唤醒 epoll_wait
 * 3. 内核仅会将有事件发生的 fd 返回给用户，用户无需遍历整个 fd 集合
 * */

 #include <stdio.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <string.h>
 #include <errno.h>
 #include <sys/epoll.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <sys/fcntl.h>
 #include <sys/types.h>
#include <time.h>
#include <thread>
#include "rtp/rtp.h"

#define MAXEVENTS 1024

#define H264_FILE_NAME "data/test.h264"
#define SERVER_PORT 8554
#define SERVER_RTP_PORT 55532
#define SERVER_RTCP_PORT 55533
#define BUF_MAX_SIZE (1024*1024)
#define MALLOC_FILE_BUFFER 650000
 
#ifdef _WIN32
    // windows
#elif defined(__linux__)
    // linux
#else
    // other plateform
#endif

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

int handleCmd_SETUP(char* result, int CSeq, int clientRtpPort)
{
    sprintf(result,"RTSP/1.0 200 OK \r\n"
        "CSeq: %d\r\n"
        "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
        "Session: 66334873\r\n"
        "\r\n",
        CSeq,
        clientRtpPort,
        clientRtpPort + 1,
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

bool startCode3(char* rec)
{
    if (rec[0] == 0x00 && rec[1] == 0x00 && rec[2] == 0x01)
    {
        return true;
    }

    return false;
}

bool startCode4(char* rec)
{
    if (rec[0] == 0x00 && rec[1] == 0x00 && rec[2] == 0x00 && rec[3] == 0x01)
    {
        return true;
    }

    return false;
}

int extract_content(char *rec, FILE* fp, int& filesize, const int& size)
{
    int step;
    if (startCode3(rec))
    {
        step = 3;
        // printf("step is 3 \n",step);
    }
    else if (startCode4(rec))
    {   
        step = 4;
        // printf("step is 4 \n",step);
    }
    else
    {
        return -1;
    }

    int startNum = step + 1;
    rec = rec + step;
    // printf("%x \n",rec[0]);
    int done = 0;
    for (int i = 0; i < size - 3; i++)
    {
        // oversize problem
        if (startCode3(rec) || startCode4(rec))
        {
            done = 1;
            break;
        }
        startNum++;
        rec = rec + 1;
    }


    if (!done)
    {
        return -1;
    }
    else
    {
        filesize = startNum - 1;
        fseek(fp,filesize - size, SEEK_CUR);
        return 0;
    }

}

int sendOverUdp(RtpPacket* rtpPacket,int serverRtpSockfd, char* ip, int& port, int filesize)
{

    struct  sockaddr_in addr;
    int ret;

    memset(&addr, 0, sizeof(addr));

    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

    ret = sendto(serverRtpSockfd, (char*)rtpPacket, filesize + RTP_HEADER_SIZE, 0,
        (struct sockaddr*)&addr, sizeof(addr));


    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);


    return ret;

}

int sendManager(RtpPacket* rtpPacket,int serverRtpSockfd,char* rec, int& filesize, char* ip, int& port)
{
    uint8_t naluType;
    int sendBytes = 0;
    int ret;

    naluType = rec[0];
    
    printf("framsize = %d \n", filesize);
    // printf("%hhx %hhx %hhx %hhx %hhx \n", rec[0], rec[1], rec[2], rec[3], rec[4]);
    // rtppack encapsulation  1400 
    
    // single merge split three ways

    // single
    if (filesize <= RTP_MAX_PKT_SIZE)
    {

        memcpy(rtpPacket->payload, rec, filesize);
        ret = sendOverUdp(rtpPacket, serverRtpSockfd, ip, port, filesize);
        if (ret < 0)
        {
            return -1;
        }
        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
        if ((naluType & 0x1F) == 6  ||  (naluType & 0x1F) == 7  || (naluType & 0x1F) == 8 )
        {
            goto out;
        }       

    }
    else
    {
        //  size of split packet
        int  split_size = filesize / RTP_MAX_PKT_SIZE;
        //  size of remain
        int  remain_size = filesize % RTP_MAX_PKT_SIZE;

        int i, pos = 1;

        for (i = 0; i < split_size; i++)
        {

            // divison two park ,one is head ,other is middle part
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            
            if (i == 0)
            { 
                rtpPacket->payload[1] = (naluType & 0x1F) | 0x80;
            }
            else
            if ((remain_size == 0) && (i == split_size - 1))
            {
                rtpPacket->payload[1]  = (naluType & 0x1F) | 0x40;
             
            }else
            {
                rtpPacket->payload[1] = naluType & 0x1F;
            }

            memcpy(rtpPacket->payload + 2, rec + pos, RTP_MAX_PKT_SIZE);
            

            ret = sendOverUdp(rtpPacket, serverRtpSockfd, ip, port, RTP_MAX_PKT_SIZE + 2);
            if (ret < 0)
            {
               return -1;
            }
            sendBytes += ret;
            rtpPacket->rtpHeader.seq++;

            pos += RTP_MAX_PKT_SIZE;
        }

        if (remain_size > 0)
        {
             // end part
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = (naluType & 0x1F) | 0x40;

            memcpy(rtpPacket->payload + 2, rec + pos, remain_size);

            ret = sendOverUdp(rtpPacket, serverRtpSockfd, ip, port, remain_size + 2);
            if (ret < 0)
            {
               return -1;
            }
            sendBytes += ret;
            rtpPacket->rtpHeader.seq++;
        }
        
    }

    rtpPacket->rtpHeader.timestamp += 90000 / 30;

    usleep(20000);
out:
    return sendBytes;
}

int initTcpServer(int port);
int serverRtpSockfd = -1, serverRtcpSockfd = -1;
int doclient(int epollfd, struct epoll_event& ev, struct epoll_event events[MAXEVENTS], int i){
    
    char method[40];
    char url[100];
    char version [40];
    int CSeq;
    int is_playing_ = 0;
    int is_alive_ = 0;


    int clientRtpPort, clientRtcpPort;
    char* rBuf = (char*)malloc(BUF_MAX_SIZE);
    char* sBuf = (char*)malloc(BUF_MAX_SIZE);

    char ip[INET_ADDRSTRLEN];
    int clientPort;
    if (print_peer_address(events[i].data.fd, ip, &clientPort) < 0)
    {
        printf("parse client ip and port failed \n");
        return -1;
    }
    // 客户端有数据过来或客户端的socket连接被断开。
    //   char buffer[1024];
    //  memset(buffer, 0, sizeof(buffer));
        

    // 读取客户端的数据。
    ssize_t isize = recv(events[i].data.fd, rBuf, BUF_MAX_SIZE,0);
    // 发生了错误或socket被对方关闭。
    if (isize <= 0)
    {
        printf("client(eventfd=%d) disconnected.\n", events[i].data.fd);
        
        memset(&ev, 0, sizeof(struct epoll_event));
        ev.data.fd = events[i].data.fd;
        ev.events = EPOLLIN;
        // 从 epollfd 实例中移除对该 fd 的事件监视
        epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
        close(events[i].data.fd);
        return -1;
    }
    rBuf[isize] = '\0';

    printf("-----recv(eventfd=%d,size=%ld):-----\n%s\n", events[i].data.fd, isize, rBuf);
    
    const char* sep = "\n";
    char* line = strtok(rBuf,sep);
    
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

        line = strtok(NULL,sep);
        
    }
    
    if (!strcmp(method, "OPTIONS"))
    {
        if (handleCmd_OPTIONS(sBuf, CSeq))
        {
            printf("handleCmd_OPTIONS is wrong");
            return -1;
        }
        
    }
    else if(!strcmp(method, "DESCRIBE"))
    {
        if (handleCmd_DESCRIBE(sBuf, CSeq, url))
        {
            printf("handleCmd_DESCRIBE is wrong");
            return -1;
        }
    } 
    else if(!strcmp(method, "SETUP"))
    {
        if (handleCmd_SETUP(sBuf, CSeq, clientRtpPort))
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
        is_playing_ = 0;
        if (handleCmd_PAUSE(sBuf, CSeq))
        {
            printf("handleCmd_PAUSE is wrong");
            return -1;
        }
    }
    else if(!strcmp(method, "TEARDOWN"))
    {
        is_alive_ = 0;
        if (handleCmd_TEARDOWN(sBuf, CSeq))
        {
            printf("handleCmd_PAUSE is wrong");
            return -1;
        }
    }
    else
    {
        printf("unsupported methods \n");
        return -1;
    }
    
    printf("-----sent(eventfd=%d,size=%ld):-----\n%s\n", events[i].data.fd, strlen(sBuf), sBuf);
    
    // 把收到的报文发回给客户端。
    send(events[i].data.fd, sBuf, strlen(sBuf),0);

    
    if (!strcmp(method, "PLAY"))
    {   

        
        FILE* fp = fopen(H264_FILE_NAME,"rb");

        if (fp == nullptr)
        {
            printf("failed to open file \n");
            return -1;
        }
        char* rec = (char*)malloc(MALLOC_FILE_BUFFER);
        
        // fseek(fp, 0, SEEK_END);
        // long len = ftell(fp);
        // fseek(fp, 0, SEEK_CUR);
        // printf("len: %d \n", len);
        // sleep(10);
        size_t size = 0;
        int filesize = 0;
        struct RtpPacket* rtpPacket = (struct RtpPacket*)malloc(MALLOC_FILE_BUFFER);

        rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0, 
            0, 0, 0X88923423);
        

        while ( (size = fread(rec,1,MALLOC_FILE_BUFFER, fp)) > 0)
        {
            // printf("%p \n",rec);
            // printf("not find %hhx %hhx %hhx %hhx %hhx %hhx\n", rec[0], rec[1], rec[2], rec[3], rec[4], rec[5]);
        
            if (extract_content(rec, fp, filesize, size))
            {
                printf("something is wrong0000 \n");
                break;
            }

            int step = 0;
            
            if (startCode3(rec))
            {
               step = 3;
            }

            if (startCode4(rec))
            {
                step = 4;
            }
            
            filesize = filesize - step;
            if (sendManager(rtpPacket, serverRtpSockfd, rec + step, filesize, ip, clientRtpPort) == -1)
            {
                printf("something is wrong111 \n");
                break;
            }

        }


        // is_playing_ = 1;
        // is_alive_ = 1;
 
        // while (is_playing_ && is_alive_)
        // {

        //     printf("areyouok? %d\n", is_playing_);
        //     sleep(2);
        // }
        
        // printf("being a pause state\n");
        free(rtpPacket);
        free(rec);
    }


    // if (!is_alive_)
    // {
    //     printf("teardown\n");
    // }
    

    // memset(method, 0, sizeof(method));
    // memset(url, 0, sizeof(url));
    // CSeq = 0;

    free(rBuf);
    free(sBuf);
    // if (serverRtpSockfd)
    // {
    //     close(serverRtpSockfd);
    // }
    // if (serverRtcpSockfd)
    // {
    //     close(serverRtcpSockfd);
    // }
    
    
    return 1;
 }
 
 int main(int argc, char *argv[])
 {
     if (argc != 2)
     {
         printf("usage: ./epollserverdemo port\n");
         return -1;
     }
 
     // 用于监听的 socket
     int listensock = initTcpServer(atoi(argv[1]));
     if (listensock < 0)
     {
         printf("initserver() failed.\n");
         return -1;
     }
     printf("listensock=%d\n", listensock);
 
     // 创建一个新的 epoll 实例，返回对应的 fd，参数无用，大于0即可
     int epollfd = epoll_create(1);
 
     /**
      *     typedef union epoll_data {
                void    *ptr;
                int      fd;
                uint32_t u32;
                uint64_t u64;
            } epoll_data_t;
 
            struct epoll_event {
                uint32_t     events;    // Epoll events 
                epoll_data_t data;      // User data variable
            };
      * 
      * */
 
     struct epoll_event ev;
     ev.data.fd = listensock;
     // 相关的 fd 有数据可读的情况，包括新客户端的连接、客户端socket有数据可读和客户端socket断开三种情况
     ev.events = EPOLLIN;
 
     /**
      * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
      * 
      * 用于向 epfd 实例注册，修改或移除事件
      * */
     epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev);
 
     while (1)
     {
         // 用于存放有事件发生的数组
         struct epoll_event events[MAXEVENTS];
 
         /**
          * int epoll_wait(int epfd, struct epoll_event *events,
                       int maxevents, int timeout);
 
          * epoll 将会阻塞，直到：
          * 1. fd 收到事件
          * 2. 调用被信号 interrupt
          * 3. 超时
          * 
          * 返回值：返回有事件发生的 fd 数量，0 表示 timeout 期间都没有事件发生，-1 error
          * */
         int readyfds = epoll_wait(epollfd, events, MAXEVENTS, -1);
         if (readyfds == -1)
         {
             perror("epoll() failed");
             break;
         }
 
         // 检查有事情发生的socket，包括监听和客户端连接的socket。
         for (int i = 0; i < readyfds; i++)
         {
             if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                 (!(events[i].events & EPOLLIN))) {
                 // error case
                 printf("epoll error\n");
                 close(events[i].data.fd);
                 continue;
             }
             // 新的客户端连接
             if (events[i].data.fd == listensock)
             {
                 struct sockaddr_in client;
                 socklen_t len = sizeof(client);
 
                 /**
                  * It extracts the first
        connection request on the queue of pending connections for the
        listening socket, sockfd, creates a new connected socket, and
        returns a new file descriptor referring to that socket.  The
        newly created socket is not in the listening state.  The original
        socket sockfd is unaffected by this call.
                  **/
 
                 // 从 pending 的连接队列中取出第一个给 listensock，创建一个新的已连接的 socket，并返回 fd
                 int clientsock = accept(listensock, (struct sockaddr *)&client, &len);
 
                 if (clientsock < 0)
                 {
                     printf("accept() failed.\n");
                     continue;
                 }
 
                 printf("client(socket=%d) connected ok.\n", clientsock);
 
                 memset(&ev, 0, sizeof(struct epoll_event));
                 ev.data.fd = clientsock;
                 ev.events = EPOLLIN;
                 epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev);
 
                 continue;    
             }
             else
             {
                 // my business logic is doclient function
                 if (!doclient(epollfd,ev,events,i))
                 { 
                     continue;
                 }
                 
             }
         }
     }
 
     // 别忘了最后关闭 epollfd
     close(epollfd); 
 
     return 0;
 }
 
 // 初始化服务端的监听端口。
 int initTcpServer(int port)
 {
     int sock = socket(AF_INET, SOCK_STREAM, 0);
     if (sock < 0)
     {
         printf("socket() failed.\n");
         return -1;
     }
 
     int opt = 1;
     unsigned int len = sizeof(opt);
     int size = 1024 * 1024; // 1MB buffer size
 
     setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);
     setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, len);
     setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
     setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
 
     struct sockaddr_in servaddr;
     servaddr.sin_family = AF_INET;
     // INADDR_ANY is used when you don't need to bind a socket to a specific IP.
     // When you use this value as the address when calling bind(), the socket accepts connections to all the IPs of the machine.
     servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
     servaddr.sin_port = htons(port);
 
     // When a socket is created with socket(2), it exists in a namespace (address family) but has no address assigned to it.
     // bind() assigns the address specified by addr to the socket referred to by the file descriptor sockfd.
     if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
     {
         printf("bind() failed.\n");
         close(sock);
         return -1;
     }
 
     //  listen() marks the socket referred to by sockfd as a passive socket, that is,
     // as a socket that will be used to accept incoming connection requests using accept(2).
     if (listen(sock, 5) != 0)
     {
         printf("listen() failed.\n");
         close(sock);
         return -1;
     }
 
     return sock;
 }