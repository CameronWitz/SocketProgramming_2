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
#define PORTB "30659"
#define PORTC "30659"
#define MAINPORT "33659"

#define MAXDATASIZE 100 // max number of bytes we can get at once 

void askForDepts(int backendServer, std::unordered_map<std::string, std::string> &dept_to_server, struct addrinfo *ps[], int *sockfds){
    std::cout << "Querying " << backendServer << std::endl;
    int numbytes = sendto(backendServer, "*list", 5, 0, ps[backendServer]->ai_addr, ps[backendServer]->ai_addrlen);
    if(numbytes < 0){
        perror("list request send");
        exit(1);
    }
    char buf[MAXDATASIZE];
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    int numbytes = recvfrom(sockfds[indexMain], buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&their_addr, &addr_len);
    if(numbytes < 0){
        perror("list request recv");
        return;
    }
    buf[numbytes] = '\0';
    std::string response(buf);
    std::cout << "Received " << response << std::endl;

}


int main(int argc, char *argv[])
{
    int numbytes;
    int* sockfds;
    const char* ports[4] = {MAINPORT, PORTA, PORTB, PORTC};

    char buf[MAXDATASIZE];
    struct addrinfo *ps[4] = {0, 0, 0, 0};
    struct addrinfo hints, *servinfo, *p;
    int rv;
   
    // setup the sockets
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
        sockfds[i] = mysockfd;
        ps[i] = p;
    }

    std::cout << "Main server is up and running" << std::endl;

    // query backend servers for departments
    std::unordered_map<std::string, std::string> dept_to_server;
    
    askForDepts(sockfds[indexA], dept_to_server, ps, sockfds);
    askForDepts(sockfds[indexB], dept_to_server, ps, sockfds);
    askForDepts(sockfds[indexC], dept_to_server, ps, sockfds);

    return 0;

    while(1){
        std::string dept_query;
        std::cout << "Enter Department Name: ";
        std::cin >> dept_query; // read in the query

        
        

    return 0;
   
}