#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8899
#define BUFSIZE 1024

void send_file_list(int client_sock) {
    struct dirent **namelist;
    int n = scandir(".", &namelist, NULL, alphasort);
    if (n < 0) {
        perror("scandir");
        return;
    }
    for (int i = 0; i < n; i++) {
        if (namelist[i]->d_type == DT_REG) {
            send(client_sock, namelist[i]->d_name, strlen(namelist[i]->d_name), 0);
            send(client_sock, "\n", 1, 0);
        }
        free(namelist[i]);
    }
    free(namelist);
    send(client_sock, "END\n", 4, 0);
}

void receive_file(int client_sock) {
    char filename[256] = {0};
    long filesize;

    int pos = 0;
    char ch;
    while (recv(client_sock, &ch, 1, 0) > 0 && ch != '\n' && pos < 255) {
        filename[pos++] = ch;
    }
    filename[pos] = '\0';

    // 检查文件名是否包含非法字符
    if (strstr(filename, "..") != NULL || strchr(filename, '/') != NULL) {
        fprintf(stderr, "Invalid filename\n");
        return;
    }

    int len = recv(client_sock, &filesize, sizeof(filesize), 0);
    if (len != sizeof(filesize)) {
        fprintf(stderr, "Failed, received %d bytes\n", len);
        return;
    }
    // 添加文件大小限制
    const long MAX_FILE_SIZE = 1024 * 1024 * 100; // 100MB
    if (filesize <= 0 || filesize > MAX_FILE_SIZE) {
        fprintf(stderr, "Invalid file size\n");
        return;
    }

    char filepath[512] = {0};
    snprintf(filepath, sizeof(filepath), "server_dl/%s", filename);
    
    printf("Received file: %s (%ld bytes)\n", filename, filesize);


    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        perror("fopen");
        printf("Filepath: %s\n", filepath);
        return;
    }

    char buffer[BUFSIZE];
    long received = 0;
    while (received < filesize) {
        int len = recv(client_sock, buffer, BUFSIZE, 0);
        if (len <= 0) break;
        fwrite(buffer, 1, len, fp);
        received += len;
    }

    fclose(fp);
}


void send_file(int client_sock,char *filename) {

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        printf("Filename: %s\n", filename);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    send(client_sock, &filesize, sizeof(filesize), 0);

    char buffer[BUFSIZE];
    long sent = 0;
    while (!feof(fp)) {
        int len = fread(buffer, 1, BUFSIZE, fp);
        send(client_sock, buffer, len, 0);
        sent += len;
    }

    fclose(fp);
    printf("Sent file: %s (%ld bytes)\n", filename, filesize);
}


int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0); // AF_INET for IPv4, SOCK_STREAM for TCP

    struct stat st = {0};
    if (stat("server_dl", &st) == -1) {
        mkdir("server_dl", 0700);
    }

    if (server_sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to any address

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    
    if (listen(server_sock, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening port : %d\n", PORT);

    while ( 1 ) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);

        char command[0x10];
        char filename[0x100];
        recv(client_sock, command, sizeof(command), 0);
        if ( strcmp(command, "SEND") == 0 ) {
            receive_file(client_sock);
        } 
        else if (strcmp(command, "DOWNLOAD") == 0) {
            send_file_list(client_sock);
            printf("Waiting to choose file ...\n");
            recv(client_sock, filename, sizeof(filename), 0);
            printf("File:%s\n", filename);
            
            send_file(client_sock, filename);
        }
        else {
            printf("Unknown command: %s\n", command);
        }

        close(client_sock);
    }

}