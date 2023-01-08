//
// Created by User001 on 8.1.2023.
//

#ifndef GITHUB_SERVER_H
#define GITHUB_SERVER_H

#include <pthread.h>

#define VELKOST_STRINGU 256
#define VELKOST_MENA 30

typedef struct server{
    int pocetHracov;
    int maximalnyPocHracov;

    int vyherca;
    char *menoVyhercu;
    int preslo;

    int zacinajuciHrac;
    char **mapaHry;
    int velkostMapy;
    char *infoVyhry;

    pthread_barrier_t *cakaj;
    pthread_mutex_t *mutex;
}SERVERD;

typedef struct hrac{
    int idHraca;
    int sockfd;

    char *infoHraca;
    char *infoHry;
    char *okInfo;

    SERVERD * spoluD;
}HRACD;

int server(int argc, char *argv[]);
#endif //GITHUB_SERVER_H
