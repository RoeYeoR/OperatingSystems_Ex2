#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: mync \"ttt + (9 different digits as argument)\" [options]\n";
        return 1;
    }

    std::string command;
    int inputFd = -1;
    int outputFd = -1;

    // Treat the whole command-line argument list as the command to run
    for (int i = 1; i < argc; ++i) {
        command += argv[i];
        command += " ";
    }
    command.pop_back(); // Remove the trailing space

    executeProgram(command, inputFd, outputFd);

    if (inputFd != -1) {
        close(inputFd);
    }
    if (outputFd != -1 && outputFd != inputFd) {
        close(outputFd);
    }

    return 0;
}
