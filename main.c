#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <sys/select.h>

struct icmp {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequenceNumber;
};

u_int16_t checksum(struct icmp* packet);
int ping(int sock_fd, int count, struct sockaddr_in* addr, socklen_t addr_len);
void printbytes(int bytes, char buffer[]);
int ping_cmd(char* addr_str, int count);
int create_sockaddr_in(char* addr_str, struct sockaddr_in* destination_address);

int main(int argc, char* argv[]) {

    if (argc != 4) {
        printf("Incorrect Arguments\n");
        return 1;
    }

    char* cmd =  argv[1];
    char* dest_address_str =  argv[2];

    if (strcmp(cmd, "ping") == 0) {
        if (ping_cmd(dest_address_str, atoi(argv[3])) == -1) {
            printf("%s\n", strerror(errno));
            return 1;
        }
    } else if (strcmp(cmd, "map") == 0) {

    }

    return 0;
}

int create_sockaddr_in(char* addr_str, struct sockaddr_in* destination_address) {
    memset(destination_address, 0, sizeof(*destination_address));
    destination_address->sin_family = AF_INET;
    destination_address->sin_port = 0;
    return inet_pton(AF_INET, addr_str, &destination_address->sin_addr);
}

int ping_cmd(char* addr_str, int count) {
    int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd == -1) {
        printf("%s\n", strerror(errno));
        return -1;
    }
    struct timespec timeout;
    timeout.tv_sec = 5;  // 5 seconds timeout
    timeout.tv_nsec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    struct sockaddr_in destination_address;
    if (create_sockaddr_in(addr_str, &destination_address) == -1) {
        close(sock_fd);
        return -1;
    }
    
    if (ping(sock_fd, count, &destination_address, sizeof(destination_address)) == -1) {
        close(sock_fd);
        return -1;
    }

    return 0;
}

int ping(int sock_fd, int count, struct sockaddr_in* addr, socklen_t addr_len) {
    fd_set fdset; 
    FD_ZERO(&fdset);
    FD_SET(sock_fd, &fdset);

    struct icmp packet;
    struct icmp recvpacket;
    struct timespec start, end;
    float msg_delay_sum = 0.0;
    char res_buffer[1024];
    int sequence_num = 0;
    int packets_lost = 0;
    for (int i = 0; i < count; i++) {
        // Construct packet
        memset(&packet, 0, sizeof(packet));
        packet.type = 0x08;
        packet.code = 0x0;
        packet.identifier = htons(0x11);
        packet.sequenceNumber = htons(++sequence_num);
        packet.checksum = checksum(&packet);

        clock_gettime(CLOCK_MONOTONIC, &start);
        int bytesSent = sendto(sock_fd, &packet, sizeof(packet), 0, (const struct sockaddr *)addr, addr_len);
        if (bytesSent == -1) {
            return -1;
        }

        struct timeval timeout = {0, 1000000};
        int ready = select(sock_fd + 1, &fdset, NULL, NULL, &timeout);

        if (ready == -1) {
            return -1;
        } else if (ready == 0) {
            packets_lost++;
            continue;
        }

        memset(&res_buffer, 0, sizeof(res_buffer));
        int bytes_received = recvfrom(sock_fd, &res_buffer, sizeof(res_buffer), 0, (struct sockaddr *)addr, &addr_len);
        clock_gettime(CLOCK_MONOTONIC, &end);
        if (bytes_received == -1) {
            return -1;
        }

        memcpy(&recvpacket, &res_buffer[20], sizeof(recvpacket));
        if (recvpacket.type != 0) {
            printf("Destination Host Unreachable\n");
            return 0;
        }

        double diff = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
        msg_delay_sum += diff;
        sleep(1);
    }
    printf("%f\n", msg_delay_sum/count);
    printf("%d lost out of %d\n", packets_lost, count);
    return 0;
}

void printbytes(int bytes, char buffer[]) {
    for (int i = 0; i < bytes; i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");
}

u_int16_t checksum(struct icmp *packet) {
    uint16_t *byte_ptr = (uint16_t *)packet;
    uint16_t sum = 0;

    int count = sizeof(*packet);
    while (count > 1) {
        sum += *byte_ptr;
        byte_ptr++;
        count -= 2;
    }

    if (count > 0) {
        sum += *(uint8_t *)byte_ptr;
    }

    return ~sum;
}