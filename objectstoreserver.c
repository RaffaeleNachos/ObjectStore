/**
 * @file objectstoreserver.c
 * @author Raffaele Apetino - Matricola 549220 (r.apetino@studenti.unipi.it)
 * @brief 
 * server object store
 * @version 4.0 final? I hope so...
 * it wasn't..
 * @version 5.0 final
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

#define UNIX_PATH_MAX 108
#define SOCKNAME "./objstore.sock"
#define MAXMSG 128
#define MAXREGUSR 1024 /*dimensione tabella hash*/

static pthread_mutex_t cmtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t fmtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dmtx = PTHREAD_MUTEX_INITIALIZER;

static volatile sig_atomic_t fire_alarm = 0;
static volatile sig_atomic_t numclientconn = 0;
static volatile sig_atomic_t numtotfile = 0;
static volatile sig_atomic_t totsize = 0;
/* atomic relative to signal handling
per garantire la comunicazione con il signal handler, i dati devono essere consistenti.
la caratteristica di sig_atomic_t è il fatto che esso è garantito essere letto e scritto in una sola operazione*/

typedef struct _client { //per vedere se il cliente è già registrato basta controllare tramite hashing
    char* name;
    int isonline; //per controllare che non sia già online
    char* clientdir;
} clientinfo;

clientinfo *client_arr[MAXREGUSR]; //la mia tabella hash ad indirizzamento diretto

void cleanup(){
    unlink(SOCKNAME);
}

static void* checksignals(void *arg) {
    sigset_t *set = (sigset_t*)arg;
    
    struct sigaction s; /*struct per ignorare SIGPIPE*/
    memset(&s, 0, sizeof(s));
    s.sa_handler=SIG_IGN; /*ignore SIGPIPE*/
    sigaction(SIGPIPE,&s,NULL);

    while(1) {
	    int sig;
	    int rterrn = sigwait(set, &sig);
	    if (rterrn != 0) {
	        errno = rterrn;
	        perror("errore sigwait");
	        return NULL;
	    }
	    switch(sig) {
	    case SIGINT:
			//printf("ricevuto segnale sigint\n");
            fire_alarm=1;
            return NULL;
	    case SIGQUIT:
			//printf("ricevuto segnale sigquit\n");
            fire_alarm=1;
            return NULL;
        case SIGTERM:
			//printf("ricevuto segnale sigterm\n");
            fire_alarm=1;
            return NULL;
	    case SIGSEGV:
            //printf("ricevuto segnale sigsegv\n");
            writen(1, "Errore grave!, memoria non consistente!", 40);
            fire_alarm=1;
            return NULL;
        case SIGUSR1:
			//printf("ricevuto segnale sigusr1\n"); 
            fire_alarm=2;
            return NULL;
	    default:    ; 
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
    char* strreceived = malloc(MAXMSG*sizeof(char));
    char* crequest = NULL;
    char* last = NULL;
    if(read(fd,strreceived,MAXMSG)<0){
        perror("impossibile leggere richiesta");
        writen(fd, "KO", 3);
    }
    crequest=strtok_r(strreceived, " ", &last);
    while(strcmp(crequest,"LEAVE")!=0){
        if (strcmp(crequest,"REGISTER")==0){
            crequest=strtok_r(NULL, " ", &last); //leggo name
            printf("Registro %s\n", crequest);
            index = hash(crequest); /*calcolo indice nella tabella hash*/
            if((client_arr[index]!=NULL)){ /* se è già registrato vuol dire che c'è una entry nella tabella*/
                if(client_arr[index]->isonline==1){
                    perror("Ricevuto utente già online");
                    writen(fd, "KO", 3);
                    continue;
                }
                else {
                    client_arr[index]->isonline=1;
                    pthread_mutex_lock(&cmtx);
                    numclientconn++;
                    pthread_mutex_unlock(&cmtx);
                    writen(fd, "OK", 3);
                }
            }
            else{ /* se non è registrato */
                //printf("ricevuto utente non registrato\n");
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
            int nread = 6; //byte STORE + spazio
            long tmpfiledesc;
            crequest=strtok_r(NULL, " ", &last); //leggo nome del file
            nread=nread+strlen(crequest)+1; //byte name + spazio
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char));
			memset(pathtofile, 0, UNIX_PATH_MAX);
            strcpy(pathtofile, client_arr[index]->clientdir); //è la directory all'interno di data con nome name
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            //printf("path %s\n", pathtofile);
            if ((tmpfiledesc=open(pathtofile, O_WRONLY | O_CREAT, 0777)) == -1){
                perror("errore creazione e scrittura file store");
                writen(fd,"KO", 3);
            }
            crequest=strtok_r(NULL, " ", &last); //leggo len
            nread=nread+strlen(crequest)+1; //byte len + spazio
            int dimbyte = strtol(crequest, NULL, 10);
            crequest=strtok_r(NULL, " ", &last); //"\n"
            nread=nread+strlen(crequest)+1; //byte \n + spazio
            //printf("n byte letti dall'header: %d\n", nread);
            nread=MAXMSG-nread; //nread diventa la parte restante dell'header
            if(dimbyte<=nread){
                writen(tmpfiledesc, last, dimbyte);
            }
            else {
                writen(tmpfiledesc, last, nread);
            }
            //fino a qui ho letto parte dei dati (o tutti) e ho scritto quella parte sul file.
            int btoread=dimbyte-nread; //sono i byte che devo ancora leggere, cioè len-byte letti dall'header
            if (btoread>0){
                char* data = malloc(btoread*sizeof(char));
			    memset(data, 0, btoread);
                readn(fd,data,btoread);
                writen(tmpfiledesc,data,btoread);
                close(tmpfiledesc);
                free(data);
                data=NULL;
            }
            close(tmpfiledesc);
            free(pathtofile);
            pathtofile=NULL;
            pthread_mutex_lock(&fmtx);
            numtotfile++;
            pthread_mutex_unlock(&fmtx);
            pthread_mutex_lock(&dmtx);
            totsize+=dimbyte;
            pthread_mutex_unlock(&dmtx);
            writen(fd,"OK",3);
        }
        else if (strcmp(crequest,"RETRIEVE")==0){
            int readfiledesc;
            crequest=strtok_r(NULL, " ", &last);
            char* pathtofile = malloc(UNIX_PATH_MAX*sizeof(char));
            memset(pathtofile, 0, UNIX_PATH_MAX);
            strcpy(pathtofile, client_arr[index]->clientdir);
            strcat(pathtofile, "/");
            strcat(pathtofile, crequest);
            //printf("nome del file %s\n", crequest);
            //printf("path %s\n", pathtofile);
            if ((readfiledesc=open(pathtofile, O_RDONLY)) == -1){
                perror("errore creazione e scrittura file retrieve");
                writen(fd,"KO", 3);
                continue;
            }
            struct stat fst;
            fstat(readfiledesc, &fst);
            int dimbyte = fst.st_size;
            char* buffer = malloc(dimbyte*sizeof(char)+1);
            memset(buffer, 0, dimbyte+1);
            readn(readfiledesc,buffer,dimbyte);
            dprintf(fd, "DATA %d \n %s", dimbyte, buffer);
            close(readfiledesc);
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
            //printf("%s\n", pathtofile);
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
        //printf("Terminato, attendo prossima op.\n");
        memset(strreceived, 0, MAXMSG);
        crequest=NULL;
        last=NULL;
        if(read(fd,strreceived,MAXMSG)<0){
            perror("impossibile leggere richiesta");
            writen(fd, "KO", 3);
        }
        crequest=strtok_r(strreceived, " ", &last);
    }
    printf("Disconnetto %s\n", client_arr[index]->name);
    client_arr[index]->isonline=0;
    pthread_mutex_lock(&cmtx);
    numclientconn--;
    pthread_mutex_unlock(&cmtx);
    writen(fd, "OK", 3);
    close(fd);
    free(strreceived);
    strreceived=NULL;
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
    /*creazione thread in modalità detached così da non dover fare la th_wait */
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
    long fd_skt = -1;
    long fd_c = -1; /*file descriptor socket e client*/ 
    errno = 0;
    if ((fd_skt = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
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
    while(fire_alarm==0){
        errno=0;
        if ((fd_c = accept(fd_skt, NULL, 0)) == -1){ /*da qui uso fd_c del client accettato */
			/*socket server non blocking
			socket:"MI HAI DETTO DI NON ASPETTARE! ti mando EAGAIN come segnale." */
            if(errno==EAGAIN){ /*essendo la socket del server non blocking ti avvisa che le hai detto di non aspettare ma non ha più nulla da fare*/
                continue;
            }
            perror("Non è stato possibile creare un nuovo socket per la comunicazione");
            exit(EXIT_FAILURE);
        }
        spawnmythread(fd_c);
    }
    close(fd_skt);
}

int main (void) {
    cleanup(); //unlink
    atexit(cleanup); //anche nel caso di uscita
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if(mkdir("./data", 0777)==-1){
        perror("directory già creata");
    }

    /*setto gestione dei segnali prima di mandare il server in run */
    /*setto maschera che mi dice quali segnali devo bloccare ma non la associo ancora*/
    /*i segnali che non blocco possono far terminare il mio processo*/
    sigset_t mask;
    sigemptyset(&mask); //azzero la maschera
    sigaddset(&mask, SIGINT); 
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGSEGV);

    /*necessito di modificare la signal mask dei thread che invocherò così da non far terminare i thread se ricevo i segnali con bit a 1*/
    if (pthread_sigmask(SIG_SETMASK, &mask,NULL) != 0) { /*la maschera dei bit viene ereditata con SIG_BLOCK ottengo un OR della vecchia e nuova maschera*/
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
    if(fire_alarm!=0){
        if(fire_alarm==2){
            printf("numero client connessi %d\n", numclientconn);
            printf("numero file totali %d\n", numtotfile);
            printf("size totale in byte %d\n", totsize);
        }
        for (int i=0; i<MAXREGUSR; i++) {
            if(client_arr[i]!=NULL){
                free(client_arr[i]->clientdir);
                free(client_arr[i]->name);
                free(client_arr[i]);
            }
        }
    }
    exit(EXIT_SUCCESS);
    return 0;
}
