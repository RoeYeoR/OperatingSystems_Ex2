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
#include <cstring>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>
#include <csignal>
#include <sys/un.h>

#define PATH "/tmp/newserver"

// Open a TCP server on the specified port and accept a single connection
void start_tcp_server(int port, int *fd)
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
    *fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (*fd < 0)
    {
        perror("accept");
        close(sockfd);
        exit(1);
    }

    std::cout << "The connection was established successfully" << std::endl;

    // Close the listening socket (not needed after accepting a connection)
    close(sockfd);
}

// Function to start a TCP client
void start_tcp_client(const char *hostname, int port, int *fd)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create a socket
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*fd < 0)
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
    if (connect(*fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(*fd);
        exit(1);
    }

    std::cout << "The connection was established successfully" << std::endl;
}

// Function to receive data from client and redirect it to stdin
void receive_data_and_redirect_to_stdin(int sockfd)
{
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    char buffer[256];

    // Receive data from a client
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cli_addr, &clilen);
    if (n < 0)
    {
        perror("recvfrom");
        close(sockfd);
        exit(1);
    }
    buffer[n] = '\0'; // Null-terminate the received data

    // Open a file to redirect stdout
    FILE *file = fopen("output.txt", "w");
    if (file == NULL)
    {
        perror("fopen");
        close(sockfd);
        exit(1);
    }

    // Duplicate the file descriptor to stdout
    if (dup2(fileno(file), STDIN_FILENO) == -1)
    {
        perror("dup2");
        fclose(file);
        close(sockfd);
        exit(1);
    }
    std::cout << "im herffgfge" << std::endl;

    // Restore stdout to its original state
    fclose(file);
}

// Function to start a UDP server
void start_udp_server(int port, int *fd)
{
    struct sockaddr_in serv_addr;

    // Create a socket
    *fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*fd < 0)
    {
        perror("socket");
        exit(1);
    }

    // Initialize serv_addr structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(*fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        close(*fd);
        exit(1);
    }

    std::cout << "UDP server is listening on port: " << port << std::endl;
}

void start_udp_client(const char *hostname, int port, int *fd)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create a socket
    *fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*fd < 0)
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
    if (connect(*fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(*fd);
        exit(1);
    }

    std::cout << "UDP client is set up to send to " << hostname << " on port " << port << std::endl;
}

int openUDSStreamServer(const char *path, int *fd)
{
    int server_fd;
    struct sockaddr_un addr;

    // Create a Unix domain stream socket
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket error");
        return -1;
    }

    // Zero out the address structure and set the family and path
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    // Unlink the path if it already exists
    unlink(path);

    // Bind the socket to the specified path
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("bind error");
        close(server_fd);
        return -1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1)
    {
        perror("listen error");
        close(server_fd);
        unlink(path);
        return -1;
    }

    std::cout << "Server listening on " << path << std::endl;

    // Accept a single incoming connection
    *fd = accept(server_fd, NULL, NULL);
    if (*fd == -1)
    {
        perror("accept error");
        close(server_fd);
        unlink(path);
        return -1;
    }

    std::cout << "Client connected" << std::endl;

    // Close the server socket and unlink the path
    close(server_fd);
    unlink(path);

    return 0;
}

int openUDSStreamClient(const char *path, int *client_fd)
{

    struct sockaddr_un addr;

    // Create a Unix domain stream socket
    *client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (*client_fd == -1)
    {
        perror("socket error");
        return -1;
    }

    // Zero out the address structure and set the family and path
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    // Connect to the server
    if (connect(*client_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("connect error");
        close(*client_fd);
        return -1;
    }

    std::cout << "Connected to server on " << path << std::endl;

    // Return the client socket file descriptor
    return *client_fd;
}

int openUDSDatagramServer(const char *path, int *fd)
{
    struct sockaddr_un addr;

    // Create a Unix domain datagram socket
    *fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (*fd == -1)
    {
        perror("socket error");
        return -1;
    }

    // Zero out the address structure and set the family and path
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    // Unlink the path if it already exists
    unlink(path);

    // Bind the socket to the specified path
    if (bind(*fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("bind error");
        close(*fd);
        return -1;
    }

    std::cout << "Unix Domain Datagram server is listening on " << path << std::endl;

    return 0;
}

int connectUDSDatagramClient(const char *path, int *fd)
{
    // create a socket
    *fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (*fd == -1)
    {
        perror("error creating socket");
        return -1;
    }

    // setup the server address structure
    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, path);

    // connect the socket to the server address
    if (connect(*fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("error connecting socket");
        close(*fd);
        return -1;
    }

    return 0;
}
// Function to transfer data from input_fd to output_fd
void transfer_data(int input_fd, int output_fd)
{
    char buffer[1024];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(input_fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        // Write the buffer to the output_fd
        bytes_written = write(output_fd, buffer, bytes_read);
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

void handle_redirection_e(int argc, char *argv[], const char *local_host, int &input_fd, int &output_fd, int &in_and_out_fd)
{
    // Handle input redirection
    if (strncmp(argv[3], "-i", 2) == 0)
    {

        if (strncmp(argv[4], "TCPS", 4) == 0)
        {
            start_tcp_server(atoi(argv[4] + 4), &input_fd);
        }
        else if (strncmp(argv[4], "TCPC", 4) == 0)
        {
            start_tcp_client(local_host, atoi(argv[4] + 14), &input_fd);
        }
        else if (strncmp(argv[4], "UDPS", 4) == 0)
        {
            start_udp_server(atoi(argv[4] + 4), &input_fd);
        }
        else if (strncmp(argv[4], "UDPC", 4) == 0)
        {
            start_udp_client(local_host, atoi(argv[4] + 14), &input_fd);
        }
        else if (strncmp(argv[4], "UDSSS", 5) == 0)
        {
            openUDSStreamServer(PATH, &input_fd);
        }
        else if (strncmp(argv[4], "UDSSD", 5) == 0)
        {
            openUDSDatagramServer(PATH, &input_fd);
        }

        // if argc>5
        if (argc > 5)
        {
            if (strncmp(argv[5], "-o", 2) == 0)
            {
                if (strncmp(argv[6], "TCPC", 4) == 0)
                {
                    start_tcp_client(local_host, atoi(argv[6] + 14), &output_fd);
                }
                else if (strncmp(argv[6], "TCPS", 4) == 0)
                {
                    start_tcp_server(atoi(argv[6] + 4), &output_fd);
                }
                else if (strncmp(argv[6], "UDPS", 4) == 0)
                {
                    start_udp_server(atoi(argv[6] + 4), &output_fd);
                }
                else if (strncmp(argv[6], "UDPC", 4) == 0)
                {
                    start_udp_client(local_host, atoi(argv[6] + 14), &output_fd);
                }
                else if (strncmp(argv[6], "UDSCS", 5) == 0)
                {
                    openUDSStreamClient(PATH, &output_fd);
                }
            }
        }
    }

    else if (strncmp(argv[3], "-o", 2) == 0)
    {
        if (strncmp(argv[4], "TCPS", 4) == 0)
        {
            start_tcp_server(atoi(argv[4] + 4), &output_fd);
        }
        else if (strncmp(argv[4], "TCPC", 4) == 0)
        {
            start_tcp_client(local_host, atoi(argv[4] + 14), &output_fd);
        }
        else if (strncmp(argv[4], "UDPS", 4) == 0)
        {
            start_udp_server(atoi(argv[4] + 14), &output_fd);
        }
        else if (strncmp(argv[4], "UDPC", 4) == 0)
        {
            start_udp_client(local_host, atoi(argv[4] + 14), &output_fd);
        }
        else if (strncmp(argv[4], "UDSCS", 5) == 0)
        {
            openUDSStreamClient(PATH, &output_fd);
        }
        else if (strncmp(argv[4], "UDSCD", 5) == 0)
        {
            connectUDSDatagramClient(PATH, &output_fd);
        }
    }
    else if (strncmp(argv[3], "-b", 2) == 0)
    {
        if (strncmp(argv[4], "TCPS", 4) == 0)
        {
            start_tcp_server(atoi(argv[4] + 4), &in_and_out_fd);
        }
        else if (strncmp(argv[4], "TCPC", 4) == 0)
        {
            start_tcp_client(local_host, atoi(argv[4] + 14), &in_and_out_fd);
        }
    }
}

void handle_redirection(int argc, char *argv[], const char *local_host, int &input_fd, int &output_fd, int &in_and_out_fd)
{
    // Handle input redirection

    if (strncmp(argv[1], "-i", 2) == 0)
    {
        if (strncmp(argv[2], "TCPS", 4) == 0)
        {
            start_tcp_server(atoi(argv[2] + 4), &input_fd);
        }
        else if (strncmp(argv[2], "TCPC", 4) == 0)
        {
            start_tcp_client(local_host, atoi(argv[2] + 14), &input_fd);
        }

        else if (strncmp(argv[2], "UDPS", 4) == 0)
        {
            start_udp_server(atoi(argv[2] + 4), &input_fd);
        }

        else if (strncmp(argv[2], "UDPC", 4) == 0)
        {
            start_udp_client(local_host, atoi(argv[2] + 14), &input_fd);
        }
         else if (strncmp(argv[2], "UDSSS", 5) == 0)
        {
            openUDSStreamServer(PATH, &input_fd);
        }
        else if (strncmp(argv[2], "UDSSD", 5) == 0)
        {
            openUDSDatagramServer(PATH, &input_fd);
        }

        if (argc > 3)
        {

            if (strncmp(argv[3], "-o", 2) == 0)
            {
                if (strncmp(argv[4], "TCPC", 4) == 0)
                {
                    start_tcp_client(local_host, atoi(argv[4] + 14), &output_fd);
                }
                else if (strncmp(argv[4], "TCPS", 4) == 0)
                {
                    start_tcp_server(atoi(argv[4] + 4), &output_fd);
                }
                else if (strncmp(argv[4], "UDPS", 4) == 0)
                {
                    start_udp_server(atoi(argv[4] + 4), &output_fd);
                }
                else if (strncmp(argv[4], "UDPC", 4) == 0)
                {
                    start_udp_client(local_host, atoi(argv[4] + 14), &output_fd);
                }
            }
        }
    }
    else if (strncmp(argv[1], "-o", 2) == 0)
    {
        if (strncmp(argv[2], "TCPS", 4) == 0)
        {
            start_tcp_server(atoi(argv[2] + 4), &output_fd);
        }
        else if (strncmp(argv[2], "TCPC", 4) == 0)
        {
            start_tcp_client(local_host, atoi(argv[2] + 14), &output_fd);
        }
        else if (strncmp(argv[2], "UDPS", 4) == 0)
        {
            start_udp_server(atoi(argv[2] + 4), &output_fd);
        }
        else if (strncmp(argv[2], "UDPC", 4) == 0)
        {
            start_udp_client(local_host, atoi(argv[2] + 14), &output_fd);
        }
         else if (strncmp(argv[2], "UDSCS", 5) == 0)
        {
            openUDSStreamClient(PATH, &output_fd);
        }
        else if (strncmp(argv[2], "UDSCD", 5) == 0)
        {
            connectUDSDatagramClient(PATH, &output_fd);
        }
    }
    else if (strncmp(argv[1], "-b", 2) == 0)
    {
        if (strncmp(argv[2], "TCPS", 4) == 0)
        {
            start_tcp_server(atoi(argv[2] + 4), &in_and_out_fd);
        }
        else if (strncmp(argv[2], "TCPC", 4) == 0)
        {
            start_tcp_client(local_host, atoi(argv[2] + 14), &in_and_out_fd);
        }
    }
}

// Signal handler for SIGALRM
void alarmHandler(int signum)
{
    std::cout << "Alarm signal (" << signum << ") received. Terminating all specified processes.\n";
    // Command to kill all processes. Adjust the command as needed.
    system("pkill -9 ttt"); // Replace "ttt" with the actual name of the process to kill
    exit(signum);
}

int main(int argc, char *argv[])
{
    const char *local_host = "127.0.0.1"; // Localhost IP address
    int input_fd = -1, output_fd = -1, in_and_out_fd = -1;

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " [-e command] [-i|-o|-b <TCPS|TCPC><port>]" << std::endl;
        return 1;
    }

    // Check if the first argument is "-e"
    bool execute_program = strcmp(argv[1], "-e") == 0;
    if (execute_program && argc < 3)
    {
        std::cout << "Please provide the program name and its arguments after -e" << std::endl;
        return 1;
    }

    if (execute_program)
    {
        // Split the second argument into program name and arguments
        char *program = strtok(argv[2], " ");
        char *arguments = strtok(NULL, "");
        char *new_argv[] = {program, arguments, NULL};

        // Handle input/output redirection
        handle_redirection_e(argc, argv, local_host, input_fd, output_fd, in_and_out_fd);

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

            // Redirect both standard input and output if in_and_out_fd is set
            if (in_and_out_fd != -1)
            {
                dup2(in_and_out_fd, STDIN_FILENO);
                dup2(in_and_out_fd, STDOUT_FILENO);
                close(in_and_out_fd);
            }
            execvp(program, new_argv);

            // If execvp returns, there was an error
            perror("execvp");
            return 1;
        }
        else
        {
            // Parent process
            // Check if argv[5] == 't' and set an alarm for the time specified in argv[6]
            if (argc > 5)
            {

                if (strcmp(argv[5], "-t") == 0)
                {
                    signal(SIGALRM, alarmHandler);
                    int alarm_time = std::atoi(argv[6]);
                    alarm(alarm_time);
                }
            }
            int status;
            waitpid(pid, &status, 0);
        }
    }
    else
    {
        // Handle redirection
        handle_redirection(argc, argv, local_host, input_fd, output_fd, in_and_out_fd);

        if (argc == 3)
        {

            if (input_fd != -1)
            {
                transfer_data(input_fd, STDOUT_FILENO);
                close(input_fd);
            }

            if (output_fd != -1)
            {
                transfer_data(STDIN_FILENO, output_fd);
                close(output_fd);
            }
            if (in_and_out_fd != -1)
            {
                transfer_data(in_and_out_fd, in_and_out_fd);
                close(in_and_out_fd);
            }
        }

        else
        {

            transfer_data(input_fd, output_fd);
        }
    }

    return 0;
}