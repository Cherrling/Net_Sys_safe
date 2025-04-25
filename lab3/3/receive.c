#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8000
#define BUFLEN 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFLEN];
    socklen_t addr_len = sizeof(client_addr);
    int recv_len;

    // 创建 UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置本地地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 接收所有地址数据
    server_addr.sin_port = htons(PORT);

    // 绑定端口
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Listening for UDP packets on port %d...\n", PORT);

    // 循环接收数据
    while (1) {
        memset(buffer, 0, BUFLEN);
        recv_len = recvfrom(sockfd, buffer, BUFLEN, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            printf("Received packet from %s: %s\n", client_ip, buffer);
        }
    }

    close(sockfd);
    return 0;
}
