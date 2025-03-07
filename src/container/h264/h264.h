#ifndef H264_H
#define H264_H
#include "src/handler/rtp/rtp.h"
#include <cstdio>
#include <string.h>
#include <netinet/in.h>
#include <bits/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

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
int sendOverTcp(int clientFd, RtpPacket* rtpPacket,int filesize,char channel)
{
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

    uint32_t rtpSize = RTP_HEADER_SIZE + filesize;
    char* tempBuf = (char*)malloc(4 + rtpSize);
    tempBuf[0] = 0x24;//$
    tempBuf[1] = channel;
    tempBuf[2] = (uint8_t)(((rtpSize) & 0xFF00) >> 8);
    tempBuf[3] = (uint8_t)((rtpSize) & 0xFF);
    memcpy(tempBuf + 4, (char*)rtpPacket, rtpSize);

    int ret = send(clientFd, tempBuf, 4 + rtpSize, 0);

    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

    free(tempBuf);
    tempBuf = NULL;

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

int sendManagerTcp(int clientFd,RtpPacket* rtpPacket,char* rec, int& filesize)
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
        ret = sendOverTcp(clientFd, rtpPacket, filesize, 0x00);
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
            

            ret = sendOverTcp(clientFd,rtpPacket, RTP_MAX_PKT_SIZE + 2, 0x00);
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

            ret = sendOverTcp(clientFd,rtpPacket, remain_size + 2, 0x00);
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

#endif