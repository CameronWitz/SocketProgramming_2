/*
** client.cpp -- a stream socket client 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <unordered_map>

#include <arpa/inet.h>

#define indexMain 0
#define indexA 1
#define indexB 2
#define indexC 3

#define PORTA "30659"
#define PORTB "31659"
#define PORTC "32659"
#define MAINPORT "33659"

#define MAXDATASIZE 100 // max number of bytes we can get at once 

void askForDepts(int backendServer, int backendServerInd, std::unordered_map<std::string, int> &dept_to_server, struct addrinfo *ps[], int *sockfds){
    std::cout << "Querying " << backendServer << std::endl;
    int numbytes = sendto(backendServer, "*list", 5, 0, ps[backendServerInd]->ai_addr, ps[backendServerInd]->ai_addrlen);
    if(numbytes < 0){
        perror("list request send");
        exit(1);
    }
    char buf[MAXDATASIZE];
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    numbytes = recvfrom(sockfds[indexMain], buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&their_addr, &addr_len);
    if(numbytes < 0){
        perror("list request recv");
        return;
    }
    buf[numbytes] = '\0';
    std::string response(buf);
    std::cout << "Received " << response << std::endl;
    
    size_t beginning = 0;
    for(size_t i = 0; i < response.length(); i++){
        if(response[i] == ';'){
            std::string dept = response.substr(beginning, i - beginning);
            beginning = i + 1;
            dept_to_server[dept] = backendServerInd;
        }
    }

}


int main(int argc, char *argv[])
{
    int numbytes;
    int sockfds[4] = {0};
    const char* ports[4] = {MAINPORT, PORTA, PORTB, PORTC};

    char buf[MAXDATASIZE];
    struct addrinfo *ps[4];
    struct addrinfo hints, *servinfo, *p;
    int rv;

    for (int i = 0; i < 4; i++) {
        ps[i] = new struct addrinfo;
    }
   
    // setup the sockets
    //FIXME: change back to 4
    for(int i = 0; i < 4; i ++){
        
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET6; // ipv6
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        // store linked list of potential hosting ports in servinfo
        if ((rv = getaddrinfo("localhost", ports[i], &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop through all the results and bind to the first we can
        int mysockfd;
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((mysockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("server: socket");
                continue;
            }
            
            if(i == indexMain){
                if (bind(mysockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(mysockfd);
                    perror("server: bind");
                    continue;
                }
            }

            break;
        }

         // all done with this structure

        if (p == NULL)  {
            fprintf(stderr, "server: failed to bind\n");
            exit(1);
        }
        sockfds[i] = mysockfd;
        memcpy(ps[i], p, sizeof(struct addrinfo));
        freeaddrinfo(servinfo);
    }

    std::cout << "Main server is up and running" << std::endl;

    // query backend servers for departments
    std::unordered_map<std::string, int> dept_to_server;
    
    askForDepts(sockfds[indexA], indexA, dept_to_server, ps, sockfds);
    askForDepts(sockfds[indexB], indexB, dept_to_server, ps, sockfds);
    askForDepts(sockfds[indexC], indexC, dept_to_server, ps, sockfds);

    while(1){
        std::string dept_query;
        std::cout << "Enter Department Name: ";
        std::cin >> dept_query; // read in the query

        // find the server that this dept is associated with
        if(dept_to_server.find(dept_query) == dept_to_server.end()){
            std::cout << "Department does not appear in backend servers." << std::endl;
        }
        else{
            int server = dept_to_server[dept_query];
            struct sockaddr_storage their_addr;
            socklen_t addr_len = sizeof their_addr;
            numbytes = sendto(sockfds[server], dept_query.c_str(), 5, 0, ps[server]->ai_addr, ps[server]->ai_addrlen);
            if(numbytes < 0){
                perror("request send");
                return -1;
            }
            numbytes = recvfrom(sockfds[indexMain], buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&their_addr, &addr_len);
            if(numbytes < 0){
                perror("request recv");
                return -1;
            }
            buf[numbytes] = '\0';
            std::string response(buf);
            size_t beginning = 0;
            std::cout << "Received the following ids:" << std::endl;
            std::cout << response << std::endl;
            for(size_t i = 0; i < response.length(); i++){
                if(response[i] == ';'){
                    std::string id = response.substr(beginning, i - beginning);
                    beginning = i + 1;
                    std::cout << id << std::endl;
                }
            }
        }

    }
        

    return 0;
   
}