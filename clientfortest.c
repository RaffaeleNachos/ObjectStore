/**
 * @file clientfortest.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * client per test
 * @version 2.0
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
#define STR "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed eget purus a enim pulvinar dictum. STOP"

static int num_ok_op = 0;
static int num_fail_op = 0;

int main(int argc, char* argv[]){
    if(argc!=3){
        perror("./eseguibile nomeclient numerotest");
        exit(EXIT_FAILURE);
    }
    int test = strtol(argv[2],NULL,10);
    struct rusage clientusage;
    /*register*/
    if(os_connect(argv[1])==1){
        printf("REGISTER %s OK\n", argv[1]);
        num_ok_op++;
    }
    else{
        printf("REGISTER %s FAIL\n", argv[1]);
        num_fail_op++;
    }
    
    /* casi di test */
    if(test==1){
        char* filename = malloc(NAME_MAX*sizeof(char));
        for(int i = 1; i<=20; i++){
            memset(filename, 0, NAME_MAX);
            snprintf(filename, NAME_MAX, "f%d.txt", i);
            
            /*creazione dei file*/
            char* datablock = malloc(strlen(STR)*i*sizeof(char)+1); //+1 per strcat che aggiunge '\0' alla fine
            memset(datablock, 0, strlen(STR)*i*sizeof(char)+1);
            for(int j = 1; j<=(i); j++){
                strncat(datablock,STR,sizeof(STR));
            }
            long newfile;
            if ((newfile=open(filename, O_CREAT | O_WRONLY, 0777)) == -1){
                perror("errore creazione e scrittura file client");
                continue;
            }
            writen(newfile, datablock, strlen(STR)*i*sizeof(char));
            close(newfile);
            
            /*esecuzione store*/
            long inputfile;
            if ((inputfile = open(filename, O_RDONLY)) == -1){
                perror("Immettere nome file esistente");
                continue;
            }
            struct stat fst;
            fstat(inputfile, &fst);
            if(os_store(filename, (void*)inputfile, fst.st_size)==1){
                printf("STORE %s di %s OK\n", filename, argv[1]);
                num_ok_op++;
            }
            else{
                printf("STORE %s di %s FAIL\n", filename, argv[1]);
                num_fail_op++;
            }
            close(inputfile);
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
                if(strcmp(received,buffer)==0){
                    printf("RETRIEVE %s di %s OK\n", filename, argv[1]);
                    num_ok_op++;
                }
                else {
                    printf("RETRIEVE %s di %s FAIL\n", filename, argv[1]);
                    num_fail_op++;
                }
            }
            else{
                printf("RETRIEVE %s di %s FAIL\n", filename, argv[1]);
                num_fail_op++;
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
                printf("DELETE %s di %s OK\n", filename, argv[1]);
                num_ok_op++;
            }
            else{
                printf("DELETE %s di %s FAIL\n", filename, argv[1]);
                num_fail_op++;
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
        printf("DISCONNECT %s OK\n", argv[1]);
        num_ok_op++;
    }
    else{
        printf("DISCONNECT %s FAIL\n", argv[1]);
        num_fail_op++;
    }
    if((getrusage(RUSAGE_SELF,&clientusage))==-1){
        perror("impossibile ottenere info su uso client");
    }

    printf("########## RIEPILOGO CLIENT ##########\n");
    printf("Numero operazioni effettuate: %d\n", num_ok_op+num_fail_op);
    printf("Numero operazioni success: %d\n", num_ok_op);
    printf("Numero operazioni failed: %d\n", num_fail_op);
    printf("Tempo in UserMode: %ld.%lds\n", clientusage.ru_utime.tv_sec, clientusage.ru_utime.tv_usec);
    printf("Tempo in KernelMode: %ld.%lds\n", clientusage.ru_stime.tv_sec, clientusage.ru_stime.tv_usec);
    exit(EXIT_SUCCESS); 
}
