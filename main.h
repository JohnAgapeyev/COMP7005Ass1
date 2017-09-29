#ifndef MAIN_H
#define MAIN_H

#define LISTENPORT 7005
#define MAX_USER_BUFFER 1024
#define MAX_READ_BUFFER 4096
#define MAX_EPOLL_EVENTS 100
extern int listenSocket;
extern int messageSocket;
extern int dataSocket;
extern bool isClient;
extern bool isServer;

struct client {
    int messageSocket;
    int dataSocket;
};

void startServer(void);
void startClient(void);
int createSocket(void);
void setNonBlocking(int sock);
void bindSocket(const int sock, const unsigned short port);
struct hostent * getDestination(void);
char *getUserInput(const char *prompt);
void sendFile(int commSock, int dataSock, const char *filename);
int createEpollFD(void);
void addEpollSocket(const int epollfd, const int sock, struct epoll_event *ev);
void getCommand(char **command, char **filename);
int waitForEpollEvent(const int epollfd, struct epoll_event *events);

#endif
