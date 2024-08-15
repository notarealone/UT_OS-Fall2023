#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <cjson/cJSON.h>

#define TRUE 1
#define FALSE 0

#define STDOUT 1
#define STDIN 0
#define LOGON_MSG "Please enter your username : "
#define LOGIN_ERR "Pleas login first !"

#define MAX_TCP 6000
#define MIN_TCP 5000

#define MAX_CONNECTION 10

#define BUF_LENGTH 128

#define MAX_USERNAME_LEN 30
#define MAX_USERNAME_NUM 30

#define LOCKED 1
#define UNLOCKED 0

#define OPENED 1
#define CLOSED 0

int numOfUsers = 0;
int lock = LOCKED;
int status = CLOSED;

static int BROADCAST_PORT;
struct sockaddr_in BROADCAST_ADDR;

int setupBroadcast(int bc_port){
    int sock, bc = 1, opt = 1;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    BROADCAST_ADDR.sin_family = AF_INET;
    BROADCAST_ADDR.sin_port = htons(bc_port);
    BROADCAST_ADDR.sin_addr.s_addr = inet_addr("127.0.1.1");

    bind(sock, (struct sockaddr *)&BROADCAST_ADDR, sizeof(BROADCAST_ADDR));
    
    return sock;
}


int setupTCP(int port) {
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, MAX_CONNECTION);

    return server_fd;
}

int reqTCP(int port){
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(fd, (struct sockaddr *)&server_address, sizeof(server_address));

    return fd;
}

int acceptTCP(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}


int main(int argc, const char *argv[]){

    char username[MAX_USERNAME_NUM][MAX_USERNAME_LEN] = {'\0'};
    char buffer[BUF_LENGTH] = {0};
    fd_set all_fd, ready_fd;
    FD_ZERO(&all_fd);
    FD_SET(STDIN, &all_fd);
    if (argc != 2){
        write(STDOUT, "Wrong number of arguments!", strlen("Wrong number of arguments!"));
        exit(1);
    }

    BROADCAST_PORT = atoi(argv[1]);
    int broadcast_fd = setupBroadcast(BROADCAST_PORT);
    FD_SET(broadcast_fd, &all_fd);

    write(STDOUT, LOGON_MSG, strlen(LOGON_MSG));
    int user_length = read(STDIN, buffer, sizeof(buffer));
    buffer[user_length-1] = '\0';
    strcpy(username[0], buffer);
    write(STDOUT, "Welcome ", strlen("Welcome "));
    write(STDOUT, buffer, user_length - 1);
    write(STDOUT, " as Restaurant !\n", strlen(" as Restaurant !\n"));

    while(TRUE){
        ready_fd = all_fd;

        select(FD_SETSIZE, &ready_fd, NULL, NULL, NULL);

        for(int i = 0; i < FD_SETSIZE; i++){
            if(FD_ISSET(i, &ready_fd)){
                if(i == broadcast_fd){
                    recv(broadcast_fd, buffer, BUF_LENGTH, 0);
                }
                if(i == STDIN){
                    if(lock == UNLOCKED){
                        int temp = read(STDIN, buffer, BUF_LENGTH);
                        buffer[temp] = '\0';
                        if(strcmp(buffer, "start working")){
                            if(status = CLOSED){
                                status = OPENED;
                                char bc_msg[BUF_LENGTH];
                                strcpy(bc_msg, username[0]);
                                strcat(bc_msg, " Restaurant opened !\n");
                                sendto(broadcast_fd, bc_msg, strlen(bc_msg), 0, (struct sockaddr *)&BROADCAST_ADDR, sizeof(BROADCAST_ADDR));
                            }
                            else {
                                write(STDOUT, "Already Open! \n", strlen("Already Open! \n"));
                            }
                        }
                        else if(strcmp(buffer, "break")){
                            if(status == OPENED){    
                                status = CLOSED;
                                char bc_msg[BUF_LENGTH];
                                strcpy(bc_msg, username[0]);
                                strcat(bc_msg, " Restaurant closed !\n");
                                sendto(broadcast_fd, bc_msg, strlen(bc_msg), 0, (struct sockaddr *)&BROADCAST_ADDR, sizeof(BROADCAST_ADDR));
                            }
                            else {
                                write(STDOUT, "Already Closed! \n", strlen("Already Closed! \n"));
                            }
                        }
                    }
                    else {
                        write(STDOUT, LOGIN_ERR, strlen(LOGIN_ERR));
                    }
                    FD_CLR(i, &ready_fd);
                }
            }
        }
    }

    return 0;
}