#include "tcp_server.h"

void service(int sockfd)
{
    ssize_t n;
    char mesg[MAXLINE];
    char user[MAXLINE], instr[MAXLINE];
again:
    while((n = read(sockfd, mesg, MAXLINE)) > 0) {
        mesg[n] = '\0';
        sscanf(mesg, "%s %s", user, instr);
        DEBUG("%s-%c\n", user, instr[0]);
        switch(instr[0]) {
            case 'L': case 'R': case 'E': case 'X':
                account_processing(sockfd, mesg);
                break;
            case 'I': case 'F':
                list_infomation(sockfd, mesg);
                break;
            case 'T': case 'Y':
                //p2p_chat_system(mesg);
                break;
            case 'D': case 'U':
                //p2p_file_system(mesg);
                break;
            default:
                break;
        }
        write(sockfd, mesg, MAXLINE);
        bzero(mesg, MAXLINE);
    }
    if (n < 0 && errno == EINTR) goto again;
    else if (n < 0) printf("str_echo: read error");
}

static void* serve(void* arg)
{
    int connfd = *((int*) arg);
    free(arg);

    pthread_detach(pthread_self());

    service(connfd);

    close(connfd);
    return NULL;
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    pthread_t tid;
    socklen_t addrlen, len;
    struct sockaddr* clientaddr;

    if (argc < 2) exit_err("usage: <server> [<host>] <service/port>");
    listenfd = tcp_listen(NULL, argv[1], &addrlen);
    clientaddr = (struct sockaddr*) malloc(addrlen);

    for(;;) {
        len = addrlen;
        connfdp = (int*) malloc(sizeof(int));
        *connfdp = accept(listenfd, clientaddr, &len);

        printf("%d\n", *connfdp);

        pthread_create(&tid, NULL, &serve, (void*) connfdp);
    }

    return 0;
}

