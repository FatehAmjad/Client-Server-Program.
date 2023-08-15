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
#include "range.h"
#include "string.h"

volatile sig_atomic_t flag = 0;
int sockfd = 0;     
char nickname[LENGTH_NAME] = {};

void forceExit(int sig) {
    flag = 1;
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

void recvHandler() {
    char receiveMessage[LENGTH_SEND] = {};
    
    while (1) {
        int receive = recv(sockfd, receiveMessage, LENGTH_SEND, 0);
        if (receive > 0) {
            printf("\r%s\n", receiveMessage);
            overwriteString();
            char** storeMsg = splitWords(receiveMessage);           
            if (strcmp(storeMsg[0], "Error:") == 0)
            {
                forceExit(2);
            }
            free(storeMsg);
        } else if (receive == 0) {
            break;
        } else { 
            // -1 
        }
    }
}

void sendHandler() {
    char message[LENGTH_MSG] = {};
    
    while (1) {
        overwriteString();
        while (fgets(message, LENGTH_MSG, stdin) != NULL) {
            cutString(message, LENGTH_MSG);
            if (strlen(message) == 0) {
                overwriteString();
            } else {
                break;
            }
        }

        char** cWords = splitWords(message);
        send(sockfd, message, LENGTH_MSG, 0);
        
        if (strcasecmp(message, "quit") == 0) {
            free(cWords);
            break;
        }
        if (strcasecmp(cWords[0], "whois") == 0)
        {
            printf("WHOIS status:\n");
        }
        else if (strcasecmp(cWords[0], "Msg") == 0)
        {
            printf("Message delivered to target.\n");
        }
         else if (strcasecmp(cWords[0], "Join") == 0)
        {
            printf("Join made.\n");
        }
        else if (strcasecmp(cWords[0], "Time") == 0)
        {
            printf("Time displayed.\n");
        }
        else if (strcasecmp(cWords[0], "Alive") == 0)
        {
            printf("Server kept alive.\n");
        }
        free(cWords);
    }
    forceExit(2);
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Enter as follows: ./Client <IP-address> <port>\n");
        exit(-1);
    }
    
    signal(SIGINT, forceExit);

    /* Creating Names */
    printf("Please enter your name <Join> <Nickname> <Realname> : ");
    if (fgets(nickname, LENGTH_NAME, stdin) != NULL) {
        cutString(nickname, LENGTH_NAME);
    }
    if (strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("\nName must be more than one and less than thirty characters.\n");
        exit(EXIT_FAILURE);
    }

    /* Socket creation */
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1) {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    /* Socket informaton */
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(argv[1]);
    server_info.sin_port = htons(atoi(argv[2]));

    /* Connecting to server */
    int err = connect(sockfd, (struct sockaddr *)&server_info, s_addrlen);
    
    if (err == -1) {
        printf("Connection to Server error!\n");
        exit(EXIT_FAILURE);
    }

    /* Name retrieval */
    getsockname(sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
    getpeername(sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("\n --- Connection to server successful! ---\nConnected to Server: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
    printf("\nYou are: %s:%d\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

    send(sockfd, nickname, LENGTH_NAME, 0);

    pthread_t send_msg_thread;
    int* freeST;
    
    if (pthread_create(&send_msg_thread, NULL, (void *) sendHandler, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    pthread_t recv_msg_thread;
    int* freeRT;
    
    if (pthread_create(&recv_msg_thread, NULL, (void *) recvHandler, NULL) != 0) {
        printf ("Create pthread error!\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if(flag) {
            printf("Server: Goodbye!\n");
            break;
        }
    }

    pthread_join(send_msg_thread, (void**)&freeST);
    pthread_join(recv_msg_thread, (void**)&freeRT);

    free(freeST);
    free(freeRT);

    close(sockfd);
    return 0;
}