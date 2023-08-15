#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/uio.h>
#include "range.h"
#include "server.h"

int server_sockfd = 0, client_sockfd = 0;
int trackCLients = 0;
Clients *root, *current;

void forceExit(int sig) {
    Clients *temp;
    
    while (root != NULL) {
        printf("\nClosing socketfd: %d\n", root->data);
        close(root->data); 
        temp = root;
        root = root->link;
        free(temp);
    }
    
    printf("Server: Goodbye! \n");
    exit(EXIT_SUCCESS);
}

void send_toClient(Clients *np, char tmp_buffer[], char target[]) {
    Clients *temp = root->link;
    char sendline[LENGTH_SEND];
    snprintf(sendline, sizeof(sendline), "%s: %s", np->petName, tmp_buffer);
   
    while (temp != NULL) {
        if (np->data != temp->data) { 
            if (strcmp(temp->petName, target) == 0) {
                printf("Send to sockfd %d: \"%s\" \n", temp->data, sendline);
                send(temp->data, sendline, LENGTH_SEND, 0);
            }
        }
        temp = temp->link;
    }
}

void send_allClients(Clients *np, char tmp_buffer[]) {
    Clients *temp = root->link;
    
    while (temp != NULL) {
        if (np->data != temp->data) {
            printf("\nSend to sockfd %d: \"%s\" \n", temp->data, tmp_buffer);
            send(temp->data, tmp_buffer, LENGTH_SEND, 0);    
        }
        temp = temp->link;
    }
}

void whois_client(Clients *sender, char target[]) {
    Clients *current = root->link;
    char sendline[LENGTH_SEND];
    int clientFound = 0;
    
    while (current != NULL) {
        if (sender->data != current->data) { 
            if (strcmp(current->petName, target) == 0) {
                clientFound = 1;
                printf("\nSend to sockfd %d: \"%s\" \n", current->data, sendline);
                snprintf(sendline, sizeof(sendline), "Nickname: %s\nRealname: %s\nAddress: %s", current->petName, current->name, current->ip);
                send(sender->data, sendline, LENGTH_SEND, 0);
                break;
            }
        }
        current = current->link;
    }

    if (!clientFound) {
        snprintf(sendline, sizeof(sendline), "Server: Client does not exist.");
        send(sender->data, sendline, LENGTH_SEND, 0);
    }
}

char** splitWords(char str[]){
    int i = 0;
    char string[LENGTH_MSG];
    strcpy(string,str);
    char ** sub_str = malloc(10 * sizeof(char*));
    char * token = strtok(string, " ");

    while( token != NULL ) {
        sub_str[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    return sub_str;
}
    
int countString(char s[])
{
    int count = 0, i;
    for (i = 0;s[i] != '\0';i++)
    {
        if (s[i] == ' ' && s[i+1] != ' ')
        count++;    
    }
    return count+1;
}

void timeCheck(Clients *sender, char buff[]){
    time_t ticks = time(NULL);
    snprintf(buff, LENGTH_SEND, "Server: %.24s\r", ctime(&ticks));
    send(sender->data, buff, LENGTH_SEND, 0);
}

void client_handler(void *clientPointer) {
    int flagCheck = 0;
    char nickname[LENGTH_NAME] = {};
    char incomingBuffer[LENGTH_MSG] = {};
    char outgoingBuffer[LENGTH_SEND] = {};
    Clients *np = (Clients *)clientPointer;

    /* Naming convention */
    if (recv(np->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("%s Name not entered.\n", np->ip);
        flagCheck = 1;
    } 
    else {
        char** joinName = splitWords(nickname);
        if (strcasecmp(joinName[0], "join") == 0)
        {
            strncpy(np->petName, joinName[1], LENGTH_NAME);
            strncpy(np->name, joinName[2], LENGTH_NAME);
            printf("Real-name: %s, Nickname: %s, Address: %s.\n", np->name, np->petName, np->ip);
            sprintf(outgoingBuffer, "%s(%s) joined the chatroom.", np->name, np->ip);
            send_allClients(np, outgoingBuffer);
        }
        else {
            printf("Join unsuccessful.\n");
        }
        free(joinName);
    }

    /* Initialising of conversation */
    while (1) {
        if (flagCheck) {
            break;
        }
        int receive = recv(np->data, incomingBuffer, LENGTH_MSG, 0);
        char** cWords = splitWords(incomingBuffer);
        
        if (strcasecmp(cWords[0], "whoIS") == 0) {
            printf("--- whois called ---\n");
            whois_client(np, cWords[1]);
        }

        else if (strcasecmp(cWords[0], "Msg") == 0){
            send_toClient(np, incomingBuffer, cWords[1]);
            printf("\n--- Message sent ---\n");
        }

        else if (strcasecmp(cWords[0], "Join") == 0){
            printf("\n--- Join created ---\n");
        }

        else if (strcasecmp(cWords[0], "Time") == 0){
            printf("\n--- Time of server displayed ---\n");
            timeCheck(np, outgoingBuffer);
        }

        else if (strcasecmp(cWords[0], "Alive") == 0){
            printf("\n--- Server made alive ---\n");
        }

        else if (strcasecmp(cWords[0], "Quit") == 0){
            printf("\n--- Quit called ---\n");
            printf("%s - %s left the chatroom.\n", np->name, np->ip);
            sprintf(outgoingBuffer, "%s(%s) left the chatroom.", np->name, np->ip);
            --trackCLients;
            flagCheck = 1;
        }

        if (receive > 0) {
            if (strlen(incomingBuffer) == 0) {
                continue;
            }
            sprintf(outgoingBuffer, "%s %s from %s", np->name, incomingBuffer, np->ip);
        }
        
        else {
            printf("Fatal Error: -1\n");
            flagCheck = 1;
        }
        free(cWords);
    }
    
    /* Removal of nodes */
    close(np->data);
    if (np == current) { 
        current = np->prev;
        current->link = NULL;
    } else {
        np->prev->link = np->link;
        np->link->prev = np->prev;
    }
    free(np);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage is as follows: ./Server <Max-connections>\n");
        exit(-1);
    }

    int maxConnections = atoi(argv[1]);

    signal(SIGINT, forceExit);

    /* Socket creation (server) */
    server_sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sockfd == -1) {
        printf("Failed to create a socket.");
        exit(EXIT_FAILURE);
    }

    /* Details of socket */
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(8888);

    /* Binding and listening */
    bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen);
    listen(server_sockfd, 5);

    /* Displaying server IP */
    getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf(" --- Welcome to Client-Server program! ---\nStarting the server on Address:%s , Port:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    /* Linked list of clients */
    root = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));
    current = root;

    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
        trackCLients++;
        if (trackCLients > maxConnections)  
        {
            printf("The connections created have exceeded the limit entered.\n");
            send(client_sockfd, "Error: The server is already full.\n", LENGTH_MSG, 0);
            close(client_sockfd);
        }
        else {
            /* Displaying client IP */
            getpeername(client_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
            printf("\nClient %s:%d has joined in chatroom.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

            /* Adding and updating clients in linked list. */
            Clients *c = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
            c->prev = current;
            current->link = c;
            current = c;

            pthread_t id;
            if (pthread_create(&id, NULL, (void *)client_handler, (void *)c) != 0) {
                perror("Create pthread error!\n");
                --trackCLients;
                exit(EXIT_FAILURE);
            }
        }
    }

    return 0;
}
