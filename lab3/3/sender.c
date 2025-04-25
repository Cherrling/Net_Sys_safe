#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libnet.h>

#define DEST_MAC "\x00\x0c\x29\x85\xcb\x22" // 修改为虚拟机 A 的 MAC 地址
#define SRC_MAC  "\x08\x8e\x90\x07\x9f\x72" // 修改为虚拟机 B 的 MAC 地址
#define PAYLOAD  "0123456789"

int main() {
    libnet_t *l;
    char errbuf[LIBNET_ERRBUF_SIZE];
    u_int32_t src_ip, dst_ip;
    u_int16_t src_port = 12345, dst_port = 8000;
    int bytes_written;

    // 初始化 libnet 上下文
    l = libnet_init(LIBNET_LINK, "eth7", errbuf); 
    if (l == NULL) {
        fprintf(stderr, "libnet_init() failed: %s\n", errbuf);
        return EXIT_FAILURE;
    }

    // 解析 IP 地址
    src_ip = libnet_name2addr4(l, "192.168.3.191", LIBNET_RESOLVE); // 虚拟机 B 的 IP
    dst_ip = libnet_name2addr4(l, "192.168.3.128", LIBNET_RESOLVE); // 虚拟机 A 的 IP

    // 构建 UDP 数据包
    libnet_build_udp(
        src_port, dst_port, LIBNET_UDP_H + strlen(PAYLOAD), 0,
        (uint8_t *)PAYLOAD, strlen(PAYLOAD), l, 0
    );

    // 构建 IPv4 数据包
    libnet_build_ipv4(
        LIBNET_IPV4_H + LIBNET_UDP_H + strlen(PAYLOAD), 0, 242, 0, 64,
        IPPROTO_UDP, 0, src_ip, dst_ip, NULL, 0, l, 0
    );

    // 构建以太网帧
    libnet_build_ethernet(
        (u_int8_t *)DEST_MAC, (u_int8_t *)SRC_MAC, ETHERTYPE_IP, NULL, 0, l, 0
    );

    // 发送数据包
    bytes_written = libnet_write(l);
    if (bytes_written == -1) {
        fprintf(stderr, "libnet_write() failed: %s\n", libnet_geterror(l));
    } else {
        printf("Sent %d bytes UDP packet.\n", bytes_written);
    }

    libnet_destroy(l);
    return 0;
}
