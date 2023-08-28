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
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

using namespace std;

#define str(x) #x
#define strname(name) str(name)

#define MACHINE_NUM 1
#define MAX 80			// Max length of the control commands
#define PACKET 20		// Packet length for data transfer
#define PORT 5000 + MACHINE_NUM	   // Port of TCP Control Server
#define PORT_0 5000 

int main() {
    int ctrlsock_fd, datasock_fd, newdatasock_fd;
	struct sockaddr_in cli_addr, ctrlserv_addr, dataserv_addr;

    string cmnd;
    char mssg[MAX];
    int i, j;

    // Creating the control socket
	if((ctrlsock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
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
	ctrlserv_addr.sin_port = htons(PORT_0);
	
    
	// Connecting to the control server
	if(connect(ctrlsock_fd, (struct sockaddr *)&ctrlserv_addr, sizeof(ctrlserv_addr)) < 0){
		perror("Unable to connect to server\n");
		exit(0);
	}
    

    // Send the port at which the machine will operate control channel
    string temp_str=to_string(PORT);
    char const* port= temp_str.c_str();
    send(ctrlsock_fd, port, strlen(port)+1, 0);    


    // Prompt for grep commands
    cout << "[MACHINE " << MACHINE_NUM << "] Terminal Starting... \n";
    while(1) {
        cout << "> ";
        getline(cin, cmnd);

        // Handling the leading spaces
        i=0;
        while(cmnd[i]==' ' || cmnd[i]=='\t') i++;
        j=0;
        while(cmnd[i] != '\0'){
            mssg[j++] = cmnd[i];
            i++;
        }
        mssg[j] = '\0';

        send(ctrlsock_fd, mssg, strlen(mssg)+1, 0);
    
    }

    return 0;
}