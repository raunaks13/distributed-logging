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
#include <vector>

using namespace std;

using namespace std;

#define MAX 100			// Max length of commands
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

    // Adding null character to the end
    int last_index = byte_read_count/sizeof(read_msg[0]);
    read_msg[last_index] = '\0';
}


char* get_ip_from_domain(string domain) {
	struct hostent *ip;
	struct in_addr **adr;

    char domain_name[domain.length()+1];
    strcpy(domain_name, domain.c_str());

	// DNS query for IP address of the domain
	ip = gethostbyname(domain_name);
	if(ip == NULL){
		printf("[Error] Incorrect Domain Name");
		exit(0);
	}
	adr = (struct in_addr **)ip->h_addr_list;
    
    // cout << domain << inet_ntoa(*adr[0]) << endl;
	return inet_ntoa(*adr[0]);
}


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



int main(int argc, char *argv[]) {

    vector<string> domains = { "fa23-cs425-3701.cs.illinois.edu", 
                                "fa23-cs425-3702.cs.illinois.edu" 
                             };


    char *MACHINE_NUM = argv[1];
    int PORT = BASE_PORT + stoi(MACHINE_NUM);

    // Find the log filename
    string filename = string("MP1 Demo Data FA22/vm") + MACHINE_NUM + ".log";


    // Prompt for grep commands
    cout << "[MACHINE " << MACHINE_NUM << "] Client Terminal Starting... \n";
    while(1) {
        string cmnd;

        cout << "> ";
        getline(cin, cmnd);

        char mssg[MAX];
        char mssg_for_curr_machine[MAX];

        remove_leading_spaces(cmnd, mssg);
        strcpy(mssg_for_curr_machine, mssg);

        int total_matches = 0;

        for(int k = 1; k <= domains.size(); k++) {

            if( k == stoi(MACHINE_NUM) ) {
                cout << "Retrieving log data from current machine." << endl;
                fstream logfile;
                logfile.open(filename, ios::in);

                char *args[MAX];
                char *word;
                word = strtok (mssg_for_curr_machine, " ");
                int i = 0;
                while (word != NULL){
                    args[i++] = word;
                    word = strtok (NULL, " ");
                }
                // Append the filename and NULL in the end
                args[i++] = (char*)filename.c_str();
                args[i++] = (char*)"-c";
                args[i] = NULL;

                // If logfile can be opened
                if (logfile.is_open()) {
                    
                    char read_msg[MAX];
                    grep(read_msg, args);

                    total_matches += stoi(read_msg);
                    
                    string print_msg = "VM" + string(MACHINE_NUM) + ": " + read_msg;
                    cout << print_msg;
                }

                logfile.close();
            }

            else {

                int client_ctrlsock_fd;
                struct sockaddr_in cli_addr, ctrlserv_addr;

                if((client_ctrlsock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    perror("Socket creation failed\n");
                    continue;
                }

                memset(&ctrlserv_addr, 0, sizeof(ctrlserv_addr));
                memset(&cli_addr, 0, sizeof(cli_addr));

                int PORT = BASE_PORT + k;

                // Specifying the address of the control server at server
                ctrlserv_addr.sin_family = AF_INET;
                ctrlserv_addr.sin_addr.s_addr = INADDR_ANY;
                // ctrlserv_addr.sin_addr.s_addr = inet_addr(get_ip_from_domain(domains[k]));
                ctrlserv_addr.sin_port = htons(PORT);

                // Connecting to the control server
                while(1) {
                    int connect_status = connect(client_ctrlsock_fd, (struct sockaddr *)&ctrlserv_addr, sizeof(ctrlserv_addr));
                    if( connect_status == 0)
                        break;
                }

                // Send grep command
                cout << "Sending: " << mssg << endl;
                int sz = send(client_ctrlsock_fd, mssg, strlen(mssg)+1, 0);

                // Receive input from other machines on the grep command
                cout << "Retrieving log data from machine " << k << endl;
                char return_msg[MAX];
                int sz2 = recv(client_ctrlsock_fd, return_msg, MAX, 0);

                total_matches += stoi(return_msg);
                
                string print_msg = "VM" + to_string(k) + ": " + return_msg;
                cout << print_msg;

                close(client_ctrlsock_fd);
            }
        }

        cout << "Total Matches: " << total_matches << endl << endl;
    }


    return 0;
}