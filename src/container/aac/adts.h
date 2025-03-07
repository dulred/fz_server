
#include <cstdint>
#include <string.h>
#include <stdio.h>
#include "src/handler/rtp/rtp.h"

struct AdtsHeader {
    unsigned int syncword;  //12 bit 同步字 '1111 1111 1111'，一个ADTS帧的开始
    uint8_t id;        //1 bit 0代表MPEG-4, 1代表MPEG-2。
    uint8_t layer;     //2 bit 必须为0
    uint8_t protectionAbsent;  //1 bit 1代表没有CRC，0代表有CRC
    uint8_t profile;           //2 bit AAC级别（MPEG-2 AAC中定义了3种profile，MPEG-4 AAC中定义了6种profile）
    uint8_t samplingFreqIndex; //4 bit 采样率
    uint8_t privateBit;        //1bit 编码时设置为0，解码时忽略
    uint8_t channelCfg;        //3 bit 声道数量
    uint8_t originalCopy;      //1bit 编码时设置为0，解码时忽略
    uint8_t home;               //1 bit 编码时设置为0，解码时忽略

    uint8_t copyrightIdentificationBit;   //1 bit 编码时设置为0，解码时忽略
    uint8_t copyrightIdentificationStart; //1 bit 编码时设置为0，解码时忽略
    unsigned int aacFrameLength;               //13 bit 一个ADTS帧的长度包括ADTS头和AAC原始流
    unsigned int adtsBufferFullness;           //11 bit 缓冲区充满度，0x7FF说明是码率可变的码流，不需要此字段。CBR可能需要此字段，不同编码器使用情况不同。这个在使用音频编码的时候需要注意。

    /* number_of_raw_data_blocks_in_frame
     * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
     * 所以说number_of_raw_data_blocks_in_frame == 0
     * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
     */
    uint8_t numberOfRawDataBlockInFrame; //2 bit
};

static int parseAdtsHeader(uint8_t* in, struct AdtsHeader* res) {
    static int frame_number = 0;
    memset(res, 0, sizeof(*res));

    if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0))
    {
        res->id = ((uint8_t)in[1] & 0x08) >> 3;//第二个字节与0x08与运算之后，获得第13位bit对应的值
        res->layer = ((uint8_t)in[1] & 0x06) >> 1;//第二个字节与0x06与运算之后，右移1位，获得第14,15位两个bit对应的值
        res->protectionAbsent = (uint8_t)in[1] & 0x01;
        res->profile = ((uint8_t)in[2] & 0xc0) >> 6;
        res->samplingFreqIndex = ((uint8_t)in[2] & 0x3c) >> 2;
        res->privateBit = ((uint8_t)in[2] & 0x02) >> 1;
        res->channelCfg = ((((uint8_t)in[2] & 0x01) << 2) | (((unsigned int)in[3] & 0xc0) >> 6));
        res->originalCopy = ((uint8_t)in[3] & 0x20) >> 5;
        res->home = ((uint8_t)in[3] & 0x10) >> 4;
        res->copyrightIdentificationBit = ((uint8_t)in[3] & 0x08) >> 3;
        res->copyrightIdentificationStart = (uint8_t)in[3] & 0x04 >> 2;

        res->aacFrameLength = (((((unsigned int)in[3]) & 0x03) << 11) |
            (((unsigned int)in[4] & 0xFF) << 3) |
            ((unsigned int)in[5] & 0xE0) >> 5);

        res->adtsBufferFullness = (((unsigned int)in[5] & 0x1f) << 6 |
            ((unsigned int)in[6] & 0xfc) >> 2);
        res->numberOfRawDataBlockInFrame = ((uint8_t)in[6] & 0x03);

        return 0;
    }
    else
    {
        printf("failed to parse adts header\n");
        return -1;
    }
}

static int rtpSendAACFrame(int socket, const char* ip, int16_t port,
    struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize) {
    //打包文档：https://blog.csdn.net/yangguoyu8023/article/details/106517251/
    int ret;

    rtpPacket->payload[0] = 0x00;
    rtpPacket->payload[1] = 0x10;
    rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5; //高8位
    rtpPacket->payload[3] = (frameSize & 0x1F) << 3; //低5位

    memcpy(rtpPacket->payload + 4, frame, frameSize);

    ret = rtpSendPacketOverUdp(socket, ip, port, rtpPacket, frameSize + 4);
    if (ret < 0)
    {
        printf("failed to send rtp packet\n");
        return -1;
    }

    rtpPacket->rtpHeader.seq++;

    /*
     * 如果采样频率是44100
     * 一般AAC每个1024个采样为一帧
     * 所以一秒就有 44100 / 1024 = 43帧
     * 时间增量就是 44100 / 43 = 1025
     * 一帧的时间为 1 / 43 = 23ms
     */
    rtpPacket->rtpHeader.timestamp += 1025;

    return 0;
}


static int rtpSendAACFrameTcp(int socket,struct RtpPacket* rtpPacket, 
    uint8_t* frame, uint32_t frameSize) {
    //打包文档：https://blog.csdn.net/yangguoyu8023/article/details/106517251/
    int ret;

    rtpPacket->payload[0] = 0x00;
    rtpPacket->payload[1] = 0x10;
    rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5; //高8位
    rtpPacket->payload[3] = (frameSize & 0x1F) << 3; //低5位

    memcpy(rtpPacket->payload + 4, frame, frameSize);

    ret = rtpSendPacketOverTcp(socket,rtpPacket, frameSize + 4, 0x02);
    if (ret < 0)
    {
        printf("failed to send rtp packet\n");
        return -1;
    }

    rtpPacket->rtpHeader.seq++;

    /*
     * 如果采样频率是44100
     * 一般AAC每个1024个采样为一帧
     * 所以一秒就有 44100 / 1024 = 43帧
     * 时间增量就是 44100 / 43 = 1025
     * 一帧的时间为 1 / 43 = 23ms
     */
    rtpPacket->rtpHeader.timestamp += 1025;

    return 0;
}
