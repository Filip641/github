#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stdbool.h"
#include "pthread.h"
#include "client.h"


void posliServeru(int fd, char *buff1){
    int n;
    n = write(fd,buff1, strlen(buff1)+1);
    if (n < 0){
        perror("Error writing to socket");
    }
}

void citajZoServera(int fd, char *buff2){
    int n;
    do{
        n = read(fd, buff2, 255);
        if (n < 0){
            perror("Error reading from socket");
            break;
        }
    } while( n <= 0);
}
