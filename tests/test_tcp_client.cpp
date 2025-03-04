#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#ifdef _WIN32
    #define fseek _fseeki64
    #define ftell _ftelli64
#else
    #define fseek fseeko
    #define ftell ftello
#endif

#define BUFF_SIZE 1024

int main(int argc, char const *argv[])
{
    // byteorder();
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number \n",basename(argv[0]));
        
        return 1;
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    // create socket
    int sock = socket(AF_INET,SOCK_STREAM,0);
    assert( sock >= 0);

    // create ipv4 server socket address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        printf("connect failed is: %d \n", errno);
    }
    else
    {
        const char* data = "areyouokasfasfqwrqwrqwrwqwq4545645";

        send(sock, data,strlen(data),0);
        printf("alread send \n");
        sleep(15);

        send(sock, data,strlen(data),0);

        char buff[BUFF_SIZE];
        
        memset(buff,'\0', BUFF_SIZE);

        int ret = recv(sock, buff, BUFF_SIZE - 1, 0);

        printf("got %d bytes of normal data from server '%s'\n", ret, buff);
        sleep(2);
        FILE* file = fopen("../test.txt","rb");
        if (file == nullptr)
        {
            perror("Error open file");
            return -1;
        }

        FILE* file2 = fopen("../test2.txt","wb");
        if (file2 == nullptr)
        {
            perror("Error open file");
            return -1;
        }

        int64_t bytes_read;
        char buffer[BUFF_SIZE];

        fseek(file, 0, SEEK_END);
        int64_t filesize = ftell(file);
        fseek(file, 0, SEEK_SET);

        while ((bytes_read = fread(buffer, 1, sizeof(buffer),file)) > 0)
        {
            // fwrite(buffer, 1, bytes_read, file2);
            // fflush(file2);
            
            int64_t bytes_sent = send(sock, buffer, bytes_read, 0);
            
            if (bytes_read < 0)
            {
                perror("failed to send daa to server");
                fclose(file);
                close(sock);
                return -1;
            }
            
        }
        
        
    }
    
    
    close(sock);

    
    return 0;
}
