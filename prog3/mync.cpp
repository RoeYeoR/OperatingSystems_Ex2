#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>

void startTCPServer(int port, int *fd);
void startTCPClient(const char *hostname, int port, int *fd);
void handleRedirection(int argc, char *argv[], const char *localhost, int &in_fd, int &out_fd, int &both_fd);

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "-e") != 0) {
        std::cerr << "Usage: " << argv[0] << " -e <program> [<options>]\n";
        return 1;
    }

    char *program = strtok(argv[2], " ");
    char *arguments = strtok(NULL, "");
    char *new_argv[] = {program, arguments, NULL};
    const char *localhost = "127.0.0.1";

    int in_fd = -1, out_fd = -1, both_fd = -1;

    handleRedirection(argc, argv, localhost, in_fd, out_fd, both_fd);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        if (in_fd != -1) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (out_fd != -1) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        if (both_fd != -1) {
            dup2(both_fd, STDIN_FILENO);
            dup2(both_fd, STDOUT_FILENO);
            close(both_fd);
        }

        execvp(program, new_argv);
        perror("execvp");
        return 1;
    } else {
        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}

// Function to handle input and output redirection
void handleRedirection(int argc, char *argv[], const char *localhost, int &in_fd, int &out_fd, int &both_fd) {

    switch (argv[3][1]) {

        case 'i':
            if (argc == 5) {
                switch (argv[4][3]) {
                    case 'S':
                        startTCPServer(atoi(argv[4] + 4), &in_fd);
                        break;
                    case 'C':
                        startTCPClient(localhost, atoi(argv[4] + 4), &in_fd);
                        break;
                }
            } else {
                switch (argv[4][3]) {
                    case 'S':
                        startTCPServer(atoi(argv[4] + 4), &in_fd);
                        break;
                    case 'C':
                        startTCPClient(localhost, atoi(argv[4] + 4), &in_fd);
                        break;
                }
                switch (argv[5][1]) {
                    case 'o':
                        switch (argv[6][3]) {
                            case 'C':
                                startTCPClient(localhost, atoi(argv[6] + 4), &out_fd);
                                break;
                            case 'S':
                                startTCPServer(atoi(argv[6] + 4), &out_fd);
                                break;
                        }
                        break;
                }
            }
            break;

        case 'o':
            switch (argv[4][3]) {
                case 'S':
                    startTCPServer(atoi(argv[4] + 4), &out_fd);
                    break;
                case 'C':
                    startTCPClient(localhost, atoi(argv[4] + 4), &out_fd);
                    break;
            }
            break;

        case 'b':
            switch (argv[4][3]) {
                case 'S':
                    startTCPServer(atoi(argv[4] + 4), &both_fd);
                    break;
                case 'C':
                    startTCPClient(localhost, atoi(argv[4] + 4), &both_fd);
                    break;
            }
            break;
    }
}

// Function to start a TCP server and accept a single connection
void startTCPServer(int port, int *fd) {

    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(1);
    }

    std::cout << "Listening for client on port: " << port << std::endl;
    listen(sockfd, 5);

    *fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    if (*fd < 0) {
        perror("accept");
        close(sockfd);
        exit(1);
    }

    std::cout << "Connection established" << std::endl;
    close(sockfd);
}

// Function to start a TCP client
void startTCPClient(const char *hostname, int port, int *fd) {

    struct sockaddr_in serv_addr;
    struct hostent *server;

    *fd = socket(AF_INET, SOCK_STREAM, 0);

    if (*fd < 0) {
        perror("socket");
        exit(1);
    }

    server = gethostbyname(hostname);

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(*fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(*fd);
        exit(1);
    }

    std::cout << "Connected to server" << std::endl;
}
