#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>

using namespace std;

#define MAX 100	
#define BASE_PORT 8000

void grep(char *read_msg, char **args) {

    int pipefd[2];
    pipe(pipefd);
    pid_t grep_pid, wpid;

    // Fork and run the grep command via execvp system call. 
    // Redirect the output from the standard output file (terminal) through pipe
    if((grep_pid=fork()) == 0) {
        
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        close(pipefd[1]);
        execvp(args[0], args);
        exit(0);
    }
    else {
        close(pipefd[1]);
        wait(&grep_pid);
    }

    int byte_read_count = read(pipefd[0], read_msg, sizeof(read_msg));
    close(pipefd[0]);

    int last_index = byte_read_count/sizeof(read_msg[0]);
    read_msg[last_index] = '\0';
}

int main(int argc, char *argv[]) {

    int server_ctrlsock_fd, server_newctrlsock_fd;
    struct sockaddr_in serv_addr, ctrlcli_addr;

    char *MACHINE_NUM = argv[1];
    int PORT = BASE_PORT + stoi(MACHINE_NUM);

    cout << PORT << endl;

    // Find the log filename
    string filename = string("MP1 Demo Data FA22/vm") + MACHINE_NUM + ".log";

    char mssg[MAX];
    char cmnd[MAX];
    socklen_t clilen;
    fstream logfile;

    // Creating control socket
    if((server_ctrlsock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // IPv4, TCP
        perror("Socket creation failed\n");
        exit(0);
    }

    // Initializing the socket address to 0
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&ctrlcli_addr, 0, sizeof(ctrlcli_addr));

    // Specifying the control server address
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY; // any address
    serv_addr.sin_port = htons(PORT);

    // Setting Socket options to reuse socket address
    int var = 1;
    if (setsockopt(server_ctrlsock_fd, SOL_SOCKET, SO_REUSEADDR, &var, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(0);
    }

    // Binding the server
    if(bind(server_ctrlsock_fd,(const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("Binding error\n");
        exit(0);
    }

    // Machine accepting connections
    listen(server_ctrlsock_fd, 10);
    cout << "Server Running...\n";

    while(1) {

        clilen = sizeof(ctrlcli_addr);
        // Accept control connection from client
        server_newctrlsock_fd = accept(server_ctrlsock_fd, (struct sockaddr *)&ctrlcli_addr, &clilen);

        recv(server_newctrlsock_fd, cmnd, MAX, 0);

        char *args[MAX];
        char *word = strtok (cmnd," ");
        int i = 0;
        while (word != NULL){
            args[i++] = word;
            word = strtok (NULL, " ");
        }
        // Append the filename and NULL in the end
        args[i++] = (char*)filename.c_str();
        args[i++] = (char*)"-c";
        args[i] = NULL;

        // Open the log file
        logfile.open(filename, ios::in);


        // If file is open, read each line of the file and search for regex
        // TODO: Send through a data socket. Currently sent through control socket
        if (logfile.is_open()) {
            char send_msg[MAX];
            grep(send_msg, args);

            send(server_newctrlsock_fd, send_msg, strlen(send_msg)+1, 0);
            cout << "Sent: " << send_msg << endl;
            logfile.close();
        }
    }

    return 0;
}