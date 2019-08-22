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
#include <sys/stat.h>
#include <fcntl.h>
#include "./objectstorelib.h"
#include "rwn.h"

#define MAXMSG 128
#define NAME_MAX 255
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
        printf("REGISTER %s OK\n", name);
    }
    else{
        printf("REGISTER %s FAIL\n", name);
    }
    
    /* casi di test */
    if(test==1){
        char* filename = malloc(NAME_MAX*sizeof(char));
        for(int i = 1; i<=20; i++){
            memset(filename, 0, NAME_MAX);
            snprintf(filename, NAME_MAX, "f%d.txt", i);
            char* datablock = malloc(strlen(STR)*i*50*sizeof(char)+1);
            memset(datablock, 0, strlen(STR)*i*50+1);
            for(int j = 1; j<=(i*50); j++){
                strncat(datablock,STR,sizeof(STR));
            }
            int newfile;
            if ((newfile=open(filename, O_CREAT | O_WRONLY, 0777)) == -1){
                perror("errore creazione e scrittura file client");
                continue;
            }
            writen(newfile, datablock, strlen(STR)*i*50*sizeof(char));
            close(newfile);
            int inputfile;
            if ((inputfile = open(filename, O_RDONLY)) == -1){
                perror("Immettere nome file esistente");
                continue;
            }
            struct stat fst;
            fstat(inputfile, &fst);
            if(os_store(filename, (void*) inputfile, fst.st_size)==1){
                printf("STORE %s di %s OK\n", filename, name);
            }
            else{
                printf("STORE %s di %s FAIL\n", filename, name);
            }
            free(datablock);
            datablock=NULL;
        }
        free(filename);
        filename=NULL;
    }
    else if(test==2){
        char* filename = malloc(NAME_MAX*sizeof(char));
        for(int i = 1; i<=20; i++){
            memset(filename, 0, NAME_MAX);
            snprintf(filename, NAME_MAX, "f%d.txt", i);
            int checkfile;
            if ((checkfile=open(filename, O_RDONLY)) == -1){
                perror("errore apertura file retrieve client");
            }
            struct stat fst;
            fstat(checkfile, &fst);
            int dimbyte = fst.st_size;
            char* buffer = malloc(dimbyte*sizeof(char)+1);
            memset(buffer, 0, dimbyte+1);
            readn(checkfile,buffer,dimbyte);
            char* received = NULL;
            if((received=(char*)os_retrieve(filename))!=NULL){
                if(strcmp(received,buffer)==0) printf("RETRIEVE %s di %s OK\n", filename, name);
                else printf("RETRIEVE %s di %s FAIL\n", filename, name);
            }
            else{
                printf("RETRIEVE %s di %s FAIL\n", filename, name);
            }
            free(buffer);
            buffer=NULL;
            free(received);
            received=NULL;
            close(checkfile);
        }
        free(filename);
        filename=NULL;
    }
    else if(test==3){
        char* filename = malloc(NAME_MAX*sizeof(char));
        for(int i = 1; i<=20; i++){
            memset(filename, 0, NAME_MAX);
            snprintf(filename, NAME_MAX, "f%d.txt", i);
            if(os_delete(filename)==1){
                printf("DELETE %s di %s OK\n", filename, name);
            }
            else{
                printf("DELETE %s di %s FAIL\n", filename, name);
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
        printf("DISCONNECT %s OK\n", name);
    }
    else{
        printf("DISCONNECT %s FAIL\n", name);
    }
    free(name);
    exit(EXIT_SUCCESS); 
}
