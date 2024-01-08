# CS118-Project1

This repo contains the code from Project 1 of the UCLA's CS 118 in Fall of 2023. 

## Web Server Functionality

The first part of this project tasks us with creating a simple HTTP server to serve static files. Our implementation is in `C++`, and is designed to plain text `UTF-8` encoded files (ex: `*.html`, `*.txt`) and static image files (ex: `*.jpg`). Our solution will also handle requests for non-existant files.

### Running the Web Server Locally

To run the web server locally, take the following steps

1. Clone this repo and navigate to your local project directory
2. Configure the `DEFAULT_SERVER_PORT`, `DEFAULT_REMOTE_HOST`, and `DEFAULT_REMOTE_PORT` values in the server.cpp file
3. Run the command `make` to build the server.cpp file
4. Run the server by running `./server`
5. View a target file (in the same directory as the server) by navigating to `localhost:[DEFAULT_SERVER_PORT]/[FILE_NAME]`


## Reverse Proxy Functionality

The server's reverse proxy functionality allows the site to act as a proxy for a dedicated video server, returning a `502 Bad Gateway` error if the server is not functioning. Steps to run the reverse proxy are identical to those for the web server, and will forward all files with `.ts` extensions.