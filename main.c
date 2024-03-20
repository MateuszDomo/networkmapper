#define _XOPEN_SOURCE 600
#define _POSIX_C_SOURCE 200112L
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
int validate_address(char* address, char* cmd);
int extractCIDR(char* addr);

int main(int argc, char* argv[]) {

    char* cmd =  argv[1];
    char* dest_address_str =  argv[2];

    if (argc < 3) {
        printf("Invalid Arguments\n");
        return 1;
    }

    if (strcmp(cmd, "ping") == 0) {
        if (argc != 4) {
            printf("Invalid Arguments\n");
            return 1;
        }

        char* err; 
        int count = strtol(argv[3], &err, 10);
        if (*err) {
            printf("Ping count not a valid number: %s\n", err);
            return 1;
        }

        if (!validate_address(dest_address_str, cmd) || ping_cmd(dest_address_str, count) == -1) {
            if (errno) {
                printf("%s\n", strerror(errno));
            }
            return 1;
        }
    } else if (strcmp(cmd, "map") == 0) {
        int cidr = extractCIDR(dest_address_str);
        if (cidr == -1) {
            if (errno) {
                printf("%s\n", strerror(errno));
            }
            return -1;
        }

        if (!validate_address(dest_address_str, cmd)) {
            if (errno) {
                printf("%s\n", strerror(errno));
            }
            return 1;
        }
        printf("%d\n", cidr);
        printf("%s\n", dest_address_str);
    }

    return 0;
}

// Removes CIDR from original address and returns it
int extractCIDR(char* addr) {
    char addr_cp[1024];
    strncpy(addr_cp, addr, sizeof(addr_cp));

    char* token = strtok(addr_cp, "/");    

    strncpy(addr, token, 1024);

    token = strtok(NULL, "/");    
    if (token == NULL) {
        return 0;
    }

    char* err; 
    int cidr = strtol(token, &err, 10);
    if (*err) {
        printf("None valid CIDR: %s\n", err);
        return -1;
    }

    token = strtok(NULL, "/");    
    if (token != NULL) {
        printf("CIDR strings along bullshit: %s\n", token);
        return -1;
    }

    return cidr;
}

int validate_address(char* addr, char* cmd) {
    char addr_cp[1024];
    strncpy(addr_cp, addr, sizeof(addr_cp));
    
    char* token = strtok(addr_cp, ".");    
    int octuples = 0;

    while (token != NULL) {
        char* err; 
        int count = strtol(token, &err, 10);
        if (*err || count < 0 || count > 255) {
            printf("Octuple in address is not valid\n");
            return 0;
        }

        octuples++;
        token = strtok(NULL, ".");
    }

    if (octuples != 4) {
        printf("Address Not Valid, Broke boy\n");
        return 0;
    }

    return 1;
}

int create_sockaddr_in(char* addr_str, struct sockaddr_in* destination_address) {
    memset(destination_address, 0, sizeof(*destination_address));
    destination_address->sin_family = AF_INET;
    destination_address->sin_port = 0;
    return inet_pton(AF_INET, addr_str, &destination_address->sin_addr);
}

int map_cmd() {
    // Check for host
    return 0;
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
        int bytes_sent = sendto(sock_fd, &packet, sizeof(packet), 0, (const struct sockaddr *)addr, addr_len);
        if (bytes_sent == -1) {
            return -1;
        }
    
        struct timeval timeout = {0, 5000000};
        int ready = select(sock_fd + 1, &fdset, NULL, NULL, &timeout);
        if (ready == -1) {
            return -1;
        } else if (ready == 0) {
            packets_lost++;
            printf("Packet Lost\n");
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
        printf("Packet Success %f\n", diff);
    }

    printf("\nAverage Round Trip Time: %f\n", msg_delay_sum/count);
    printf("%d lost out of %d packets\n", packets_lost, count);
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