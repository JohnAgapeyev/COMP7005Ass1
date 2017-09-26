#include <sys/socket.h>
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
    //Do stuff now
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
    size_t MAX_SIZE = 200;
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
        if (c == EOF || isspace(c)) {
            buffer[n] = '\0';
            break;
        }
        buffer[n] = c;
        if (n == MAX_SIZE - 1) {
            MAX_SIZE *= 2;
            buffer = realloc(buffer, MAX_SIZE);
            if (buffer == NULL) {
                perror("Allocation failure");
                abort();
            }
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
