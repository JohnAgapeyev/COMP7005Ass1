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

void startServer(void);
void startClient(void);
void getMessage(void);

#endif
