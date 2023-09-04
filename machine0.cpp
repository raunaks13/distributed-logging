#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <signal.h>
#include <fcntl.h>
#include <fstream>
#include <regex>
#include <sstream>

using namespace std;

#define MACHINE_NUM 0
#define MAX 100			// Max length of commands
#define MY_PORT 8000 + MACHINE_NUM	    // Port of TCP Control Server
#define PORT_1 8001


char* remove_leading_spaces(string cmnd, char *mssg) {

    // Handling the leading spaces
    int i=0;
    while(cmnd[i]==' ' || cmnd[i]=='\t') i++;
    int j=0;
    while(cmnd[i] != '\0'){
        mssg[j++] = cmnd[i];
        i++;
    }
    mssg[j] = '\0';

    return mssg;
}

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
        // fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
        wait(&grep_pid);
    }

    int byte_read_count = read(pipefd[0], read_msg, sizeof(read_msg));
    close(pipefd[0]);

    // if (byte_read_count > 0) {
    //     string init_msg = filename + "\t" + string(read_msg);
    //     file << "[1] Own: " << init_msg << endl;
    // }

    // Adding null character to the end
    int last_index = byte_read_count/sizeof(read_msg[0]);
    read_msg[last_index] = '\0';
}

int main() {
    /*
      ctrlsock_fd = listens to other machines for command
      newctrlsock_fd = connection to socket between client and server is established after the accept command
      datasock_fd = sends the result of grep to other machines
    */
    int server_ctrlsock_fd, datasock_fd, server_newctrlsock_fd;
    struct sockaddr_in serv_addr, ctrlcli_addr, datacli_addr;

    int client_ctrlsock_fd;
    struct sockaddr_in cli_addr, ctrlserv_addr, dataserv_addr;
    
    // Find the log filename
    string temp_str = to_string(MACHINE_NUM);
    char const* machine_num = temp_str.c_str();
    string filename = string("machine.") + machine_num + ".log";

    pid_t pid;
    if ((pid = fork()) < 0) {
        perror("Unable to fork a server\n");
        exit(0);
    }

    if (pid==0) {
        /*
            Write Client side of Machine, where a user can query
        */

        string cmnd;
        int i, j;

        ofstream file("machine0_client.txt");
       
       // Creating the control socket
        if((client_ctrlsock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("Socket creation failed\n");
            exit(0);
        }

        // Initializing the socket addresses to 0
        memset(&ctrlserv_addr, 0, sizeof(ctrlserv_addr));
        memset(&dataserv_addr, 0, sizeof(dataserv_addr));
        memset(&cli_addr, 0, sizeof(cli_addr));

        // Specifying the address of the control server at server
        ctrlserv_addr.sin_family = AF_INET;
        ctrlserv_addr.sin_addr.s_addr = INADDR_ANY;
        ctrlserv_addr.sin_port = htons(PORT_1); // 8001

        // Connecting to the control server
        while(1) {
            int connect_status = connect(client_ctrlsock_fd, (struct sockaddr *)&ctrlserv_addr, sizeof(ctrlserv_addr));
            if( connect_status == 0)
                break;
        }
    
        // Prompt for grep commands
        cout << "[MACHINE " << MACHINE_NUM << "] Terminal Starting... \n";
        while(1) {
            cout << "> ";
            getline(cin, cmnd);

            char mssg[MAX];
            remove_leading_spaces(cmnd, mssg);

            // Send grep command
            send(client_ctrlsock_fd, mssg, strlen(mssg)+1, 0);

            // Fork a child process to grep on its own log
            if(fork()==0) {
                fstream logfile;
                logfile.open(filename, ios::in);

                char *args[MAX];
                char *word;
                word = strtok (mssg," ");
				i = 0;
				while (word != NULL){
					args[i++] = word;
				    word = strtok (NULL, " ");
				}
				// Append the filename and NULL in the end
				args[i++] = (char*)filename.c_str();
                args[i++] = (char*)"-c";
                args[i] = NULL;

                if (logfile.is_open()) {
                    
                    char read_msg[MAX];
                    grep(read_msg, args);
                    
                    string init_msg = filename + ": " + read_msg;
                    file << "[1] Own: " << init_msg << endl;
                    cout << "Own: " << init_msg << endl;
                }

                logfile.close();
                exit(0);
            }
            else {
                // Receive input from other machines on the grep command
                char return_msg[MAX];
                recv(client_ctrlsock_fd, return_msg, MAX, 0);
                file << "[2] Received: " << return_msg << endl;
                cout << "Received: " << return_msg << endl;
            }
        }
        file.close();
    }


    else {
        /*
            Write Server side of the Machine, which searches the log and returns to the querier
        */

        char mssg[MAX];
        char cmnd[MAX];
        int i, j, flag, p, q;
        socklen_t clilen;
        fstream logfile;

        ofstream file("machine0_server.txt");

       // Creating control socket
        if((server_ctrlsock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // IPv4, TCP
            perror("Socket creation failed\n");
            exit(0);
        }

        // Initializing the socket address to 0
        memset(&serv_addr, 0, sizeof(serv_addr));
        memset(&ctrlcli_addr, 0, sizeof(ctrlcli_addr));
        memset(&datacli_addr, 0, sizeof(datacli_addr));

        // Specifying the control server address
        serv_addr.sin_family = AF_INET; // IPv4
        serv_addr.sin_addr.s_addr = INADDR_ANY; // any address
        serv_addr.sin_port = htons(MY_PORT);

        // Setting Socket options to reuse socket address
        int var = 1;
        if (setsockopt(server_ctrlsock_fd, SOL_SOCKET, SO_REUSEADDR, &var, sizeof(int)) < 0){
            perror("setsockopt(SO_REUSEADDR) failed");
            exit(0);
        }

        // Binding the control server
        if(bind(server_ctrlsock_fd,(const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
            perror("Binding error\n");
            exit(0);
        }

        // Machine Running and accepting connections
        listen(server_ctrlsock_fd, 10);
        cout << "Server Running...\n";


        // Machine is always running
        while(1) {

            clilen = sizeof(ctrlcli_addr);
            // Accept control connection from client
            server_newctrlsock_fd = accept(server_ctrlsock_fd, (struct sockaddr *)&ctrlcli_addr, &clilen);

            // LOOP for the control commands until 'quit'

            /*  This loop check for the command.
                Performs appropriate tasks like fork() and sends data over data socket
            */
            while(1) {

                recv(server_newctrlsock_fd, cmnd, MAX, 0);

                // TODO: If command is quit, then quit
                // or else fork a child process, read the log file and then search using the regex

                // Get the regex expression from the grep command
                /*
                char regex_str[MAX];
                i = 5; j = 0;
                while(cmnd[i] != '\0'){
                    regex_str[j++] = cmnd[i];
                    i++;
                }
                regex_str[j] = '\0';
                */


                char *args[MAX];
                char *word;
                word = strtok (cmnd," ");
				i = 0;
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
                    char read_msg[MAX];
                    grep(read_msg, args);

                    // if (byte_read_count > 0) {
                        // string send_msg = filename + "\t" + string(read_msg);
                        // send(server_newctrlsock_fd, send_msg.c_str(), strlen(send_msg.c_str())+1, 0);
                        // file << "Sent: " << send_msg << endl;
                    // }
                    // else {
                    //     char send_msg[MAX] = {'\0'};
                    //     send(server_newctrlsock_fd, send_msg, strlen(send_msg)+1, 0);
                    //     file << "Sent: " << send_msg << endl;
                    // }

                    string send_msg = filename + ": " + read_msg;
                    send(server_newctrlsock_fd, send_msg.c_str(), strlen(send_msg.c_str())+1, 0);
                    file << "Sent: " << send_msg << endl;
                    logfile.close();
                }
                
            }
            file.close();
        }
    }

    return 0;
}