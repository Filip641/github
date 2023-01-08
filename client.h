//
// Created by User001 on 5.1.2023.
//

#ifndef POSSEMPRACAFINAL_CLIENT_H
#define POSSEMPRACAFINAL_CLIENT_H

#include <pthread.h>
#include <stdbool.h>

#define VELKOST_STRINGU 256
#define VELKOST_MENA 30

typedef struct spoluD{
    int pocetHracov;
    int poradieHraca;
    int velkostMapy;
    int aktualneTahov;
    bool koniecHry;

    char *menoHracaNarade;

    int hracID;
    int sockfd;

    char **mapa;
    char *infoHry;
    char *infoVyhry;
    char *okInfo;

    pthread_mutex_t * mutex;
    pthread_cond_t * hraj;
    pthread_cond_t * cakaj;
}SPOLUD;
typedef struct tahHraca{
    char *infoHraca;
    int pocetTahov;

    SPOLUD * spoluD;
}TAHHRA;
typedef struct tahProtihraca{
    int pocetTahov;

    SPOLUD * spoluD;
}TAHPRO;

int client(int argc, char *argv[]);
#endif //POSSEMPRACAFINAL_CLIENT_H
