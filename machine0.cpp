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
#include <signal.h>
#include <fstream>
#include <regex>

using namespace std;

#define MACHINE_NUM 0
#define MAX 80			// Max length of commands
#define PACKET 50		// Packet length to send data
#define PORT 5000 + MACHINE_NUM	    // Port of TCP Control Server
#define PORT_1 5001


int main() {
    /*
      ctrlsock_fd = listens to other machines for command
      newctrlsock_fd = connection to socket between client and server is established after the accept command
      datasock_fd = sends the result of grep to other machines
    */
    int ctrlsock_fd, datasock_fd, newctrlsock_fd;
    struct sockaddr_in serv_addr, ctrlcli_addr, datacli_addr;

    char mssg[MAX];
    char cmnd[MAX];
    int i, j, flag, Y;
    socklen_t clilen;
    fstream logfile;

    // Creating control socket
	if((ctrlsock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Socket creation failed\n");
		exit(0);
	}

    // Initializing the socket address to 0
	memset(&serv_addr, 0, sizeof(serv_addr));
	memset(&ctrlcli_addr, 0, sizeof(ctrlcli_addr));
	memset(&datacli_addr, 0, sizeof(datacli_addr));

    // Specifying the control server address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

    // Setting Socket options to reuse socket address
    int var = 1;
    if (setsockopt(ctrlsock_fd, SOL_SOCKET, SO_REUSEADDR, &var, sizeof(int)) < 0){
		perror("setsockopt(SO_REUSEADDR) failed");
		exit(0);
	}

	// Binding the control server
	if(bind(ctrlsock_fd,(const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		perror("Binding error\n");
		exit(0);
	}

    // Machine Running and accepting connections
    listen(ctrlsock_fd, 10);
	cout << "Server Running...\n";


    // Machine is always running
    while(1) {

        
		// Loop for the first command which must be of 'port'

		/* First Control Command must be of the port at which the client will open a TCP server.
		   Loop until we get the first command as port
		*/
		while(1){
			clilen = sizeof(ctrlcli_addr);
			// Accept control connection form client
			newctrlsock_fd = accept(ctrlsock_fd, (struct sockaddr *)&ctrlcli_addr, &clilen);

			// Receive First Control command which is the PORT address of other machines
			recv(newctrlsock_fd, cmnd, MAX, 0); 

			Y = 0;
            /* Iterate through all the spaces.
                Then get the port number and store it in a variable Y.
                The command must not have any other characters except spaces after the port number, else flag an error.
            */
            i = 0;
            while(cmnd[i] != ' ' && cmnd[i] != '\0'){
                Y = 10*Y + (int)(cmnd[i] - '0');
                i++;
            }
            if(Y>1024 && Y<65535){
                break;
            }
            else {
                close(newctrlsock_fd);
                continue;
            }
        }

        

        // LOOP for the rest of the control commands until 'quit'

		/*  This loop check for the command.
		    Performs appropriate tasks like fork() and sends data over data socket
		*/
        while(1) {
            char regex_str[MAX];

            recv(newctrlsock_fd, cmnd, 80, 0);

            // TODO: If command is quit, then quit
            // or else fork a child process, read the log file and then search using the regex


            // Get the regex expression from the grep command
            i = 5; j = 0;
            while(cmnd[i] != '\0'){
                regex_str[j++] = cmnd[i];
                i++;
            }
            regex_str[j] = '\0';

            // Open the log file
            string temp_str = to_string(MACHINE_NUM);
            char const* machine_num = temp_str.c_str();
            cout << machine_num << "\n";
            string filename = string("machine.") + machine_num + ".log";
            logfile.open(filename, ios::in);


            // If file is open, read each line of the file and search for regex
            if (logfile.is_open()) {
                string line;
                int k = 0;
                while(getline(logfile, line))
                {
                    regex expr (regex_str); // the pattern
                    bool match = regex_search (line, expr);
                    cout << match << "\n";
                    k++;

                    if(k==5) break;
                }
            }
        }
    }

    return 0;
}