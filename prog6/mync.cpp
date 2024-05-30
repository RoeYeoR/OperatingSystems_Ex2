#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstdlib>
#include "../../../../../../usr/include/x86_64-linux-gnu/sys/wait.h"

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
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            std::cout << "Program exited with status: " << exit_status << std::endl;
        }
    } else {
        perror("fork");
        exit(1);
    }

    for (char* arg : args) {
        delete[] arg;
    }
}

int createUnixDatagramServer(const std::string& path) {
    int serverFd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (serverFd == -1) {
        perror("socket");
        exit(1);
    }

    sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, path.c_str(), sizeof(serverAddr.sun_path) - 1);
    unlink(path.c_str()); // Remove any existing socket file
    if (bind(serverFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind");
        close(serverFd);
        exit(1);
    }

    return serverFd;
}

int createUnixDatagramClient(const std::string& path) {
    int clientFd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (clientFd == -1) {
        perror("socket");
        exit(1);
    }

    sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, path.c_str(), sizeof(serverAddr.sun_path) - 1);

    if (connect(clientFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        close(clientFd);
        exit(1);
    }

    return clientFd;
}

int createUnixStreamServer(const std::string& path) {
    int serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket");
        exit(1);
    }

    sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, path.c_str(), sizeof(serverAddr.sun_path) - 1);
    unlink(path.c_str()); // Remove any existing socket file
    if (bind(serverFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind");
        close(serverFd);
        exit(1);
    }

    if (listen(serverFd, 1) == -1) {
        perror("listen");
        close(serverFd);
        exit(1);
    }

    int clientFd = accept(serverFd, NULL, NULL);
    if (clientFd == -1) {
        perror("accept");
        close(serverFd);
        exit(1);
    }

    close(serverFd);
    return clientFd;
}

int createUnixStreamClient(const std::string& path) {
    int clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (clientFd == -1) {
        perror("socket");
        exit(1);
    }

    sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strncpy(serverAddr.sun_path, path.c_str(), sizeof(serverAddr.sun_path) - 1);

    if (connect(clientFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("connect");
        close(clientFd);
        exit(1);
    }

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
        std::cerr << "Usage: mync \"program_to_run\" [options]\n";
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
        } else if (arg.find("UDSSD") == 0) {
            // Start Unix domain datagram server
            std::string path = arg.substr(5);
            inputFd = createUnixDatagramServer(path);
        } else if (arg.find("UDSCD") == 0) {
            // Start Unix domain datagram client
            std::string path = arg.substr(5);
            outputFd = createUnixDatagramClient(path);
        } else if (arg.find("UDSSS") == 0) {
            // Start Unix domain stream server
            std::string path = arg.substr(5);
            inputFd = createUnixStreamServer(path);
        } else if (arg.find("UDSCS") == 0) {
            // Start Unix domain stream client
            std::string path = arg.substr(5);
            outputFd = createUnixStreamClient(path);
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

