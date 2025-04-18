/*
	* da implementare:
	implementare cookies che scadono.

	tabella url->script nella funzione handleconnection(), facilmente implementabile non ancora implementata per facilitare il debug durante lo sviluppo.

	vedere cos'altro implementare.
*/

//se non definito lo definisce, non fa includere winsocket versione 1.1 a windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <thread>
#include "server.h"
//#include "operazioniDB.h
#include "../DATABASE/operazioniDB.h"
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

typedef struct {
	int status;       // Status code (0 for success, 1 for failure, etc.)
	const char* message; // Status message as a string
	char* body;
	char* cookie;
}scriptResponse;

typedef struct {
	char method[MAX_METHOD_LENGTH];
	char path[MAX_PATH_LENGTH];
	char* headers;
	char* body;
} HttpRequest;

FILE* routes;

HttpRequest parseHttpRequest(const char* rawRequest);
void generateCookie(char cookie[]);
scriptResponse handleUserDataPost(char* input);
void replacePlaceholder(char** source, const char* placeholder, const char* replacement);
char* readHtmlFileBody(const char* filename);
char* readHtmlFile(const char* filename);
void sendHttpResponse(SOCKET clientSocket, const int statusCode, const char* statusMessage, const char* contentType, const char* cookie, const char* content);
void URLDecode(char* str);
scriptResponse handleRegisterPost(char* input);
scriptResponse handleLoginPost(char* input);
void sendFile(SOCKET clientSocket, const char* filePath, const char* contentType);
DWORD WINAPI handle_connection(LPVOID  lpParam);

int main(int argc, char* argv[]) {
	InitializeLibrary();
	fopen_s(&routes, WEBROUTES, "r");
	if (routes == NULL) {
		perror("Error opening routes file");
		exit(1);
	}
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	/* result: A pointer to a structure of type addrinfo. It will be used to store the results of the address resolution.
		ptr: Another pointer to addrinfo that is currently set to NULL.
		hints: An instance of the addrinfo structure, used to provide hints to the 'getaddrinfo' function about the type of socket you want to create.*/
	struct addrinfo* result = NULL;

	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));// sets all the fields of hints to 0

	hints.ai_family = AF_INET; // Sets the address family to AF_INET, indicating that you want to work with IPv4 addresses.
	hints.ai_socktype = SOCK_STREAM; // Sets the socket type to SOCK_STREAM, indicating that you want to create a stream (TCP) socket. ( SOCK_DGRAM for UDP)
	hints.ai_protocol = IPPROTO_TCP; // Sets the protocol to IPPROTO_TCP, specifying that you intend to use the TCP protocol.
	hints.ai_flags = AI_PASSIVE; // Sets the AI_PASSIVE flag, which indicates that you are preparing to use the socket for listening (typically for a server application). It means the resulting socket will be suitable for binding to a local address.

	/*
		This line calls the getaddrinfo function to resolve the local address and port to be used by the server.
		NULL is used for the first argument, indicating that the function should determine the local address automatically. or (char* localIPAddress = "192.168.1.100") to specify an address
		DEFAULT_PORT is used as the port number. In this case, it's DEFAULT_PORT, which was defined as a macro at the beginning of the code.
		&hints provides the hints structure to guide the address resolution.
		&result is where the results will be stored.
		The result of the getaddrinfo function is stored in iResult. If it's non-zero, it indicates an error, and an error message is printed, followed by cleanup steps.
	*/
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	/*
		Create a SOCKET for the server to listen for client connections with the previous result struct fields.
		result->ai_family: This retrieves the address family (IPv4 or IPv6) from the addrinfo structure
		result->ai_socktype: This retrieves the socket type (e.g., SOCK_STREAM for TCP or SOCK_DGRAM for UDP) from the addrinfo structure.
		result->ai_protocol: This retrieves the protocol (e.g., IPPROTO_TCP for TCP or IPPROTO_UDP for UDP) from the addrinfo structure.
	*/

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	/*
		The bind function attempts to bind the socket specified by ListenSocket to the address information provided in result->ai_addr.
		If the binding is successful, it means that your server will listen for incoming connections on the specified IP address and port.
		ai_addrlen: This is the length (in bytes) of the socket address structure pointed to by result->ai_addr.
		If the bind function returns SOCKET_ERROR, it indicates an error in the binding process.
		In this case, an error message is printed, and the necessary cleanup steps are performed, including freeing the memory associated with result,
		closing the socket, and cleaning up the Winsock library using WSACleanup().
	*/
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	freeaddrinfo(result);

	/*
		SOMAXCONN: This is a constant that typically represents the maximum length of the queue of pending connections.
		It's a recommended value to use when calling the listen function. It indicates the maximum number of clients that can be waiting to connect to the server.
		However, the actual value may vary depending on the system.
		If listen fails, the code prints an error message that includes the error code obtained from WSAGetLastError()
		closesocket, which releases any resources associated with it.
		Finally, it cleans up the Winsock library using WSACleanup() and returns 1 to indicate that there was an error during server setup.
	*/
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Create a temporary SOCKET object called clientSocket for accepting connections from clients.
	SOCKET clientSocket;
	sockaddr_in addr;
	int addrSize = sizeof(addr);
	char clientIp[16];
	int clientPort;
	clientSocket = INVALID_SOCKET;
	HANDLE hThread;
	threadArgs args;


	while (1) {
		printf("waiting for a connection...\n");
		/*
			the accept function is used in a server application to accept an incoming client connection after the server has been set up to listen for connections.
			 When a client attempts to connect If accept succeeds, it returns a new socket descriptor (clientSocket) representing the connection to the client.
			 This descriptor can then be used for communication with that client.

		*/
		// Accept a client socket
		clientSocket = accept(ListenSocket, (SOCKADDR*)&addr, &addrSize);
		if (clientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		inet_ntop(AF_INET, &addr.sin_addr, clientIp, sizeof(clientIp));// converts the ip into a string
		clientPort = ntohs(addr.sin_port);// converts the port into an u_short (used as int)

		printf("connection with %s:%d accepted!\n", clientIp, clientPort);

		args.clientSocket = clientSocket;
		args.clientAddress = addr;

		hThread = CreateThread(NULL, 0, handle_connection, &args, 0, NULL);

		if (hThread == NULL) {
			// Thread creation failed
			DWORD error = GetLastError();
			printf("Thread creation failed with error code: %d\n", error);
		}
	}
	//cleanup
	CleanupLibrary();
	WSACleanup();
	fclose(routes);
	return 0;
}




DWORD WINAPI handle_connection(LPVOID  lpParam) {
	struct threadArgs* args = (threadArgs*)lpParam;
	SOCKET clientSocket = (SOCKET)args->clientSocket;
	sockaddr_in ClientAddress = (sockaddr_in)args->clientAddress;
	int clientPort;
	char clientIp[16];
	inet_ntop(AF_INET, &args->clientAddress.sin_addr, clientIp, sizeof(clientIp));
	clientPort = ntohs(args->clientAddress.sin_port);
	/*
		recv stores the received data in the recvbuf buffer, up to a maximum of DEFAULT_BUFLEN bytes.
		If iResult is greater than 0, it means data has been received successfully. The code then echoes the received data back to the client using the send function.
		If iSendResult (the result of the send operation) is SOCKET_ERROR, it means sending data back to the client failed.
		If iResult is 0, it indicates that the client has closed the connection gracefully, so the server prints a message indicating that the connection is closing.
		If iResult is less than 0, it means an error occurred while receiving data. An error message is printed, the client socket is closed, and the Winsock library is cleaned up.
	*/

	int iResult;
	int i = 0;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int iSendResult = 0;
	char method[10] = { 0 };// initialized at 0 so that strcmp won't receive a null string in case of no input
	char path[100] = { 0 };
	scriptResponse response;
	char curRouteline[100] = { 0 };
	char* curRoutePath;
	char* curRouteFile;
	char* next_token = NULL;
	int sent = 0;
	char* headerValue;
	int bodyLength = 0;

	// Receive until the peer shuts down the connection
	while (1) {
		iResult = recv(clientSocket, recvbuf, recvbuflen - 1, 0);
		if (iResult > 0) {
			recvbuf[iResult] = 0;// null terminate the received data. 
			HttpRequest parsedRecvbuf;
			parsedRecvbuf = parseHttpRequest(recvbuf);

			//get Content-Length: value from the request header
			headerValue;
			bodyLength = 0;
			headerValue = strstr(parsedRecvbuf.headers, "Content-Length:");// get to the end of the http header
			if (headerValue != NULL) {
				sscanf_s(headerValue, "Content-Length:%d", &bodyLength);
				// check if Content-Length: and body length actually match
				if ((int)strlen(parsedRecvbuf.body) != bodyLength) {
					sendHttpResponse(clientSocket, 400, "Content Length mismatch", "text/plain", NULL, NULL);
				}
			}

			if (strcmp(parsedRecvbuf.method, "GET") == 0) {
				fseek(routes, 0, SEEK_SET);
				while (fscanf_s(routes, "%s", curRouteline, (unsigned int)sizeof(curRouteline)) == 1) { // check if the requested file is in the routes config file, and send the corrisponding response
					curRoutePath = strtok_s(curRouteline, "=", &next_token);
					curRouteFile = strtok_s(NULL, "=", &next_token);
					if (strcmp(curRoutePath, parsedRecvbuf.path) == 0) {
						sendFile(clientSocket, curRouteFile, NULL);
						sent = 1;
						break;
					}
				}
				if (sent == 0) {
					sendHttpResponse(clientSocket, 404, "Not Found", "text/plain", NULL, "File not found");
				}
			}
			else if (strcmp(parsedRecvbuf.method, "POST") == 0) {
				if (strcmp(parsedRecvbuf.path, "/login") == 0) {
					response = handleLoginPost(recvbuf);
					sendHttpResponse(clientSocket, response.status, response.message, "text/plain", response.cookie, response.body);
				}
				else if (strcmp(parsedRecvbuf.path, "/register") == 0) {
					response = handleRegisterPost(recvbuf);
					sendHttpResponse(clientSocket, response.status, response.message, "text/plain", response.cookie, response.body);
				}
				else if (strcmp(parsedRecvbuf.path, "/userData") == 0) {
					response = handleUserDataPost(recvbuf);
					sendHttpResponse(clientSocket, response.status, response.message, "text/html", response.cookie, response.body);
				}
				else {
					// Prepare the error message
					const char* errorMessage = "400 Bad Request: The request is malformed or contains invalid data.";
					sendHttpResponse(clientSocket, 400, "Bad Request", "text/plain", NULL, errorMessage);
				}
			}
			else {
				sendHttpResponse(clientSocket, 501, "Not Implemented", "text/plain", NULL, NULL);
			}

			// check if the clietn wants to close the connection
			// save the pointer to the first occurance of "Connection: "
			char* connectionHeader = strstr(recvbuf, "Connection: ");
			if (connectionHeader != NULL) {
				// Check if it contains "close" to close the connection.
				if (strstr(connectionHeader, "close") != NULL) {
					break;
				}
			}
		}
		else if (iResult == 0) {
			printf("\nConnection closing...\n");

			break;
		}
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			return NULL;
		}
	}
	// shutdown the send half of the connection since no more data will be sent
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(clientSocket);

		return NULL;
	}
	printf("Connection with %s:%d closed\n", clientIp, clientPort);
	// close the socket
	closesocket(clientSocket);
	return NULL;
}

char* strdup_n(const char* str, size_t max_length) {
	// Find the actual length of the string (up to the maximum length)
	size_t length = strnlen(str, max_length);

	// Allocate memory for the new string (including space for the null terminator)
	char* new_str = (char*)malloc(length + 1);
	// Copy the string content
	if (new_str == NULL) {
		return NULL;
	}


	strncpy_s(new_str, length + 1, str, length);
	new_str[length] = '\0';  // Null-terminate the new string

	return new_str;
}

HttpRequest parseHttpRequest(const char* rawRequest) {
	HttpRequest parsedHttpRequest;
	int headersLength;
	parsedHttpRequest.method[0] = NULL;
	parsedHttpRequest.path[0] = NULL;
	parsedHttpRequest.body = NULL;
	parsedHttpRequest.headers = NULL;


	if (rawRequest == NULL) {
		return parsedHttpRequest;
	}
	sscanf_s(rawRequest, "%9s %99s", parsedHttpRequest.method, (unsigned int)sizeof(parsedHttpRequest.method), parsedHttpRequest.path, (unsigned int)sizeof(parsedHttpRequest.path));

	parsedHttpRequest.body = (char*)strstr(rawRequest, "\r\n\r\n");// get to the end of the http header
	if (parsedHttpRequest.body == NULL) {
		return parsedHttpRequest;
	}
	parsedHttpRequest.body = &parsedHttpRequest.body[4];// exclude \r\n\r\n, and take just the body


	parsedHttpRequest.headers = (char*)strstr(rawRequest, "\r\n");// get to the start of the http header values
	if (parsedHttpRequest.headers == NULL) {
		return parsedHttpRequest;
	}
	parsedHttpRequest.headers = &parsedHttpRequest.headers[2];

	headersLength = (int)(parsedHttpRequest.body - parsedHttpRequest.headers);

	//parsedHttpRequest.headers[headersLength] = 0; // can't be done because i would be modifying the actual rawRequest
	parsedHttpRequest.headers = strdup_n(parsedHttpRequest.headers, headersLength - 4); // strdup it up untile headerslength-\r\n\r\n chars

	return parsedHttpRequest;
}



void generateCookie(char cookie[]) {
	// Seed the random number generator with the current time
	srand((unsigned int)(time(NULL)));

	static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	int i;

	if (cookie) {
		for (i = 0; i < MAX_COOKIE_LENGTH - 1; i++) {
			int index = (int)(rand() % (sizeof(charset) - 1));
			cookie[i] = charset[index];
		}
		cookie[i] = '\0'; // Null-terminate the string
	}
}


scriptResponse handleRegisterPost(char* input) {
	scriptResponse curResponse;
	if (input == NULL) {
		curResponse.status = 400;
		curResponse.message = "bad request";
		return curResponse;
	}
	HttpRequest parsedInput;

	parsedInput = parseHttpRequest(input);

	URLDecode(parsedInput.body);

	user newUser;

	char* next_token = NULL;
	char* next_token2 = NULL;
	char username[MAX_NAME_LENGTH];
	char email[MAX_EMAIL_LENGTH];
	char password[MAX_PASSWORD_LENGTH];
	int result;

	sscanf_s(parsedInput.body, "username=%[^&]&email=%[^&]&password=%s", username, (unsigned int)sizeof(username), email, (unsigned int)sizeof(email), password, (unsigned int)sizeof(password));
	if (username[0] == 0 || strlen(username) >= MAX_NAME_LENGTH || username == NULL) {
		curResponse.status = 400;
		curResponse.message = "invalid username";
		return curResponse;
	}
	if (email[0] == 0 || strlen(email) >= MAX_EMAIL_LENGTH || email == NULL) {
		curResponse.status = 400;
		curResponse.message = "invalid email";
		return curResponse;
	}
	if (password[0] == 0 || strlen(password) >= MAX_PASSWORD_LENGTH || password == NULL) {
		curResponse.status = 400;
		curResponse.message = "invalid password";
		return curResponse;
	}
	strcpy_s(newUser.username, sizeof(newUser.username), username);
	strcpy_s(newUser.email, sizeof(newUser.email), email);
	strcpy_s(newUser.password, sizeof(newUser.password), password);

	//	assign a new cookie to the user	
	generateCookie(newUser.cookie);

	result = indexedInsertRecord(newUser);
	if (result == 1) {
		curResponse.status = 409;
		curResponse.message = "account already exists";
		return curResponse;
	}

	// save the cookie to the response struct
	curResponse.status = 201;
	curResponse.message = "account created successfully";
	return curResponse;
}


scriptResponse handleLoginPost(char* input) {
	scriptResponse curResponse;
	if (input == NULL) {
		curResponse.status = 400;
		curResponse.message = "bad request";
		return curResponse;
	}
	HttpRequest parsedInput;

	parsedInput = parseHttpRequest(input);

	char* body = _strdup(parsedInput.body);

	URLDecode(body);

	user curUser;
	char* next_token = NULL;
	char* next_token2 = NULL;
	char email[MAX_EMAIL_LENGTH];
	char password[MAX_PASSWORD_LENGTH];
	int result;

	sscanf_s(body, "email=%[^&]&password=%s", email, (unsigned int)sizeof(email), password, (unsigned int)sizeof(password));
	if (email[0] == 0 || strlen(email) >= MAX_EMAIL_LENGTH || email == NULL) {
		curResponse.status = 400;
		curResponse.message = "invalid email";
		return curResponse;
	}
	if (password[0] == 0 || strlen(password) >= MAX_PASSWORD_LENGTH || password == NULL) {
		curResponse.status = 400;
		curResponse.message = "invalid password";
		return curResponse;
	}

	result = loginAuthentication(email, password);
	if (result == 1) {//login insuccessfull
		curResponse.status = 401;
		curResponse.message = "email not registered";
		return curResponse;
	}
	else if (result == 2) {
		curResponse.status = 401;
		curResponse.message = "wrong password";
		return curResponse;
	}

	curUser = getUserByEmail(email);
	curResponse.cookie = _strdup(curUser.cookie);
	curResponse.status = 200;
	curResponse.message = "login successful";
	return curResponse;
}

scriptResponse handleUserDataPost(char* input) {
	scriptResponse curResponse;
	user curUser;
	if (input == NULL) {
		curResponse.status = 400;
		curResponse.message = "bad request";
		return curResponse;
	}
	HttpRequest parsedInput;

	parsedInput = parseHttpRequest(input);

	char* headerValue;
	char cookie[MAX_COOKIE_LENGTH];
	char* html;


	headerValue = strstr(parsedInput.headers, "Cookie:");// get to the cookie value in the header
	if (headerValue == NULL) {
		curResponse.status = 422;
		curResponse.message = "invalid cookie";
		return curResponse;
	}
	headerValue = &headerValue[8];// skip 'cookie= '

	sscanf_s(headerValue, "cookie=%s", cookie, MAX_COOKIE_LENGTH);
	if (cookie[0] == 0) {
		curResponse.status = 422;
		curResponse.message = "invalid cookie";
		return curResponse;
	}

	curUser = getUserByCookie(cookie);

	if (strcmp(curUser.cookie, cookie) != 0) {
		curResponse.status = 422;
		curResponse.message = "invalid cookie";
		return curResponse;
	}

	html = readHtmlFileBody("/userData/userData.html");


	replacePlaceholder(&html, "%USERNAME%", curUser.username);
	curResponse.status = 200;
	curResponse.message = "login successful";
	curResponse.body = html;
	return curResponse;
}

// Function to replace placeholders in html documents for dynamic htlm content 
void replacePlaceholder(char** source, const char* placeholder, const char* replacement) {
	char* result = NULL;
	char* sourcePtr = *source;
	size_t placeholderLen = strlen(placeholder);
	size_t replacementLen = strlen(replacement);

	while (1) {
		char* match = strstr(sourcePtr, placeholder);
		if (match == NULL) {
			// No more occurrences of the placeholder
			break;
		}

		// Calculate the length of the content before the placeholder 
		size_t prefixLen = match - sourcePtr;

		// Calculate the length of the content after the placeholder
		size_t suffixLen = strlen(match + placeholderLen);

		// Calculate the total length of the result string
		size_t resultLen = prefixLen + replacementLen + suffixLen;

		// Allocate memory for the result
		char* newResult = (char*)malloc(resultLen + 1);
		if (newResult == NULL) {
			exit(2); // Malloc failed
		}

		// Copy the content before the placeholder
		strncpy_s(newResult, resultLen + 1, sourcePtr, prefixLen);

		// Copy the replacement content
		strcat_s(newResult, resultLen + 1, replacement);

		// Copy the content after the placeholder
		strcat_s(newResult, resultLen + 1, match + placeholderLen);

		if (result == NULL) {
			// This is the first replacement
			result = newResult;
		}
		else {
			// Concatenate the current result with the new replacement
			strcat_s(result, resultLen + 1, newResult);
			free(newResult);
		}

		// Move the source pointer to the content after the current match
		sourcePtr = match + placeholderLen;
	}

	// If any replacements were made, update the source with the result
	if (result != NULL) {
		free(*source);
		*source = result;
	}
}



// Function to decode a URL-encoded string
void URLDecode(char* str) {
	int len = (int)strlen(str);
	int i, j = 0;

	for (i = 0; i < len; i++) {
		if (str[i] == '%' && i + 2 < len) {
			char hex[3];
			hex[0] = str[i + 1];
			hex[1] = str[i + 2];
			hex[2] = '\0';

			// Convert the hexadecimal representation to a character
			int decoded_char = strtol(hex, NULL, 16);

			// Replace the encoded sequence with the decoded character
			str[j++] = decoded_char;
			i += 2;
		}
		else if (str[i] == '+') {
			// Replace the '+' character with a space
			str[j++] = ' ';
		}
		else {
			// Keep the character as is
			str[j++] = str[i];
		}
	}

	// Null-terminate the decoded string
	str[j] = '\0';
}

// Function to build and send http responses to the clientSocket.
void sendHttpResponse(SOCKET clientSocket, const int statusCode, const char* statusMessage, const char* contentType, const char* cookie, const char* content) {
	char response[DEFAULT_BUFLEN]; // Adjust the size as needed
	int contentLength;
	if (content == NULL) {
		contentLength = 0;
	}
	else {
		contentLength = (int)strlen(content);
	}
	if (cookie != NULL) {
		sprintf_s(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nSet-cookie: cookie=%s; Path=/; HttpOnly;\r\nX-XSS-Protection: 1; mode=block\r\n\r\n%s", statusCode, statusMessage, contentType, contentLength, cookie, content);

	}
	else {
		sprintf_s(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nX-XSS-Protection: 1; mode=block\r\n\r\n%s", statusCode, statusMessage, contentType, contentLength, content);

	}
	send(clientSocket, response, (int)strlen(response), 0);
}

// Function to read an html file and return its body.
char* readHtmlFileBody(const char* filename) {
	const char* fileExtension = strrchr(filename, '.');
	if (strcmp(fileExtension, ".html") != 0) {
		return NULL; //not an html file
	}

	// Define the actual path
	char actualPath[MAX_PATH_LENGTH];
	strcpy_s(actualPath, WEBPATH);
	strcat_s(actualPath, sizeof(actualPath), filename);

	FILE* fp;
	fopen_s(&fp, actualPath, "rb");
	if (fp == NULL) {
		perror("Error opening file");
		return NULL;
	}

	// Variables to store the HTML content and body content
	char* htmlContents = NULL;
	char* bodyContents = NULL;

	// Read the HTML content into a dynamically allocated buffer
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);


	// Check if file too large
	if (fileSize > MAX_ALLOWED_FILE_SIZE) {
		fclose(fp);
		return NULL;
	}

	htmlContents = (char*)malloc(fileSize + 1);

	if (htmlContents == NULL) {
		perror("Memory allocation failed");
		fclose(fp);
		exit(2);
	}

#pragma warning(disable: 6386)
	size_t bytesRead = fread(htmlContents, 1, fileSize, fp);// no buffer overrun, bytesRead is dynamically allocated based on file size.
#pragma warning(default: 6386)
	if (bytesRead != fileSize) {
		perror("Error reading file");
		free(htmlContents);
		fclose(fp);
		exit(1);
	}
	htmlContents[fileSize] = '\0';

	// Extract the body
	// Find the opening <body> tag while allowing for attributes
	char* bodyStart = strstr(htmlContents, "<body");
	if (bodyStart == NULL) {
		fclose(fp);
		free(htmlContents);
		return NULL;
	}
	// Find the closing > character after <body
	char* bodyStartGt = strchr(bodyStart, '>');
	if (bodyStartGt == NULL) {
		fclose(fp);
		free(htmlContents);
		return NULL;
	}
	// Increment bodyStartGt to point to the character after the opening <body..> tag
	bodyStartGt++;

	// Find the closing </body> tag
	char* bodyEnd = strstr(bodyStartGt, "</body>");
	if (bodyEnd == NULL) {
		fclose(fp);
		free(htmlContents);
		return NULL;
	}

	// Calculate the length of the body content
	size_t bodyLength = bodyEnd - bodyStartGt;


	// Allocate memory for the body content and copy it
	bodyContents = (char*)malloc(bodyLength + 1);
	if (bodyContents == NULL) {
		perror("Memory allocation failed");
		fclose(fp);
		free(htmlContents);
		return NULL;
	}

	// Copy bodyStartGt up to bodyLength
	memcpy(bodyContents, bodyStartGt, bodyLength);
	bodyContents[bodyLength] = '\0';

	// Cleanup and close the file
	free(htmlContents);
	fclose(fp);
	return bodyContents;
}


// Function to read an html file and return all of its contents
char* readHtmlFile(const char* filename) {
	const char* fileExtension = strrchr(filename, '.');
	if (strcmp(fileExtension, ".html") != 0) {
		return NULL; //not an html file
	}
	// defining actual path
	char actualPath[MAX_PATH_LENGTH];
	strcpy_s(actualPath, MAX_PATH_LENGTH, WEBPATH);
	strcat_s(actualPath, MAX_PATH_LENGTH, filename);
	FILE* fp;
	fopen_s(&fp, actualPath, "rb");
	if (fp == NULL) {
		perror("Error opening file");
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* htmlContents = (char*)malloc(fileSize + 1);
	if (htmlContents == NULL) {
		perror("Memory allocation failed");
		fclose(fp);
		return NULL;
	}
#pragma warning(disable: 6386)
	size_t bytes_read = fread(htmlContents, 1, fileSize, fp);// no buffer overrun, bytesRead is dynamically allocated based on file size.
#pragma warning(default: 6386)
	if (bytes_read != fileSize) {
		perror("Error reading file");
		free(htmlContents);
		fclose(fp);
		return NULL;
	}

	htmlContents[fileSize] = '\0';

	fclose(fp);
	return htmlContents;
}

//function to send a file to the clientSocket
void sendFile(SOCKET clientSocket, const char* filePath, const char* contentType) {
	FILE* fp;
	char header[DEFAULT_BUFLEN];
	char sendbuf[DEFAULT_BUFLEN];

	// Determine MIME type based on the file extension.
	const char* fileExtension = strrchr(filePath, '.');
	if (fileExtension != NULL && contentType == NULL) {
		if (strcmp(fileExtension, ".jpg") == 0 || strcmp(fileExtension, ".jpeg") == 0) {
			contentType = "image/jpeg";
		}
		else if (strcmp(fileExtension, ".png") == 0) {
			contentType = "image/png";
		}
		else if (strcmp(fileExtension, ".gif") == 0) {
			contentType = "image/gif";
		}
		else if (strcmp(fileExtension, ".gif") == 0) {
			contentType = "image/gif";
		}
		else if (strcmp(fileExtension, ".html") == 0) {
			contentType = "text/html";
		}
		else if (strcmp(fileExtension, ".css") == 0) {
			contentType = "text/css";
		}
		else if (strcmp(fileExtension, ".js") == 0) {
			contentType = "text/javascript";
		}
	}
	if (contentType == NULL) {
		sendHttpResponse(clientSocket, 404, "Not Found", "text/plain", NULL, "File not found");
		return;
	}
	// defining actual path
	char actualPath[MAX_PATH_LENGTH];
	strcpy_s(actualPath, MAX_PATH_LENGTH, WEBPATH);
	strcat_s(actualPath, MAX_PATH_LENGTH, filePath);

	fopen_s(&fp, actualPath, "rb");
	if (fp == NULL) {
		sendHttpResponse(clientSocket, 404, "Not Found", "text/plain", NULL, "File not found");
		return;
	}

	// Get the file size
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// send http header
	sprintf_s(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", contentType, fileSize);
	send(clientSocket, header, (int)strlen(header), 0);
	// send file 
	size_t bytesRead;
	while ((bytesRead = fread(sendbuf, 1, sizeof(sendbuf), fp)) > 0) {
		send(clientSocket, sendbuf, (int)bytesRead, 0);
	}

	fclose(fp);
	return;
}
