#ifndef LIST
#define LIST

char** splitWords(char str[]);
int countString(char s[]);

typedef struct Nodes {
    int data;
    struct Nodes* prev;
    struct Nodes* link;
    char ip[16];
    char name[31];
    char petName[31];
} Clients;

Clients *newNode(int sockfd, char* ip) {
    Clients *np = (Clients *)malloc( sizeof(Clients) );
    np->data = sockfd;
    np->prev = NULL;
    np->link = NULL;
    strncpy(np->ip, ip, 16);
    strncpy(np->name, "NULL", 5);
    return np;
}

#endif