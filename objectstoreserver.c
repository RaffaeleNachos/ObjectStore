/**
 * @file objectstoreserver.c
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
#include <sys/select.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#define UNIX_PATH_MAX 104 /* lunghezza massima consentita per il path */
#define SOCKNAME "./objstore.sock"
#define MAXMSG 128
#define MAXREGUSR 1024

typedef struct _client { //per vedere se il cliente è già registrato
    char* name;
    int isonline; //per report
    char* clientdir; //per opendir
    int filecounter; //per report
} clientinfo;

clientinfo *client_arr[MAXREGUSR];

void cleanup(){
    unlink(SOCKNAME);
}

unsigned long hash(char *str){
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return (hash%MAXREGUSR);
}

static void* myworker (void* arg){
    int fd = (int)arg;
    int index = -1;
    DIR* cdir = NULL;
    char* crequest = malloc(MAXMSG*sizeof(char));
    read(fd,crequest,MAXMSG);
    while(strcmp(crequest,"LEAVE")!=0){
        if (strcmp(crequest,"REGISTER")==0){
            char* tmpname = malloc(128*sizeof(char));
            read(fd,tmpname,sizeof(tmpname));
            printf("registro client %s\n", tmpname);
            index = hash(tmpname);
            if((client_arr[index]!=NULL)){ /* se è già registrato */
                errno=0;
                if ((cdir=opendir(client_arr[index]->clientdir))==NULL) {
	                perror("Apertura dir fallita");
                    write(fd,"login failed", 13);
                }
                else write(fd, "logged", 7);
            }
            else{ /* se non è registrato */
                printf("ricevuto utente non registrato\n");
                client_arr[index] = malloc(sizeof(clientinfo));
                client_arr[index]->name = malloc(MAXMSG*sizeof(char));
                client_arr[index]->clientdir = malloc(MAXMSG*sizeof(char));
                strcpy(client_arr[index]->name,tmpname);
                free(tmpname);
                if (getcwd(client_arr[index]->clientdir, MAXMSG*sizeof(char)) == NULL) {
                    perror("getcwd() error");
                }
                strcat(client_arr[index]->clientdir,"/");
                strcat(client_arr[index]->clientdir,client_arr[index]->name);
                mkdir(client_arr[index]->clientdir, 0777);
                errno=0;
                if ((cdir=opendir(client_arr[index]->clientdir))==NULL) {
	                perror("Apertura dir fallita");
                    write(fd,"register failed", 13);
                }
                write(fd, "OK", 3);
            }
            client_arr[index]->isonline=1;
        }
        else if (strcmp(crequest,"STORE")==0){
            FILE* tmpfiledesc;
            char* filename = malloc(128*sizeof(char));
            char* pathtofile = malloc(128*sizeof(char));
            char* len = malloc(128*sizeof(char));
            read(fd,filename,sizeof(filename));
            strcat(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, filename);
            printf("nome del file %s\n", filename);
            printf("path %s\n", pathtofile);
            free(filename);
            if ((tmpfiledesc=fopen(pathtofile, "wb")) == NULL){
                perror("errore creazione e scrittura file store");
                continue;
            }
            read(fd,len,sizeof(len));
            int dimbyte = strtol(len, NULL, 10);
            free(len);
            char* buffer = malloc(dimbyte*sizeof(char));
            read(fd,buffer,dimbyte);
            fwrite(buffer,dimbyte,1,tmpfiledesc);
            fclose(tmpfiledesc);
            write(fd,"OK",3);
        }
        else if (strcmp(crequest,"RETRIEVE")==0){
            printf("retrieve richiesta\n");
            FILE* readfiledesc;
            char* filename = malloc(128*sizeof(char));
            char* pathtofile = malloc(128*sizeof(char));
            read(fd,filename,sizeof(filename));
            strcat(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, filename);
            printf("nome del file %s\n", filename);
            printf("path %s\n", pathtofile);
            free(filename);
            if ((readfiledesc=fopen(pathtofile, "rb")) == NULL){
                perror("errore creazione e scrittura file store");
                continue;
            }
            int inputfd = fileno(readfiledesc);
            struct stat fst;
            fstat(inputfd, &fst);
            int dimbyte = fst.st_size;
            dprintf(fd,"%d",dimbyte);
            sleep(1);
            printf("mando dim: %d\n", dimbyte);
            char* buffer = malloc(128*sizeof(char));
            fread(buffer,dimbyte,1,readfiledesc);
            printf("dal server ho letto: %s\n", buffer);
            write(fd, buffer, dimbyte);
            write(fd, "OK", 3);
        }
        else if (strcmp(crequest,"DELETE")==0){
            char* filename = malloc(128*sizeof(char));
            char* pathtofile = malloc(128*sizeof(char));
            read(fd,filename,sizeof(filename));
            strcat(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, filename);
            if(remove(pathtofile)!=0){
                perror("errore delete server");
                write(fd,"KO",3);
            }
            else write(fd,"OK",3);
        }
        printf("ho terminato, immettere prossima op\n");
        read(fd,crequest,MAXMSG);
    }
    printf("Disconnetto client\n");
    client_arr[index]->isonline=0;
    if((closedir(cdir))==-1){
        perror("Chiusura dir fallita");
    }
    write(fd, "OK", 3);
    close(fd);
    free(crequest);
    return (void*)0;
}

void spawnmythread (int arg){
    pthread_attr_t t_attr;
    pthread_t t_id;
    errno=0;
    if (pthread_attr_init(&t_attr) != 0) {
	    perror("Inizializzazione attributi fallita");
	    close(arg);
	    return;
    }
    /*creazione thread in modalità detached così da non dover fare la wait */
    errno=0;
    if (pthread_attr_setdetachstate(&t_attr,PTHREAD_CREATE_DETACHED) != 0) {
	    perror("set in modalità detached fallita");
	    pthread_attr_destroy(&t_attr);
	    close(arg);
	    return;
    }
    errno=0;
    if (pthread_create(&t_id, &t_attr, myworker, (void*) arg) != 0) {
	    perror("Thread create fallita");
	    pthread_attr_destroy(&t_attr);
	    close(arg);
	    return;
    }
    return;
}

static void run_server(struct sockaddr_un * psa){
    int fd_skt, fd_c; /*file descriptor socket e client*/ 
    errno = 0;
    if ((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Non è stato possibile creare socket");
        exit(EXIT_FAILURE);
    } 
    errno = 0;
    if (bind(fd_skt, (struct sockaddr *)psa, sizeof(*psa)) == -1) { /*associa un indirizzo a socket*/
        perror("Non è stato possibile eseguire il bind");
        exit(EXIT_FAILURE);
    }
    errno = 0;
    if (listen(fd_skt, SOMAXCONN) == -1) { /*il socket accetta connessioni*/
        perror("Non è stato possibile accettare la connessione");
        exit(EXIT_FAILURE);
    }
    while(1){
        errno=0;
        if ((fd_c = accept(fd_skt, NULL, 0)) == -1){ /*da qui uso fd_c del client accettato */
            perror("Non è stato possibile creare un nuovo socket per la comunicazione");
            exit(EXIT_FAILURE);
        }
        spawnmythread(fd_c);
    }
}

int main (void) {
    cleanup();
    atexit(cleanup);
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    run_server(&sa);
    return 0;
}
