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
    // 连接到多个服务器
    const char* servers[] = {"127.0.0.1", "127.0.0.2"}; // 假设有两个服务器
    int num_servers = sizeof(servers) / sizeof(servers[0]);
    int sockets[num_servers];
    int max_fd = 0;

    for (int i = 0; i < num_servers; i++) {
        sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sockets[i] == -1) {
            perror("socket");
            continue;
        }

        // 设置服务器地址
        struct sockaddr_in server_addr = {0};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(554); // RTSP 默认端口
        inet_pton(AF_INET, servers[i], &server_addr.sin_addr);

        // 连接到服务器
        if (connect(sockets[i], (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(sockets[i]);
            sockets[i] = -1;
            continue;
        }

        // 设置 Socket 为非阻塞模式
        set_nonblocking(sockets[i]);

        // 更新最大文件描述符
        if (sockets[i] > max_fd) {
            max_fd = sockets[i];
        }
    }

    // 初始化 select
    fd_set read_fds;
    FD_ZERO(&read_fds);
    for (int i = 0; i < num_servers; i++) {
        if (sockets[i] != -1) {
            FD_SET(sockets[i], &read_fds);
        }
    }

    while (1) {
        // 复制 fd_set，因为 select 会修改它
        fd_set tmp_fds = read_fds;

        // 调用 select 监听 Socket
        int n = select(max_fd + 1, &tmp_fds, NULL, NULL, NULL);
        if (n == -1) {
            perror("select");
            break;
        }

        // 检查每个 Socket 是否有数据可读
        for (int i = 0; i < num_servers; i++) {
            if (sockets[i] != -1 && FD_ISSET(sockets[i], &tmp_fds)) {
                char buffer[1024];
                int len = recv(sockets[i], buffer, sizeof(buffer), 0);
                if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // 没有数据可读，继续循环
                    continue;
                } else if (len <= 0) {
                    // 服务器断开连接
                    printf("Server %s disconnected\n", servers[i]);
                    close(sockets[i]);
                    sockets[i] = -1;
                } else {
                    // 处理接收到的数据
                    buffer[len] = '\0';
                    printf("Received from server %s: %s\n", servers[i], buffer);
                }
            }
        }

        // 模拟推流（主动发送数据）
        for (int i = 0; i < num_servers; i++) {
            if (sockets[i] != -1) {
                const char* push_data = "RTSP push data";
                int len = send(sockets[i], push_data, strlen(push_data), 0);
                if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    // 发送缓冲区已满，稍后重试
                    printf("Send buffer full for server %s, retry later...\n", servers[i]);
                } else if (len <= 0) {
                    // 发送失败
                    printf("Send failed for server %s\n", servers[i]);
                    close(sockets[i]);
                    sockets[i] = -1;
                } else {
                    // 发送成功
                    printf("Sent to server %s: %s\n", servers[i], push_data);
                }
            }
        }

        // 避免 CPU 占用过高
        usleep(100000); // 休眠 100 毫秒
    }

    // 关闭所有 Socket
    for (int i = 0; i < num_servers; i++) {
        if (sockets[i] != -1) {
            close(sockets[i]);
        }
    }

    return 0;
}
