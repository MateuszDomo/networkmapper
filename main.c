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

int main() {
    char hostnameBuffer[256];
    if (gethostname(hostnameBuffer, sizeof(hostnameBuffer)) == -1) {
        printf("%s\n", strerror(errno));
        return 1;
    }
    printf("%s\n", hostnameBuffer); 

    struct hostent *host_entry;
    host_entry = gethostbyname(hostnameBuffer);
    char *IPbuffer;
    IPbuffer = inet_ntoa(*((struct in_addr*)
                        host_entry->h_addr_list[0]));
    printf("%s\n", IPbuffer);

    int sockFD = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockFD == -1) {
        printf("%s\n", strerror(errno));
        close(sockFD);
    }
    
    struct icmp packet;
    memset(&packet, 0, sizeof(packet));
    packet.type = 0x08;
    packet.code = 0x0;
    //checksum
    packet.identifier = 0x1234;
    packet.sequenceNumber = 0x1;

    struct sockaddr_in  destinationAddress;
    memset(&destinationAddress, 0, sizeof(destinationAddress));
    destinationAddress.sin_family = AF_INET;
    destinationAddress.sin_port = 0;
    inet_pton(AF_INET, "10.0.0.8", &destinationAddress.sin_addr);

    int bytesSent = sendto(sockFD, &packet, sizeof(packet), 0, (const struct sockaddr *)&destinationAddress, sizeof(destinationAddress));
    if (bytesSent == -1) {
        printf("%s\n", strerror(errno));
        close(sockFD);
        return 1;
    }

    close(sockFD);
    return 0;
}