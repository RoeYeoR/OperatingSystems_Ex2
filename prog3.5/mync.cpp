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
void initialize_tcp_server(int port, int *connection_fd);
void initialize_tcp_client(const char *hostname, int port, int *connection_fd);
void perform_data_transfer(int source_fd, int destination_fd);
void manage_redirection_with_e(int argc, char *argv[], const char *local_host, int &input_fd, int &output_fd, int &bi_directional_fd);
void manage_redirection(int argc, char *argv[], const char *local_host, int &input_fd, int &output_fd, int &bi_directional_fd);

int main(int argc, char *argv[])
{
    const char *localhost_ip = "127.0.0.1"; // Localhost IP address
    int input_fd = -1, output_fd = -1, bi_directional_fd = -1;

    // Check if there are enough arguments
    if (argc < 2)
    {
        printf("Usage: %s [-e command] [-i|-o|-b <TCPS|TCPC><port>]\n", argv[0]);
        return 1;
    }

    // Check if the first argument is "-e"
    bool is_execute_command = strcmp(argv[1], "-e") == 0;
    if (is_execute_command && argc < 3)
    {
        printf("Please provide the program name and its arguments after -e\n");
        return 1;
    }

    if (is_execute_command)
    {
        // Split the second argument into program name and arguments
        char *program = strtok(argv[2], " ");
        char *arguments = strtok(NULL, "");
        char *new_argv[] = {program, arguments, NULL};

        // Handle input/output redirection with -e parameter
        manage_redirection_with_e(argc, argv, localhost_ip, input_fd, output_fd, bi_directional_fd);

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

            // Redirect both standard input and output if bi_directional_fd is set
            if (bi_directional_fd != -1)
            {
                dup2(bi_directional_fd, STDIN_FILENO);
                dup2(bi_directional_fd, STDOUT_FILENO);
                close(bi_directional_fd);
            }

            // Execute the program with provided arguments
            execvp(program, new_argv);

            // If execvp returns, there was an error
            perror("execvp");
            return 1;
        }
        else
        {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
        }
    }
    else
    {
        // Handle redirection without -e parameter
        manage_redirection(argc, argv, localhost_ip, input_fd, output_fd, bi_directional_fd);

        // Perform data transfer based on the number of arguments
        if (argc == 3)
        {
            if (input_fd != -1)
            {
                perform_data_transfer(input_fd, STDOUT_FILENO);
                close(input_fd);
            }

            if (output_fd != -1)
            {
                perform_data_transfer(STDIN_FILENO, output_fd);
                close(output_fd);
            }

            if (bi_directional_fd != -1)
            {
                perform_data_transfer(bi_directional_fd, bi_directional_fd);
                close(bi_directional_fd);
            }
        }
        else
        {
            perform_data_transfer(input_fd, output_fd);
        }
    }

    return 0;
}

// Initialize a TCP server on the specified port and accept a single connection
void initialize_tcp_server(int port, int *connection_fd)
{
    int server_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_length = sizeof(client_address);

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("socket");
        exit(1);
    }

    // Initialize server_address structure
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    int reuse = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind the socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind");
        close(server_socket);
        exit(1);
    }

    std::cout << "Awaiting client connection on port: " << port << std::endl;

    // Listen for incoming connections
    listen(server_socket, 5);

    // Accept a connection
    *connection_fd = accept(server_socket, (struct sockaddr *)&client_address, &client_length);
    if (*connection_fd < 0)
    {
        perror("accept");
        close(server_socket);
        exit(1);
    }

    std::cout << "Connection established successfully" << std::endl;

    // Close the listening socket (not needed after accepting a connection)
    close(server_socket);
}

// Function to start a TCP client
void initialize_tcp_client(const char *hostname, int port, int *connection_fd)
{
    struct sockaddr_in server_address;
    struct hostent *server;

    // Create a socket
    *connection_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*connection_fd < 0)
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

    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET; // Set address family to IPv4

    // Copy the server's IP address to the server address structure
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port); // Convert port to network byte order and set it

    // Attempt to connect to the server
    if (connect(*connection_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("connect");
        close(*connection_fd);
        exit(1);
    }

    std::cout << "Connection established successfully" << std::endl;
}

// Function to transfer data from input_fd to output_fd
void perform_data_transfer(int source_fd, int destination_fd)
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

// Function to handle input/output redirection when the -e parameter is present
void manage_redirection_with_e(int argc, char *argv[], const char *local_host, int &input_fd, int &output_fd, int &bi_directional_fd)
{
    // Handle input redirection
    if (strncmp(argv[3], "-i", 2) == 0)
    {
        if (argc == 5)
        {
            if (strncmp(argv[4], "TCPS", 4) == 0)
            {
                initialize_tcp_server(atoi(argv[4] + 4), &input_fd);
            }
            else if (strncmp(argv[4], "TCPC", 4) == 0)
            {
                initialize_tcp_client(local_host, atoi(argv[4] + 14), &input_fd);
            }
        }
        else
        {
            if (strncmp(argv[4], "TCPS", 4) == 0)
            {
                initialize_tcp_server(atoi(argv[4] + 4), &input_fd);
            }
            else if (strncmp(argv[4], "TCPC", 4) == 0)
            {
                initialize_tcp_client(local_host, atoi(argv[4] + 14), &input_fd);
            }

            if (strncmp(argv[5], "-o", 2) == 0)
            {
                if (strncmp(argv[6], "TCPC", 4) == 0)
                {
                    initialize_tcp_client(local_host, atoi(argv[6] + 14), &output_fd);
                }
                else if (strncmp(argv[6], "TCPS", 4) == 0)
                {
                    initialize_tcp_server(atoi(argv[6] + 4), &output_fd);
                }
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
            initialize_tcp_client(local_host, atoi(argv[4] + 14), &output_fd);
        }
    }
    else if (strncmp(argv[3], "-b", 2) == 0)
    {
        if (strncmp(argv[4], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[4] + 4), &bi_directional_fd);
        }
        else if (strncmp(argv[4], "TCPC", 4) == 0)
        {
            initialize_tcp_client(local_host, atoi(argv[4] + 14), &bi_directional_fd);
        }
    }
}

// Function to handle input/output redirection when the -e parameter is not present
void manage_redirection(int argc, char *argv[], const char *local_host, int &input_fd, int &output_fd, int &bi_directional_fd)
{
    // Handle input redirection
    if (strncmp(argv[1], "-i", 2) == 0)
    {
        if (argc == 3)
        {
            if (strncmp(argv[2], "TCPS", 4) == 0)
            {
                initialize_tcp_server(atoi(argv[2] + 4), &input_fd);
            }
            else if (strncmp(argv[2], "TCPC", 4) == 0)
            {
                initialize_tcp_client(local_host, atoi(argv[2] + 14), &input_fd);
            }
        }
        else
        {
            if (strncmp(argv[2], "TCPS", 4) == 0)
            {
                initialize_tcp_server(atoi(argv[2] + 4), &input_fd);
            }
            else if (strncmp(argv[2], "TCPC", 4) == 0)
            {
                initialize_tcp_client(local_host, atoi(argv[2] + 14), &input_fd);
            }

            if (strncmp(argv[3], "-o", 2) == 0)
            {
                if (strncmp(argv[4], "TCPC", 4) == 0)
                {
                    initialize_tcp_client(local_host, atoi(argv[4] + 14), &output_fd);
                }
                else if (strncmp(argv[4], "TCPS", 4) == 0)
                {
                    initialize_tcp_server(atoi(argv[4] + 4), &output_fd);
                }
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
            initialize_tcp_client(local_host, atoi(argv[2] + 14), &output_fd);
        }
    }
    else if (strncmp(argv[1], "-b", 2) == 0)
    {
        if (strncmp(argv[2], "TCPS", 4) == 0)
        {
            initialize_tcp_server(atoi(argv[2] + 4), &bi_directional_fd);
        }
        else if (strncmp(argv[2], "TCPC", 4) == 0)
        {
            initialize_tcp_client(local_host, atoi(argv[2] + 14), &bi_directional_fd);
        }
    }
}
