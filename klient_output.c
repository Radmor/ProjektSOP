#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <fcntl.h>
#include <math.h>

#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>

/*typy komunikatow w kolejkach KOM_GR1 i KOM_GR2*/
#define ATAK                1
#define TWORZ               2
#define STAN                3
#define KONIEC              4
#define ZAKONCZ             5
#define NIEDOST_SUROWCE     6
#define BLEDNY_ATAK         7
#define PORAZKA_ATAK        8
#define PORAZKA_OBRONA      9
#define SKUTECZNY_ATAK      10
#define SKUTECZNA_OBRONA    11
#define STRATY_ATAK         12
#define STRATY_OBRONA       13
#define PODDAJSIE           14

/* typy komunikatow miedzy klientem a outputem */
#define ID 15

/*typy komunikatow w kolejce ID_KOM_INIC*/
#define ROZPOCZNIJ          1
#define AKCEPTUJ            2


/* key kolejek */

#define INIT_QUEUE_KEY 2137
#define GR1_QUEUE_KEY 1
#define GR2_QUEUE_KEY 2

#define SIGKILL 9

/* struktury */

typedef struct Game_data_struct{
    int light_infantry;
    int heavy_infantry;
    int cavalry;
    int workers;
    int stocks;
    int victory_points;
    int winner;
}Game_data_struct;

typedef struct Init_data_struct {
    int id_kolejki_kom;
    int id_gracza;
}Init_data_struct;

typedef struct Game_message {
    long mtype;
    struct Game_data_struct game_data;
}Game_message;

typedef struct Init_message {
    long mtype;
    struct Init_data_struct init_data;
}Init_message;

/* koniec struktur */


/* funkcje */

void show_player(Game_data_struct player){

    printf("\n");
    printf("Statystyki gracza\n\n");
    printf("Lekka piechota: %d\n",player.light_infantry);
    printf("Ciezka piechota: %d\n",player.heavy_infantry);
    printf("Jazda: %d\n",player.cavalry);
    printf("Robotnicy: %d\n",player.workers);
    printf("Surowce: %d\n",player.stocks);
    printf("Punkty zwyciestwa: %d\n",player.victory_points);
    printf("\n");
}

void show_casualties(Game_data_struct player){

    printf("\n");
    printf("Lekka piechota: %d\n",player.light_infantry);
    printf("Ciezka piechota: %d\n",player.heavy_infantry);
    printf("Jazda: %d\n",player.cavalry);
    printf("\n");
}

void koniec(int sleep_time,int queue_id){
    msgctl(queue_id,IPC_RMID,0);
    sleep(sleep_time);
    kill(0,SIGKILL);
}


/* koniec funkcji */


int main(int args, char* argv[]){


    if(args<2){
        printf("NIEWLASCIWA LICZBA ARGUMENTOW\n");
        exit(1);
    }

    int game_queue_key=atoi(argv[1]);

    int game_queue_id=msgget(game_queue_key,IPC_CREAT|0664);
    if(game_queue_id==-1){
        perror("BLAD PRZY TWORZENIU KOLEJKI GRY");
        exit(1);
    }

    Game_message message;
    Init_message init_message;

    msgrcv(game_queue_id,&init_message, sizeof(init_message.init_data),ID,0);
    int id_gracza=init_message.init_data.id_gracza;


    while(1){
        msgrcv(game_queue_id,&message, sizeof(message.game_data),0,0);
        if(message.mtype==STAN){
            show_player(message.game_data);
        }
        else if(message.mtype==BLEDNY_ATAK){
            printf("\033[2J\033[1;1H");
            printf("WYKONALES BLEDNY ATAK");
        }
        else if(message.mtype==NIEDOST_SUROWCE){
            printf("\033[2J\033[1;1H");
            printf("NIE MASZ WYSTARCZAJACEJ ILOSCI SUROWCOW");
        }
        else if(message.mtype==SKUTECZNY_ATAK){
            printf("\033[2J\033[1;1H");
            printf("PRZEPROWADZONO SKUTECZNY ATAK\n");
        }
        else if(message.mtype==SKUTECZNA_OBRONA){
            printf("\033[2J\033[1;1H");
            printf("PRZEPROWADZONO SKUTECZNA OBRONE\n");
        }
        else if(message.mtype==PORAZKA_ATAK){
            printf("\033[2J\033[1;1H");
            printf("ARMIA ZOSTALA ROZGROMIONA W TRAKCIE ATAKU\n");
        }
        else if(message.mtype==PORAZKA_OBRONA){
            printf("\033[2J\033[1;1H");
            printf("ARMIA ZOSTALA ROZGROMIONA W TRAKCIE OBRONY\n");
        }
        else if(message.mtype==STRATY_ATAK){
            printf("\033[2J\033[1;1H");
            printf("W CZASIE ATAKU ODNIESIONO NASTEPUJACE STRATY:\n");
            show_casualties(message.game_data);
        }
        else if(message.mtype==STRATY_OBRONA){
            printf("\033[2J\033[1;1H");
            printf("W CZASIE OBRONY ODNIESIONO NASTEPUJACE STRATY:\n");
            show_casualties(message.game_data);
        }
        else if(message.mtype==KONIEC){
            printf("\033[2J\033[1;1H");
            printf("SERWER NAGLE PRZESTAL DZIALAC\nPROGRAM ZAKONCZY SIE ZA 3 SEKUNDY\n");
            koniec(3,game_queue_id);
        }
        else if(message.mtype==ZAKONCZ){
            printf("\033[2J\033[1;1H");
            if(message.game_data.winner==id_gracza)
                printf("WYGRANA\n");
            else
                printf("PRZEGRANA\n");
            printf("PROGRAM ZAKONCZY SIE ZA 3 sekundy\n");
            koniec(3,game_queue_id);
        }
        else if(message.mtype==PODDAJSIE){
            printf("\033[2J\033[1;1H");
            printf("PODDALES SIE\nPROGRAM ZAKONCZY SIE ZA 3 SEKUNDY\n");
            koniec(3,game_queue_id);
        }
        /*else{
            printf("DZIWNA WIADOMOSC\n");
            printf("%ld\n",message.mtype);

        }*/

    }

    return 0;
}