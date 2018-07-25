#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "common.h"


sem_t sem_cs;
char* quit_command = QUIT_COMMAND;
size_t quit_command_length;

typedef struct thread_args_s {

    int client_desc;
    struct sockaddr_in* socket_address;
} thread_args_t;

void connection_handler(int client_desc, struct sockaddr_in* client_addr){
    int ret;
    char buf[1024];
    int buf_size = sizeof(buf);
    int recv_bytes = 0;
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr->sin_port);
    sprintf(buf, "[Server] Ready to receive client info\n Client IP: %s Port: %d\n",client_ip, client_port);

    int msg_len = strlen(buf);

    while((ret = send(client_desc, buf, msg_len, 0)) < 0){
        if(ret == -1 && errno == EINTR) continue;
        if(ret == -1) handle_error("Cannot write to the socket.");
    }

    memset(&buf, 0, sizeof(buf));

    do{
        ret = recv(client_desc, buf, buf_size, 0);
        if(ret == 0) handle_error("Unexpected socket closing");
        if(ret == -1 && errno == EINTR) continue;
        if(ret == -1) handle_error("Cannot read from the socket");
        recv_bytes += ret;

    }while(buf[recv_bytes++] == '\n');

    printf("[Server] Opening accesses file on append mode.\n");
    
    ret = sem_wait(&sem_cs);
    FILE * f = fopen("accesses.txt", "a+");
    printf("[Server] Writing received data into the file.\n");
    fprintf(f, "%s", buf);
    fclose(f);
    ret = sem_post(&sem_cs);

    printf("[Server] Done!\n");
    printf("[Server] Closing connection...\n");

    ret = close(client_desc);
    if(ret) handle_error("Cannot close connection.");

}

void* thread_connection_handler(void* args){
    thread_args_t* thread_args = (thread_args_t *) args;
    connection_handler(thread_args->client_desc, thread_args->socket_address);
    free(thread_args);
    pthread_exit(NULL);

}

void mainServerProcess(int socket_desc){
    int ret;

    struct sockaddr_in* client_addr = calloc(1,sizeof(struct sockaddr_in));

    int sockaddr_len = sizeof(struct sockaddr_in);

    quit_command_length = strlen(quit_command);
    pid_t pid;

    pid = fork();

        if(pid == -1){
            handle_error("Cannot create process.");
        }else if(pid == 0){
            while(1){
                int client_desc = accept(socket_desc, (struct sockaddr*)client_addr, &sockaddr_len);
                if(client_desc == -1 && errno == EINTR) continue;
                if(client_desc < 0) handle_error("Cannot accept incoming connection");
                
                printf("[Server] Incoming connection accepted.\n");

                thread_args_t * args = malloc(sizeof(thread_args_t));
                args->client_desc = client_desc;
                args->socket_address = client_addr;

                pthread_t thread;
                ret = pthread_create(&thread, NULL, thread_connection_handler, (void*) args);
                if(ret) handle_error_en(ret,"Cannot create thread for incoming connection");

                ret = pthread_detach(thread);
                if(ret) handle_error_en(ret, "Cannot detach thread.");

                printf("[Server] Listening on port: %d...\n",SERVER_PORT);

            }
        
        }

        sleep(3);

        while(1){
            char buf[64];
        
            printf("[Server]: (QUIT to close the server)\n");
            scanf("%s",buf);
            int msg_len = strlen(buf);
            if(msg_len == quit_command_length && !memcmp(buf, quit_command, quit_command_length)){
                printf("[Server] Closing server...\n");
                ret = close(socket_desc);
                if(ret) handle_error("Cannot close socket");
                exit(EXIT_SUCCESS);
            }
            printf("[Server] ERR: Command not find.\n");
            memset(&buf, 0, sizeof(buf));
        }
}


int main(int argc, char const *argv[]){

    int ret;
    int socket_desc;

    ret = sem_init(&sem_cs, 0, 1);
    if(ret) handle_error("Cannot initialize semaphore.");

    struct sockaddr_in server_addr = {0};

    printf("[Server] Starting....\n");

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc < 0) handle_error_en(socket_desc, "cannot create socket.");

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    int reuse_addr_opt = 1;

    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEPORT, &reuse_addr_opt, sizeof(reuse_addr_opt));
    if(ret) handle_error("cannot use SO_REUSEPORT");

    ret = bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
    if(ret) handle_error_en(ret, "cannot bind on the choosen port.");

    ret = listen(socket_desc, MAX_CONN_SUPPORTED);
    if(ret) handle_error("cannot listen on socket");
    printf("[Server] Listening on port: %d...\n",SERVER_PORT);

    mainServerProcess(socket_desc);

    return 0;
}
