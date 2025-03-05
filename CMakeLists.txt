cmake_minimum_required(VERSION 3.10)
project(fz_socket)

set(CMAKE_BUILD_TYPE Debug)

include_directories(.)

file(GLOB_RECURSE SOURCES "src/server/*.cpp" "src/handler/*.cpp" 
"src/utility/*.cpp" "src/session/*.cpp")

# add_executable(test_process ${SOURCES} tests/test_process.cpp)

# add_executable(test_thread ${SOURCES} tests/test_thread.cpp)

# add_executable(test_tcp_client ${SOURCES} tests/test_tcp_client.cpp)
# add_executable(non_io_tcp_server ${SOURCES} tests/non_io_tcp_server.cpp)
# add_executable(epollserverdemo tests/epollserverdemo.cpp)
# add_executable(test_udp_client tests/test_udp_client.cpp)
# add_executable(test_udp_server tests/test_udp_server.cpp)
# add_executable(select_udp_server tests/select_udp_server.cpp)
# add_executable(rtsp_server src/rtsp/rtsp_server.cpp src/rtsp/rtp/rtp.cpp)
add_executable(main ${SOURCES} main.cpp)
add_executable(tmp tmp/test.cpp)

target_link_libraries(tmp pthread)
target_link_libraries(main pthread)
# target_link_libraries(rtsp_server pthread)
# target_link_libraries(test_thread pthread)