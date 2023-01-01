#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>

#define WATCHDOG_PORT 3000

int main(int argc, char *argv[])
{
    // Open the listening socket
    int listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == -1) {
        printf("Could not create listening socket : %d\n", errno);
        return 1;
    }

    //fo reusing of port
    int enableReuse = 1;
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        printf("setsockopt() failed with error code : %d\n", errno);
        return 1;
    }
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(WATCHDOG_PORT);

    // Bind the socket to the port with any IP at this port
    int bindResult = bind(listeningSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (bindResult == -1) {
        printf("Bind failed with error code : %d\n", errno);
        close(listeningSocket);
        return -1;
    }

    // Make the socket listen.
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1) {
        printf("listen() failed with error code : %d\n", errno);
        close(listeningSocket);
        return -1;
    }

    // Accept and incoming connection
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(clientAddress));
    socklen_t len_clientAddress = sizeof(clientAddress);
    int clientSocket = accept(listeningSocket, (struct sockaddr *) &clientAddress, &len_clientAddress);
    if (clientSocket == -1) {
        printf("listen failed with error code : %d\n", errno);
        close(listeningSocket);
        return -1;
    }

    //Change the sockets into non-blocking state
    fcntl(listeningSocket, F_SETFL, O_NONBLOCK);
    fcntl(clientSocket,F_SETFL, O_NONBLOCK);//Change the socket into non-blocking state

    //for calculate the time
    struct timeval start, end;
    float seconds = 0;
    sleep(1);
    //checking if watchdog receive something from ping if yes then update the start time if no then the time will stop updating and then second>10 -> shut down
    while (seconds <= 10){
        int new_ping_ready = 1;
        int r= recv(clientSocket, &new_ping_ready, sizeof(int), 0);
        if(r>0){
            gettimeofday(&start, 0);
        }
        gettimeofday(&end, 0);
        seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0f;
    }

    close(clientSocket);//close the clientSocket
    close(listeningSocket);//listeningSocket
    printf("server <%s> cannot be reached.\n", argv[1]);

    //reboot the system
    kill(getppid(),SIGKILL);

    return 0;

}