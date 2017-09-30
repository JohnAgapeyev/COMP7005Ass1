/*
 * HEADER FILE: network.h - The key networking components of the application
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

#ifndef NETWORK_H
#define NETWORK_H

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
