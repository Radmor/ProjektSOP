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

/*typy komunikatow w kolejce ID_KOM_INIC*/
#define ROZPOCZNIJ          1
#define AKCEPTUJ            2


/* key kolejek */

#define INIT_QUEUE_KEY 2137
#define GR1_QUEUE_KEY 1
#define GR2_QUEUE_KEY 2

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


/* koniec funkcji */


int main(int args, char argv[]){

    int init_queue_id=msgget(INIT_QUEUE_KEY,IPC_CREAT|0664);

    Init_message init_message;
    init_message.mtype=ROZPOCZNIJ;
    init_message.init_data.id_gracza=rand()%10000;

    Game_message game_message;

    msgsnd(init_queue_id,&init_message,sizeof(init_message.init_data),0);

    //printf("ZYJE");

    msgrcv(init_queue_id,&init_message, sizeof(init_message.init_data),AKCEPTUJ,0);

    int game_queue_id=init_message.init_data.id_kolejki_kom;



    if(fork()==0){
            while(1){
                msgrcv(game_queue_id,&game_message, sizeof(game_message.game_data),STAN,0);
                // clear the screen
                //show_player(game_message.game_data);
            }
    }
    else{

        int decyzja;
        Game_message train_message;
        train_message.mtype=TWORZ;
        Game_message battle_message;
        battle_message.mtype=ATAK;

        Game_data_struct train_list;
        Game_data_struct battle_list;

        int liczba;

        while(1){
            printf("\033[2J\033[1;1H");
            printf("WYBIERZ AKCJE:\n1-trening jednostek\n2-atak\n\n");
            scanf("%d",&decyzja);

            switch(decyzja){
                case 1:
                    printf("Wybierz liczbe jednostek lekkiej piechoty\n");
                    scanf("%d",&train_list.light_infantry);
                    printf("Wybierz liczbe jednostek ciezkiej piechoty\n");
                    scanf("%d",&train_list.heavy_infantry);
                    printf("Wybierz liczbe jednostek jazdy\n");
                    scanf("%d",&train_list.cavalry);
                    printf("Wybierz liczbe robotnikow\n");
                    scanf("%d",&train_list.workers);

                    train_message.game_data=train_list;

                    msgsnd(game_queue_id,&train_message, sizeof(train_message.game_data),0);
                    printf("WYSLANO POLECENIE TRENINGU\n");
                case 2:
                    printf("Wybierz liczbe jednostek lekkiej piechoty\n");
                    scanf("%d",&battle_list.light_infantry);
                    printf("Wybierz liczbe jednostek ciezkiej piechoty\n");
                    scanf("%d",&battle_list.heavy_infantry);
                    printf("Wybierz liczbe jednostek jazdy\n");
                    scanf("%d",&battle_list.cavalry);

                    battle_message.game_data=battle_list;

                    msgsnd(game_queue_id,&battle_message, sizeof(battle_message.game_data),0);
                    //printf("WYSLANO POLECENIE ATAKU\n");
                default:

                    printf("NIE MA TAKIEJ KOMENDY\n");


            }
        }


    }


    return 0;
}