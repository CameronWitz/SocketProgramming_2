/*
** server.cpp -- a stream socket server 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <set>

#define MYPORT "30659"  // the port users will be connecting to
#define MAINPORT "33659"

#define BACKLOG 10  // how many pending connections queue will hold
#define MAXDATASIZE 1024 // max request size (Department name), unlikely to be larger than this

// This function takes in two unordered_maps and populates them with the relevant data from list.tx
// at the end of this function, dept_to_server is a hashmap which given a department, returns it's associated server.
// server_to_dept alternatively takes in a server and returns a vector of strings where each element of the vector is a distinct department that server is associated with. 
void readData(std::unordered_map<std::string, std::set<std::string>> &dept_to_ids, std::string filename){
    std::ifstream infile;
    infile.open(filename);
    std::string department_name;
    std::string student_ids;
    size_t beginning;
    while(infile >> department_name){
        infile >> student_ids;
        beginning = 0;
        std::set<std::string> ids_set;
        for(size_t i = 0; i < student_ids.length(); i++){
            char cur = student_ids[i];
            if(cur == ';'){
                std::string cur_id = student_ids.substr(beginning, i-beginning);
                if(ids_set.find(cur_id) != ids_set.end()) // making sure they are unique
                    ids_set.insert(cur_id);
                beginning = i + 1;
                
            }
        }
        std::string last_id = student_ids.substr(beginning, student_ids.length()-beginning);
        if(ids_set.find(last_id) != ids_set.end()) // making sure they are unique
            ids_set.insert(last_id);
                
    }

}

// invoked due to sigaction
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;//, my_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int clients = 0;
    int yes=1;
    // char s[INET6_ADDRSTRLEN];
    int rv;
    std::unordered_map<std::string, std::set<std::string>> dept_to_ids;

    std::cout << "Main server is up and running." << std::endl;
    // read in the data#.txt file into the unordered_map
    readData(dept_to_ids, "dataA.txt");

    // Specify the type of connection we want to host
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; //IPV4 and IPV6 both fine
    hints.ai_socktype = SOCK_DGRAM; // UDP
    hints.ai_flags = AI_PASSIVE; // use my IP

    // store linked list of potential hosting ports in servinfo
    if ((rv = getaddrinfo("localhost", MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        clients ++;
              
        if (!fork()) { // this is the child process
            int cur_client = clients;
            close(sockfd); // child doesn't need the listener

            while(1){
                // Respond to any queries from the client
                int numbytes;
                char buf[MAXDATASIZE];

                if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }

                if(numbytes == 0){
                    // std::cout << "Client closed connection, this process will exit" << std::endl;
                    close(new_fd);
                    exit(0);
                }

                buf[numbytes] = '\0';
                std::string request(buf);

                std::cout << "Main server has received the request on Department " << request;
                std::cout << " from client " << cur_client << " using TCP over port " << PORT << std::endl;

                std::string reply;

                if(dept_to_server.find(request) == dept_to_server.end()){
                    reply = "Not Found"; 
                    std::cout << "Department " << request << " does not show up in backend server ";
                    int firsttime = 1;
                    for(auto &entry : server_to_dept){
                        if(!firsttime){
                            std::cout << ", ";
                        }
                        firsttime = 0;
                        std::cout << entry.first;
                    }
                    std::cout << std::endl;
                }
                else{
                    reply = dept_to_server[request]; 
                    std::cout << request << " shows up in backend server " << reply << std::endl;
                }

                if (send(new_fd, reply.c_str(), reply.length(), 0) == -1){
                    perror("send");
                }
                if(reply == "Not Found"){
                    std::cout << "The Main Server has sent “Department Name: Not found” to client " ;
                    std::cout << cur_client << " using TCP over port " << PORT << std::endl;
                }
                else{
                    std::cout << "Main Server has sent searching result to client " << cur_client;
                    std::cout << " using TCP over port " << PORT << std::endl;
                }

            }
        }

        close(new_fd);  // parent doesn't need this
    }

    return 0;
}