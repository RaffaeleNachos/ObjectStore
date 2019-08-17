/**
 * @file typicalclient.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h> /* necessario per ind su macchiona locale AF_UNIX */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "./objectstorelib.h"
#include <sys/stat.h>
#include <fcntl.h>

#define MAXMSG 128

int main(){
    char* name = malloc(MAXMSG*sizeof(char)); 
    char* command = malloc(MAXMSG*sizeof(char));
    scanf("%s", command);
    while((strcmp(command,"LEAVE"))!=0){
        if (strcmp(command, "REGISTER")==0){
            scanf("%s", name);
            os_connect(name);
        }
        if (strcmp(command, "STORE")==0){
            scanf("%s", name);
            FILE* inputfile;
            if ((inputfile = fopen(name, "rb")) == NULL){
                perror("file input apertura");
            }
            int inputfd = fileno(inputfile);
            struct stat fst;
            fstat(inputfd, &fst);
            os_store(name, (void*) inputfile, fst.st_size);
        }
        if (strcmp(command, "RETRIEVE")==0){
            scanf("%s", name);
            os_retrieve(name);
        }
        if (strcmp(command, "DELETE")==0){
            scanf("%s", name);
            os_delete(name);
        }
        scanf("%s", command);
    }
    os_disconnect();
    exit(EXIT_SUCCESS); 
}