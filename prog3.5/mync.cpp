#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <csignal>
#include <sys/wait.h>

using namespace std;

bool run = true;


// Function to start a TCP server
int TCPServer(const string &port)
{
int s_sock = socket(AF_INET, SOCK_STREAM, 0);
if (s_sock < 0)
{
    perror("Error creating socket");
    exit(EXIT_FAILURE);
}

int opt = 1;
if (setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
{
    perror("Error setting socket options");
    close(s_sock);
    exit(EXIT_FAILURE);
}

struct sockaddr_in server_addr = {};
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(stoi(port));
server_addr.sin_addr.s_addr = INADDR_ANY;

if (bind(s_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
{
    perror("Error binding socket");
    close(s_sock);
    exit(EXIT_FAILURE);
}

if (listen(s_sock, 1) < 0)
{
    perror("Error listening on socket");
    close(s_sock);
    exit(EXIT_FAILURE);
}

return s_sock;
}

// Function to handle a client connection for input redirection
void client_input(int c_sock)
{
dup2(c_sock, STDIN_FILENO);
}

// Function to handle a client connection for output redirection
void client_output(int c_sock)
{
dup2(c_sock, STDOUT_FILENO);
dup2(c_sock, STDERR_FILENO); // Also redirect stderr to the client socket
}



int TCPClient(const string &hostname, const string &port)
{
int c_sock = socket(AF_INET, SOCK_STREAM, 0);
if (c_sock < 0)
{
    perror("Error creating socket");
    exit(EXIT_FAILURE);
}

struct hostent *server = gethostbyname(hostname.c_str());
if (server == nullptr)
{
    cerr << "Error: No such host" << endl;
    close(c_sock);
    exit(EXIT_FAILURE);
}

struct sockaddr_in server_addr = {};
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(stoi(port));
memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
cout << "Connecting to " << hostname << " on port " << port << endl;
if (connect(c_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
{
    perror("Error connecting to server");
    close(c_sock);
    exit(EXIT_FAILURE);
}
cout << "  Connected to server " << endl;
return c_sock;
}

void signal(int s)
{
if (s == SIGINT)
{
    run = false;
}
}

// Function to seperate a string by spaces into a vector of strings
vector<string> seperate(const string &str)
{
vector<string> result;
istringstream iss(str);
for (string s; iss >> s;)
{
    result.push_back(s);
}
return result;
}

int main(int argc, char *argv[])
{
signal(SIGINT, signal); 

int opt;
char *program = nullptr;
string input_redirect, output_redirect;


while ((opt = getopt(argc, argv, "e:i:o:b:")) != -1)
{
    switch (opt)
    {
    case 'e':
        program = optarg;
        break;
    case 'i':
        input_redirect = optarg;
        break;
    case 'o':
        output_redirect = optarg;
        if (!program)
        {
            input_redirect = optarg;
        }
        break;
    case 'b':
        input_redirect = optarg;
        output_redirect = optarg;
        break;
    default:
        cerr << "Usage: " << argv[0] << " -e <program> [args] [-i <input_redirect>] [-o <output_redirect>] [-b <bi_redirect>]" << endl;
        return EXIT_FAILURE;
    }
}

if (program)
{
    cout << "Input redirection: " << input_redirect << endl;
    cout << "Output redirection: " << output_redirect << endl;

    
    string program_str = "./" + string(program);

    vector<string> seperate_program = seperate(program_str); 
    vector<char *> args;                               
    for (const auto &arg : seperate_program)
    { 
        args.push_back(const_cast<char *>(arg.c_str()));
    }
    args.push_back(NULL);

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Error forking process"); 
        return EXIT_FAILURE;
    }

    if (pid == 0)
    { 
        if (!input_redirect.empty())
        {
            if (input_redirect.substr(0, 4) == "TCPS")
            {
                int port = stoi(input_redirect.substr(4));
                int s_sock = TCPServer(to_string(port));
                int c_sock = accept(s_sock, nullptr, nullptr);
                if (c_sock < 0)
                {
                    perror("Error accepting connection");
                    close(s_sock);
                    return EXIT_FAILURE;
                }
                client_input(c_sock);
                if (input_redirect == output_redirect)
                {
                    client_output(c_sock);
                }
                close(s_sock);
            }
            
        }

        if (!output_redirect.empty() && output_redirect != input_redirect)
        {
            if (output_redirect.substr(0, 4) == "TCPS")
            {
                int port = stoi(output_redirect.substr(4));
                int s_sock = TCPServer(to_string(port));
                int c_sock = accept(s_sock, nullptr, nullptr);
                if (c_sock < 0)
                {
                    perror("Error accepting connection");
                    close(s_sock);
                    return EXIT_FAILURE;
                }
                client_output(c_sock);
                close(s_sock);
            }
            else if (output_redirect.substr(0, 4) == "TCPC")
            {
                string host_port = output_redirect.substr(4);
                size_t comma_pos = host_port.find(',');
                if (comma_pos != string::npos)
                {
                    string hostname = host_port.substr(0, comma_pos);
                    string port = host_port.substr(comma_pos + 1);
                    int c_sock = TCPClient(hostname, port);
                    client_output(c_sock);
                }
                else
                {
                                        cerr << "Invalid TCPC format. Expected TCPC<hostname,port>" << endl;
                return EXIT_FAILURE;
            }
            
        }

        // Set the buffer to be line-buffered
        setvbuf(stdout, nullptr, _IOLBF, BUFSIZ);

        // Executing the specified program with the given arguments
        execvp(args[0], args.data());
        // execvp only returns if an error occurs
        perror("Error executing program");
        return EXIT_FAILURE;
    }
    else
    { // Parent process
        int status;
        while (run && waitpid(pid, &status, WNOHANG) == 0)
        {
            sleep(1);
        }
        if (!run)
        {
            kill(pid, SIGTERM);
        }
    }
}
else
{ // If no program is provided, set up a live chat between two terminals
    if (!input_redirect.empty())
    {
        if (input_redirect.substr(0, 4) == "TCPS")
        {
            int port = stoi(input_redirect.substr(4));
            int s_sock = TCPServer(to_string(port));
            int c_sock = accept(s_sock, nullptr, nullptr);
            if (c_sock < 0)
            {
                perror("Error accepting connection");
                close(s_sock);
                return EXIT_FAILURE;
            }
            cout << "Connected to client" << endl;
            cout << "Enter message to send to client" << endl;
            fd_set read_fds;
            char buffer[1024];
            ssize_t n;

            while (run)
            {
                FD_ZERO(&read_fds);
                FD_SET(c_sock, &read_fds);
                FD_SET(STDIN_FILENO, &read_fds);

                int max_fd = max(c_sock, STDIN_FILENO) + 1;
                int activity = select(max_fd, &read_fds, nullptr, nullptr, nullptr);

                if (activity < 0 && errno != EINTR)
                {
                    perror("Error in select");
                    break;
                }

                if (FD_ISSET(c_sock, &read_fds))
                {
                    n = read(c_sock, buffer, sizeof(buffer) - 1);
                    if (n <= 0)
                    {
                        break;
                    }
                    buffer[n] = '\0';
                    cout << buffer << flush;
                }

                if (FD_ISSET(STDIN_FILENO, &read_fds))
                {
                    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (n <= 0)
                    {
                        break;
                    }
                    buffer[n] = '\0';
                    write(c_sock, buffer, n);
                }
            }

            close(c_sock);
            close(s_sock);
        }
        else if (input_redirect.substr(0, 4) == "TCPC")
        {
            string host_port = input_redirect.substr(4);
            size_t comma_pos = host_port.find(',');
            if (comma_pos != string::npos)
            {
                string hostname = host_port.substr(0, comma_pos);
                string port = host_port.substr(comma_pos + 1);
                int c_sock = TCPClient(hostname, port);

                fd_set read_fds;
                char buffer[1024];
                ssize_t n;

                while (run)
                {
                    FD_ZERO(&read_fds);
                    FD_SET(c_sock, &read_fds);
                    FD_SET(STDIN_FILENO, &read_fds);

                    int max_fd = max(c_sock, STDIN_FILENO) + 1;
                    int activity = select(max_fd, &read_fds, nullptr, nullptr, nullptr);

                    if (activity < 0 && errno != EINTR)
                    {
                        perror("Error in select");
                        break;
                    }

                    if (FD_ISSET(c_sock, &read_fds))
                    {
                        n = read(c_sock, buffer, sizeof(buffer) - 1);
                        if (n <= 0)
                        {
                            break;
                        }
                        buffer[n] = '\0';
                        cout << buffer << flush;
                    }

                    if (FD_ISSET(STDIN_FILENO, &read_fds))
                    {
                        n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                        if (n <= 0)
                        {
                            break;
                        }
                        buffer[n] = '\0';
                        write(c_sock, buffer, n);
                    }
                }

                close(c_sock);
            }
            else
            {
                cerr << "Invalid TCPC format. Expected TCPC<hostname,port>" << endl;
                return EXIT_FAILURE;
            }
        }
    }
    else
    {
        cerr << "Usage: " << argv[0] << " -e <program> [args] [-i <input_redirect>] [-o <output_redirect>] [-b <bi_redirect>]" << endl;
        return EXIT_FAILURE;
    }
}

return 0;
}
}

