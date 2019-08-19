/**
 * @file objectstorelib.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "./objectstorelib.h"
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h> /* necessario per ind su macchiona locale AF_UNIX */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define UNIX_PATH_MAX 104 /* lunghezza massima consentita per il path */
#define SOCKNAME "./objstore.sock"
#define MAXMSG 128

int fd_skt;
char* response; //buffer dove vado a salvare le risposte del server con la read

int os_connect(char* name){
    /*richiedo al server di creare la directory, controllo se non c'è già ecc */
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    errno = 0;
    if ((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Non è stato possibile creare socket");
        return 0;
    }
    while (connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        if (errno == ENOENT) { /*no file o directory presente */
            sleep(1); /* sock non esiste quindi ciclo*/ 
        }
        else return 0; 
    }
    dprintf(fd_skt, "REGISTER %s \n", name);
    sleep(1);
    response = malloc(MAXMSG*sizeof(char));
    read(fd_skt,response,sizeof(response));
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
    char* data = malloc(len*sizeof(char));
    fread(data,len,1,block);
    //printf("%s\n", data);
    dprintf(fd_skt, "STORE %s %zu \n %s", name, len, data);
    sleep(1);
    response = malloc(MAXMSG*sizeof(char));
    read(fd_skt,response,sizeof(response));
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
    dprintf(fd_skt, "RETRIEVE %s \n", name);
    response = malloc(MAXMSG*sizeof(char));
    char* buffer;
    char* last;
    read(fd_skt,response,MAXMSG);
    response=strtok_r(response, " ", &last);
    if (strcmp(response,"DATA")==0){
        response=strtok_r(NULL, " ", &last);
        int dimbyte = strtol(response, NULL, 10);
        printf("dimbyte ricevuta %d\n", dimbyte);
        response=strtok_r(NULL, " ", &last);
        int nread=0;
        nread=strlen(last)*sizeof(char);
        printf("nread da last: %d\n", nread);
        printf("byte da leggere %d\n", dimbyte-nread);
        buffer = malloc((dimbyte)*sizeof(char));
        strcpy(buffer,last);
        char* data = malloc(dimbyte*sizeof(char)); /*tmp */
        lseek(fd_skt,0,SEEK_SET);
        int lung=0;
        int btoread=dimbyte-nread;
        while((lung=read(fd_skt,data,btoread))>0){
            printf("%d\n", lung);
            btoread-=lung;
            printf("devo leggere ancora: %d\n", btoread);
            strncat(buffer,data,lung);
        }
        free(data);
        data=NULL;
    }
    else if(strcmp(response,"KO")==0){
        free(response);
        response=NULL;
        return (void*)NULL;
    }
    return (void*)buffer;
}

int os_delete(char* name){
    dprintf(fd_skt, "DELETE %s \n", name);
    sleep(1);
    response = malloc(MAXMSG*sizeof(char));
    read(fd_skt,response,sizeof(response));
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
    write(fd_skt, "LEAVE", 6);
    response = malloc(MAXMSG*sizeof(char));
    read(fd_skt,response,sizeof(response));
    if (strcmp(response, "KO")==0){
        free(response);
        response=NULL;
        return 0;
    }
    errno=0;
    if(close(fd_skt)!=0){
        perror("Errore durante la disconnessione");
    };
    return 1;
}