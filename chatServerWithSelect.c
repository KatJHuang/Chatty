//
//  chatServerWithSelect.c
//  
//
//  Created by Catherine Huang on 2016-11-03.
//
//

/*
 ** selectserver.c -- a cheezy multiperson chat server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "3490"   // port we're listening on
#define MAX_BUF_LEN 1024

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//======== DATA STRUCTURE TO KEEP TRACK OF CONNECTED CLIENTS =========
typedef struct Client{
    char port[256];
    char name[256];
}Client;

typedef struct Client_list_node{
    Client client;
    struct Client_list_node* next;
}Node;

void print_node(Node *node){
    printf("port #: %s\t", node->client.port);
    printf("name: %s\n", node->client.name);
}

void print_all_nodes(Node *node){
    while (node != NULL){
        print_node(node);
        node = node->next;
    }
}

void push(Node **head, Client client){
    Node *client_node = malloc(sizeof(Node));
    client_node->client = client;
    client_node->next = *head;
    *head = client_node;
}

Node* search(Node *head, char* name){
    Node *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->client.name, name) == 0)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

Node* delete(Node **head, char* name){
    Node *prev = NULL;
    Node *curr = *head;
    while (curr != NULL) {
        if (strcmp(curr->client.name, name) == 0){
            if (curr == *head){
                *head = (*head)->next;
                printf("new head\n");
                print_node(*head);
                printf("yup\n");
            }
            else{
                prev->next = curr->next;
            }
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }
    return NULL;
}
//======== END OF DATA STRUCTURE TO KEEP TRACK OF CONNECTED CLIENTS =========

//======== SUPPORTED FUNCTIONS OF SERVER =========

void matriculate(int sockfd){
    printf("inside matriculate - server\n");
    char* buf;
    int length = recv(sockfd, buf, MAX_BUF_LEN, 0);
    printf("string: %s length = %d\n", buf, length);
}

void broadcast(int sockfd){
    printf("inside broadcast - server\n");
    char* buf;
    
    int length = recv(sockfd, buf, MAX_BUF_LEN, 0);
    printf("broadcast: %s length = %d\n", buf, length);
}
//======== END OF SUPPORTED FUNCTIONS OF SERVER =========

int main(void)
{
    
    // ======== TESTING LINKED LIST ========
    Node *head = NULL;
    
    char names[3][256];
    strcpy(names[0], "hey");
    strcpy(names[1], "hi");
    strcpy(names[2], "hello");
    
    int counter = 0;
    for (counter = 0; counter < 3; counter++){
        Client *new_client = malloc(sizeof(Client));
        strcpy(new_client->port, "1234");
        strcpy(new_client->name, names[counter]);
        
        push(&head, *new_client);
    }
    print_all_nodes(head);
    
    printf("searching a node\n");
    char *name = "hi";
    Node* search_result = search(head, name);
    if (search_result != NULL)
        print_node(search_result);
    else
        printf("no node with name %s is found\n", name);
    
    printf("delete a node\n");
    Node *deleted = delete(&head, "hello");
    print_all_nodes(head);
    // ======== DONE TESTING LINKED LIST ========
    
    
    
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    
    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    
    char buf[256];    // buffer for client data
    int nbytes;
    
    char remoteIP[INET6_ADDRSTRLEN];
    
    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;
    
    struct addrinfo hints, *ai, *p;
    
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        
        break;
    }
    
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    
    freeaddrinfo(ai); // all done with this
    
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    
    // add the listener to the master set
    FD_SET(listener, &master);
    
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    
    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                                   (struct sockaddr *)&remoteaddr,
                                   &addrlen);
                    
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                               "socket %d\n",
                               inet_ntop(remoteaddr.ss_family,
                                         get_in_addr((struct sockaddr*)&remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // we got some data from a client
                        for(j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}
