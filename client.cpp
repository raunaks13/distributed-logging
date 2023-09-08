#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <chrono>


using namespace std;
using namespace std::chrono;


#define MAX 100			// Max length of commands
#define BASE_PORT 8000



char* get_ip_from_domain(string domain) {
	struct hostent *ip_addr;
	struct in_addr **addr;

    char domain_name[domain.length()+1];
    strcpy(domain_name, domain.c_str());

	// DNS query for IP address of the domain
    cout << domain_name << endl;
	ip_addr = gethostbyname(domain_name);
	if(ip_addr == NULL){
		printf("Domain Name is wrong");
		exit(0);
	}
	addr = (struct in_addr **)ip_addr->h_addr_list;
    
	return inet_ntoa(*addr[0]);
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


    char *MACHINE_NUM = argv[1];
    int PORT = BASE_PORT + stoi(MACHINE_NUM);


    // Prompt for grep commands
    cout << "[MACHINE " << MACHINE_NUM << "] Client Terminal Starting... \n";

    while(1) {
        string cmnd;

        cout << "> ";
        getline(cin, cmnd);

        char mssg[MAX];
        char mssg_for_curr_machine[MAX];

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
                cout << "Sending: " << mssg << endl;
                int sz = send(client_ctrlsock_fd, mssg, strlen(mssg)+1, 0);
                if (sz < 1 && sizeof(mssg) < 1)
                    throw new exception;

                // Receive input from other machines on the grep command
                cout << "Retrieving log data from machine " << k << endl;
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
    }


    return 0;
}