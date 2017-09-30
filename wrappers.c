/*
 * SOURCE FILE: wrappers.c - Implementation of functions declared in wrappers.h
 *
 * PROGRAM: COMP7005Ass1
 *
 * DATE: Sept. 30, 2017
 *
 * FUNCTIONS:
 * int createSocket(void);
 * void setNonBlocking(int sock);
 * void bindSocket(const int sock, const unsigned short port);
 * struct hostent * getDestination(void);
 * char *getUserInput(const char *prompt);
 * void sendFile(int commSock, int dataSock, const char *filename);
 * int createEpollFD(void);
 * void addEpollSocket(const int epollfd, const int sock, struct epoll_event *ev);
 * void getCommand(char **command, char **filename);
 * int waitForEpollEvent(const int epollfd, struct epoll_event *events);
 * void saveToFile(const char *filename);
 * void forwardUserCommand(const char *cmd, const char *filename);
 *
 * DESIGNER: John Agapeyev
 *
 * PROGRAMMER: John Agapeyev
 */

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

/*
 *  FUNCTION: createSocket
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
 *  int createSocket(void)
 *
 *  RETURNS:
 *  int - A socket file desciptor
 *
 *  NOTES:
 *  This function wraps the socket creation api call.
 *  It also sets the SO_REUSEADDR option on the socket
 *  for quick debugging.
 */
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

/*
 *  FUNCTION: setNonBlocking
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
 *  void setNonBlocking(int sock)
 *
 *  PARAMETERS:
 *  int sock - The socket desciptor to set to non-blocking mode
 *
 *  RETURNS:
 *  void
 */
void setNonBlocking(int sock) {
    if (fcntl(sock, F_SETFL, O_NONBLOCK | SO_REUSEADDR) == -1) {
        perror("Non blockign set failed");
        exit(EXIT_FAILURE);
    }
}

/*
 *  FUNCTION: bindSocket
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
 *  void bindSocket(const int sock, const unsigned short port);
 *
 *  PARAMETERS:
 *  const int sock - The socket to bind
 *  const unsigned short - The port number in host byte order to bind to
 *
 *  RETURNS:
 *  void
 */
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

/*
 *  FUNCTION: getDestination
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
 *  struct hostent * getDestination(void);
 *
 *  RETURNS:
 *  struct hostent * - A pointer to a hostent structure of the destination's address
 */
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

/*
 *  FUNCTION: getUserInput
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
 *  char *getUserInput(const char *prompt);
 *
 *  PARAMETERS:
 *  const char **prompt - A pointer to a string to be used to prompt the user
 *
 *  RETURNS:
 *  char * - A string containing the user's input
 *
 *  NOTES:
 *  This function strips leading whitespace and is guaranteed to append a null character.
 *  Input max length is set to 1024 for simplicity, and attempting to surpass this limit results
 *  in standard in being flushed, and the input buffer being zero'd out.
 */
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

/*
 *  FUNCTION: sendFile
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
 *  void sendFile(int commSock, int dataSock, const char *filename);
 *
 *  PARAMETERS:
 *  int commSock - The socket used for message communication
 *  int dataSock - The socket used for data transfer
 *  const char *filename - The filename of the file to send
 *
 *  RETURNS:
 *  void
 *
 *  NOTES:
 *  This function will send a single char over the commSock corresponding as to whether the
 *  desired file was found on the server.
 *  This allows the client to quickly address an invalid output, while confirming if the request is valid.
 */
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

/*
 *  FUNCTION: createEpollFD
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
 *  int createEpollFD(void);
 *
 *  RETURNS:
 *  int - The epoll file descriptor that was created.
 */
int createEpollFD(void) {
    int epollfd;
    if ((epollfd = epoll_create1(0)) == -1) {
        perror("Epoll create");
        exit(EXIT_FAILURE);
    }
    return epollfd;
}

/*
 *  FUNCTION: addEpollSocket
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
 *  void addEpollSocket(const int epollfd, const int sock, struct epoll_event *ev);
 *
 *  PARAMETERS:
 *  const int epollfd - The epoll file desciptor to add the socket to
 *  const int sock - The socket to add
 *  struct epoll_event *ev - A pointer to an epoll event struct containing the relevant flags for the socket addition
 *
 *  RETURNS:
 *  void
 */
void addEpollSocket(const int epollfd, const int sock, struct epoll_event *ev) {
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, ev) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
}

/*
 *  FUNCTION: getCommand
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
 *  void getCommand(char **command, char **filename);
 *
 *  PARAMETERS:
 *  char **command - A pointer to a string in which to store the command
 *  char **filename - A pointer to a string in which to store the filename
 *
 *  RETURNS:
 *  void
 *
 *  NOTES:
 *  This function calls getUserInput and parses the result in order to get the filename and command.
 *  The input is then validated before the method exits.
 *  In getUserInput, the input buffer length is capped at 1024.
 *  The reason for that cap is that the command and filename buffers passed to this method must also be at least that big
 *  in order to guarantee no overflows occur.
 */
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

/*
 *  FUNCTION: waitForEpollEvent
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
 *  int waitForEpollEvent(const int epollfd, struct epoll_event *events);
 *
 *  PARAMETERS:
 *  const int epollfd - The epoll desciptor to wait on
 *  struct epoll_event *events - The list of epoll_event structs to fill with the detected events
 *
 *  RETURNS:
 *  int - The number of events that occurred
 */
int waitForEpollEvent(const int epollfd, struct epoll_event *events) {
    int nevents;
    if ((nevents = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1)) == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }
    return nevents;
}

/*
 *  FUNCTION: saveToFile
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
 *  void saveToFile(const char *filename);
 *
 *  PARAMETERS:
 *  const char *filename - The filename of the file in which to save the data
 *
 *  RETURNS:
 *  void
 */
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

/*
 *  FUNCTION: forwardUserCommand
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
 *  void forwardUserCommand(const char *cmd, const char *filename);
 *
 *  PARAMETERS:
 *  const char *cmd - The command to pass on
 *  const char *filename - The filename to pass on
 *
 *  RETURNS:
 *  void
 *
 *  NOTES:
 *  This method is simply a wrapper around creating a buffer, filling it with the command and filename
 *  and then sending it over the message socket.
 */
void forwardUserCommand(const char *cmd, const char *filename) {
    const size_t BUF_SIZE = strlen(cmd) + strlen(filename) + 1;
    char *buffer = malloc(BUF_SIZE);
    memcpy(buffer, cmd, strlen(cmd));
    buffer[strlen(cmd)] = ' ';
    memcpy(buffer + strlen(cmd) + 1, filename, strlen(filename));

    send(messageSocket, buffer, BUF_SIZE, 0);

    free(buffer);
}
