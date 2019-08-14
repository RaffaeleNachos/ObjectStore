#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h> /* necessario per ind su macchiona locale AF_UNIX */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "./objectstorelib.h"

#define MAXMSG 128

int main(){
    char* name = malloc(MAXMSG*sizeof(char)); 
    char* command = malloc(MAXMSG*sizeof(char));
    char* len = malloc(MAXMSG*sizeof(char));
    scanf("%s", command);
    while((strcmp(command,"LEAVE"))!=0){
        if (strcmp(command, "REGISTER")==0){
            scanf("%s", name);
            os_connect(name);
        }
        if (strcmp(command, "STORE")==0){
            scanf("%s", name);
            scanf("%s", len);
            os_store(name,block,strtol(len, (char **)NULL, 10)));
        }
        scanf("%s", command);
    }
    os_disconnect();
    exit(EXIT_SUCCESS); 
}