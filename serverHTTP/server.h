#define DEFAULT_PORT "80"
#define DEFAULT_BUFLEN 5000
#define WEBPATH "../web"
#define MAXPATHLENGTH 100
#include <stdio.h>
#include <string.h>
typedef struct threadArgs {
	SOCKET clientSocket;
	sockaddr_in clientAddress;
}threadArgs;