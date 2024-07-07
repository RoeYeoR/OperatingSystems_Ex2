#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>

// Function declarations
void initialize_tcp_server(int port, int *server_fd);
void initialize_tcp_client(const char *hostname, int port, int *client_fd);
void configure_redirection(int argc, char *argv[], const char *localhost, int &input_fd, int &output_fd, int &both_fd);

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments are provided and the -e flag is set
    if (argc < 3 || strcmp(argv[1], "-e") != 0) {
        printf("Please enter the -e parameter, the program name, and the sequence of computer selections\n");
        return 1;
    }

    // Split the second argument into program name and arguments
    char *program = strtok(argv[2], " ");
    char *arguments = strtok(NULL, "");
    char *new_argv[] = {program, arguments, NULL};
    const char *localhost = "127.0.0.1"; // Localhost IP address

    int input_fd = -1, output_fd = -1, both_fd = -1;

    // Handle redirection
    configure_redirection(argc, argv, localhost, input_fd, output_fd, both_fd);

    // Fork the process to create a child process
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // Child process
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        if (both_fd != -1) {
            dup2(both_fd, STDIN_FILENO);
            dup2(both_fd, STDOUT_FILENO);
            close(both_fd);
        }

        execvp(program, new_argv);

        // If execvp returns, there was an error
        perror("execvp");
        return 1;
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}

// Initialize a TCP server on the specified port and accept a single connection
void initialize_tcp_server(int port, int *server_fd) {
    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // Initialize serv_addr structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind the socket to the specified port
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    std::cout << "Waiting for client connection on port: " << port << std::endl;

    // Listen for incoming connections
    listen(sockfd, 5);

    // Accept a connection
    *server_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (*server_fd < 0) {
        perror("accept");
        close(sockfd);
        exit(1);
    }

    std::cout << "The connection was established successfully" << std::endl;

    // Close the listening socket (not needed after accepting a connection)
    close(sockfd);
}

// Initialize a TCP client
void initialize_tcp_client(const char *hostname, int port, int *client_fd) {
    struct sockaddr_in serv_addr; 
    struct hostent *server;      

    // Create a socket
    *client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_fd < 0) {
        perror("socket");
        exit(1);
    }

    // Get server information by hostname
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // Set address family to IPv4

    // Copy the server's IP address to the server address structure
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port); // Convert port to network byte order and set it

    // Attempt to connect to the server
    if (connect(*client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(*client_fd);
        exit(1);
    }

    std::cout << "The connection was established successfully" << std::endl;
}

// Handle input/output redirection based on command-line arguments
void configure_redirection(int argc, char *argv[], const char *localhost, int &input_fd, int &output_fd, int &both_fd) {
    // Handle input redirection
    if (strncmp(argv[3], "-i", 2) == 0) {
        if (argc == 5) {
            if (strncmp(argv[4], "TCPS", 4) == 0) {
                initialize_tcp_server(atoi(argv[4] + 4), &input_fd);
            } else if (strncmp(argv[4], "TCPC", 4) == 0) {
                initialize_tcp_client(localhost, atoi(argv[4] + 14), &input_fd);
            }
        } else {
            if (strncmp(argv[4], "TCPS", 4) == 0) {
                initialize_tcp_server(atoi(argv[4] + 4), &input_fd);
            } else if (strncmp(argv[4], "TCPC", 4) == 0) {
                initialize_tcp_client(localhost, atoi(argv[4] + 14), &input_fd);
            }

            if (strncmp(argv[5], "-o", 2) == 0) {
                if (strncmp(argv[6], "TCPC", 4) == 0) {
                    initialize_tcp_client(localhost, atoi(argv[6] + 14), &output_fd);
                } else if (strncmp(argv[6], "TCPS", 4) == 0) {
                    initialize_tcp_server(atoi(argv[6] + 4), &output_fd);
                }
            }
        }
    } else if (strncmp(argv[3], "-o", 2) == 0) {
        if (strncmp(argv[4], "TCPS", 4) == 0) {
            initialize_tcp_server(atoi(argv[4] + 4), &output_fd);
        } else if (strncmp(argv[4], "TCPC", 4) == 0) {
            initialize_tcp_client(localhost, atoi(argv[4] + 14), &output_fd);
        }
    } else if (strncmp(argv[3], "-b", 2) == 0) {
        if (strncmp(argv[4], "TCPS", 4) == 0) {
            initialize_tcp_server(atoi(argv[4] + 4), &both_fd);
        } else if (strncmp(argv[4], "TCPC", 4) == 0) {
            initialize_tcp_client(localhost, atoi(argv[4] + 14), &both_fd);
        }
    }
}