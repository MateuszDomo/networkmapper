#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

struct icmp {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequenceNumber;
};

u_int16_t checksum(struct icmp* packet);
int ping(int sockFD, char* address);

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("Incorrect Arguments\n");
        return 1;
    }

    char* cmd =  argv[1];
    char* destAddress =  argv[2];

    int sockFD = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockFD == -1) {
        printf("%s\n", strerror(errno));
        close(sockFD);
        return 1;
    }

    if (ping(sockFD, destAddress) == -1) {
        printf("%s\n", strerror(errno));
        close(sockFD);
        return -1;
    }

    close(sockFD);
    return 0;
}

int ping(int sockFD, char* address) {
    struct icmp packet;
    memset(&packet, 0, sizeof(packet));
    packet.type = 0x08;
    packet.code = 0x0;
    packet.identifier = htons(0x11);
    packet.sequenceNumber = htons(0x01);
    packet.checksum = checksum(&packet);

    struct sockaddr_in  destinationAddress;
    socklen_t destAddrLen = sizeof(destinationAddress);
    memset(&destinationAddress, 0, sizeof(destinationAddress));
    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = 0;
    if (inet_pton(AF_INET, address, &destinationAddress.sin_addr) == -1) {
        return -1;
    }

    int bytesSent = sendto(sockFD, &packet, sizeof(packet), 0, (const struct sockaddr *)&destinationAddress, destAddrLen);
    if (bytesSent == -1) {
        return 1;
    }
    printf("Sent %d bytes\n", bytesSent);

    char buffer[1024];
    memset(&buffer, 0, sizeof(buffer));
    printf("%ld\n", sizeof(buffer));
    int bytesReceived = recvfrom(sockFD, &buffer, sizeof(buffer), 0, (struct sockaddr *)&destinationAddress, &destAddrLen);
    if (bytesReceived == -1) {
        return -1;
    }
    printf("Received %d bytes\n", bytesReceived);
    for (int i = 0; i < bytesReceived; i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");
    return 0;
}

u_int16_t checksum(struct icmp *packet) {
    uint16_t *startByte = (uint16_t *)packet;
    uint16_t sum = 0;

    int count = sizeof(*packet);
    while (count > 1) {
        sum += *startByte;
        startByte++;
        count -= 2;
    }

    if (count > 0) {
        sum += *(uint8_t *)startByte;
    }

    return ~sum;
}