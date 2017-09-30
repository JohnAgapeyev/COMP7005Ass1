#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include "wrappers.h"
#include "network.h"

int createSocket(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        perror("Socket options failed");
        exit(EXIT_FAILURE);
    }
    return sock;
}

void setNonBlocking(int sock) {
    if (fcntl(sock, F_SETFL, O_NONBLOCK | SO_REUSEADDR) == -1) {
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
    if (addr == NULL) {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    free(userAddr);
    return addr;
}

char *getUserInput(const char *prompt) {
    char *buffer = calloc(MAX_USER_BUFFER, sizeof(char));
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
        if (n == MAX_USER_BUFFER - 1) {
            printf("Message too big\n");
            memset(buffer, 0, MAX_USER_BUFFER);
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
    if (isServer) {
        send(commSock, &good, 1, 0);
    }
    char buffer[MAX_READ_BUFFER];
    memset(&buffer, 0, MAX_READ_BUFFER);
    int nread;
    while ((nread = fread(buffer, sizeof(char), MAX_READ_BUFFER, fp)) > 0) {
        int dataToSend = nread;
        int nSent;
        char *bp = buffer;

start:
        nSent = send(dataSock, bp, dataToSend, 0);
        if (nSent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            goto start;
        }
        if (nSent < dataToSend) {
            dataToSend -= nSent;
            bp += nSent;
            goto start;
        }
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

void addEpollSocket(const int epollfd, const int sock, struct epoll_event *ev) {
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
            if (strncmp(*command, "GET", 3) == 0
                    || (strncmp(*command, "SEND", 4) == 0 && access(*filename, R_OK) == 0)) {
                break;
            }
        }
        printf("Invalid input\n");
    }
}

int waitForEpollEvent(const int epollfd, struct epoll_event *events) {
    int nevents;
    if ((nevents = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1)) == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }
    return nevents;
}

void saveToFile(const char *filename) {
    char *dataBuf = calloc(MAX_READ_BUFFER, sizeof(char));
    int n;
    FILE *fp = fopen(filename, "w");
    while((n = recv(dataSocket, dataBuf, MAX_READ_BUFFER, 0)) > 0) {
        fwrite(dataBuf, sizeof(char), n, fp);
    }
    free(dataBuf);
    fclose(fp);
}

void forwardUserCommand(const char *cmd, const char *filename) {
    const size_t BUF_SIZE = strlen(cmd) + strlen(filename) + 1;
    char *buffer = malloc(BUF_SIZE);
    memcpy(buffer, cmd, strlen(cmd));
    buffer[strlen(cmd)] = ' ';
    memcpy(buffer + strlen(cmd) + 1, filename, strlen(filename));

    send(messageSocket, buffer, BUF_SIZE, 0);

    free(buffer);
}
