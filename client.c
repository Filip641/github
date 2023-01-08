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
void *protihracHraFun(void *arg){
    TAHPRO * dataP = arg;
    int riadok, stlpec;
    char suradnice[3];
    int protihrac = 0;

    while(!dataP->spoluD->koniecHry) {
        pthread_mutex_lock(dataP->spoluD->mutex);
        while(dataP->spoluD->poradieHraca == dataP->spoluD->hracID){
            pthread_cond_wait(dataP->spoluD->hraj, dataP->spoluD->mutex);
        }
        pthread_mutex_unlock(dataP->spoluD->mutex);
        printf("Cakaj kym odohra protihrac menom (-%s-)...\n", dataP->spoluD->menoHracaNarade);

        pthread_mutex_lock(dataP->spoluD->mutex);
        if (!dataP->spoluD->koniecHry){

            posliServeru(dataP->spoluD->sockfd, dataP->spoluD->okInfo);
            do{
                citajZoServera(dataP->spoluD->sockfd, dataP->spoluD->infoHry);
            } while (strncmp(dataP->spoluD->infoHry, "CIEL ",5));

            strcpy(suradnice, dataP->spoluD->infoHry + 5);

            riadok = suradnice[0] - '0';
            stlpec = suradnice[2] - '0';
            if (dataP->spoluD->pocetHracov > 2){
                if (protihrac == 0){
                    dataP->spoluD->mapa[riadok][stlpec] = 'O';
                    protihrac++;
                } else{
                    dataP->spoluD->mapa[riadok][stlpec] = 'Z';
                    protihrac = 0;
                }
            } else {
                dataP->spoluD->mapa[riadok][stlpec] = 'O';
            }

            zobrazMapu(dataP->spoluD->mapa, dataP->spoluD->velkostMapy);

            posliServeru(dataP->spoluD->sockfd, dataP->spoluD->okInfo);
            do{
                citajZoServera(dataP->spoluD->sockfd, dataP->spoluD->infoVyhry);
            } while (strncmp(dataP->spoluD->infoVyhry, "WINNER", 6));
            if ( strncmp(dataP->spoluD->infoVyhry, "WINNER ", 7) == 0 ){
                dataP->spoluD->koniecHry = true;
            }
            bzero(dataP->spoluD->infoHry, VELKOST_STRINGU);

            dataP->spoluD->aktualneTahov++;
            dataP->spoluD->poradieHraca++;
            dataP->spoluD->poradieHraca = dataP->spoluD->poradieHraca % dataP->spoluD->pocetHracov;
            if (dataP->spoluD->aktualneTahov == dataP->pocetTahov){
                dataP->spoluD->koniecHry = true;
            }
            if (!dataP->spoluD->koniecHry){
                strcpy(dataP->spoluD->menoHracaNarade, dataP->spoluD->infoVyhry + 9);
            }
        }
        pthread_cond_signal(dataP->spoluD->cakaj);
        pthread_mutex_unlock(dataP->spoluD->mutex);
    }
    pthread_exit(NULL);
}
/** ******* **/

int client(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    if (argc < 3)
    {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        return 1;
    }

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        return 2;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(atoi(argv[2]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        return 3;
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error connecting to socket");
        return 4;
    }


    char infoHraca[VELKOST_STRINGU];
    char infoHry[VELKOST_STRINGU];
    char infoVyhry[VELKOST_STRINGU];
    char menoHraca[VELKOST_MENA];
    char menoHracaNaRade[VELKOST_MENA];
    char okInfo[3];
    strcpy(okInfo, "OK");


    int zacinajuciHrac = 0;
    int pocetHracov = 0;
    int velkostMapy = 0;
    int hracID = 0;
    int pocetTahov;


    citajZoServera(sockfd, infoHry); // zistanie velkosti mapy
    //printf("%s\n",infoHry);
    pocetHracov = atoi(strtok(infoHry, " "));
    zacinajuciHrac = atoi(strtok(infoHry+2, " "));
    hracID = atoi(strtok(infoHry+4, " "));
    velkostMapy = atoi(infoHry+6);
    //printf("%d--%d--%d\n",velkostMapy,zacinajuciHrac,pocetHracov);
    pocetTahov = velkostMapy*velkostMapy;

    char **mapa = malloc(velkostMapy * sizeof(char *));
    for (int i = 0; i < velkostMapy; i++) {
        mapa[i] = malloc(velkostMapy* sizeof(char));
        for(int j=0; j<velkostMapy; j++){
            mapa[i][j] = '-' ;
        }
    }

    pthread_t hracHra,protihracHra;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_t hraj, cakaj;
    pthread_cond_init(&hraj,NULL);
    pthread_cond_init(&cakaj,NULL);


    printf("*** Prosim zadaj svoje meno: \n");
    scanf("%s", menoHraca);
    strcpy(infoHraca, menoHraca);

    posliServeru(sockfd, infoHraca);  // send the name to server

    printf("Vitaj %s, si hrac s cislom: %d \n", menoHraca, hracID);

    SPOLUD spoluD = {pocetHracov, zacinajuciHrac, velkostMapy, 0, false, menoHracaNaRade,hracID,sockfd,  mapa, infoHry, infoVyhry,okInfo, &mutex, &hraj, &cakaj};
    TAHHRA tahHracaD = { infoHraca, pocetTahov, &spoluD};
    TAHPRO tahProtihracaD = {  pocetTahov,&spoluD};

    do{
        citajZoServera(sockfd, infoHry);
    } while (strncmp(infoHry, "ZACINA SA HRA:",14));
    printf("ZACINA SA HRA:\n");
    strcpy(menoHracaNaRade, infoHry + 14);

    pthread_create(&hracHra, NULL, hracHraFun, &tahHracaD);
    pthread_create(&protihracHra, NULL, protihracHraFun, & tahProtihracaD);

    // JOIN PTHREADS
    pthread_join(hracHra, NULL);
    pthread_join(protihracHra, NULL);

    if (strncmp(infoVyhry,"WINNER ",7) == 0){
        if (strcmp(menoHraca,menoHracaNaRade) == 0){
            printf("\n$$$ Gratuluujem Vyhrali ste! $$$\n\n");
        }else{
            printf("\n$$$ Vyhral hrac menom (-%s-) $$$\n\n", menoHracaNaRade);
        }
    }else {
        printf("\n$$$ V tomto kole nevyhral nikto! $$$\n\n");
    }


    for (int i = 0; i < velkostMapy; ++i) {
        free(mapa[i]);
        mapa[i] = NULL;
    }
    free(mapa);
    mapa = NULL;
    close(sockfd);

    return 0;
}
