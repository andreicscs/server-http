# C Web Server

A lightweight HTTP and archive server implementation in C with cookie-based authentication, dynamic HTML content, and file serving capabilities.
The objective of the project was to learn more in depth about how http servers and archives work together.
The project is showcased with simple auth forms made in html and css.

![image](https://github.com/user-attachments/assets/928e5957-f3ce-4f88-9640-031d8cdb09dc)


## 🌟 Features

### 🖥️ Core Functionality
- **Multi-threaded architecture** - Handles multiple connections simultaneously
- **HTTP/1.1 compliant** - Supports GET/POST methods and proper status codes
- **Cookie-based sessions** - Secure user authentication system
- **Dynamic content** - HTML template rendering with placeholder replacement

### 📡 Supported Endpoints
| Endpoint    | Method | Description                     |
|-------------|--------|---------------------------------|
| `/login`    | POST   | User authentication             |
| `/register` | POST   | New user registration           |
| `/userData` | POST   | Protected user profile page     |
| * (others)  | GET    | Static file serving             |

## 🛠️ Technical Implementation

### ⚙️ Server Architecture
```c
typedef struct {
    int status;         // HTTP status code
    const char* message; // Status message
    char* body;         // Response body
    char* cookie;       // Set-Cookie header
} scriptResponse;

typedef struct {
    char method[MAX_METHOD_LENGTH];
    char path[MAX_PATH_LENGTH];
    char* headers;
    char* body;
} HttpRequest;
```
### 🔌 Key Components
- **Winsock2** - Windows socket API for network communication

- **Multi-threading** - CreateThread for concurrent connections

- **Parser** - HTTP request parsing with URL decoding

- **Template Engine** - Dynamic HTML content generation

## 🚀 Getting Started
### 📋 Prerequisites
- Visual Studio 2022
- Windows SDK (latest)
- C++ CMake tools for Windows (VS extension)
### 🔧 Building the Solution
1. **Clone**:
   ```powershell
   git clone --recursive https://github.com/andreicscs/server-http.git
   ```
2. **Open in Visual Studio**:
  - Double-click CWebServer.sln
  - Set as Startup Project: Right-click Server project → "Set as Startup Project"

### ⚠️ Runtime Requirements
- Must run as Administrator (for port binding)

- web_routes.conf must be in executable directory

- Database files require write permissions

## ⚙️ Configuration
Edit WEBROUTES in server.h to configure file paths:
```c
#define WEBROUTES "web_routes.conf"
#define WEBPATH "web/"
#define DEFAULT_PORT "8080"
```
## 📚 API Reference
### 🔄 Request Handling
```c
HttpRequest parseHttpRequest(const char* rawRequest);
void sendHttpResponse(SOCKET clientSocket, int statusCode, 
                     const char* statusMessage, const char* contentType, 
                     const char* cookie, const char* content);
```
### 🍪 Authentication
```c
scriptResponse handleLoginPost(char* input);
scriptResponse handleRegisterPost(char* input);
scriptResponse handleUserDataPost(char* input);
void generateCookie(char cookie[]);
```
### 📁 File Serving
```c
void sendFile(SOCKET clientSocket, const char* filePath, const char* contentType);
char* readHtmlFile(const char* filename);
char* readHtmlFileBody(const char* filename);
```

## 🛡️ Security Features
- **HttpOnly cookies** - Prevents XSS attacks

- **XSS Protection** - Header automatically added to responses

- **Input validation** - Strict length checks on all user input

- **Content-Length verification** - Prevents request smuggling

## 📈 Performance
- **Connection pooling** - Reuses socket resources
