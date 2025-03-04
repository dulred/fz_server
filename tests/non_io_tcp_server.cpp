#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define BUF_SIZE 1024

static bool stop = false;

static void handle_term(int sig)
{
    stop = true;
}


int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
}

int main(int argc, char const *argv[])
{
    signal(SIGTERM, handle_term);

    // byteorder();
    if (argc <= 3)
    {
        printf("usage: %s ip_address port_number backlog \n",basename(argv[0]));
        
        return 1;
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);
    
    
    // create socket
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock == -1)
    {
        perror("socket");
        return 1;
    }
    
    // bind socket

    // create ipv4 socket address
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    if (bind(sock, (struct sockaddr*)&address, sizeof(address)) == -1)
    {
        perror("bind");
        close(sock);
        return 1;
    }

    // listen socket
    if (listen(sock, backlog) == -1)
    {
        perror("listen");
        close(sock);
        return 1;
    }
    
    set_nonblocking(sock);

    // client list
    int client_sockets[1024];
    int num_clients = 0;

    // wait listen
    while (!stop)
    {
        struct sockaddr_in client;
        socklen_t client_addrlength = sizeof(client);
        int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
        
        if (connfd != -1)
        {
            set_nonblocking(connfd);
            client_sockets[num_clients++] = connfd;
            printf("New client connected: %d \n", connfd);
            // print client ip and port when accept success
            char remote[INET_ADDRSTRLEN];
            printf("connected with ip : %s and port : %d \n", inet_ntop(AF_INET,
                &client.sin_addr, 
                remote, 
                INET_ADDRSTRLEN),ntohs(client.sin_port));
            
        }else if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("accept");
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            printf("accept no user ,you need wait \n");
            sleep(10);
        }
        

        // check all the client socket when there are any data ready
        for (int i = 0; i < num_clients; i++)
        {
            char buffer[1024];
            int len = recv(client_sockets[i], buffer, sizeof(buffer), 0);
            if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                printf("recv no data ,you need wait \n");
                sleep(10);
                continue;
            }else if (len <= 0)
            {
                printf("Client disconnected: %d \n", client_sockets[i]);
                close(client_sockets[i]);

                // remote the socket from the socket list
                for (int j= i; j < num_clients - 1; j++)
                {
                    client_sockets[j] = client_sockets[j + 1];
                }

                num_clients--;
                i--;
            }else
            {
                buffer[len] = '\0';
                printf("Received from client %d: %s \n", client_sockets[i], buffer);
            }

        }   
        //avoid cpu load hight
        usleep(1000);  
    }

    close(sock);
    return 0;
}
