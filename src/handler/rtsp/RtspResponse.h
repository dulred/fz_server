#ifndef RTSP_RESPONSE_H
#define RTSP_RESPONSE_H

#include <string>

// RTSP状态码枚举
enum class RtspStatusCode {
    OK = 200,
    BadRequest = 400,
    Unauthorized = 401,
    NotFound = 404,
    InternalServerError = 500,
    NotImplemented = 501
};

// 生成RTSP响应字符串的函数声明
std::string generateRtspResponse(RtspStatusCode code, int cseq, const std::string& headers = "", const std::string& body = "");

#endif // RTSP_RESPONSE_H
