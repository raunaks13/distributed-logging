#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <chrono>
#include <bits/stdc++.h>


using namespace std;
using namespace std::chrono;


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
    cout << domain_name << endl;
	ip = gethostbyname(domain_name);
	if(ip == NULL){
		printf("[Error] Incorrect Domain Name");
		exit(0);
	}
	adr = (struct in_addr **)ip->h_addr_list;
    
    // cout << domain << inet_ntoa(*adr[0]) << endl;
	return inet_ntoa(*adr[0]);
}




int main(int argc, char *argv[]) {

    vector<string> domains = { "fa23-cs425-3701.cs.illinois.edu", 
                                "fa23-cs425-3702.cs.illinois.edu",
                                "fa23-cs425-3703.cs.illinois.edu", 
                                "fa23-cs425-3704.cs.illinois.edu",
                                // "fa23-cs425-3705.cs.illinois.edu",
                                // "fa23-cs425-3706.cs.illinois.edu" ,
                                // "fa23-cs425-3707.cs.illinois.edu" ,
                                // "fa23-cs425-3708.cs.illinois.edu" ,
                                // "fa23-cs425-3709.cs.illinois.edu" ,
                                // "fa23-cs425-3710.cs.illinois.edu" ,
                             };


    // char *MACHINE_NUM = argv[1];
    // int PORT = BASE_PORT + stoi(MACHINE_NUM);


    // Find the log filename
    // string filename = string("MP1 Demo Data FA22/vm") + MACHINE_NUM + ".log";



    string query = "Mozilla";

    string cmnd = "grep -c " + query;
    char mssg[MAX];
    strcpy(mssg, cmnd.c_str());

    int total_matches = 0;

    auto start = high_resolution_clock::now();

    for (int k = 1; k <= domains.size(); k++) {

        char return_msg[MAX];
        try
        {
            int client_ctrlsock_fd;
            struct sockaddr_in cli_addr, ctrlserv_addr;

            
            if((client_ctrlsock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Socket creation failed\n");
            }

            memset(&ctrlserv_addr, 0, sizeof(ctrlserv_addr));
            memset(&cli_addr, 0, sizeof(cli_addr));

            int PORT = BASE_PORT + k;

            // Specifying the address of the control server at server
            ctrlserv_addr.sin_family = AF_INET;
            ctrlserv_addr.sin_addr.s_addr = INADDR_ANY;
            // ctrlserv_addr.sin_addr.s_addr = inet_addr(get_ip_from_domain(domains[k-1]));
            ctrlserv_addr.sin_port = htons(PORT);

            // Connecting to the control server
            int connect_status = connect(client_ctrlsock_fd, (struct sockaddr *)&ctrlserv_addr, sizeof(ctrlserv_addr));
            if (connect_status < 0)
                throw new exception;

            
            // Send grep command
            int sz = send(client_ctrlsock_fd, mssg, strlen(mssg)+1, 0);
            if (sz < 1 && sizeof(mssg) < 1)
                throw new exception;

            // Receive input from other machines on the grep command
            int sz2 = recv(client_ctrlsock_fd, return_msg, MAX, 0);

            total_matches += stoi(return_msg);
            
            string print_msg = "VM" + to_string(k) + ": " + return_msg;
            cout << print_msg;

            close(client_ctrlsock_fd);
        }
        catch (...) {
        }

    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    cout << "Total Matches: " << total_matches << endl;
    cout << "Time Taken: " << duration.count() << " microseconds" << endl << endl;

    
    // string ground_truth_grep_mssg = "grep " + string(query) + " MP1\\ Demo\\ Data\\ FA22/* -c | awk -F: '{ g=g+$2 } END { print g }'";
    // remove_leading_spaces(ground_truth_grep_mssg, mssg);
    
    // char *args[MAX];
    // char *word;
    // word = strtok (mssg," ");
    // int i = 0;
    // while (word != NULL){
    //     args[i++] = word;
    //     word = strtok (NULL, " ");
    // }
    // // Append NULL in the end
    // args[i] = NULL;

    char *query_msg;
    strcpy(query_msg, query.c_str());
    

    char *args[] = {"grep", query_msg, "MP1\\ Demo\\ Data\\ FA22/*", "-c" , "|", "awk", "-F:", "{ g=g+$2 } ", "END", " { print g }"};


    char *gt_grep_mssg;
    grep(gt_grep_mssg, args);
    cout << gt_grep_mssg << endl;
    int gt_total_match = stoi(gt_grep_mssg);

    if (gt_total_match == total_matches)
        cout << "UNIT TEST PASSED :) !!" << endl << endl;
    else
        cout << "UNIT TEST FAILED :( !!" << endl << endl;


    return 0;
}