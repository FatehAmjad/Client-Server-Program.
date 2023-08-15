#include <stdio.h>
#include "text.h"

void cutString (char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { 
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void overwriteString() {
    printf("\r%s", "> ");
    fflush(stdout);
}
