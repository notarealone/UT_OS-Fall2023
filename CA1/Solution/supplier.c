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

#define BUF_LENGTH 128

#define MAX_USERNAME_LEN 30
#define MAX_USERNAME_NUM 30

int numOfUsers = 0;

static int BROADCAST_PORT;
struct sockaddr_in BROADCAST_ADDR;

int setupBroadcast(int bc_port){
    int sock, bc = 1, opt = 1;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    BROADCAST_ADDR.sin_family = AF_INET;
    BROADCAST_ADDR.sin_port = htons(bc_port);
    BROADCAST_ADDR.sin_addr.s_addr = inet_addr("192.168.1.255");

    bind(sock, (struct sockaddr *)&BROADCAST_ADDR, sizeof(BROADCAST_ADDR));
    
    return sock;
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
    write(STDOUT, "Welcome ", strlen("Welcome "));
    write(STDOUT, buffer, user_length - 1);
    write(STDOUT, " as Supplier !", strlen(" as Supplier !"));

    while(TRUE){
        ready_fd = all_fd;

        select(FD_SETSIZE, &ready_fd, NULL, NULL, NULL);

        for(int i = 0; i < FD_SETSIZE; i++){
            if(FD_ISSET(i, &ready_fd)){
                if(i == broadcast_fd){
                    recv(broadcast_fd, buffer, BUF_LENGTH, 0);
                    FD_CLR(i, &ready_fd);
                }
            }
        }
    }

    return 0;
}