#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <time.h>

#include <time.h>

#define INET_ADDRSTRLEN 16

// 添加全局变量
FILE *output_file;
char current_protocol[32];
char source_ip[INET_ADDRSTRLEN];
char dest_ip[INET_ADDRSTRLEN];

// 添加全局变量用于存储端口信息
uint16_t source_port;
uint16_t dest_port;
// 函数声明
void tcp_callback(const u_char *packet);
void udp_callback(const u_char *packet);
void ip_callback(const u_char *packet);


struct ethernet{
    u_char dest[6]; // 目的MAC地址
    u_char src[6];  // 源MAC地址
    u_short type;   // 协议类型

};

struct ip_header{
    u_char version_ihl; // 版本和首部长度
    u_char tos;         // 服务类型
    u_short total_length;// 总长度
    u_short id;        // 标识符
    u_short flags_offset;// 标志和片偏移
    u_char ttl;        // 生存时间
    u_char protocol;   // 协议类型
    u_short checksum;  // 校验和
    u_char src_ip[4]; // 源IP地址
    u_char dest_ip[4];// 目的IP地址
};


struct tcp{
    u_short src_port; // 源端口
    u_short dest_port;// 目的端口
    u_int seq_num;    // 序列号
    u_int ack_num;    // 确认号
    u_char offset;     // 首部长度和保留位
    u_char flags;     // 标志位
    u_short window;   // 窗口大小
    u_short checksum; // 校验和
    u_short urgent_ptr;// 紧急指针
};
struct udp{
    u_short src_port; // 源端口
    u_short dest_port;// 目的端口
    u_short length;   // 长度
    u_short checksum; // 校验和
};

// ...existing code...

// 在全局变量区域添加
void write_to_file(const struct pcap_pkthdr *pkthdr) {
    static int first_packet = 1;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    // 如果是第一个数据包，写入表头
    if (first_packet) {
        fprintf(output_file, "%-19s | %-15s | %-15s | %-15s | %-5s | %-5s | %-10s\n",
                "捕获时间", "源IP", "目的IP", "协议", "源端口", "目的端口", "数据包大小");
        fprintf(output_file, "-------------------+-----------------+-----------------+");
        fprintf(output_file, "-----------------+--------+--------+------------\n");
        first_packet = 0;
    }

    // 写入数据包信息
    fprintf(output_file, "%-19s | %-15s | %-15s | %-15s | %-5d | %-5d | %-10d\n",
            time_str,
            strlen(source_ip) ? source_ip : "-",
            strlen(dest_ip) ? dest_ip : "-",
            current_protocol,
            source_port ? source_port : 0,
            dest_port ? dest_port : 0,
            pkthdr->len);
    
    fflush(output_file);  // 立即写入文件
}



void ethernet_callback(u_char *args, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    struct ethernet *eth_header = (struct ethernet *)packet;
    u_short eth_type = ntohs(eth_header->type);
    
    // 打印时间戳
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
    printf("\n时间: %s\n", time_str);
    
    // 打印MAC地址
    printf("源MAC: ");
    for(int i = 0; i < 6; i++) {
        printf("%02x", eth_header->src[i]);
        if(i < 5) printf(":");
    }
    printf("\n目的MAC: ");
    for(int i = 0; i < 6; i++) {
        printf("%02x", eth_header->dest[i]);
        if(i < 5) printf(":");
    }
    printf("\n");

    
    // 判断上层协议类型
    switch(eth_type) {
        case 0x0800:
            strcpy(current_protocol, "IPv4");
            printf("协议类型: IPv4\n");
            ip_callback(packet + sizeof(struct ethernet));
            break;
            
        case 0x0806:
            strcpy(current_protocol, "ARP");
            printf("协议类型: ARP\n");
            break;
            
        case 0x8035:
            strcpy(current_protocol, "RARP");
            printf("协议类型: RARP\n");
            break;
            
        case 0x86DD:
            strcpy(current_protocol, "IPv6");
            printf("协议类型: IPv6\n");
            break;
            
        case 0x880B:
            strcpy(current_protocol, "PPP");
            printf("协议类型: PPP\n");
            break;
            
        case 0x8863:
        case 0x8864:
            strcpy(current_protocol, "PPPoE");
            printf("协议类型: PPPoE\n");
            break;
            
        default:
            sprintf(current_protocol, "未知协议(0x%04x)", eth_type);
            printf("协议类型: %s\n", current_protocol);
            break;
    }
    
    printf("数据包长度: %d 字节\n", pkthdr->len);
    printf("----------------------------------------\n");
    // 在处理完协议后调用 write_to_file
    write_to_file(pkthdr);

    // 重置端口信息（为下一个数据包做准备）
    source_port = 0;
    dest_port = 0;
}

void ip_callback(const u_char *packet) {
    struct ip_header *ip_header = (struct ip_header *)packet;
    char protocol[16];
    // 转换IP地址为字符串格式
    inet_ntop(AF_INET, ip_header->src_ip, source_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, ip_header->dest_ip, dest_ip, INET_ADDRSTRLEN);
    
    // 打印IP地址
    printf("源IP: %s\n", source_ip);
    printf("目的IP: %s\n", dest_ip);
    
    // 判断上层协议类型
    switch(ip_header->protocol) {
        case 0x01:
            strcpy(protocol, "ICMP");
            printf("协议: ICMP\n");
            break;
            
        case 0x02:
            strcpy(protocol, "IGMP");
            printf("协议: IGMP\n");
            break;
            
        case 0x06:
            strcpy(protocol, "TCP");
            printf("协议: TCP\n");
            // 计算IP头部长度并跳过，调用TCP回调
            tcp_callback(packet + (ip_header->version_ihl & 0x0F) * 4);
            break;
            
        case 0x11:
            strcpy(protocol, "UDP");
            printf("协议: UDP\n");
            // 计算IP头部长度并跳过，调用UDP回调
            udp_callback(packet + (ip_header->version_ihl & 0x0F) * 4);
            break;
            
        default:
            sprintf(protocol, "未知协议(0x%02x)", ip_header->protocol);
            printf("协议: %s\n", protocol);
            break;
    }
    
    strcat(current_protocol, "/");
    strcat(current_protocol, protocol);
}


// 在文件末尾添加TCP回调函数实现
void tcp_callback(const u_char *packet) {
    struct tcp *tcp_header = (struct tcp *)packet;
    
    // 获取端口号（需要进行网络字节序转换）
    source_port = ntohs(tcp_header->src_port);
    dest_port = ntohs(tcp_header->dest_port);
    
    // 打印端口信息
    printf("TCP源端口: %d\n", source_port);
    printf("TCP目的端口: %d\n", dest_port);
    
}

// 在文件末尾添加UDP回调函数实现
void udp_callback(const u_char *packet) {
    struct udp *udp_header = (struct udp *)packet;
    
    // 获取端口号（需要进行网络字节序转换）
    source_port = ntohs(udp_header->src_port);
    dest_port = ntohs(udp_header->dest_port);
    
    // 打印端口信息
    printf("UDP源端口: %d\n", source_port);
    printf("UDP目的端口: %d\n", dest_port);
    
}

int main(int argc, char *argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct bpf_program fp;
    char *filter_exp = NULL;  // 过滤条件
    pcap_if_t *alldevs, *d;
    int i = 0, inum;
    
    // 步骤1：获取网络设备列表
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "获取网络设备列表失败: %s\n", errbuf);
        return 1;
    }
    
    // 打印设备列表供用户选择
    printf("可用网络设备列表:\n");
    for(d = alldevs; d; d = d->next) {
        printf("%d. %s", ++i, d->name);
        if (d->description)
            printf(" (%s)\n", d->description);
        else
            printf(" (无描述信息)\n");
    }
    
    if (i == 0) {
        printf("未找到网络设备\n");
        return 1;
    }
    
    printf("\n请选择要监听的网卡 (1-%d): ", i);
    scanf("%d", &inum);
    
    if (inum < 1 || inum > i) {
        printf("选择的网卡序号无效\n");
        pcap_freealldevs(alldevs);
        return 1;
    }
    
    // 获取选中的设备
    for(d = alldevs, i = 0; i < inum-1; d = d->next, i++);
    
    // 步骤2：打开设备进行捕获（非混杂模式）
    handle = pcap_open_live(d->name, BUFSIZ, 0, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "无法打开设备 %s: %s\n", d->name, errbuf);
        pcap_freealldevs(alldevs);
        return 1;
    }
    
    pcap_freealldevs(alldevs); // 释放设备列表
    
    // 步骤3：设置过滤条件（如果有参数）
    if (argc > 1) {
        filter_exp = argv[1];
        if (pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
            fprintf(stderr, "无法编译过滤器 %s: %s\n", 
                    filter_exp, pcap_geterr(handle));
            return 1;
        }
        
        if (pcap_setfilter(handle, &fp) == -1) {
            fprintf(stderr, "无法设置过滤器: %s\n", pcap_geterr(handle));
            return 1;
        }
    }
    
    // 步骤4：创建输出文件
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[64];
    strftime(filename, sizeof(filename), "capture_%Y_%m_%d_%H_%M_%S.txt", t);
    
    output_file = fopen(filename, "w");
    if (output_file == NULL) {
        fprintf(stderr, "无法创建输出文件 %s\n", filename);
        return 1;
    }
    
    // 写入文件头信息
    fprintf(output_file, "捕获开始时间: %04d-%02d-%02d %02d:%02d:%02d\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    fprintf(output_file, "过滤条件: %s\n", filter_exp ? filter_exp : "无");
    fprintf(output_file, "----------------------------------------\n");
    
    printf("\n开始捕获数据包，输出文件: %s\n", filename);
    printf("按Ctrl+C停止捕获\n\n");
    
    // 步骤5：开始捕获数据包
    pcap_loop(handle, -1, ethernet_callback, NULL);
    
    // 清理资源
    if (argc > 1) {
        pcap_freecode(&fp);
    }
    pcap_close(handle);
    fclose(output_file);
    
    return 0;
}