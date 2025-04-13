
#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>

#define DEFAULT_PORT "80"
#define DEFAULT_BUFLEN 5000
#define WEBPATH "../web"
#define WEBROUTES "../web/routesConfig.txt"
#define MAX_PATH_LENGTH 100
#define MAX_METHOD_LENGTH 10



typedef struct threadArgs {
	SOCKET clientSocket;
	sockaddr_in clientAddress;
}threadArgs;

#endif