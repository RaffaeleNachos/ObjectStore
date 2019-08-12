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

int main(){
    int registered = 0;
    int fd_skt;
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
    char* command = malloc(MAXMSG*sizeof(char));
    char* response = malloc(MAXMSG*sizeof(char));
    scanf("%s", command);
    while((strcmp(command,"LEAVE"))!=0){
        if ((strcmp(command,"REGISTER"))!=0 && registered==0){
            printf("Non è possibile effettuare operazione se non si è prima registrati\n");
            scanf("%s", command);
            continue;
        }
        if ((strcmp(command,"REGISTER"))==0){
            if (registered==0){
                registered=1;
                write(fd_skt, command, MAXMSG);
                read(fd_skt, response, MAXMSG);
                printf("Client got: %s\n", response);
            }
            else{
                printf("Sei già registrato!\n");
            }
        }
        scanf("%s", command);
    }
    close(fd_skt);
    exit(EXIT_SUCCESS); 
}