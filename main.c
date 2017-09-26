#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "main.h"

int listenSocket;
int messageSocket;
int dataSocket;

int main(int argc, char **argv) {
    bool isClient = false;
    bool isServer = false;

    int option;

    while ((option = getopt(argc, argv, "cs")) != -1) {
        switch (option) {
            case 'c':
                isClient = true;
                break;
            case 's':
                isServer = true;
                break;
        }
    }
    if (isClient == isServer) {
        puts("This program must be run with either the -c or -s flag, but not both.");
        puts("Please re-run this program with one of the above flags.");
        puts("-c represents client mode, -s represents server mode");
        return EXIT_SUCCESS;
    }

    listenSocket = createSocket();
    bindSocket(listenSocket, LISTENPORT);
    listen(listenSocket, 5);

    if (isServer) {
        startServer();
    } else {
        startClient();
    }
    return EXIT_SUCCESS;
}

void startServer(void) {
    struct sockaddr_in clientAddr; 
    socklen_t clientAddrSize = sizeof(struct sockaddr_in);
    memset(&clientAddr, 0, sizeof(struct sockaddr_in));

    if ((messageSocket = accept(listenSocket, (struct sockaddr *) &clientAddr, &clientAddrSize)) == -1) {
        perror("Accept failure");
        exit(EXIT_FAILURE);
    }
    
    if (connect(dataSocket, (struct sockaddr *) &clientAddr, clientAddrSize) == -1) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }
    setNonBlocking(dataSocket);
    //Do stuff now
}

void startClient(void) {
    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(struct sockaddr_in));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(LISTENPORT);
    destAddr.sin_addr.s_addr = *(getDestination()->h_addr_list[0]);

    if (connect(messageSocket, (struct sockaddr *) &destAddr, sizeof(destAddr)) == -1) {
        perror("Connection error");
        exit(EXIT_FAILURE);
    }
    if ((dataSocket = accept(listenSocket, NULL, NULL)) == -1) {
        perror("Accept failure");
        exit(EXIT_FAILURE);
    }
    setNonBlocking(dataSocket);
    //Do stuff now
    char *cmd = malloc(1024);
    char *filename = malloc(1024);
    getCommand(&cmd, &filename);
    if (cmd[0] == 'G') {
        //GET command
        const size_t BUF_SIZE = strlen(cmd) + strlen(filename) + 1;
        char *buffer = malloc(BUF_SIZE);
        memcpy(buffer, cmd, strlen(cmd));
        buffer[strlen(cmd)] = ' ';
        memcpy(buffer + strlen(cmd) + 1, filename, strlen(filename));

        send(messageSocket, buffer, BUF_SIZE, 0);

        char response;
        recv(messageSocket, &response, 1, 0);
        switch(response) {
            case 'G':
                //Do reading
                break;
            case 'B':
                printf("%s does not exist on the server\n", filename);
                break;
            default:
                puts("Server sent back bad response");
                exit(EXIT_FAILURE);
        }
        free(buffer);
    } else {
        //SEND comand
        sendFile(messageSocket, dataSocket, filename);
    }
    free(cmd);
    free(filename);
}

int createSocket(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return sock;
}

void setNonBlocking(int sock) {
    if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
        perror("Non blockign set failed");
        exit(EXIT_FAILURE);
    }
}

void bindSocket(const int sock, const unsigned short port) {
    struct sockaddr_in myAddr;
    memset(&myAddr, 0, sizeof(struct sockaddr_in));
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(port);
    myAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &myAddr, sizeof(struct sockaddr_in)) == -1) {
        perror("Socket binding failed");
        exit(EXIT_FAILURE);
    }
}

struct hostent * getDestination(void) {
    char *userAddr = getUserInput("Enter the destination address: ");
    struct hostent *addr = gethostbyname(userAddr);
    free(userAddr);
    return addr;
}

char * getUserInput(const char *prompt) {
    const size_t MAX_SIZE = 1024;
    char *buffer = calloc(MAX_SIZE, sizeof(char));
    if (buffer == NULL) {
        perror("Allocation failure");
        abort();
    }
    printf("%s", prompt);
    int c;
    for (;;) {
        c = getchar();
        if (c == EOF) {
            break;
        }
        if (!isspace(c)) {
            ungetc(c, stdin);
            break;
        }
    }
    size_t n = 0;
    for (;;) {
        c = getchar();
        if (c == EOF || (isspace(c) && c != ' ')) {
            buffer[n] = '\0';
            break;
        }
        buffer[n] = c;
        if (n == MAX_SIZE - 1) {
            printf("Message too big\n");
            memset(buffer, 0, MAX_SIZE);
            while ((c = getchar()) != '\n' && c != EOF) {}
            n = 0;
            continue;
        }
        ++n;
    }
    return buffer;
}

void sendFile(int commSock, int dataSock, const char *filename) {
    FILE *fp = fopen(filename, "r");
    const char bad = 'B';
    const char good = 'G';
    if (fp == NULL) {
        send(commSock, &bad, 1, 0);
        return;
    }
    send(commSock, &good, 1, 0);
    const size_t BUF_SIZE = 4096;
    char buffer[BUF_SIZE];
    memset(&buffer, 0, BUF_SIZE);
    int nread;
    while ((nread = fread(buffer, sizeof(char), BUF_SIZE, fp)) > 0) {
        send(dataSock, &buffer, nread, 0);
    }
    fclose(fp);
}

int createEpollFD(void) {
    int epollfd;
    if ((epollfd = epoll_create1(0)) == -1) {
        perror("Epoll create");
        exit(EXIT_FAILURE);
    }
    return epollfd;
}

void addEpollSoocket(const int epollfd, const int sock, struct epoll_event *ev) {
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

void getCommand(char **command, char **filename) {
    char *message;
    int n;
    for(;;) {
        message = getUserInput("Enter GET/SEND followed by the desired filename: "); 
        if ((n = sscanf(message, "%s %s", *command, *filename)) == EOF) {
            perror("sscanf");
            exit(EXIT_FAILURE);
        }
        free(message);
        if (n == 2) {
            //Validate
            if (strncmp(*command, "GET", 3) == 0) {
                if (access(*filename, R_OK) == 0) {
                    break;
                }
            } else if (strncmp(*command, "SEND", 4) == 0) {
                if (access(*filename, R_OK) == 0) {
                    break;
                }
            }
        }
        printf("Invalid input\n");
    }
}
