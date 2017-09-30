/*
 * SOURCE FILE: network.c - Implementation of functions declared in network.h
 *
 * PROGRAM: COMP7005Ass1
 *
 * DATE: Sept. 30, 2017
 *
 * FUNCTIONS:
 * void startServer(void);
 * void startClient(void);
 * void getMessage(void);
 *
 * VARIABLES:
 * int listenSocket - The socket that listens for incoming connections
 * int messageSocket - The socket that transfers commands
 * int dataSocket - The socket that transfers data
 * bool isClient - Whether the application is running as the client
 * bool isServer - Whether the application is running as the server
 *
 * DESIGNER: John Agapeyev
 *
 * PROGRAMMER: John Agapeyev
 */

#include <sys/epoll.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "network.h"
#include "wrappers.h"

int listenSocket;
int messageSocket;
int dataSocket;

bool isClient = false;
bool isServer = false;

/*
 *  FUNCTION: main
 *
 *  DATE:
 *  Sept. 30, 2017
 *
 *  DESIGNER:
 *  John Agapeyev
 *
 *  PROGRAMMER:
 *  John Agapeyev
 *
 *  INTERFACE:
 *  int main(int argc, char **argv);
 *
 *  PARAMETERS:
 *  int argc - The argument count
 *  char **argv - The list of command arguments
 *
 *  RETURNS:
 *  int
 */
int main(int argc, char **argv) {
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
    close(listenSocket);
    return EXIT_SUCCESS;
}

/*
 *  FUNCTION: startServer
 *
 *  DATE:
 *  Sept. 30, 2017
 *
 *  DESIGNER:
 *  John Agapeyev
 *
 *  PROGRAMMER:
 *  John Agapeyev
 *
 *  INTERFACE:
 *  void startServer(void);
 *
 *  RETURNS:
 *  void
 *
 *  NOTES:
 *  This function wraps the core functionality of the server
 */
void startServer(void) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(struct sockaddr_in);
    memset(&clientAddr, 0, sizeof(struct sockaddr_in));

    for (;;) {
        dataSocket = createSocket();

        if ((messageSocket = accept(listenSocket, (struct sockaddr *) &clientAddr, &clientAddrSize)) == -1) {
            perror("Accept failure");
            exit(EXIT_FAILURE);
        }

        clientAddr.sin_port = htons(LISTENPORT);

        if (connect(dataSocket, (struct sockaddr *) &clientAddr, clientAddrSize) == -1) {
            perror("Connection error");
            exit(EXIT_FAILURE);
        }
        setNonBlocking(messageSocket);

        getMessage();

        close(dataSocket);
        close(messageSocket);
    }
}

/*
 *  FUNCTION: startClient
 *
 *  DATE:
 *  Sept. 30, 2017
 *
 *  DESIGNER:
 *  John Agapeyev
 *
 *  PROGRAMMER:
 *  John Agapeyev
 *
 *  INTERFACE:
 *  void startClient(void);
 *
 *  RETURNS:
 *  void
 *
 *  NOTES:
 *  This function wraps the main functionality of the client
 */
void startClient(void) {
    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(struct sockaddr_in));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(LISTENPORT);
    struct hostent *hp = getDestination();
    memcpy(&destAddr.sin_addr, hp->h_addr_list[0], hp->h_length);

    for (;;) {
        messageSocket = createSocket();

        if (connect(messageSocket, (struct sockaddr *) &destAddr, sizeof(destAddr)) == -1) {
            perror("Connection error");
            exit(EXIT_FAILURE);
        }
        if ((dataSocket = accept(listenSocket, NULL, NULL)) == -1) {
            perror("Accept failure");
            exit(EXIT_FAILURE);
        }
        //Do stuff now
        char *cmd = malloc(MAX_USER_BUFFER);
        char *filename = malloc(MAX_USER_BUFFER);
        getCommand(&cmd, &filename);
        forwardUserCommand(cmd, filename);
        if (cmd[0] == 'G') {
            //GET command
            char response;
            recv(messageSocket, &response, 1, 0);
            switch(response) {
                case 'G':
                    //Do reading
                    saveToFile(filename);
                    break;
                case 'B':
                    printf("%s does not exist on the server\n", filename);
                    break;
                default:
                    puts("Server sent back bad response");
                    exit(EXIT_FAILURE);
            }
        } else {
            //SEND comand
            sendFile(messageSocket, dataSocket, filename);
        }
        free(cmd);
        free(filename);

        close(dataSocket);
        close(messageSocket);
    }
}

/*
 *  FUNCTION: getMessage
 *
 *  DATE:
 *  Sept. 30, 2017
 *
 *  DESIGNER:
 *  John Agapeyev
 *
 *  PROGRAMMER:
 *  John Agapeyev
 *
 *  INTERFACE:
 *  void getMessage(void)
 *
 *  RETURNS:
 *  void
 *
 *  NOTES:
 *  This method is only ever called by the server.
 *  This method waits on the message socket until there is an incoming request,
 *  then handles it appropriately.
 */
void getMessage(void) {
    int epollfd = createEpollFD();

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    ev.data.fd = messageSocket;

    addEpollSocket(epollfd, messageSocket, &ev);

    struct epoll_event *eventList = calloc(MAX_EPOLL_EVENTS, sizeof(struct epoll_event));

    int nevents = waitForEpollEvent(epollfd, eventList);
    for (int i = 0; i< nevents; ++i) {
        if (eventList[i].events & EPOLLERR) {
            perror("Socket error");
            close(eventList[i].data.fd);
            continue;
        } else if (eventList[i].events & EPOLLHUP) {
            close(eventList[i].data.fd);
            continue;
        } else if (eventList[i].events & EPOLLIN) {
            //Read from message socket
            char *buffer = calloc(MAX_READ_BUFFER, sizeof(char));
            int n;
            char *bp = buffer;
            size_t maxRead = MAX_READ_BUFFER - 1;
            while ((n = recv(eventList[i].data.fd, bp, maxRead, 0)) > 0) {
                bp += n;
                maxRead -= n;
            }
            buffer[MAX_READ_BUFFER - 1 - maxRead] = '\0';

            if (buffer[0] == 'G') {
                //Get message
                sendFile(messageSocket, dataSocket, buffer + 4);
            } else {
                //Send message
                if (buffer[0] == '\0') {
                    //Client closed the connection on us
                    exit(EXIT_SUCCESS);
                }
                saveToFile(buffer + 5);
            }
            free(buffer);
        }
    }
    free(eventList);
    close(epollfd);
}
