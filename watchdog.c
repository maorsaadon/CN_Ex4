#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>

#define PING_PORT 3000
#define BUFFER_SIZE 16

int main()
{
    // (1)
    // Open the listening socket
    int listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == -1) {
        printf("Could not create listening socket : %d", errno);
        return 1;
    }

    // for reusing of port
    int enableReuse = 1;
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        printf("setsockopt() failed with error code : %d", errno);
        return 1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in serverAddress;

    /*
    fills the first 'sizeof(serverAddress)' bytes of the memory
    area pointed to by &serverAddress with the constant 0.
    */
    memset(&serverAddress, 0, sizeof(serverAddress));

    // address family, AF_INET(while using TCP) undighned
    serverAddress.sin_family = AF_INET;
    // any IP at this port (Address to accept any incoming messages)
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    // change the port number from little endian => network endian(big endian):
    serverAddress.sin_port = htons(PING_PORT);  // network order (makes byte order consistent)

    // (2)
    // Bind the socket to the port with any IP at this port
    int bindResult = bind(listeningSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    // checking
    if (bindResult == -1) {
        printf("Bind failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    } else printf("executed Bind() successfully\n");

    // (3)
    // Make the socket listen.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections = 3
    int listenResult = listen(listeningSocket, 3);
    //checking
    if (listenResult == -1) {
        printf("listen() failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    } else printf("Waiting for incoming TCP-connections...\n");

    // Accept and incoming connection
    struct sockaddr_in clientAddress;

    /*
    fills the first 'sizeof(clientAddress)' bytes of the memory
    area pointed to by &clientAddress with the constant 0.
    */
    memset(&clientAddress, 0, sizeof(clientAddress));

    // saving the size of clientAddress in socklen_t variable
    socklen_t len_clientAddress = sizeof(clientAddress);

    // (4)
    // accept a connection on a socket
    int clientSocket = accept(listeningSocket, (struct sockaddr *) &clientAddress, &len_clientAddress);
    // checking
    if (clientSocket == -1) {
        printf("listen failed with error code : %d", errno);
        close(listeningSocket);
        return -1;
    } else printf("A new client connection accepted\n");

    while (1){
        int signal = 0;
        int r= recv(clientSocket, &signal, sizeof(int), 0);//receiving from new_ping the sign
        if(r==-1){
            break;
        }
        sleep(10);
    }
    int out = 1;
    int check = send(clientSocket, &out, sizeof(int), 0);
    if (check == -1) printf("send() failed with error code : %d", errno);
    else if (check == 0) printf("peer has closed the TCP connection prior to send().\n");
//    // (5)
//    struct timeval start, end;
//    gettimeofday(&start, 0);
//
//    int signal = 0;
//    recv(clientSocket, &signal, sizeof(int), 0);//receiving from new_ping the sign
//
//    gettimeofday(&end, 0);
//
//    float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
//
//    if(milliseconds >=10 ) {
//        int out = 1;
//        int check = send(clientSocket, &out, sizeof(int), 0);
//        if (check == -1) printf("send() failed with error code : %d", errno);
//        else if (check == 0) printf("peer has closed the TCP connection prior to send().\n");
//    }



    printf("Exit\n");
    close(clientSocket);//close the clientSocket
    close(listeningSocket);//listeningSocket
    return 0;

}