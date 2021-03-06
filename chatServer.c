//
//  chatServer.c
//  
//
//  Created by Catherine Huang on 2016-11-02.
//
//

/*
 ** server.c -- a stream socket server demo
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

#define PORT "3490"  // the port users will be connecting to
#define MAX_BUF_LEN 1024

#define BACKLOG 10     // how many pending connections queue will hold

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

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    
    Node *head = NULL;
    
    char names[3][256];
    strcpy(names[0], "hey");
    strcpy(names[1], "hi");
    strcpy(names[2], "hello");
    
    int i = 0;
    for (i = 0; i < 3; i++){
        Client *new_client = malloc(sizeof(Client));
        strcpy(new_client->port, "1234");
        strcpy(new_client->name, names[i]);
        
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
    
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
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
    
    printf("server: waiting for connections...\n");
    
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");
            
            matriculate(new_fd);
            broadcast(new_fd);
            
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }
    
    return 0;
}
