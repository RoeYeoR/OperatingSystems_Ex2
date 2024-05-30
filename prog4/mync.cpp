#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

// Global variable to store the timeout value
int timeout = 0;

// Signal handler for alarm
void alarmHandler(int signal) {
    std::cerr << "Timeout reached. Killing processes..." << std::endl;
    exit(0);
}

void executeProgram(const std::string& command, int inputFd, int outputFd) {
    std::istringstream iss(command);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    std::vector<char*> args;
    for (const std::string& s : tokens) {
        char* arg = new char[s.size() + 1];
        std::copy(s.begin(), s.end(), arg);
        arg[s.size()] = '\0';
        args.push_back(arg);
    }
    args.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        if (inputFd != -1) {
            dup2(inputFd, STDIN_FILENO);
            close(inputFd);
        }
        if (outputFd != -1) {
            dup2(outputFd, STDOUT_FILENO);
            close(outputFd);
        }

        execvp(args[0], args.data());
        perror("execvp failed..");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            std::cout << "Program exited with status: " << exit_status << std::endl;
        }
    } else {
        perror("fork failed..");
        exit(1);
    }

    for (char* arg : args) {
        delete[] arg;
    }
}

int createTCPServer(int port) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket failed..");
        exit(1);
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind failed..");
        close(serverFd);
        exit(1);
    }

    if (listen(serverFd, 1) == -1) {
        perror("listen failed..");
        close(serverFd);
        exit(1);
    }

    int clientFd = accept(serverFd, NULL, NULL);
    if (clientFd == -1) {
        perror("accept failed..");
        close(serverFd);
        exit(1);
    }

    close(serverFd);
    return clientFd;
}

int createTCPClient(const std::string& hostname, int port) {
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd == -1) {
        perror("socket failed..");
        exit(1);
    }

    hostent* server = gethostbyname(hostname.c_str());
    if (!server) {
        std::cerr << "Error: no such host\n";
        close(clientFd);
        exit(1);
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(port);

    if (connect(clientFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect failed..");
        close(clientFd);
        exit(1);
    }

    return clientFd;
}

int createUDPServer(int port) {
    int serverFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverFd == -1) {
        perror("socket failed..");
        exit(1);
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind failed..");
        close(serverFd);
        exit(1);
    }

    return serverFd;
}

int createUDPClient(const std::string& hostname, int port) {
       int clientFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientFd == -1) {
        perror("socket failed..");
        exit(1);
    }

    hostent* server = gethostbyname(hostname.c_str());
    if (!server) {
        std::cerr << "Error: no such host\n";
        close(clientFd);
        exit(1);
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(port);

    // No need to bind for UDP client

    return clientFd;
}

void handleTimeout(int seconds) {
    if (seconds > 0) {
        timeout = seconds;
        alarm(seconds);
    } else {
        timeout = 0;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: mync \"ttt + (9 different digits as argument)\" [options]\n";
        return 1;
    }

    std::string command;
    int inputFd = -1;
    int outputFd = -1;

    // Process command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" || arg == "-o" || arg == "-b") {
            // Handle input/output redirection
            // ...
        } else if (arg.find("UDPS") == 0) {
            // Start UDP server
            // ...
        } else if (arg.find("UDPC") == 0) {
            // Start UDP client
            // ...
        } else if (arg == "-t") {
            // Timeout parameter
            if (i + 1 < argc) {
                int timeoutValue = std::stoi(argv[++i]);
                handleTimeout(timeoutValue);
            } else {
                std::cerr << "Error: Timeout value missing\n";
                return 1;
            }
        } else {
            command += arg;
            command += " ";
        }
    }

    // Remove trailing space from command
    if (!command.empty()) {
        command.pop_back();
    }

    // Execute the program
    executeProgram(command, inputFd, outputFd);

    if (inputFd != -1) {
        close(inputFd);
    }
    if (outputFd != -1 && outputFd != inputFd) {
        close(outputFd);
    }

    return 0;
}

