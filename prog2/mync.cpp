#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>

void executeProgram(const std::string& command) {
    // Split the command into program and arguments
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

    // Fork a process to execute the command
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(args[0], args.data());
        // If execvp returns, an error occurred
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            std::cout << "Program exited with status: " << exit_status << std::endl;
        }
    } else {
        // Fork failed
        perror("fork");
        exit(1);
    }

    // Free allocated memory
    for (char* arg : args) {
        delete[] arg;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "-e") {
        std::cerr << "Usage: " << argv[0] << " -e \"ttt 123456789\"\n";
        return 1;
    }

    std::string command = argv[2];
    executeProgram(command);

    return 0;
}
