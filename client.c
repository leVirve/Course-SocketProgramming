#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"

#define max(a,b) ((a) > (b) ? (a) : (b))

int exit_err(char* str);
void service(int sockfd);

int main (int argc, char **argv)
{
    int sockfd, n, i;
    char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;

    if(argc != 2) exit_err("usage: a.out <IPaddress>");
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) exit_err("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        exit_err("inet_ption error for %s");//, argv[1]);
    if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
        exit_err("connect error");

    service(sockfd);

    exit(0);
}

void service(int sockfd)
{
    char send[MAXLINE + 1];
    char recv[MAXLINE + 1];
    int n;

    printf("[C]hange folder [L]ist [U]pload [D]ownload [E]xit\n");
    while(fgets(send, MAXLINE, stdin) != NULL && send[0] != 'E') {
        write(sockfd, send, strlen(send));
        n = read(sockfd, recv, MAXLINE);
        recv[n] = '\0';
        printf("%s", recv);
    	printf("[C]hange folder [L]ist [U]pload [D]ownload [E]xit\n");
    }
    puts("Exit");
}

int exit_err(char* str)
{
    perror(str);
    exit(1);
}
