#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 设置 Socket 为非阻塞模式
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    // 创建 UDP Socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        perror("socket");
        return 1;
    }

    // 设置 Socket 为非阻塞模式
    if (set_nonblocking(udp_socket) == -1) {
        perror("set_nonblocking");
        close(udp_socket);
        return 1;
    }

    // 绑定地址
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345); // 绑定端口 12345
    server_addr.sin_addr.s_addr = INADDR_ANY; // 绑定所有网络接口

    if (bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(udp_socket);
        return 1;
    }

    // 初始化 select
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(udp_socket, &read_fds);

    while (1) {
        // 复制 fd_set，因为 select 会修改它
        fd_set tmp_fds = read_fds;

        // 调用 select 监听 Socket
        int n = select(udp_socket + 1, &tmp_fds, NULL, NULL, NULL);
        if (n == -1) {
            perror("select");
            break;
        }

        // 检查 Socket 是否有数据可读
        if (FD_ISSET(udp_socket, &tmp_fds)) {
            char buffer[1024];
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);

            // 接收数据
            int len = recvfrom(udp_socket, buffer, sizeof(buffer), 0,
                               (struct sockaddr*)&client_addr, &addr_len);
            if (len == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 没有数据可读，继续循环
                    continue;
                } else {
                    // 其他错误
                    perror("recvfrom");
                    break;
                }
            } else if (len == 0) {
                // 对端关闭连接（UDP 不会发生）
                printf("Client disconnected\n");
                break;
            } else {
                // 处理接收到的数据
                buffer[len] = '\0';
                printf("Received from %s:%d: %s\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
            }
        }
    }

    // 关闭 Socket
    close(udp_socket);
    return 0;
}
