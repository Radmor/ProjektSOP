#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

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



#define MAX 1000


/* zmienne globalne */

int game_queue_id;
int output_queue_id;



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

void interrupt(){

    printf("NAGLE ZAKONCZONO PROGRAM\n");

    Game_message game_message;
    game_message.mtype=PODDAJSIE;

    msgsnd(game_queue_id,&game_message, sizeof(game_message.game_data),0);
    msgsnd(output_queue_id,&game_message, sizeof(game_message.game_data),0);

    sleep(1);

    msgctl(game_queue_id,IPC_RMID,0);

    kill(0,SIGKILL);

}


void flush_input(FILE *in)
{
    int c;

    while ((c=fgetc(in))!=EOF && c!='\n')
        ;

    clearerr (in);
}


/* koniec funkcji */


int main(int args, char* argv[]){

    signal(SIGINT,interrupt);

    printf("OCZEKIWANIE NA DRUGIEGO GRACZA\n");

    /* SPRAWDZANIE POPRAWNOSCI ARGUMENTOW */

    if(args<4){
        printf("NIEWLASCIWA LICZBA ARGUMENTOW\n");
        exit(1);
    }

    int init_queue_key=atoi(argv[1]);
    int id_gracza=atoi(argv[2]);

    int init_queue_id=msgget(init_queue_key,IPC_CREAT|0664);
    if(init_queue_id==-1){
        perror("BLAD STWORZENIA KOLEJKI INIT");
        exit(1);
    }



    Init_message init_message;
    init_message.mtype=ROZPOCZNIJ;
    init_message.init_data.id_gracza=id_gracza;
    msgsnd(init_queue_id,&init_message,sizeof(init_message.init_data),0);

    msgrcv(init_queue_id,&init_message, sizeof(init_message.init_data),AKCEPTUJ,0);

    int game_queue_key=init_message.init_data.id_kolejki_kom;

    game_queue_id=msgget(game_queue_key,IPC_CREAT|0664);
    if(game_queue_id==-1){
        perror("Blad przy otwarciu kolejki do komunikacji");
        exit(1);
    }


    int output_queue_key=atoi(argv[3]);
    output_queue_id=msgget(output_queue_key,IPC_CREAT|0664);
    if(output_queue_id==-1){
        perror("Blad przy tworzeniu kolejki output");
        exit(1);
    }

    init_message.mtype=ID;
    init_message.init_data.id_gracza=id_gracza;
    msgsnd(output_queue_id,&init_message, sizeof(init_message.init_data),0);

    if(fork()==0){

        Game_message train_message;
        train_message.mtype=TWORZ;

        Game_message battle_message;
        battle_message.mtype=ATAK;

        Game_message surrender_message;
        surrender_message.mtype=PODDAJSIE;

        Game_data_struct train_list;
        Game_data_struct battle_list;

        char decyzja;

        char buffer[MAX]="";
        char character;
        char temp;
        int position;

        int wynik;
        int tekst;

        while(1){
            printf("\033[2J\033[1;1H");
            //printf("ID TWOJEJ KOLEJKI TO: %d\n",output_queue_id);
            printf("ID GRACZA TO: %d\n",id_gracza);
            printf("WYBIERZ AKCJE:\n1-trening jednostek\n2-atak\n3-poddaj sie\n\n");
            //scanf("%d",&decyzja);

            decyzja=fgetc (stdin);

            flush_input(stdin);


            if(decyzja=='1') {

                //fseek(stdin,0,SEEK_END);
                printf("Wybierz liczbe jednostek lekkiej piechoty\n");
                fgets (buffer, MAX, stdin);
                train_list.light_infantry=(int)strtol(buffer,NULL,0);

                printf("Wybierz liczbe jednostek ciezkiej piechoty\n");
                fgets (buffer, MAX, stdin);
                train_list.heavy_infantry=(int)strtol(buffer,NULL,0);

                printf("Wybierz liczbe jednostek jazdy\n");
                fgets (buffer, MAX, stdin);
                train_list.cavalry=(int)strtol(buffer,NULL,0);

                printf("Wybierz liczbe robotnikow\n");
                fgets (buffer, MAX, stdin);
                train_list.workers=(int)strtol(buffer,NULL,0);

                train_message.game_data=train_list;
                msgsnd(game_queue_id,&train_message, sizeof(train_message.game_data),0);
                printf("WYSLANO POLECENIE TRENINGU\n");
            }
            else if(decyzja=='2') {
                printf("Wybierz liczbe jednostek lekkiej piechoty\n");
                fgets (buffer, MAX, stdin);
                battle_list.light_infantry=(int)strtol(buffer,NULL,0);

                printf("Wybierz liczbe jednostek ciezkiej piechoty\n");
                fgets (buffer, MAX, stdin);
                battle_list.heavy_infantry=(int)strtol(buffer,NULL,0);

                printf("Wybierz liczbe jednostek jazdy\n");
                fgets (buffer, MAX, stdin);
                battle_list.cavalry=(int)strtol(buffer,NULL,0);

                battle_message.game_data=battle_list;

                msgsnd(game_queue_id,&battle_message, sizeof(battle_message.game_data),0);
                printf("WYSLANO POLECENIE ATAKU\n");
            }
            else if(decyzja=='3'){
                printf("Poddales sie\n");
                msgsnd(game_queue_id,&surrender_message, sizeof(surrender_message.game_data),0);
                msgsnd(output_queue_id,&surrender_message,sizeof(surrender_message.game_data),0);

                kill(0,SIGKILL);

            }
            else {
                printf("NIE MA TAKIEJ KOMENDY\n");
            }

        }

    }
    else {

        Game_message message;

        msgsnd(output_queue_id, &message, sizeof(message.game_data), 0);

        while (1) {
            msgrcv(game_queue_id, &message, sizeof(message.game_data), 0, 0);
            if (message.mtype == KONIEC || message.mtype == ZAKONCZ) {
                msgsnd(output_queue_id, &message, sizeof(message.game_data), 0);

                sleep(1);

                msgctl(game_queue_id,IPC_RMID,0);
                msgctl(output_queue_id,IPC_RMID,0);

                kill(0, SIGKILL);
            }
            else if (message.mtype == ATAK || message.mtype == TWORZ || message.mtype == PODDAJSIE) {
                msgsnd(game_queue_id, &message, sizeof(message.game_data), 0);
            }
            else {
                msgsnd(output_queue_id, &message, sizeof(message.game_data), 0);
            }

        }

    }
    return 0;
}