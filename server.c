#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "pthread.h"
#include "server.h"

void citajHraca(int fd, char *buff1){
    int n;
    bzero(buff1,VELKOST_STRINGU);
    do{
        n = read(fd, buff1, 255);
        if ( n < 0 ){
            perror("Error reading from socket");
            break;
        }
    } while( n <= 0);
}
void zapisHracovi(int fd, char *buff2){
    int n;

    n = write(fd, buff2, strlen(buff2)+1);
    if (n < 0){
        perror("Error writing to socket");
    }
}

void *hraciFun(void *arg){
    HRACD * hracD = arg;
    int fd = hracD->sockfd;
    int idHraca = hracD->idHraca;
    int newsockfd;

    char menoHraca[VELKOST_MENA];
    char c[2];
    char suradnice[3];
    int riadok,stlpec;
    int n;

    int potrebnePripojit;
    int pocetTahov;
    bool koniecHry = false;
    int tahy = 0;
    char *bufferWin = malloc(VELKOST_STRINGU * sizeof(char));
    strcpy(bufferWin, "WINNER_NO");

    struct sockaddr_in cli_addr;
    socklen_t cli_len;

    cli_len = sizeof(cli_addr);
    newsockfd = accept(fd, (struct sockaddr*)&cli_addr, &cli_len);
    if (newsockfd < 0)
    {
        perror("ERROR on accept");
    }

    sprintf(hracD->infoHry,"%d",hracD->spoluD->maximalnyPocHracov); //poslanie velkosti mapy
    strcat(hracD->infoHry, " ");
    sprintf(hracD->infoHry+2,"%d",hracD->spoluD->zacinajuciHrac); //poslanie zacinajuceho hraca
    strcat(hracD->infoHry, " ");
    sprintf(hracD->infoHry+4,"%d",idHraca); //poslanie poctu hracov
    strcat(hracD->infoHry, " ");
    sprintf(hracD->infoHry+6,"%d",hracD->spoluD->velkostMapy); //poslanie poctu hracov
    zapisHracovi(newsockfd,hracD->infoHry);

    citajHraca(newsockfd, hracD->infoHraca);
    strcpy(menoHraca, hracD->infoHraca);

    printf("** Pripojil sa hrac s ID(-%d-) s menom : (%s)\n", idHraca, menoHraca);

    /* Use mutex to prevent fault */
    pthread_mutex_lock(hracD->spoluD->mutex);
    hracD->spoluD->pocetHracov++; // Ked sa novy hrac pripoji
    potrebnePripojit = hracD->spoluD->maximalnyPocHracov - hracD->spoluD->pocetHracov;
    pocetTahov = hracD->spoluD->velkostMapy * hracD->spoluD->velkostMapy;
    pthread_mutex_unlock(hracD->spoluD->mutex);
    /* connected to client! */

    if(potrebnePripojit > 1){
        printf("Caka sa na %d hracov...\n",potrebnePripojit);
    } else if (potrebnePripojit == 1){
        printf("Caka sa na posledneho hraca...\n");
    } else {
        printf("Hra sa spusta!...\n\n");
    }

    pthread_barrier_wait(hracD->spoluD->cakaj);

    pthread_mutex_lock(hracD->spoluD->mutex);
    if(idHraca == hracD->spoluD->zacinajuciHrac){
        strcpy(hracD->spoluD->menoVyhercu, menoHraca);
    }
    pthread_mutex_unlock(hracD->spoluD->mutex);

    /** **********     Zaciatok hry     ************ **/

    /* Ked sa vsetci hraci pripoja */
    sleep(1);
    strcpy(hracD->infoHry, "ZACINA SA HRA:");
    strcat(hracD->infoHry, hracD->spoluD->menoVyhercu);
    zapisHracovi(newsockfd, hracD->infoHry);


    while (!koniecHry) {
        //sleep(1);
        n = 1; // kontrola

        strcpy(hracD->okInfo,"");


        if ( idHraca == hracD->spoluD->zacinajuciHrac ){
            pthread_mutex_lock(hracD->spoluD->mutex);
            hracD->spoluD->preslo = 0;
            pthread_mutex_unlock(hracD->spoluD->mutex);

            printf(" Na rade je hrac '%s' s ID (-%d-)\n", menoHraca,idHraca);

            do{
                do{
                    citajHraca(newsockfd, hracD->infoHry);
                } while (strncmp(hracD->infoHry, "CIEL ",5));

                strcpy(suradnice, hracD->infoHry + 5);
                riadok = suradnice[0] - '0';
                stlpec = suradnice[2] - '0';

                if ((riadok < 0) || (riadok > hracD->spoluD->velkostMapy - 1) || (stlpec < 0) || (stlpec > hracD->spoluD->velkostMapy - 1)){
                    n = 0;
                    strcpy(hracD->infoHraca, "CIEL_ERROR");
                } else if (hracD->spoluD->mapaHry[riadok][stlpec] != '-' ){
                    n = 0;
                    strcpy(hracD->infoHraca, "CIEL_ERROR");
                } else {
                    strcpy(hracD->infoHraca, "CIEL_OK");
                    n = 1;
                }
                zapisHracovi(newsockfd, hracD->infoHraca);

            } while(n == 0);

            pthread_mutex_lock(hracD->spoluD->mutex);
            if (idHraca == 0) {
                hracD->spoluD->mapaHry[riadok][stlpec] = 'X';
            }else if(idHraca == 1){
                hracD->spoluD->mapaHry[riadok][stlpec] = 'O';
            }else {
                hracD->spoluD->mapaHry[riadok][stlpec] = 'Z';
            }
            pthread_mutex_unlock(hracD->spoluD->mutex);

            // Winner check
            for(int i=0; i< hracD->spoluD->velkostMapy - 2; i++){
                for(int j=0; j<hracD->spoluD->velkostMapy; j++){
                    if (hracD->spoluD->mapaHry[i][j] == 'X' && hracD->spoluD->mapaHry[i + 1][j] == 'X' && hracD->spoluD->mapaHry[i + 2][j] == 'X' ) strcpy(bufferWin, "WINNER 0.");
                    if (hracD->spoluD->mapaHry[i][j] == 'O' && hracD->spoluD->mapaHry[i + 1][j] == 'O' && hracD->spoluD->mapaHry[i + 2][j] == 'O' ) strcpy(bufferWin, "WINNER 1.");
                    if (hracD->spoluD->mapaHry[i][j] == 'Z' && hracD->spoluD->mapaHry[i + 1][j] == 'Z' && hracD->spoluD->mapaHry[i + 2][j] == 'Z' ) strcpy(bufferWin, "WINNER 2.");
                }
            }
            for(int i=0; i<hracD->spoluD->velkostMapy; i++){
                for(int j=0; j< hracD->spoluD->velkostMapy - 2; j++){
                    if (hracD->spoluD->mapaHry[i][j] == 'X' && hracD->spoluD->mapaHry[i][j + 1] == 'X' && hracD->spoluD->mapaHry[i][j + 2] == 'X' ) strcpy(bufferWin, "WINNER 0.");
                    if (hracD->spoluD->mapaHry[i][j] == 'O' && hracD->spoluD->mapaHry[i][j + 1] == 'O' && hracD->spoluD->mapaHry[i][j + 2] == 'O' ) strcpy(bufferWin, "WINNER 1.");
                    if (hracD->spoluD->mapaHry[i][j] == 'Z' && hracD->spoluD->mapaHry[i][j + 1] == 'Z' && hracD->spoluD->mapaHry[i][j + 2] == 'Z' ) strcpy(bufferWin, "WINNER 2.");                }
            }
            for(int i=0; i< hracD->spoluD->velkostMapy - 2; i++){
                for(int j=0; j< hracD->spoluD->velkostMapy - 2; j++){
                    if (hracD->spoluD->mapaHry[i][j] == 'X' && hracD->spoluD->mapaHry[i + 1][j + 1] == 'X' && hracD->spoluD->mapaHry[i + 2][j + 2] == 'X' ) strcpy(bufferWin, "WINNER 0.");
                    if (hracD->spoluD->mapaHry[i][j] == 'O' && hracD->spoluD->mapaHry[i + 1][j + 1] == 'O' && hracD->spoluD->mapaHry[i + 2][j + 2] == 'O' ) strcpy(bufferWin, "WINNER 1.");
                    if (hracD->spoluD->mapaHry[i][j] == 'Z' && hracD->spoluD->mapaHry[i + 1][j + 1] == 'Z' && hracD->spoluD->mapaHry[i + 2][j + 2] == 'Z' ) strcpy(bufferWin, "WINNER 2.");                }
            }
            for(int i=2; i<hracD->spoluD->velkostMapy; i++){
                for(int j=0; j< hracD->spoluD->velkostMapy - 2; j++){
                    if (hracD->spoluD->mapaHry[i][j] == 'X' && hracD->spoluD->mapaHry[i - 1][j + 1] == 'X' && hracD->spoluD->mapaHry[i - 2][j + 2] == 'X' ) strcpy(bufferWin, "WINNER 0.");
                    if (hracD->spoluD->mapaHry[i][j] == 'O' && hracD->spoluD->mapaHry[i - 1][j + 1] == 'O' && hracD->spoluD->mapaHry[i - 2][j + 2] == 'O' ) strcpy(bufferWin, "WINNER 1.");
                    if (hracD->spoluD->mapaHry[i][j] == 'Z' && hracD->spoluD->mapaHry[i - 1][j + 1] == 'Z' && hracD->spoluD->mapaHry[i - 2][j + 2] == 'Z' ) strcpy(bufferWin, "WINNER 2.");                }
            }

            pthread_mutex_lock(hracD->spoluD->mutex);
            strcpy(hracD->spoluD->infoVyhry, bufferWin);
            pthread_mutex_unlock(hracD->spoluD->mutex);
        }
        if ( idHraca != hracD->spoluD->zacinajuciHrac ){
            //do {
            citajHraca(newsockfd, hracD->okInfo);
            printf("OK--%d\n",idHraca);
            //}while (strcmp(hracD->okInfo, "OK"));

            while (strcmp(hracD->infoHraca, "CIEL_OK")){
                printf("OK--CIEL%d\n",idHraca);
            }
            //sleep(1);

            zapisHracovi(newsockfd, hracD->infoHry);

        }

        //do {
        citajHraca(newsockfd, hracD->okInfo);
        printf("OK%d\n",idHraca);
        //}while (strcmp(hracD->okInfo, "OK"));



        pthread_barrier_wait(hracD->spoluD->cakaj); // Cakaj na ostatnych

        pthread_mutex_lock(hracD->spoluD->mutex);
        if (hracD->spoluD->preslo == 0){
            hracD->spoluD->zacinajuciHrac++;
            hracD->spoluD->zacinajuciHrac = hracD->spoluD->zacinajuciHrac % hracD->spoluD->maximalnyPocHracov;
            hracD->spoluD->preslo++;
        }
        if (hracD->spoluD->zacinajuciHrac == idHraca){
            strcpy(hracD->spoluD->menoVyhercu,menoHraca);
            strcat(hracD->spoluD->infoVyhry, hracD->spoluD->menoVyhercu);
            //printf("%s\n",hracD->spoluD->menoVyhercu);
        }

        pthread_mutex_unlock(hracD->spoluD->mutex);

        pthread_barrier_wait(hracD->spoluD->cakaj); // Cakaj na ostatnych

        pthread_mutex_lock(hracD->spoluD->mutex);
        strcpy(hracD->infoHraca, " ");
        zapisHracovi(newsockfd, hracD->spoluD->infoVyhry);

        if ( strncmp(hracD->spoluD->infoVyhry, "WINNER ", 7) == 0 ){
            koniecHry = true;
            hracD->spoluD->vyherca = atoi(hracD->spoluD->infoVyhry + 7);
            if (hracD->spoluD->vyherca == idHraca){
                strcpy(hracD->spoluD->menoVyhercu,menoHraca);
            }
        }
        pthread_mutex_unlock(hracD->spoluD->mutex);
        printf("OK%d\n",idHraca);
        tahy++;

        //pthread_barrier_wait(hracD->spoluD->cakaj);
        if (tahy == pocetTahov) {
            koniecHry = true;
        }

    }

    free(bufferWin);
    bufferWin = NULL;
    close(newsockfd);


    /** ******************************************** **/



    pthread_exit(NULL);
}


int server(int argc, char *argv[]){
    srand(time(NULL));
    int sockfd;
    struct sockaddr_in serv_addr;

    if (argc < 2)
    {
        fprintf(stderr,"usage %s port\n", argv[0]);
        return 1;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        return 1;
    }

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Error binding socket address");
        return 2;
    }
    listen(sockfd, 5);

    char *infoHraca = malloc(VELKOST_STRINGU* sizeof(char));
    char *infoHry = malloc(VELKOST_STRINGU* sizeof(char));
    char *infoVyhry = malloc(VELKOST_STRINGU* sizeof(char));
    char *okInfo = malloc(VELKOST_STRINGU* sizeof(char));
    char *menoVyhercu = malloc(VELKOST_MENA* sizeof(char));

    strcpy(infoVyhry, "WINNER_NO");

    int maxPocetHracov = 0;
    int velkostMapy = 0;
    int zacinajuci = 0;

    printf("------------------------------------------------------\n");
    printf("Server sa  spustil, nastavte parametre hry.\n");
    printf("------------------------------------------------------\n\n");

    while (maxPocetHracov < 2 || maxPocetHracov > 3){
        printf("*** Vyberte z maximalneho poctu hracov (2-3) ***\n");
        scanf("%d",&maxPocetHracov);
        if (maxPocetHracov < 2 || maxPocetHracov > 3){
            printf("Zadali ste nespravnu volbu, zadajte znova !\n");
        }
    }

    if (maxPocetHracov == 2){
        while (velkostMapy < 3 || velkostMapy > 9){
            printf("*** Vyberte z rozmedzia velkosti mapy, minimalne (3), maximalne (9) ***\n");
            scanf("%d",&velkostMapy);
            if (velkostMapy < 3 || velkostMapy > 9){
                printf("Zadali ste nespravnu volbu, zadajte znova !\n");
            }
        }
    } else{
        while (velkostMapy < 5 || velkostMapy > 9){
            printf("*** Vyberte z rozmedzia velkosti mapy, minimalne (5), maximalne (9) ***\n");
            scanf("%d",&velkostMapy);
            if (velkostMapy < 5 || velkostMapy > 9){
                printf("Zadali ste nespravnu volbu, zadajte znova !\n");
            }
        }
    }

    zacinajuci = rand() % maxPocetHracov;
    char **mapa = malloc(velkostMapy * sizeof(char *));
    for (int i = 0; i < velkostMapy; ++i) {
        mapa[i] = malloc(velkostMapy* sizeof(char));
        for(int j=0; j<velkostMapy; j++){
            mapa[i][j] = '-' ;
        }
    }

    printf("------------------------------------------------------\n");
    printf("Hra bola uspesne nastavena s parametrami:\npocet hracov %d\nvelkost mapy:%dx%d\n",maxPocetHracov,velkostMapy,velkostMapy);
    printf("------------------------------------------------------\n\n");

    pthread_t hraci[maxPocetHracov], kontrola;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex,NULL);
    pthread_barrier_t cakajHracov;
    pthread_barrier_init(&cakajHracov,NULL,maxPocetHracov);

    SERVERD servD = {0, maxPocetHracov, 0, menoVyhercu, 0, zacinajuci, mapa, velkostMapy, infoVyhry, &cakajHracov, &mutex};
    HRACD * hraciD = malloc(maxPocetHracov * sizeof(HRACD));

    printf("Caka sa na %d hracov...\n",servD.maximalnyPocHracov);

    /* Pthread 'create' / 'join' */
    for (int i = 0; i < maxPocetHracov; i++) {
        hraciD[i].idHraca = i;
        hraciD[i].sockfd = sockfd;
        hraciD[i].infoHraca = infoHraca;
        hraciD[i].infoHry = infoHry;
        hraciD[i].okInfo = okInfo;
        hraciD[i].spoluD = &servD;
        pthread_create(&hraci[i], NULL, hraciFun, &hraciD[i]);
    }


    for (int i = 0; i < maxPocetHracov; i++) {
        pthread_join(hraci[i], NULL);
    }
    /* ************************ */



    if (strncmp(infoVyhry,"WINNER ",7) == 0){
        printf("\n$$$ Vyhral hrac menom (-%s-) $$$\n\n", servD.menoVyhercu);
    } else{
        printf("\n$$$ V tomto kole nevyhral nikto! $$$\n\n");
    }

    printf("------------------------------------------------------\n");
    printf("Server Skoncil\n");
    printf("------------------------------------------------------\n\n");

    free(infoVyhry);
    infoVyhry = NULL;
    free(infoHry);
    infoHry = NULL;
    free(infoHraca);
    infoHraca = NULL;
    free(okInfo);
    okInfo = NULL;
    free(menoVyhercu);
    menoVyhercu = NULL;

    free(hraciD);
    hraciD = NULL;
    for (int i = 0; i < velkostMapy; ++i) {
        free(mapa[i]);
        mapa[i] = NULL;
    }
    free(mapa);
    mapa = NULL;
    close(sockfd);

    return 0;
}