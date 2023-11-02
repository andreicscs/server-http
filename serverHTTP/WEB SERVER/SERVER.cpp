

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
#include "../server.h"
#include <string.h>
#include "../DATABASE/operazioniDB.cpp"


#pragma comment(lib, "Ws2_32.lib")


void replacePlaceholder(char** source, const char* placeholder, const char* replacement);
char* readHtmlFile(const char* filename);
void sendHttpResponse(int clientSocket, const char* statusCode, const char* contentType, const char* content);
void URLDecode(char* str);
char* handleRegisterPost(int clientSocket, char* input);
char* handleLoginPost(int clientSocket, char* input);
void sendFile(int clientSocket, const char* filePath, const char* contentType);
DWORD WINAPI handle_connection(LPVOID  lpParam);

int main(int argc, char* argv[]) {
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
		DEFAULT_PORT is used as the port number. In this case, it's "27015," which was defined as a macro at the beginning of the code.
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

	// Create a SOCKET for the server to listen for client connections with the previous result struct fields.
	// result->ai_family: This retrieves the address family (IPv4 or IPv6) from the addrinfo structure
	// result->ai_socktype: This retrieves the socket type (e.g., SOCK_STREAM for TCP or SOCK_DGRAM for UDP) from the addrinfo structure.
	// result->ai_protocol: This retrieves the protocol (e.g., IPPROTO_TCP for TCP or IPPROTO_UDP for UDP) from the addrinfo structure.
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
	clientSocket = INVALID_SOCKET;
	int clientPort;




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


		inet_ntop(AF_INET, &addr.sin_addr, clientIp, sizeof(clientIp));
		clientPort = ntohs(addr.sin_port);

		printf("connection with %s:%d accepted!\n", clientIp, clientPort);


		threadArgs args;
		args.clientSocket = clientSocket;
		args.clientAddress = addr;


		//handle_connection(&clientSocket); //non thread.

		HANDLE hThread;
		hThread = CreateThread(NULL, 0, handle_connection, &args, 0, NULL);

		if (hThread != NULL) {
			// Thread creation was successful
			printf("Thread created successfully! connection established.\n\n");
		}
		else {
			// Thread creation failed
			DWORD error = GetLastError();
			printf("Thread creation failed with error code: %d\n", error);
		}

	}

	//cleanup
	WSACleanup();
	return 0;
}


DWORD WINAPI handle_connection(LPVOID  lpParam) {
	struct threadArgs* args = (threadArgs*)lpParam;
	SOCKET clientSocket = args->clientSocket;
	sockaddr_in ClientAddress = args->clientAddress;

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
	int iSendResult = 0;
	int recvbuflen = DEFAULT_BUFLEN;
	char sendbuf[DEFAULT_BUFLEN];
	char method[10];
	char path[100];
	char* input;
	char header[DEFAULT_BUFLEN];
	char* response;

	// Receive until the peer shuts down the connection
	while (1) {
		iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
		recvbuf[iResult] = 0;// null terminate the received data.

		if (iResult > 0) {
			//printf("Bytes received: %ld (%s)\n", iResult, recvbuf);
			sscanf_s(recvbuf, "%9s %99s", method, (unsigned int)sizeof(method), path, (unsigned int)sizeof(path));


			if (strcmp(method, "GET") == 0) {
				if (strcmp(path, "/") == 0) {
					sendFile(clientSocket, "/login.html", "text/html");

				}
				else {
					sendFile(clientSocket, path, NULL);
				}
			}
			else if (strcmp(method, "POST") == 0) {
				input = strstr(recvbuf, "\r\n\r\n");// get to the end of the http header
				input = &input[4];// exclude \r\n\r\n, and take just the body

				if (strcmp(path, "/login") == 0) {
					
					response = handleLoginPost(clientSocket, input);
					if (response!=NULL) {
						sendHttpResponse(clientSocket, "200 OK", "text/html", response);
						free(response);
					}
					else {
						sendHttpResponse(clientSocket, "400 Bad Request", "text/html", response);
						free(response);
					}
					
				}
				else if(strcmp(path, "/register") == 0){
					response = handleRegisterPost(clientSocket, input);

					if (response==0) {
						sendFile(clientSocket, "/login.html", "text/html");

					}
					else {
						sendFile(clientSocket, "/register.html", "text/html");
					}
					


					// send http header
					//sprintf_s(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", "text/html", strlen(response));
					//send(clientSocket, header, strlen(header), 0);

					// send the response generated by the script
					//send(clientSocket, response, strlen(response), 0);
				
					free(response);
				}
				else {
					// Prepare the error message
					const char* errorMessage = "400 Bad Request: The request is malformed or contains invalid data.";
					sendHttpResponse(clientSocket, "400 Bad Request", "text/plain", errorMessage);
				}

				

			}
			else {
				sendHttpResponse(clientSocket, "501 Not Implemented", "text/plain", NULL);
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
	printf("Connection closed\n");
	// close the socket
	closesocket(clientSocket);
	return NULL;
}

char* handleRegisterPost(int clientSocket, char* input) {
	char* modifiableInput;
	modifiableInput = _strdup(input);

	URLDecode(modifiableInput);

	char* response;
	response = _strdup(modifiableInput);


	user newUser;


	char* next_token = NULL;
	char* next_token2 = NULL;
	char* token;
	char* username;
	char* email;
	char* password;

	// Split the input string based on the '&' character to separete the input.
	token = strtok_s(modifiableInput, "&", &next_token);

	// split the string based on the '=' character, to get the value.
	username = _strdup(token);
	username = strtok_s(username, "=", &next_token2);
	username = strtok_s(NULL, "=", &next_token2);
	if (username==NULL) {
		free(modifiableInput);
		return NULL;
	}
	strcpy(newUser.username, username);

	token = strtok_s(NULL, "&", &next_token); // Get the next token
	// split the string based on the '=' character, to get the value.
	email = _strdup(token);
	email = strtok_s(email, "=", &next_token2);
	email = strtok_s(NULL, "=", &next_token2);
	if (email == NULL) {
		free(modifiableInput);
		return NULL;
	}
	strcpy(newUser.email,email);

	token = strtok_s(NULL, "&", &next_token); // Get the next token
	// split the string based on the '=' character, to get the value.
	password = _strdup(token);
	password = strtok_s(password, "=", &next_token2);
	password = strtok_s(NULL, "=", &next_token2);
	if (password == NULL) {
		free(modifiableInput);
		return NULL;
	}
	strcpy(newUser.password, password);


	//printf("username: %s, email: %s, password: %s\n", newUser.username, newUser.email, newUser.password);
	indexedInsertRecord(newUser);
	
	free(modifiableInput);
	return 0;
}




char* handleLoginPost(int clientSocket, char* input) {
	char* modifiableInput;
	modifiableInput = _strdup(input);

	URLDecode(modifiableInput);



	char* token;
	char* next_token = NULL;
	char* next_token2 = NULL;
	char* email;
	char* password;
	int result;

	// Split the input string based on the '&' character to separete the input.
	token = strtok_s(modifiableInput, "&", &next_token);

	// split the string based on the '=' character, to get the value.
	email = _strdup(token);
	email = strtok_s(email, "=", &next_token2);
	email = strtok_s(NULL, "=", &next_token2);
	if (email == NULL) {
		free(modifiableInput);
		return NULL;
	}

	token = strtok_s(NULL, "&", &next_token); // Get the next token
	// split the string based on the '=' character, to get the value.
	password = _strdup(token);
	password = strtok_s(password, "=", &next_token2);
	password = strtok_s(NULL, "=", &next_token2);
	if (email == NULL || password==NULL) {
		free(modifiableInput);
		return NULL;
	}


	
	result=loginAuthentication(email,password);
	
	if (result!=0) {//login insuccessfull
		return readHtmlFile("/login.html");
	}

	// login successfull

	user requestedUser;
	requestedUser=getUser(email);



	char* html;
	char* placeHolder;
	html=readHtmlFile("/userData.html");
	replacePlaceholder(&html, "%USERNAME%", requestedUser.username);
	
	free(modifiableInput);;
	return html;
}

void replacePlaceholder(char** source, const char* placeholder, const char* replacement) {
	char* result = NULL;
	char* source_ptr = *source;
	size_t placeholder_len = strlen(placeholder);
	size_t replacement_len = strlen(replacement);

	while (1) {
		char* match = strstr(source_ptr, placeholder);
		if (match == NULL) {
			// No more occurrences of the placeholder
			break;
		}

		// Calculate the length of the content before the placeholder
		size_t prefix_len = match - source_ptr;

		// Calculate the length of the content after the placeholder
		size_t suffix_len = strlen(match + placeholder_len);

		// Calculate the total length of the result string
		size_t result_len = prefix_len + replacement_len + suffix_len;

		// Allocate memory for the result
		char* new_result = (char*)malloc(result_len + 1);

		// Copy the content before the placeholder
		strncpy(new_result, source_ptr, prefix_len);
		new_result[prefix_len] = '\0';

		// Copy the replacement content
		strcat(new_result, replacement);

		// Copy the content after the placeholder
		strcat(new_result, match + placeholder_len);

		if (result == NULL) {
			// This is the first replacement
			result = new_result;
		}
		else {
			// Concatenate the current result with the new replacement
			strcat(result, new_result);
			free(new_result);
		}

		// Move the source pointer to the content after the current match
		source_ptr = match + placeholder_len;
	}

	// If any replacements were made, update the source with the result
	if (result != NULL) {
		free(*source);
		*source = result;
	}
}

// Function to decode a URL-encoded string in place
void URLDecode(char* str) {
	int len = strlen(str);
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


void sendHttpResponse(int clientSocket, const char* statusCode, const char* contentType, const char* content) {
	char response[1024]; // Adjust the size as needed
	int contentLength;
	if (content==NULL) {
		contentLength = 0;
	}
	else {
		contentLength = strlen(content);
	}

	sprintf(response, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\n\r\n%s", statusCode, contentType, contentLength, content);

	send(clientSocket, response, strlen(response), 0);
}



char* readHtmlFile(const char* filename) {
	// defining actual path
	char actualPath[MAXPATHLENGTH];
	strcpy_s(actualPath, MAXPATHLENGTH, WEBPATH);
	strcat_s(actualPath, MAXPATHLENGTH, filename);
	FILE* fp;
	fopen_s(&fp, actualPath, "rb");
	if (fp == NULL) {
		perror("Error opening file");
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* html_contents = (char*)malloc(fileSize + 1);
	if (html_contents == NULL) {
		perror("Memory allocation failed");
		fclose(fp);
		return NULL;
	}

	size_t bytes_read = fread(html_contents, 1, fileSize, fp);
	if (bytes_read != fileSize) {
		perror("Error reading file");
		free(html_contents);
		fclose(fp);
		return NULL;
	}

	html_contents[fileSize] = '\0';

	fclose(fp);
	return html_contents;
}


void sendFile(int clientSocket, const char* filePath, const char* contentType) {
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
		sendHttpResponse(clientSocket, "404 Not Found", "text/plain", "File not found");
		return;
	}


	// defining actual path
	char actualPath[MAXPATHLENGTH];
	strcpy_s(actualPath, MAXPATHLENGTH, WEBPATH);
	strcat_s(actualPath, MAXPATHLENGTH, filePath);

	fopen_s(&fp, actualPath, "rb");
	if (fp == NULL) {
		sendHttpResponse(clientSocket, "404 Not Found", "text/plain", "File not found");
		return;
	}

	// Get the file size
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);


	// send http header
	sprintf_s(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", contentType, fileSize);
	send(clientSocket, header, strlen(header), 0);
	// send file 
	size_t bytesRead;
	while ((bytesRead = fread(sendbuf, 1, sizeof(sendbuf), fp)) > 0) {
		send(clientSocket, sendbuf, bytesRead, 0);
	}

	//printf("%s sent\n", filePath);
	fclose(fp);

	return;
}