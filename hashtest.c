#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long hash(char *str){
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

int main(){
    char* str = malloc(1024*sizeof(char));
    int *array = calloc(1024, sizeof(int));
    scanf("%s", str);
    while(strcmp(str,"quit")!=0){
        int index = hash1(str)%1024;
        if (array[index]==1){
            perror("trovato");
        }
        else array[index]=1;
        scanf("%s", str);
    }
    return 0;
}