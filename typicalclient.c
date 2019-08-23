/**
 * @file typicalclient.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief
 * client interattivo
 * @version 4.0
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "./objectstorelib.h"
#include "rwn.h"

#define MAXMSG 128
#define NAME_MAX 255

int main(){
    char* name = malloc(MAXMSG*sizeof(char)); 
    char* command = malloc(MAXMSG*sizeof(char));
    scanf("%s", command);
    while((strcmp(command,"LEAVE"))!=0){
        if (strcmp(command, "REGISTER")==0){
            scanf("%s", name);
            if(os_connect(name)==1){
                printf("REGISTER andata a buon fine\n");
            }
            else{
                printf("REGISTER fallita\n");
            }
        }
        if (strcmp(command, "STORE")==0){
            scanf("%s", name);
            long inputfile;
            if ((inputfile = open(name, O_RDONLY)) == -1){
                perror("Immettere nome file esistente");
                continue;
            }
            struct stat fst;
            fstat(inputfile, &fst);
            if(os_store(name, (void*)inputfile, fst.st_size)==1){
                printf("STORE andata a buon fine\n");
            }
            else{
                printf("STORE fallita\n");
            }
        }
        if (strcmp(command, "RETRIEVE")==0){
            scanf("%s", name);
            int checkfile;
            if ((checkfile=open(name, O_RDONLY)) == -1){
                perror("errore apertura file retrieve client");
            }
            struct stat fst;
            fstat(checkfile, &fst);
            int dimbyte = fst.st_size;
            char* buffer = malloc(dimbyte*sizeof(char)+1);
            memset(buffer, 0, dimbyte+1);
            readn(checkfile,buffer,dimbyte);
            char* received = NULL;
            if((received=(char*)os_retrieve(name))!=NULL){
                if(strcmp(received,buffer)==0){
                    printf("RETRIEVE andata a buon fine\n");
                }
                else {
                    printf("RETRIEVE fallita\n");
                }
            }
            else{
                printf("RETRIEVE fallita\n");
            }
            free(buffer);
            buffer=NULL;
            free(received);
            received=NULL;
            close(checkfile);
        }
        if (strcmp(command, "DELETE")==0){
            scanf("%s", name);
            if(os_delete(name)==1){
                printf("DELETE andata a buon fine\n");
            }
            else{
                printf("DELETE fallita\n");
            }
        }
        scanf("%s", command);
    }
    if(os_disconnect()==1){
        printf("DISCONNECT andata a buon fine\n");
    }
    else{
        printf("DISCONNECT fallita\n");
    }
    free(name);
    free(command);
    exit(EXIT_SUCCESS); 
}