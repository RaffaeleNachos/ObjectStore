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
        exit(EXIT_FAILURE);
    }
    while (connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        if (errno == ENOENT) { /*no file o directory presente */
            sleep(1); /* sock non esiste quindi ciclo*/ 
        }
        else exit(EXIT_FAILURE); 
    }
    response = malloc(128*sizeof(char));
    write(fd_skt, "REGISTER", 9);
    sleep(1);
    dprintf(fd_skt, "%s", name);
    read(fd_skt,response,sizeof(response));
    printf("%s\n", response);
    return 0;
}

int os_store(char* name, void* block, size_t len){
    return 0;
}

void *os_retrieve(char* name){
    return 0;
}

int os_delete(char* name){
    return 0;
}

int os_disconnect(){
    write(fd_skt, "LEAVE", 6);
    sleep(1);
    read(fd_skt,response,sizeof(response));
    printf("%s\n", response);
    errno=0;
    if(close(fd_skt)!=0){
        perror("Errore durante la disconnessione");
    };
    return 0;
}