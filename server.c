#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "const.h"

#define OPEN_MAX 1024
#define LISTENQ 1024
#define INFTIM -1

int exit_err(char* str)
{
    perror(str);
    exit(1);
}

void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    pid = wait(&stat);
    printf("child %d terminated\n", pid);
    return;
}

void initial()
{
    system("mkdir -p Upload");
}

void print(int sockfd, char* str)
{
    struct sockaddr client;
    socklen_t clilen = sizeof(client);
    getpeername(sockfd, &client, &clilen);
    struct sockaddr_in* clientaddr = (struct sockaddr_in*) &client;
    printf("%s:%d %s\n", inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port), str);
}

void serve_for(int sockfd, char *line)
{
    int saved_stdout = dup(1);
    dup2(sockfd, 1);

    char instruction = line[0];
    char filename[255];
    char* buffer = line+2;
    size_t n, c, recv_bytes, filesize;
    FILE* f;
    struct stat fst;

    switch(instruction) {
        case 'L': 
            system("ls -al");
            break;
        case 'C': 
            
            // TO-DO: on `master`
            
            chdir(buffer);
            system("pwd");
            break;
        case 'U':
                  dup2(saved_stdout, 1);
                  sprintf(filename, "./Upload/%s", buffer); // No need
                  f = fopen(filename, "wb");
                  printf("[DEBUG] filename=%s\n", buffer);

                  bzero(buffer, MAXLINE);
                  recv_bytes = 0;
                  recv(sockfd, buffer, MAXLINE, 0);
                  sscanf(buffer, "%zd", &filesize);
                  printf("[DEBUG] filesize=%zd\n", filesize);

                  while ((n = recv(sockfd, buffer, MAXLINE, 0)) > 0) {
                      if ((c = fwrite(buffer, sizeof(char), n, f)) < n) {
                          printf("write error c=%zd < n=%zd\n", c, n);
                          exit(1);
                      }
                      recv_bytes += n;
                      bzero(buffer, MAXLINE);

                      if (recv_bytes >= filesize) break;
                  }
                  fclose(f);

                  printf("[DEBUG] Write finished\n");
                  sprintf(buffer, "Uploaded sucess!\n");
                  write(sockfd, buffer, MAXLINE);
                  break;
        case 'D':
                  dup2(saved_stdout, 1);
                  sprintf(filename, "./Upload/%s", buffer); // No need
                  f = fopen(filename, "rb");
                  if (f == NULL) {
                      // TO-DO: send msg to client
                      printf("[DEBUG] No such file: %s\n", filename);
                      return;
                  }
                  fstat(fileno(f), &fst);
                  filesize = fst.st_size;
                  printf("[DEBUG] filename=%s (size: %zd)\n", filename, filesize);

                  bzero(buffer, MAXLINE);
                  sprintf(buffer, "%zd", filesize);
                  send(sockfd, buffer, MAXLINE, 0);

                  while((n = fread(buffer, sizeof(char), MAXLINE, f)) > 0) {
                      if (send(sockfd, buffer, n, 0) < 0) exit_err("send error");
                      bzero(buffer, MAXLINE);
                  }
                  fclose(f);
                  printf("[DEBUG] Send finished\n");
                  break;
        default: execlp("echo", "echo", "No instruction", NULL); break;
    }
    dup2(saved_stdout, 1);
}

int main(int argc, char **argv)
{
    int listenfd, connfd, sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t clilen;
    ssize_t n;
    pid_t pid;
    int i;
    char line[MAXLINE + 1];

    initial();

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        exit_err("Socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        exit_err("Binding error");

    if(listen(listenfd, LISTENQ) == -1)
        exit_err("Listen error");

    //signal (SIGCHLD, sig_chld);

    for(;;) {
        clilen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clilen);
        print(connfd, "connected");

        if ((pid = fork()) == 0) {
            close(listenfd);
            while(1) {
                if ((n = read(connfd, line, MAXLINE)) > 0) {

                    line[n-1] = '\0';
                    printf("[DEBUG] recv %s\n", line);

                    serve_for(connfd, line);
                } else {
                    break;
                }
            }
            print(connfd, "disconnected");
            exit(0);
        } else {
            close(connfd);
            //int wpid, status = 0;
                  
            //while ((wpid = wait(&status)) > 0) {
            //    printf("Exit status of %d was %d (%s)\n", (int)wpid, status,
            //            (status > 0) ? "accept" : "reject");
            //}
        }
    }

    return 0;
}

