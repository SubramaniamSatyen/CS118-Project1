#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <map>
#include <fstream>
#include <iterator>
#include <vector>
#include <regex>

using namespace std;

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, string path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = (char*)malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // Parse the header and extract essential fields, e.g. file name.
    // If the requested path is "/" (root), default to index.html
    string req = buffer;

    int file_index = req.find(' ') + 1;
    string file_name = "." + req.substr(file_index, req.find(" HTTP/", file_index) - file_index);
    
    if (file_name == "./")
        file_name = "./index.html";
    file_name = regex_replace(file_name, regex("%20"), " ");

    // Implement proxy and call the function under condition
    if (file_name.find(".ts") != string::npos) {
        //proxy_remote_file(app, client_socket, file_name.c_str());
        proxy_remote_file(app, client_socket, buffer);
    
    } 
    else {
        serve_local_file(client_socket, file_name);
    }
    free(request);
}

void serve_local_file(int client_socket, string path) {
    // Implements the following logic
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    // Content mapping
    map<string, string> CONTENT_TYPE_MAPPING = {
        { "html", "text/html; charset=UTF-8" },
        { "jpg", "image/jpeg" },
        { "txt", "text/plain; charset=UTF-8" }
    };
    string content_type = "application/octet-stream";
    const string file_path = path;

    // Confirm file exists
    ifstream f(path.c_str());
    if (f.good() == 1) {
        ifstream input( path, ios::binary );

        // Copies all data into buffer
        vector<unsigned char> buffer(istreambuf_iterator<char>(input), {});

        // Allocating response
        if (file_path.substr(1).rfind('.') != string::npos){
            content_type = CONTENT_TYPE_MAPPING[file_path.substr(file_path.rfind('.'))];
        }

        string response = "HTTP/1.0 200 OK\r\n"
                    "Content-Type: " + content_type + "\r\n"
                    "Content-Length: " + to_string(buffer.size()) + "\r\n"
                    "\r\n";
                      
        char* response_array = new char[response.length() + buffer.size() + 1]; 
        strcpy(response_array, response.c_str());

        // Sending response and clean up
        send(client_socket, response_array, strlen(response_array), 0);
        send(client_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
        delete[] response_array; 
    }

    // Default to file not found error
    char not_found_response[] = "HTTP/1.0 404 Not Found\r\n\r\n";
    send(client_socket, not_found_response, strlen(not_found_response), 0);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    const char SERVER[] = "131.179.176.34";
    const int PORT = 5001;

    // printf("creating socket\n");
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
    }

    // printf("connect to socket\n");
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
    }

    // send request
    send(sock, request, BUFFER_SIZE, 0);
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read = recv(sock, buffer, BUFFER_SIZE, MSG_PEEK);
    // printf("%ld\n",bytes_read);
    printf("----\n%s\n----\n",request);
    printf("----\n%s\n----\n", buffer);


    char* start = strstr(buffer,"Content-Length: " )+16;
    char* end = strchr(start,' ');
    int leng = end - start;
    char* number = (char*) malloc(leng);
    strncpy(number, start, leng);
    
    printf("number: %s\n", number);
    int file_size = atoi(number);
    printf("file size : %d\n", file_size);




    //send(sock, request, strlen(request), 0);
    //valread = read(sock, buffer, 1024);
    //send(client_socket, buffer, strlen(buffer), 0);

    
    // TODO: Modify to provide Bad Gateway request
    char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
    free(number);
}   