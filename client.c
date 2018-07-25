#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <time.h>

#include "common.h"

int main(int argc, char* argv[]) {
    int ret;

    // variables for handling a socket
    int socket_desc;
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    // create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0) handle_error("Could not create socket");

    // set up parameters for the connection
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    // initiate a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if(ret) handle_error("Could not create connection");

    char buf[1024];
    size_t buf_len = sizeof(buf);
    int msg_len;

    // display welcome message from server
    while ( (msg_len = recv(socket_desc, buf, buf_len - 1, 0)) < 0 ) {
        if (errno == EINTR) continue;
        handle_error("Cannot read from socket");
    }
    buf[msg_len] = '\0';
    printf("%s", buf);

    
    memset(&buf, 0, sizeof(buf));
    sprintf(buf, "%lu\n", (unsigned long)time(NULL));
    msg_len = strlen(buf);

        // send message to server
    while ( (ret = send(socket_desc, buf, msg_len, 0)) < 0) {
        if (errno == EINTR) continue;
        handle_error("Cannot write to socket");
    }

    // close the socket
    ret = close(socket_desc);
    if(ret) handle_error("Cannot close socket");

    exit(EXIT_SUCCESS);
}
