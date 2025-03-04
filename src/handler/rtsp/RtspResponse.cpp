
#include "src/handler/rtsp/RtspResponse.h"
#include <sstream>


// 生成RTSP响应字符串的实现
std::string generateRtspResponse(RtspStatusCode code, int cseq, const std::string& headers, const std::string& body) {
    std::ostringstream response;

    // 状态行
    response << "RTSP/1.0 " << static_cast<int>(code) << " ";
    switch (code) {
        case RtspStatusCode::OK:
            response << "OK";
            break;
        case RtspStatusCode::BadRequest:
            response << "Bad Request";
            break;
        case RtspStatusCode::Unauthorized:
            response << "Unauthorized";
            break;
        case RtspStatusCode::NotFound:
            response << "Not Found";
            break;
        case RtspStatusCode::InternalServerError:
            response << "Internal Server Error";
            break;
        case RtspStatusCode::NotImplemented:
            response << "Not Implemented";
            break;
        default:
            response << "Unknown Status";
            break;
    }
    response << "\r\n"; // 状态行以 \r\n 结尾

    // CSeq 头部字段
    response << "CSeq: " << cseq << "\r\n";

    // 其他头部字段
    if (!headers.empty()) {
        response << headers;
        // 确保头部字段以 \r\n 结尾
        if (headers.size() < 2 || headers.substr(headers.size() - 2) != "\r\n") {
            response << "\r\n";
        }
    }

    // 空行分隔头部和消息体
    response << "\r\n";

    // 消息体
    if (!body.empty()) {
        response << body;
    }

    return response.str();
}
