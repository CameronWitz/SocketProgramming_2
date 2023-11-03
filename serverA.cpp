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

// Reads the data from data#.txt where # is A, B or C
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
                if(ids_set.find(cur_id) == ids_set.end() || ids_set.empty()) // making sure they are unique
                    ids_set.insert(cur_id);
                beginning = i + 1;
                
            }
        }
        std::string last_id = student_ids.substr(beginning, student_ids.length()-beginning);
        if(ids_set.find(last_id) == ids_set.end() || ids_set.empty()) // making sure they are unique
            ids_set.insert(last_id);
        
        dept_to_ids[department_name] = ids_set;
    }

}

int main(void)
{
    int mysockfd, serversockfd;
    std::string myServer = "A";
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXDATASIZE];

    // First set up for listening on our port
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // ipv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    

    std::unordered_map<std::string, std::set<std::string>> dept_to_ids;

    readData(dept_to_ids, "data" + myServer + ".txt"); //FIXME: Change this for the different servers

    // store linked list of potential hosting ports in servinfo
    if ((rv = getaddrinfo("localhost", MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((mysockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(mysockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(mysockfd);
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

    // now setup for sending to their port..
    // clear this out again
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // ipv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    // store linked list of potential hosting ports in servinfo
    if ((rv = getaddrinfo("localhost", MAINPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((serversockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        // don't need to bind to servermain socket
        break;
    }

    // TODO: see if commenting in is going to break
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }


    // SETUP IS DONE
    std::cout << "Server " << myServer << "is up and running using UDP on port " << MYPORT << std::endl;
  
    while(1) {  // respond to requests
        socklen_t addr_len = sizeof their_addr;
        numbytes = recvfrom(mysockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *) &their_addr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom");
            continue;
        }
        buf[numbytes] = '\0';
        std::string request(buf);

        std::cout << "Server " + myServer + " has received a request for " << request << std::endl;
        
        std::string response = "";
        // special initial request
        if(request == "*list"){
            // send all the departments for which we are available:
            for (const auto & it : dept_to_ids) {
                response += it.first + ";";
            }
            std::cout << "Server " + myServer + " has sent a department list to Main Server" << std::endl;
        }

        // get the actual data for the associated request
        else{
            int found = 0;
            if(dept_to_ids.find(request) != dept_to_ids.end())
                found = 1;
            
            if(found){
                std::cout << "Server " + myServer + " found " << dept_to_ids[request].size() << "distinct students for " << request << ": ";
                int first = 1;
                for(auto &elem : dept_to_ids[request]){
                    std::cout << first ? elem : ", " + elem;
                    response += elem + ";";
                    first = 0;
                }
            }
            else{
                response = "Not Found.";
            }
        }
        // SEND RESPONSE
        numbytes = sendto(serversockfd, response.c_str(), response.length(), 0, p->ai_addr, p->ai_addrlen);
        if(numbytes < 0){
            perror("sendto");
            exit(1);
        }
        std::cout << "Server " << myServer << "has sent the results to Main Server" << std::endl;

    }

    return 0;
}