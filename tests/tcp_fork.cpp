#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    // 忽略 SIGCHLD 信号，避免僵尸进程
    signal(SIGCHLD, SIG_IGN);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    while (1) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (fork() == 0) { // 子进程
            char buffer[1024];
            recv(client_socket, buffer, sizeof(buffer), 0);
            printf("Received: %s\n", buffer);
            close(client_socket);
            exit(0); // 子进程退出
        }
        close(client_socket); // 父进程关闭客户端套接字
    }

    return 0;
}
