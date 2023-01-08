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

void zobrazMapu(char **mapa, int velkostMapy){
    int flag;

    printf("\n\n  ");

    for(int i=0; i<velkostMapy; i++){
        printf("  %d ", i);
    }
    printf("\n  ");

    for(int i=0; i<velkostMapy; i++){
        printf(" ___");
    }
    printf("\n  ");

    for(int i=0; i<velkostMapy*3; i++){
        flag = (i+1) % 3;
        for(int j=0; j<velkostMapy; j++){
            if (flag < 1){
                printf("|___");
            } else if (flag == 2){
                printf("| %c ", mapa[i/3][j]);
            } else  printf("|   ");
        }
        if (flag == 1){
            printf("|\n%d ", i/3);
        } else  printf("|\n  ");
    }

    printf("\n\n");
}
/** vlakna **/
void *hracHraFun(void *arg){
    TAHHRA * dataH = arg;
    int riadok, stlpec;
    char suradnice[3];

    while(!dataH->spoluD->koniecHry){
        pthread_mutex_lock(dataH->spoluD->mutex);
        while(dataH->spoluD->poradieHraca != dataH->spoluD->hracID){
            if (dataH->spoluD->koniecHry){
                break;
            }
            pthread_cond_wait(dataH->spoluD->cakaj,dataH->spoluD->mutex);
        }
        pthread_mutex_unlock(dataH->spoluD->mutex);
        printf("*** Zadajte miesto na hracej ploche ( riadok,stlpec ) ( 0 - %d ) *** \n",dataH->spoluD->velkostMapy-1);  //play

        pthread_mutex_lock(dataH->spoluD->mutex);
        if (!dataH->spoluD->koniecHry){
            do{

                scanf("%s", suradnice);

                riadok = suradnice[0] - '0';
                stlpec = suradnice[2] - '0';

                if (riadok < dataH->spoluD->velkostMapy && riadok >= 0 && stlpec < dataH->spoluD->velkostMapy && stlpec >= 0){
                    if (dataH->spoluD->mapa[riadok][stlpec] == '-'){
                        dataH->spoluD->mapa[riadok][stlpec] = 'X';
                        zobrazMapu(dataH->spoluD->mapa, dataH->spoluD->velkostMapy);
                    } else{
                        printf("Na tuto poziciu uz odpoved bola zadaná!\nVyberte si ine policko!\n");
                    }
                } else {
                    printf("Pozicia mimo dany rozzsah nie je mozna!\nProsim vyberte si policko zo zadaneho rozsahu!\n");
                }

                // send to server
                strcpy(dataH->infoHraca, "CIEL ");
                strcat(dataH->infoHraca, suradnice);
                posliServeru(dataH->spoluD->sockfd, dataH->infoHraca);

                do{
                    citajZoServera(dataH->spoluD->sockfd, dataH->spoluD->infoHry);
                } while (strncmp(dataH->spoluD->infoHry, "CIEL",4));


            } while (strcmp(dataH->spoluD->infoHry, "CIEL_ERROR") == 0);

            posliServeru(dataH->spoluD->sockfd, dataH->spoluD->okInfo);
            do{
                citajZoServera(dataH->spoluD->sockfd, dataH->spoluD->infoVyhry);
            } while (strncmp(dataH->spoluD->infoVyhry, "WINNER", 6));
            if ( strncmp(dataH->spoluD->infoVyhry, "WINNER ", 7) == 0 ){
                dataH->spoluD->koniecHry = true;
            }
            bzero(dataH->spoluD->infoHry, VELKOST_STRINGU);
            bzero(dataH->infoHraca, VELKOST_STRINGU);

            dataH->spoluD->aktualneTahov++;
            dataH->spoluD->poradieHraca++;
            dataH->spoluD->poradieHraca = dataH->spoluD->poradieHraca % dataH->spoluD->pocetHracov;
            if (dataH->spoluD->aktualneTahov == dataH->pocetTahov){
                dataH->spoluD->koniecHry = true;
            }
            if (!dataH->spoluD->koniecHry){
                strcpy(dataH->spoluD->menoHracaNarade, dataH->spoluD->infoVyhry + 9);
            }
        }
        pthread_cond_signal(dataH->spoluD->hraj);
        pthread_mutex_unlock(dataH->spoluD->mutex);

    }

    pthread_exit(NULL);
}