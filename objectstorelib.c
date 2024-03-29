/**
 * @file objectstorelib.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * libreria utilizzata dal client
 * maggiori informazioni contenute nel file objectstorelib.h
 * @version 4.0 final
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "./objectstorelib.h"
#include "rwn.h"
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define UNIX_PATH_MAX 108
#define SOCKNAME "./objstore.sock"
#define MAXMSG 128

int fd_skt = -1;
char* response; //buffer dove vado a salvare le risposte del server con la read

int os_connect(char* name){
    if(fd_skt!=-1){ //se ho già eseguito la register lato client (necessario per il client interattivo)
        return 0;
    }
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if ((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Non è stato possibile creare socket");
        return 0;
    }
    while (connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        if (errno == ENOENT) { /*no file o directory presente */
            sleep(1); /* sock non esiste quindi ciclo */ 
        }
        else return 0; 
    }
    dprintf(fd_skt, "REGISTER %s \n", name); /*mando al server header*/
    response = malloc(MAXMSG*sizeof(char));
    if(readcn(fd_skt,response,sizeof(response))<0){
        free(response);
        response=NULL;
        return 0;
    }
    if (strcmp(response, "KO")==0){
        free(response);
        response=NULL;
        return 0;
    }
    free(response);
    response=NULL;
    return 1;
}

int os_store(char* name, void* block, size_t len){
    if(fd_skt==-1){ //se non ho ancora fatto la register
        return 0;
    }
    char* data = malloc(len*sizeof(char)+1);
    memset(data, 0, len*sizeof(char)+1);
    readn((long)block,data,len); /*leggo len dall'oggetto puntato da block*/
    dprintf(fd_skt, "STORE %s %zu \n %s", name, len, data); /*invio come da protocollo*/
    response = malloc(MAXMSG*sizeof(char));
    if(readcn(fd_skt,response,sizeof(response))<0){
        free(response);
        response=NULL;
        return 0;
    }
    if (strcmp(response, "KO")==0){
        free(response);
        free(data);
        data=NULL;
        response=NULL;
        return 0;
    }
    free(response);
    free(data);
    data=NULL;
    response=NULL;
    return 1;
}

void *os_retrieve(char* name){
    if(fd_skt==-1){
        return (void*)NULL;
    }
    dprintf(fd_skt, "RETRIEVE %s \n", name);
    response = malloc(MAXMSG*sizeof(char)+1);
    char* buffer = NULL;
    char* last = NULL;
    char* token = NULL;
    if(readcn(fd_skt,response,MAXMSG)<0){
        free(response);
        response=NULL;
        return (void*)NULL;
    }
    token=strtok_r(response, " ", &last);
    if (strcmp(token,"DATA")==0){
        int nread = 5; //byte DATA + spazio
        token=strtok_r(NULL, " ", &last); //leggo len
        nread=nread+strlen(token)+1; //byte len + spazio
        int dimbyte = strtol(token, NULL, 10);
        token=strtok_r(NULL, " ", &last); //leggo "\n"
        nread=nread+strlen(token)+1; //byte \n + spazio
        //printf("byte letti dall'header: %d\n", nread);
        nread=MAXMSG-nread; //nread diventa il resto della str
        buffer = malloc((dimbyte)*sizeof(char)+1);
        memset(buffer, 0, dimbyte+1);
        if(dimbyte<=nread){
            strncpy(buffer,last,dimbyte);
        }
        else{
            strncpy(buffer,last,nread); /*copio la prima parte*/
        }
        int btoread=dimbyte-nread; /*i byte da leggere sono i byte totale - i byte letti dal messaggio*/
        if(btoread>0){
            char* data = malloc(btoread*sizeof(char)); /*dove vado a copiare temporaneamente*/
            memset(data, 0, btoread);
            readn(fd_skt,data,btoread);
            strncat(buffer,data,btoread);
            free(data);
            data=NULL;
        }
        free(response);
        response=NULL;
    }
    else if(strcmp(response,"KO")==0){
        free(response);
        response=NULL;
        return (void*)NULL;
    }
    return (void*)buffer;
}

int os_delete(char* name){
    if(fd_skt==-1){
        return 0;
    }
    dprintf(fd_skt, "DELETE %s \n", name);
    response = malloc(MAXMSG*sizeof(char));
    if(readcn(fd_skt,response,sizeof(response))<0){
        free(response);
        response=NULL;
        return 0;
    }
    if (strcmp(response, "KO")==0){
        free(response);
        response=NULL;
        return 0;
    }
    free(response);
    response=NULL;
    return 1;
}

int os_disconnect(){
    if(fd_skt==-1){
        return 1;
    }
    writen(fd_skt, "LEAVE \n", 8);
    response = malloc(MAXMSG*sizeof(char));
    if(readcn(fd_skt,response,sizeof(response))<0){
        free(response);
        response=NULL;
        return 0;
    }
    if (strcmp(response, "OK")==0){
        if(close(fd_skt)!=0){
            perror("Errore durante la disconnessione");
        }
        free(response);
        response=NULL;
        return 1;
    }
    return 1;
}
