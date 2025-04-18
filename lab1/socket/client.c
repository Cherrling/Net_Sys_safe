#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h> 

#define SERVER_IP "127.0.0.1"  // 替换为服务端 IP
#define SERVER_PORT 8899
#define BUFSIZE 1024


void print_menu() {
    printf("=== Client ===\n");
    printf("0. quit\n");
    printf("1. send\n");
    printf("2. download\n");
    printf("Input：");
}


void send_file(int sock) {
    char filename[0x100];
    chdir("../");
    printf("File list:\n");
    getchar();
    struct dirent **namelist;
    int n = scandir(".", &namelist, NULL, alphasort);
    if (n < 0) {
        perror("scandir");
        return;
    }

    for (int i = 0; i < n; i++) {
        if (namelist[i]->d_type == DT_REG) {
            printf(" - %s\n", namelist[i]->d_name);
        }
        free(namelist[i]);
    }
    free(namelist);

    printf("Input the filename to send:");

    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';  

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);

    send(sock, filename, strlen(filename), 0);
    send(sock, "\n", 1, 0);  // 告诉服务端文件名结束
    send(sock, &filesize, sizeof(filesize), 0);

    char buffer[BUFSIZE];
    while (!feof(fp)) {
        int len = fread(buffer, 1, BUFSIZE, fp);
        send(sock, buffer, len, 0);
    }

    fclose(fp);
    printf("File Transfered\n");
}

void download_file(int sock) {
    char filename[256];
    char buffer[BUFSIZE];
    printf("File list:\n");
    char line[BUFSIZE];
    int line_pos = 0;

    while (1) {
        int len = recv(sock, buffer, BUFSIZE - 1, 0);
        if (len <= 0) {
            printf("Connection error\n");
            return;
        }
        buffer[len] = '\0';

        // 按行处理
        for (int i = 0; i < len; ++i) {
            if (buffer[i] == '\n') {
                line[line_pos] = '\0';
                if (strcmp(line, "END") == 0) {
                    goto done_list;
                }
                printf("%s\n", line);
                line_pos = 0;
            } else {
                if (line_pos < BUFSIZE - 1) {
                    line[line_pos++] = buffer[i];
                }
            }
        }
    }
    done_list:
    ;
    printf("Input the filename to download:");
    read(STDIN_FILENO, filename, sizeof(filename));
    filename[strcspn(filename, "\n")] = '\0';  // 去掉换行符
    printf("Downloading:%s\n", filename);
    send(sock, filename, sizeof(filename), 0);
    long filesize;
    recv(sock, &filesize, sizeof(filesize), 0);
    FILE *fp = fopen(filename, "wb");
    long received = 0;
    while (received < filesize) {
        int len = recv(sock, buffer, BUFSIZE, 0);
        fwrite(buffer, 1, len, fp);
        received += len;
    }
    fclose(fp);
    printf("File downloaded\n");
}


int main() {
    system("mkdir -p client_dl");
    chdir("client_dl");
    while(1) {
        
        print_menu();
        char choice[0x10];
        scanf("%s", choice);
        if(!strcmp(choice, "0")) {
            printf("Exit\n");
            break;
        }
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        struct sockaddr_in server_addr = {0};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (strcmp(choice, "1") == 0) {
            send(sock, "SEND", 16, 0);
            send_file(sock);
        } else if (strcmp(choice, "2") == 0) {
            send(sock, "DOWNLOAD", 16, 0);
            download_file(sock);
        } else {
            printf("Unknown command\n");
        }
        close(sock);
    }

    return 0;
}