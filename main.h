#ifndef MAIN_H
#define MAIN_H

#define LISTENPORT 7005
#define MAX_BUFFER 1024
extern int listenSocket;
extern int messageSocket;
extern int dataSocket;

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
void addEpollSoocket(const int epollfd, const int sock, struct epoll_event *ev);
void getCommand(char **command, char **filename);

#endif
