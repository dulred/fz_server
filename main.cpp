
#include "src/server/epollServer.h"

using namespace fz::epollServer;

int main(int argc, char const *argv[])
{
    // get epoll server
    EpollServer* epollServer = Singleton<EpollServer>::instance();

    // start server
    epollServer->run();

    // epollServer->init();

    // register rtspOverUdp aac_handler

    // register rtspOverUdp h264_handler

    // register rtspOverTcp h264_and_aac_handler

    return 0;
}