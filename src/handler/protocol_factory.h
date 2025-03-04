#ifndef PROTOCOL_FACTORY_H
#define PROTOCOL_FACTORY_H

#include <iostream>
#include <memory>

#include "src/handler/protocol_handler.h"
#include "src/handler/rtsp/rtsp_handler.h"
#include "src/handler/rtmp/rtmp_handler.h"

class ProtocolFactory {
public:
    static std::unique_ptr<ProtocolHandler> createHandler(const std::string& protocol) {
        if (protocol == "RTSP") {
            return std::make_unique<RtspHandler>();
        } else if (protocol == "RTMP") {
            return std::make_unique<RtmpHandler>();
        }
        return nullptr;
    }
};

#endif