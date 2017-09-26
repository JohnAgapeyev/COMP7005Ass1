
#define LISTENPORT 7005
extern int listenSocket;
extern int messageSocket;
extern int dataSocket;

void startServer(void);
void startClient(void);
int createSocket(void);
void setNonBlocking(int sock);
void bindSocket(const int sock, const unsigned short port);
struct hostent * getDestination(void);
char * getUserInput(const char *prompt);
void sendFile(int commSock, int dataSock, const char *filename);
