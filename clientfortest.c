/**
 * @file clientfortest.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * client per test
 * @version 0.1
 * @date 2019-08-19
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
#include "./objectstorelib.h"
#include <sys/stat.h>
#include <fcntl.h>

#define MAXMSG 128
#define MAXFNAME 10
#define STR "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed eget purus a enim pulvinar dictum. STOP"

int main(int argc, char* argv[]){
    if(argc!=3){
        perror("./eseguibile nomeclient numerotest");
        exit(EXIT_FAILURE);
    }
    char* name = malloc(MAXMSG*sizeof(char)+1);
    strcpy(name,argv[1]);
    int test = strtol(argv[2],NULL,10);
    
    /*register*/
    if(os_connect(name)==1){
        printf("REGISTER andata a buon fine\n");
    }
    else{
        printf("REGISTER fallita\n");
    }
    
    /* casi di test */
    if(test==1){
        char* filename = malloc(MAXFNAME*sizeof(char));
        for(int i = 1; i<=20; i++){
            memset(filename, 0, MAXFNAME);
            snprintf(filename, MAXFNAME, "f%d.txt", i);
            char* datablock = malloc(strlen(STR)*i*50*sizeof(char)+1);
            memset(datablock, 0, strlen(STR)*i*50+1);
            for(int j = 1; j<=(i*50); j++){
                strncat(datablock,STR,sizeof(STR));
            }
            FILE* newfile;
            if ((newfile=fopen(filename, "wb")) == NULL){
                perror("errore creazione e scrittura file client");
            }
            fwrite(datablock,strlen(STR)*i*50*sizeof(char),1,newfile);
            fclose(newfile);
            printf("ho scritto tutto clientfortest\n");
            FILE* inputfile;
            if ((inputfile = fopen(filename, "rb")) == NULL){
                perror("Immettere nome file esistente");
                continue;
            }
            int inputfd = fileno(inputfile);
            struct stat fst;
            fstat(inputfd, &fst);
            if(os_store(filename, (void*) inputfile, fst.st_size)==1){
                printf("STORE andata a buon fine\n");
            }
            else{
                printf("STORE fallita\n");
            }
            free(datablock);
            datablock=NULL;
        }
        free(filename);
        filename=NULL;
    }
    else if(test==2){
        char* filename = malloc(MAXFNAME*sizeof(char));
        for(int i = 1; i<=20; i++){
            memset(filename, 0, MAXFNAME);
            snprintf(filename, MAXFNAME, "f%d.txt", i);
            FILE* newfile;
            if ((newfile=fopen(filename, "wb")) == NULL){
                perror("errore creazione e scrittura file client");
            }
            char* received = NULL;
            if((received=(char*)os_retrieve(filename))!=NULL){
                fwrite(received,strlen(received)*sizeof(char),1,newfile);
                printf("RETRIEVE andata a buon fine\n");
            }
            else{
                printf("RETRIEVE fallita\n");
            }
            free(received);
            received=NULL;
            fclose(newfile);
        }
        free(filename);
        filename=NULL;
    }
    else if(test==3){
        char* filename = malloc(7*sizeof(char));
        for(int i = 1; i<=20; i++){
            memset(filename, 0, 7);
            snprintf(filename, MAXFNAME, "f%d.txt", i);
            if(os_delete(filename)==1){
                 printf("DELETE andata a buon fine\n");
            }
            else{
                printf("DELETE fallita\n");
            }
        }
        free(filename);
        filename=NULL;
    }
    else{
        perror("numero test errato");
        exit(EXIT_FAILURE);
    }
    
    /*disconnessione*/
    if(os_disconnect()==1){
        printf("DISCONNECT andata a buon fine\n");
    }
    else{
        printf("DISCONNECT fallita\n");
    }
    free(name);
    exit(EXIT_SUCCESS); 
}
