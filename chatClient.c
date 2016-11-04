//
//  chatClient.c
//  
//
//  Created by Catherine Huang on 2016-11-02.
//
//

/*
 ** client.c -- a stream socket client demo
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

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to
#define MAX_INPUT_LEN 1024
#define MAXDATASIZE 100 // max number of bytes we can get at once

char my_port[MAX_INPUT_LEN];

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void matriculate(int sockfd, char* name_, char* port_number_){
    printf("👶 Matriculate \n");
    
    char msg[MAX_INPUT_LEN] = "0 ";
    char separator[2] = " ";
    
    char name[256];
    strcpy(name, name_);
    
    char port_number[256];
    strcpy(port_number, port_number_);
    
    strcat(name, separator);
    strcat(msg, name);
    strcat(msg, port_number);
    
    int len, bytes_sent;
    len = strlen(msg);

    bytes_sent = send(sockfd, msg, len, 0);
}

void broadcast(int sockfd, char* my_msg_, char* my_name_){
    char msg[MAX_INPUT_LEN] = "1 ";
    char separator[2] = " ";
    
    char my_name[256];
    strcpy(my_name, my_name_);
    
    char my_msg[256];
    strcpy(my_msg, my_msg_);
    
    strcat(my_name, separator);
    strcat(msg, my_name);
    strcat(msg, my_msg);
    
    int len, bytes_sent;
    len = strlen(msg);
    printf("📡 I want to broadcast: %s \n", msg);
    bytes_sent = send(sockfd, msg, len, 0);
}

void unicast(int sockfd, char* my_msg_, char* my_name_, char* your_name_){
    char msg[MAX_INPUT_LEN] = "2 ";
    char separator[2] = " ";
    
    char my_name[256];
    strcpy(my_name, my_name_);
    
    char your_name[256];
    strcpy(your_name, your_name_);
    
    char my_msg[256];
    strcpy(my_msg, my_msg_);
    
    strcat(my_name, separator);
    strcat(your_name, separator);
    strcat(msg, my_name);
    strcat(msg, your_name);
    strcat(msg, my_msg);
    
    int len, bytes_sent;
    len = strlen(msg);
    printf("📡 I want to say '%s' to %s \n", my_msg, your_name);
    bytes_sent = send(sockfd, msg, len, 0);
}

void request_client_list(){
    
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    
    char client_name[256];
    char client_port[256];
    
    strcpy(client_name, argv[1]);
    strcpy(client_port, argv[2]);
    
    if (argc != 3) {
        fprintf(stderr,"usage: client name port number\n");
        exit(1);
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char IP_addr[256] = "127.0.0.1";
    if ((rv = getaddrinfo(IP_addr, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        
        
        // int sockfd, char* name, char* port_number
        matriculate(sockfd, client_name, client_port);
        
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    //printf("client: connecting to %s\n", s);
    
    freeaddrinfo(servinfo); // all done with this structure
    for(;;){
        
        //broadcast(sockfd, "hey", client_name);
        char op_sel[MAX_INPUT_LEN];
        char operand[MAX_INPUT_LEN];
        scanf("%s %s", op_sel, operand);
        
        if (strcmp(op_sel, "broadcast") == 0){
            printf("client sends broadcast request... \n");
            broadcast(sockfd, operand, client_name);
        }
        else if (strcmp(op_sel, "list") == 0){
            
        }
        else if (strcmp(op_sel, "exit") == 0){
            
        }
        else{
            //unicast(int sockfd, char* my_msg_, char* my_name_, char* your_name_)
            printf("client sends unicast request... \n");
            unicast(sockfd, operand, client_name, op_sel);
        }
        
        char buf[MAXDATASIZE];
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }
        else{
            buf[numbytes] = '\0';
            printf("✨ %s\n",buf);
        }
        
    }
    
    close(sockfd);
    
    return 0;
}
