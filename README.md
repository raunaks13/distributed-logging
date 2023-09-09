# CS-425 MP1

### Setup (run on each VM):

1. `git clone https://gitlab.engr.illinois.edu/sc134/cs-425-mp1.git`
2. `cd cs-425-mp1`
3. Download log files and compile `server.cpp` and `client.cpp`: `bash setup_vm.sh`
4. Start the server: `./server.out i`, where i is the machine number we are starting

Now, start the client on any machine of your choice, say machine k: `./client.out k`
You will see a terminal prompt `>` where you can enter any grep commands. Enter commands in the format `grep -c XYZ`, **without** any quotes around the desired expression.

### For Unit Testing:
1. Repeat steps 1-3 above.
2. Copy log files to testing directory: `cp -r "MP1 Demo Data FA22" "../test/MP1 Demo Data FA22"`
3. `cd test`
4. `g++ -o client_test.out client_test.cpp`
4. `./client_test.out grep -c Mozilla`, where you can replace the grep expression by any value of your choice. The program will print out the ground truth grep output as well as the one generated by our distributed program.
```