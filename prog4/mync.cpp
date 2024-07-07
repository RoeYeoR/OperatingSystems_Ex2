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
#include <arpa/inet.h>
#include <sstream>
#include <fstream>
#include <csignal>

// Function declarations
void initialize_tcp_server(int port, int *server_fd);
void initialize_tcp_client(const char *hostname, int port, int *client_fd);
void initialize_udp_server(int port, int *server_fd);
void initialize_udp_client(const char *hostname, int port, int *client_fd);
void transfer_io_data(int source_fd, int destination_fd);
void configure_redirection(int argc, char *argv[], const char *localhost, int &input_fd, int &output_fd, int &both_fd);
void configure_redirection_with_exec(int argc, char *argv[], const char *localhost, int &input_fd, int &output_fd, int &both_fd);
void signal_handler(int signum);

int main(int argc, char *argv[])
{
    const char *localhost = "127.0.0.1"; // Localhost IP address
    int input_fd = -1, output_fd = -1, both_fd = -1;

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " [-e command] [-i|-o|-b <TCPS|TCPC><port>]" << std::endl;
        return 1;
    }

    // Check if the first argument is "-e"
    bool exec_program = strcmp(argv[1], "-e") == 0;
    if (exec_program && argc < 3)
    {
        std::cout << "Please provide the program name and its arguments after -e" << std::endl;
        return 1;
    }

    if (exec_program)
    {
        // Split the second argument into program name and arguments
        char *program = strtok(argv[2], " ");
        char *arguments = strtok(NULL, "");
        char *new_argv[] = {program, arguments, NULL};

        // Handle input/output redirection
        configure_redirection_with_exec(argc, argv, localhost, input_fd, output_fd, both_fd);

        // Fork the process to create a child process
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            return 1;
        }
        else if (pid == 0)
        {
            // Child process
            // Redirect standard input if input_fd is set
            if (input_fd != -1)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            // Redirect standard output if output_fd is set
            if (output_fd != -1)
            {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }

            // Redirect both standard input and output if both_fd is set
            if (both_fd != -1)
            {
                dup2(both_fd, STDIN_FILENO);
                dup2(both_fd, STDOUT_FILENO);
                close(both_fd);
            }
            execvp(program, new_argv);

            // If execvp returns, there was an error
            perror("execvp");
            return 1;
        }
        else
        {
            // Parent process
            // Check if argv[5] == '-t' and set an alarm for the time specified in argv[6]
            if (argc > 5 && strcmp(argv[5], "-t") == 0)
            {
                signal(SIGALRM, signal_handler);
                int alarm_time = std::atoi(argv[6]);
                alarm(alarm_time);
            }
            int status;
            waitpid(pid, &status, 0);
        }
    }
    else
    {
        // Handle redirection
        configure_redirection(argc, argv, localhost, input_fd, output_fd, both_fd);

        if (argc == 3)
        {
            if (input_fd != -1)
            {
                transfer_io_data(input_fd, STDOUT_FILENO);
                close(input_fd);
            }

            if (output_fd != -1)
            {
                transfer_io_data(STDIN_FILENO, output_fd);
                close(output_fd);
            }
            if (both_fd != -1)
            {
                transfer_io_data(both_fd, both_fd);
                close(both_fd);
            }
        }
        else
        {
            transfer_io_data(input_fd, output_fd);
        }
    }

    return 0;
}

// Initialize a TCP server on the specified port and accept a single connection
void initialize_tcp_server(int port, int *server_fd)
{
    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
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
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    std::cout << "Waiting for client connection on port: " << port << std::endl;

    // Listen for incoming connections
    listen(sockfd, 5);

    // Accept a connection
    *server_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (*server_fd < 0)
    {
        perror("accept");
        close(sockfd);
        exit(1);
    }

    std::cout << "The connection was established successfully" << std::endl;

    // Close the listening socket (not needed after accepting a connection)
    close(sockfd);
}

// Initialize a TCP client
void initialize_tcp_client(const char *hostname, int port, int *client_fd)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create a socket
    *client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    // Get server information by hostname
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // Set address family to IPv4

    // Copy the server's IP address to the server address structure
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port); // Convert port to network byte order and set it

    // Attempt to connect to the server
    if (connect(*client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(*client_fd);
        exit(1);
    }

    std::cout << "The connection was established successfully" << std::endl;
}

// Initialize a UDP server on the specified port
void initialize_udp_server(int port, int *server_fd)
{
    struct sockaddr_in serv_addr;

    // Create a socket
    *server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*server_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    // Set socket options to allow reuse of the address
    int reuse = 1;
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt");
        close(*server_fd);
        exit(1);
    }

    // Initialize serv_addr structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(*server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        close(*server_fd);
        exit(1);
    }

    std::cout << "UDP server is listening on port: " << port << std::endl;
}

// Initialize a UDP client
void initialize_udp_client(const char *hostname, int port, int *client_fd)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create a socket
    *client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*client_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    // Get server information by hostname
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    // Connect the socket to the server
    if (connect(*client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(*client_fd);
        exit(1);
    }

    std::cout << "UDP client is set up to send to " << hostname << " on port " << port << std::endl;
}

// Transfer data from source_fd to destination_fd
void transfer_io_data(int source_fd, int destination_fd)
{
    char buffer[1024];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(source_fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        // Write the buffer to the destination_fd
        bytes_written = write(destination_fd, buffer, bytes_read);
        if (bytes_written < 0)
        {
            perror("write");
            break;
        }
    }

    if (bytes_read < 0)
    {
        perror("read");
    }
}

// Configure input/output redirection with exec
void configure_redirection_with_exec(int argc, char *argv[], const char *localhost, int &input_fd, int &output_fd, int &both_fd)
{
    // Handle input redirection
    if (strncmp(argv[3], "-i", 2) == 0)
    {
        if (strncmp(argv[4], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[4] + 4), &input_fd);
        }
        else if (strncmp(argv[4], "TCPC", 4) == 0)
        {
            initialize_tcp_client(localhost, atoi(argv[4] + 14), &input_fd);
        }
        else if (strncmp(argv[4], "UDPS", 4) == 0)
        {
            initialize_udp_server(atoi(argv[4] + 4), &input_fd);
        }
        else if (strncmp(argv[4], "UDPC", 4) == 0)
        {
            initialize_udp_client(localhost, atoi(argv[4] + 14), &input_fd);
        }

        if (argc > 5 && strncmp(argv[5], "-o", 2) == 0)
        {
            if (strncmp(argv[6], "TCPC", 4) == 0)
            {
                initialize_tcp_client(localhost, atoi(argv[6] + 14), &output_fd);
            }
            else if (strncmp(argv[6], "TCPS", 4) == 0)
            {
                initialize_tcp_server(atoi(argv[6] + 4), &output_fd);
            }
            else if (strncmp(argv[6], "UDPS", 4) == 0)
            {
                initialize_udp_server(atoi(argv[6] + 4), &output_fd);
            }
            else if (strncmp(argv[6], "UDPC", 4) == 0)
            {
                initialize_udp_client(localhost, atoi(argv[6] + 14), &output_fd);
            }
        }
    }
    else if (strncmp(argv[3], "-o", 2) == 0)
    {
        if (strncmp(argv[4], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[4] + 4), &output_fd);
        }
        else if (strncmp(argv[4], "TCPC", 4) == 0)
        {
            initialize_tcp_client(localhost, atoi(argv[4] + 14), &output_fd);
        }
        else if (strncmp(argv[4], "UDPS", 4) == 0)
        {
            initialize_udp_server(atoi(argv[4] + 14), &output_fd);
        }
        else if (strncmp(argv[4], "UDPC", 4) == 0)
        {
            initialize_udp_client(localhost, atoi(argv[4] + 14), &output_fd);
        }
    }
    else if (strncmp(argv[3], "-b", 2) == 0)
    {
        if (strncmp(argv[4], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[4] + 4), &both_fd);
        }
        else if (strncmp(argv[4], "TCPC", 4) == 0)
        {
            initialize_tcp_client(localhost, atoi(argv[4] + 14), &both_fd);
        }
    }
}

// Configure input/output redirection
void configure_redirection(int argc, char *argv[], const char *localhost, int &input_fd, int &output_fd, int &both_fd)
{
    // Handle input redirection
    if (strncmp(argv[1], "-i", 2) == 0)
    {
        if (strncmp(argv[2], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[2] + 4), &input_fd);
        }
        else if (strncmp(argv[2], "TCPC", 4) == 0)
        {
            initialize_tcp_client(localhost, atoi(argv[2] + 14), &input_fd);
        }
        else if (strncmp(argv[2], "UDPS", 4) == 0)
        {
            initialize_udp_server(atoi(argv[2] + 4), &input_fd);
        }
        else if (strncmp(argv[2], "UDPC", 4) == 0)
        {
            initialize_udp_client(localhost, atoi(argv[2] + 14), &input_fd);
        }

        if (argc > 3 && strncmp(argv[3], "-o", 2) == 0)
        {
            if (strncmp(argv[4], "TCPC", 4) == 0)
            {
                initialize_tcp_client(localhost, atoi(argv[4] + 14), &output_fd);
            }
            else if (strncmp(argv[4], "TCPS", 4) == 0)
            {
                initialize_tcp_server(atoi(argv[4] + 4), &output_fd);
            }
            else if (strncmp(argv[4], "UDPS", 4) == 0)
            {
                initialize_udp_server(atoi(argv[4] + 4), &output_fd);
            }
            else if (strncmp(argv[4], "UDPC", 4) == 0)
            {
                initialize_udp_client(localhost, atoi(argv[4] + 14), &output_fd);
            }
        }
    }
    else if (strncmp(argv[1], "-o", 2) == 0)
    {
        if (strncmp(argv[2], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[2] + 4), &output_fd);
        }
        else if (strncmp(argv[2], "TCPC", 4) == 0)
        {
            initialize_tcp_client(localhost, atoi(argv[2] + 14), &output_fd);
        }
        else if (strncmp(argv[2], "UDPS", 4) == 0)
        {
            initialize_udp_server(atoi(argv[2] + 4), &output_fd);
        }
        else if (strncmp(argv[2], "UDPC", 4) == 0)
        {
            initialize_udp_client(localhost, atoi(argv[2] + 14), &output_fd);
        }
    }
    else if (strncmp(argv[1], "-b", 2) == 0)
    {
        if (strncmp(argv[2], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[2] + 4), &both_fd);
        }
        else if (strncmp(argv[2], "TCPC", 4) == 0)
        {
            initialize_tcp_client(localhost, atoi(argv[2] + 14), &both_fd);
        }
    }
}

// Signal handler for SIGALRM
void signal_handler(int signum)
{
    std::cout << "Alarm signal (" << signum << ") received. Terminating all specified processes.\n";
    // Command to kill all processes. Adjust the command as needed.
    system("pkill -9 ttt"); // Replace "ttt" with the actual name of the process to kill
    exit(signum);
}