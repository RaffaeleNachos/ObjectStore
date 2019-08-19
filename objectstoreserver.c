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
#define MAXREGUSR 1024 /*dimensione tabella hash*/

typedef struct _client { //per vedere se il cliente è già registrato basta controllare tramite hashing
    char* name;
    int isonline; //per report
    char* clientdir; //per opendir
    int filecounter; //per report
} clientinfo;

clientinfo *client_arr[MAXREGUSR];

void cleanup(){
    unlink(SOCKNAME);
}

unsigned long hash(char *str){ /*funzione hash basata su stringhe*/
    unsigned long hash = 5381; /*numero primo molto grande*/
    int c;
    while ((c = *str++)) /*per ogni carattere viene associato il suo valore ascii*/
        hash = ((hash << 5) + hash) + c; /* esegue uno shift ciclico a sinistra di 5 posizioni sul valore hash, sarebbe come fare hash * 32 + c */
    return (hash%MAXREGUSR); /*ritorna valore in modulo per far si che index si trovi all'interno dell'hash*/
}

static void* myworker (void* arg){ /*thread detached worker che gestisce un singolo client*/
    int fd = (int)arg;
    int index = -1;
    DIR* cdir = NULL;
    char* strreceived = malloc(MAXMSG*sizeof(char));
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
                client_arr[index]->isonline=1;
                errno=0;
                if ((cdir=opendir(client_arr[index]->clientdir))==NULL) {
	                perror("Apertura dir fallita");
                    write(fd,"KO", 3);
                    continue;
                }
                else write(fd, "OK", 3);
            }
            else{ /* se non è registrato */
                printf("ricevuto utente non registrato\n");
                client_arr[index] = malloc(sizeof(clientinfo));
                client_arr[index]->isonline=1;
                client_arr[index]->name = malloc(MAXMSG*sizeof(char));
                client_arr[index]->clientdir = malloc(UNIX_PATH_MAX*sizeof(char));
                client_arr[index]->filecounter=0;
                strcpy(client_arr[index]->name,crequest);
                if (getcwd(client_arr[index]->clientdir, MAXMSG*sizeof(char)) == NULL) { /*mi faccio restituire la current directory per creare la folder all'interno di data*/
                    perror("getcwd() error");
                    write(fd,"KO", 3);
                    continue;
                }
                strcat(client_arr[index]->clientdir,"/data/");
                strcat(client_arr[index]->clientdir,client_arr[index]->name);
                mkdir(client_arr[index]->clientdir, 0777); /*crea la directory con name*/
                errno=0;
                if ((cdir=opendir(client_arr[index]->clientdir))==NULL) {
	                perror("Apertura dir fallita");
                    write(fd,"KO", 3);
                    continue;
                }
                write(fd, "OK", 3);
            }
        }
        else if (strcmp(crequest,"STORE")==0){
            FILE* tmpfiledesc;
            crequest=strtok_r(NULL, " ", &last);
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char));
            strcpy(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            printf("nome del file %s\n", crequest);
            printf("path %s\n", pathtofile);
            if ((tmpfiledesc=fopen(pathtofile, "wb")) == NULL){
                perror("errore creazione e scrittura file store");
                write(fd, "KO", 3);
            }
            crequest=strtok_r(NULL, " ", &last);
            int dimbyte = strtol(crequest, NULL, 10);
            printf("%d\n", dimbyte);
            crequest=strtok_r(NULL, " ", &last);
            int nread=0;
            nread=strlen(last)*sizeof(char);
            printf("nread da last: %d\n", nread);
            printf("byte da leggere %d\n", dimbyte-nread);
            fwrite(last,nread,1,tmpfiledesc);
            printf("ho scritto %s\n", last);
            char* data = malloc(dimbyte*sizeof(char));
            lseek(fd,0,SEEK_SET);
            int lung=0;
            int btoread=dimbyte-nread;
            while((lung=read(fd,data,btoread))>0){
                printf("%d\n", lung);
                btoread-=lung;
                printf("devo leggere ancora: %d\n", btoread);
                fwrite(data,lung,1,tmpfiledesc);
            }
            fclose(tmpfiledesc);
            client_arr[index]->filecounter++;
            free(data);
            free(pathtofile);
            write(fd,"OK",3);
            pathtofile=NULL;
            data=NULL;
        }
        else if (strcmp(crequest,"RETRIEVE")==0){
            FILE* readfiledesc;
            crequest=strtok_r(NULL, " ", &last);
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char));
            strcpy(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            printf("nome del file %s\n", crequest);
            printf("path %s\n", pathtofile);
            if ((readfiledesc=fopen(pathtofile, "rb")) == NULL){
                perror("errore creazione e scrittura file store");
                write(fd, "KO", 3);
                continue;
            }
            int inputfd = fileno(readfiledesc);
            struct stat fst;
            fstat(inputfd, &fst);
            int dimbyte = fst.st_size;
            char* buffer = malloc(dimbyte*sizeof(char));
            fread(buffer,dimbyte,1,readfiledesc);
            dprintf(fd, "DATA %d \n %s", dimbyte, buffer);
            free(pathtofile);
            free(buffer);
            pathtofile=NULL;
            buffer=NULL;
        }
        else if (strcmp(crequest,"DELETE")==0){
            crequest=strtok_r(NULL, " ", &last);
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char));
            strcpy(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            printf("%s\n", pathtofile);
            if(remove(pathtofile)!=0){
                perror("errore delete server");
                write(fd,"KO",3);
            }
            else {
                client_arr[index]->filecounter--;
                free(pathtofile);
                write(fd,"OK",3);
            }
            pathtofile=NULL;
        }
        printf("ho terminato, immettere prossima op\n");
        memset(strreceived, 0, MAXMSG);
        read(fd,strreceived,MAXMSG);
        crequest=strtok_r(strreceived, " ", &last);
    }
    printf("Disconnetto client\n");
    client_arr[index]->isonline=0;
    if((closedir(cdir))==-1){
        perror("Chiusura dir fallita");
        write(fd, "KO", 3);
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
    DIR* data = NULL;
    mkdir("./data", 0777);
    if((data = opendir("data"))==NULL){
        perror("error opening data folder");
        exit(EXIT_FAILURE);
    }
    run_server(&sa);
    return 0;
}
