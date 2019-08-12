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

#define UNIX_PATH_MAX 104 /* lunghezza massima consentita per il path */
#define SOCKNAME "./objstore.sock"
#define MAXMSG 128

void cleanup(){
    unlink(SOCKNAME);
}

static void* myworker (void* arg){
    int fd = (int)arg;
    char* crequest = malloc(MAXMSG*sizeof(char));
    read(fd,crequest,MAXMSG);
    printf("Server got: %s\n", crequest);
    write(fd, "ok", 3);
    close(fd);
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
