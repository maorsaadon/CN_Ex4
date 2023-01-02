#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define ICMP_HDRLEN 8// ICMP header len for echo req
#define WATCHDOG_PORT 3000//watchdog port by request
#define SERVER_IP_ADDRESS "127.0.0.1"

//function declaration
unsigned short calculate_checksum(unsigned short *paddress, int len);
int validateNumber(char *str);
int validateIp(char *ip);

int main(int argc, char *argv[]){
    //check the IP
    if (argc != 2) {
        perror("you need to put IP!");
        exit(-1);
    }
    char ptr_IP[15];
    strcpy(ptr_IP , argv[1]);
    int flage = validateIp(ptr_IP);
    if (flage == 0)
    {
        printf("Ip isn't valid!\n");
        exit(-1);
    }

    // run 2 programs using fork + exec
    // command: make clean && make all && ./partb
    char *args[2];
    // compiled watchdog.c by makefile
    args[0] = "./watchdog";
    args[1] = argv[1];
    int status;
    int pid = fork();
    if (pid == 0)
    {
        execvp(args[0], args);
    }

    //open a socket for watchdog
    int watchdogSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (watchdogSocket == -1) {
        printf("Could not create socket : %d\n", errno);
        return -1;
    }
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(WATCHDOG_PORT);
    int rval = inet_pton(AF_INET, (const char *) SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }

    sleep(3);

    // Make a connection with watchdog
    int connectResult = connect(watchdogSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d\n", errno);
        close(watchdogSocket);
        return -1;
    }

    //open socket for ping
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        fprintf(stderr, "socket() failed with error: %d", errno);
        fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
        return -1;
    }
    struct sockaddr_in dest_in;
    memset(&dest_in, 0, sizeof(struct sockaddr_in));
    dest_in.sin_family = AF_INET;
    dest_in.sin_addr.s_addr = inet_addr(argv[1]);// The port is irrelant for Networking and therefore was zeroed.

    // ICMP-header
    struct icmp icmphdr;
    char data[IP_MAXPACKET] = "This is the ping.\n";
    int datalen = strlen(data) + 1;



    //for calculate the time
    struct timeval start, end;
    int  icmp_seq_counter = 0;//seq counter
    printf("PING %s (%s): %d data bytes\n", argv[1], argv[1], datalen);
    while (1) {

        icmphdr.icmp_type = ICMP_ECHO;// Message Type (8 bits): ICMP_ECHO_REQUEST
        icmphdr.icmp_code = 0;// Message Code (8 bits): echo request
        icmphdr.icmp_id = 18;
        icmphdr.icmp_seq = 0;// Sequence Number (16 bits): starts at 0
        icmphdr.icmp_cksum = 0;// ICMP header checksum (16 bits): set to 0 not to include into checksum calculation
        char packet[IP_MAXPACKET];// Combine the packet
        memcpy((packet), &icmphdr, ICMP_HDRLEN); // Next, ICMP header
        memcpy(packet + ICMP_HDRLEN, data, datalen);// After ICMP header, add the ICMP data.
        icmphdr.icmp_cksum = calculate_checksum((unsigned short *) (packet),
                                                ICMP_HDRLEN + datalen);// Calculate the ICMP header checksum
        memcpy((packet), &icmphdr, ICMP_HDRLEN);


        //if NEW_PING don't receive a stop signal -> send to watch dog that NEW_PING ready to start sending ping
        int ready = 1;
        int s = send(watchdogSocket, &ready, sizeof(int), 0);

        gettimeofday(&start, 0);
        int sl =3*(icmp_seq_counter+1);//for test - check if the watchdog make the NEW_PING to shut down
        sleep(sl);

        // Send the packet using sendto() for sending datagrams.
        int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *) &dest_in, sizeof(dest_in));
        if (bytes_sent == -1) {
            fprintf(stderr, "sendto() failed with error: %d", errno);
            return -1;
        }


        // Get the ping response
        bzero(packet, IP_MAXPACKET);
        socklen_t len = sizeof(dest_in);
        ssize_t bytes_received = -1;
        while ((bytes_received = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *) &dest_in, &len))) {
            if (bytes_received > 0) {
                // Check the IP header
                struct iphdr *iphdr = (struct iphdr *) packet;
                struct icmphdr *icmphdr = (struct icmphdr *) (packet + (iphdr->ihl * 4));
                break;
            }
        }

        gettimeofday(&end, 0);

        //calculate the time
        float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
        printf("%ld bytes from %s: icmp_seq=%d ttl=10 time=%f ms)\n", bytes_received, argv[1], icmp_seq_counter++,
               milliseconds);
    }

    // Close the raw socket descriptor.
    close(sock);


    wait(&status);// waiting for child to finish before exiting
    return 0;
}


// Compute checksum (RFC 1071).
unsigned short calculate_checksum(unsigned short *paddress, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *((unsigned char *)&answer) = *((unsigned char *)w);
        sum += answer;
    }

    // add back carry-outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
    sum += (sum >> 16);                 // add carry
    answer = ~sum;                      // truncate to 16 bits

    return answer;
}

int validateNumber(char *str) {
    while (*str) {
        if(!isdigit(*str)){ //if the character is not a number, return
            false;
            return 0;
        }
        str++; //point to next character
    }
    return 1;
}
int validateIp(char *ip) { //check whether the IP is valid or not
    int i, num, dots = 0;
    char *ptr;
    char *temp = ip;
    if (temp == NULL)
        return 0;
    ptr = strtok(temp, "."); //cut the string using dor delimiter
    if (ptr == NULL)
        return 0;
    while (ptr) {
        if (!validateNumber(ptr)) //check whether the sub string is holding only number or not
            return 0;
        num = atoi(ptr); //convert substring to number
        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, "."); //cut the next part of the string
            if (ptr != NULL)
                dots++; //increase the dot count
        } else
            return 0;
    }
    if (dots != 3) //if the number of dots are not 3, return false
        return 0;
    return 1;
}

