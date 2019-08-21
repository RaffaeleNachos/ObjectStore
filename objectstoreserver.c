/**
 * @file objectstoreserver.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * server object store
 * @version 1.0
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/un.h> /* necessario per ind su macchina locale AF_UNIX */
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
#include <signal.h>
#include "rwn.h"

#define UNIX_PATH_MAX 108 /* lunghezza massima consentita per il path */
#define SOCKNAME "./objstore.sock"
#define MAXMSG 128
#define MAXREGUSR 1024 /*dimensione tabella hash*/

static pthread_mutex_t cmtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fmtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dmtx = PTHREAD_MUTEX_INITIALIZER;

static int numclientconn = 0;
static int numtotfile = 0;
static int totsize = 0;

typedef struct _client { //per vedere se il cliente è già registrato basta controllare tramite hashing
    char* name;
    int isonline; //per controllare che non sia già online
    char* clientdir;
} clientinfo;

clientinfo *client_arr[MAXREGUSR];

void cleanup(){
    unlink(SOCKNAME);
}

static void* checksignals(void *arg) {
    sigset_t *set = (sigset_t*)arg;
    while(1) {
	    int sig;
	    int rterrn = sigwait(set, &sig);
	    if (rterrn != 0) {
	        errno = rterrn;
	        perror("errore sigwait");
	        return NULL;
	    }
	    switch(sig) {
	    case SIGINT: //pulizia e chiusura
	    case SIGTERM: //pulizia
	    case SIGQUIT:
	        //printf("ricevuto segnale %s, esco\n", (sig==SIGINT) ? "SIGINT": ((sig==SIGTERM)?"SIGTERM":"SIGQUIT") );
	        return NULL;
	    default:  ; 
	    }
    }
    return NULL;	   
}

unsigned long hash(char *str){ /*funzione hash basata su stringhe*/
    unsigned long hash = 5381; /*numero primo molto grande*/
    int c;
    while ((c = *str++)) /*per ogni carattere viene associato il suo valore ascii*/
        hash = ((hash << 5) + hash) + c; /* esegue uno shift ciclico a sinistra di 5 posizioni sul valore hash, sarebbe come fare hash * 32 + c */
    return (hash%MAXREGUSR); /*ritorna valore in modulo per far si che index si trovi all'interno dell'hash*/
}

static void* myworker (void* arg){ /*thread detached worker che gestisce un singolo client*/
    long fd = (long)arg;
    int index = -1;
    char* strreceived = malloc(MAXMSG*sizeof(char)+1);
    char* crequest = malloc(MAXMSG*sizeof(char));
    char* last;
    read(fd,strreceived,MAXMSG);
    crequest=strtok_r(strreceived, " ", &last);
    while(strcmp(crequest,"LEAVE")!=0){
        if (strcmp(crequest,"REGISTER")==0){
            crequest=strtok_r(NULL, " ", &last);
            printf("registro client %s\n", crequest);
            index = hash(crequest); /*calcolo indice nella tabella hash*/
            if((client_arr[index]!=NULL)){ /* se è già registrato */
                if(client_arr[index]->isonline==1){
                    perror("Ricevuto utente già online");
                    writen(fd, "KO", 3);
                    continue;
                }
                else {
                    pthread_mutex_lock(&cmtx);
                    numclientconn++;
                    pthread_mutex_unlock(&cmtx);
                    writen(fd, "OK", 3);
                }
            }
            else{ /* se non è registrato */
                printf("ricevuto utente non registrato\n");
                client_arr[index] = malloc(sizeof(clientinfo));
                client_arr[index]->isonline=1;
                client_arr[index]->name = malloc(MAXMSG*sizeof(char));
                client_arr[index]->clientdir = malloc(UNIX_PATH_MAX*sizeof(char));
                strcpy(client_arr[index]->name,crequest);
                if (getcwd(client_arr[index]->clientdir, UNIX_PATH_MAX) == NULL) { /*mi faccio restituire la current directory per creare la folder all'interno di data*/
                    perror("getcwd() error");
                    writen(fd, "KO", 3);
                    continue;
                }
                strcat(client_arr[index]->clientdir,"/data/");
                strcat(client_arr[index]->clientdir,client_arr[index]->name);
                mkdir(client_arr[index]->clientdir, 0777); /*crea la directory con name*/
                pthread_mutex_lock(&cmtx);
                numclientconn++;
                pthread_mutex_unlock(&cmtx);
                writen(fd, "OK", 3);
            }
        }
        else if (strcmp(crequest,"STORE")==0){
            FILE* tmpfiledesc;
            crequest=strtok_r(NULL, " ", &last);
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char)+1);
			memset(pathtofile, 0, UNIX_PATH_MAX);
            strcpy(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            printf("path %s\n", pathtofile);
            if ((tmpfiledesc=fopen(pathtofile, "wb")) == NULL){
                perror("errore creazione e scrittura file store");
                writen(fd,"KO", 3);
            }
            crequest=strtok_r(NULL, " ", &last);
            int dimbyte = strtol(crequest, NULL, 10);
            //printf("%d\n", dimbyte);
            crequest=strtok_r(NULL, " ", &last);
            int nread = strlen(last)*sizeof(char);
            //printf("nread da last: %d\n", nread);
            //sono triste
            //printf("byte da leggere %d\n", dimbyte-nread);
            fwrite(last,nread,1,tmpfiledesc);
            char* data = malloc(dimbyte*sizeof(char));
			memset(data, 0, dimbyte);
            lseek(fd,0,SEEK_SET);
            int btoread=dimbyte-nread;
            readn(fd,data,btoread);
            fwrite(data,dimbyte,1,tmpfiledesc);
            fclose(tmpfiledesc);
            free(data);
            free(pathtofile);
            pathtofile=NULL;
            data=NULL;
            pthread_mutex_lock(&fmtx);
            numtotfile++;
            pthread_mutex_unlock(&fmtx);
            pthread_mutex_lock(&dmtx);
            totsize+=dimbyte;
            pthread_mutex_unlock(&dmtx);
            writen(fd,"OK",3);
        }
        else if (strcmp(crequest,"RETRIEVE")==0){
            FILE* readfiledesc;
            crequest=strtok_r(NULL, " ", &last);
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char));
            memset(pathtofile, 0, UNIX_PATH_MAX);
            strcpy(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            //printf("nome del file %s\n", crequest);
            //printf("path %s\n", pathtofile);
            if ((readfiledesc=fopen(pathtofile, "rb")) == NULL){
                perror("errore creazione e scrittura file retrieve");
                writen(fd,"KO", 3);
                continue;
            }
            int inputfd = fileno(readfiledesc);
            struct stat fst;
            fstat(inputfd, &fst);
            int dimbyte = fst.st_size;
            char* buffer = malloc(dimbyte*sizeof(char)+1);
            memset(buffer, 0, dimbyte+1);
            int btoread=dimbyte;
            fread(buffer,btoread,1,readfiledesc);
            dprintf(fd, "DATA %d \n %s", dimbyte, buffer);
            fclose(readfiledesc);
            free(pathtofile);
            free(buffer);
            pathtofile=NULL;
            buffer=NULL;
        }
        else if (strcmp(crequest,"DELETE")==0){
            crequest=strtok_r(NULL, " ", &last);
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char)+1);
            strcpy(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            printf("%s\n", pathtofile);
            struct stat st;
            stat(pathtofile, &st);
            if(remove(pathtofile)!=0){
                perror("errore delete server");
                free(pathtofile);
                writen(fd,"KO", 3);
            }
            else {
                free(pathtofile);
                pthread_mutex_lock(&fmtx);
                numtotfile--;
                pthread_mutex_unlock(&fmtx);
                pthread_mutex_lock(&dmtx);
                totsize-=st.st_size;
                pthread_mutex_unlock(&dmtx);
                writen(fd,"OK",3);
            }
            pathtofile=NULL;
        }
        printf("Terminato, attendo prossima op.\n");
        memset(strreceived, 0, MAXMSG);
        read(fd,strreceived,MAXMSG);
        crequest=strtok_r(strreceived, " ", &last);
    }
    printf("Disconnetto client\n");
    client_arr[index]->isonline=0;
    pthread_mutex_lock(&cmtx);
    numclientconn--;
    pthread_mutex_unlock(&cmtx);
    writen(fd, "OK", 3);
    close(fd);
    free(strreceived);
    strreceived=NULL;
    printf("numero client connessi %d\n", numclientconn);
    printf("numero file totali %d\n", numtotfile);
    printf("size totoale %d\n", totsize);
    return (void*)0;
}

void spawnmythread (long arg){
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
    if (pthread_create(&t_id, &t_attr, myworker, (void *) arg) != 0) {
	    perror("Thread create fallita");
	    pthread_attr_destroy(&t_attr);
	    close(arg);
	    return;
    }
    return;
}

static void run_server(struct sockaddr_un * psa){
    long fd_skt, fd_c; /*file descriptor socket e client*/ 
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
    DIR* data = NULL;
    mkdir("./data", 0777);
    if((data = opendir("data"))==NULL){
        perror("error opening data folder");
        exit(EXIT_FAILURE);
    }
    /*setto gestione dei segnali prima di mandare il server in run */
    /*setto maschera che mi dice quali segnali devo bloccare ma non la associo ancora*/
    sigset_t mask;
    sigemptyset(&mask); //azzero la maschera
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);

    /*necessito di modificare la signal mask dei thread che invocherò*/
    if (pthread_sigmask(SIG_BLOCK, &mask,NULL) != 0) { /*la maschera dei bit viene ereditata con SIG_BLOCK ottengo un OR della vecchia e nuova maschera*/
	    perror("errore set mask");
	    return 0;
    }

    pthread_t checksignals_th;
    if (pthread_create(&checksignals_th, NULL, checksignals, &mask) != 0) {
	    perror("errore creazione thread segnali");
	    return 0;
    }
    run_server(&sa);
    pthread_join(checksignals_th, NULL);
    return 0;
}
