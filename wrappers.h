/*
 * HEADER FILE: wrappers.h - A collection is helpful functions used throughout the program
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

#ifndef WRAPPERS_H
#define WRAPPERS_H

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
void saveToFile(const char *filename);
void forwardUserCommand(const char *cmd, const char *filename);

#endif
