/**
 * @file objectstorelib.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * libreria utilizzata dal client
 * @version 1.0
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "./objectstorelib.h"
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h> /* necessario per ind su macchina locale AF_UNIX */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define UNIX_PATH_MAX 108 /* lunghezza massima consentita per il path */
#define SOCKNAME "./objstore.sock"
#define MAXMSG 128

int fd_skt = -1;
char* response; //buffer dove vado a salvare le risposte del server con la read

int os_connect(char* name){
    if(fd_skt!=-1){
        return 0;
    }
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
    if(fd_skt==-1){
        return 0;
    }
    char* data = malloc(len*sizeof(char)+1);
    memset(data, 0, len*sizeof(char)+1);
    printf("fin qui ci sono\n");
    int lung=0;
    int btoread=len;
    while((lung=fread(data,btoread,1,block))>0){
        printf("%d\n", lung);
        btoread-=lung;
        printf("devo leggere ancora: %d\n", btoread);
    }
    dprintf(fd_skt, "STORE %s %zu \n %s", name, len, data);
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
    if(fd_skt==-1){
        return (void*)NULL;
    }
    dprintf(fd_skt, "RETRIEVE %s \n", name);
    response = malloc(MAXMSG*sizeof(char));
    char* buffer;
    char* last;
    char* token;
    read(fd_skt,response,MAXMSG);
    token=strtok_r(response, " ", &last);
    if (strcmp(token,"DATA")==0){
        token=strtok_r(NULL, " ", &last);
        int dimbyte = strtol(token, NULL, 10);
        printf("dimbyte ricevuta %d\n", dimbyte);
        token=strtok_r(NULL, " ", &last);
        int nread=0;
        nread=strlen(last)*sizeof(char);
        printf("nread da last: %d\n", nread);
        printf("byte da leggere %d\n", dimbyte-nread);
        buffer = malloc((dimbyte)*sizeof(char)+1);
        memset(buffer, 0, dimbyte);
        strcpy(buffer,last);
        char* data = malloc(dimbyte*sizeof(char)); /*tmp */
        memset(data, 0, dimbyte);
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
    if(fd_skt==-1){
        return 1;
    }
    write(fd_skt, "LEAVE", 6);
    response = malloc(MAXMSG*sizeof(char));
    read(fd_skt,response,sizeof(response));
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
