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
    int result;
    scanf("%s", command);
    while((strcmp(command,"LEAVE"))!=0){
        if (strcmp(command, "REGISTER")==0){
            scanf("%s", name);
            result=os_connect(name);
            if(result==1){
                printf("REGISTER andata a buon fine\n");
            }
            else{
                printf("REGISTER fallita\n");
            }
        }
        if (strcmp(command, "STORE")==0){
            scanf("%s", name);
            FILE* inputfile;
            if ((inputfile = fopen(name, "rb")) == NULL){
                perror("Immettere nome file esistente");
                continue;
            }
            int inputfd = fileno(inputfile);
            struct stat fst;
            fstat(inputfd, &fst);
            result=os_store(name, (void*) inputfile, fst.st_size);
            if(result==1){
                printf("STORE andata a buon fine\n");
            }
            else{
                printf("STORE fallita\n");
            }
        }
        if (strcmp(command, "RETRIEVE")==0){
            scanf("%s", name);
            FILE* newfiledesc;
            if ((newfiledesc=fopen(name, "wb")) == NULL){
                perror("errore creazione e scrittura retrieve client");
                continue;
            }
            char* received = NULL;
            if((received=(char*)os_retrieve(name))!=NULL){
                fwrite(received,strlen(received)*sizeof(char),1,newfiledesc); //qui la dimensione va cambiata non è MAXMSG
                printf("RETRIEVE andata a buon fine\n");
            }
            else{
                printf("RETRIEVE fallita\n");
            }
            received=NULL;
            fclose(newfiledesc);
        }
        if (strcmp(command, "DELETE")==0){
            scanf("%s", name);
            result=os_delete(name);
            if(result==1){
                printf("DELETE andata a buon fine\n");
            }
            else{
                printf("DELETE fallita\n");
            }
        }
        result=0;
        scanf("%s", command);
    }
    result=os_disconnect();
    if(result==1){
        printf("DISCONNECT andata a buon fine\n");
    }
    else{
        printf("DISCONNECT fallita\n");
    }
    free(name);
    free(command);
    exit(EXIT_SUCCESS); 
}