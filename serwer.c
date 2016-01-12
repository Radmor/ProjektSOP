#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <math.h>

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

/* inne zmienne globalne */

#define INIT_STOCKS_VALUE 300
#define INIT_STOCKS_ADD 50
#define WORKER_STOCKS_ADD 5


/* jednostki */
///////////////////////////////////////////////////////////////////////////

/* lekka piechota */

#define LIGHT_INFANTRY_PRICE 100
#define LIGHT_INFANTRY_DAMAGE 1
#define LIGHT_INFANTRY_DEFENSE 1.2
#define LIGHT_INFANTRY_PRODUCTION_TIME 2

/* ciezka piechota */

#define HEAVY_INFANTRY_PRICE 250
#define HEAVY_INFANTRY_DAMAGE 1.5
#define HEAVY_INFANTRY_DEFENSE 3
#define HEAVY_INFANTRY_PRODUCTION_TIME 3

/* jazda */

#define CAVALRY_PRICE 550
#define CAVALRY_DAMAGE 3.5
#define CAVALRY_DEFENSE 1.2
#define CAVALRY_PRODUCTION_TIME 5


/* robotnicy */

#define WORKER_PRICE 150
#define WORKER_DAMAGE 0
#define WORKER_DEFENSE 0
#define WORKER_PRODUCTION_TIME 2

/* struktury */
///////////////////////////////////////////////////////////////////////////////

typedef struct Message{
    long mtype;
    int liczba;

}Message;

typedef struct Game_data_struct{
    int light_infantry;
    int heavy_infantry;
    int cavalry;
    int workers;
    int stocks;
    int victory_points;
    int winner;
}Game_data_struct;
/*
typedef struct Game_data_struct {
    int lek_piech;
    int cie_piech;
    int jazda;
    int robot;
    int surowce;
    int skut_ataki;
    int wygral;
}Game_data_struct;
*/
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

/* funkcje */
///////////////////////////////////////////////////////////////////////////////

/* funkcje pomocnicze */
///////////////////////////////////////
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

/* funkcje rozgrywki */
///////////////////////////////////////


void add_stock(Game_data_struct &player){
    player.stocks+=INIT_STOCKS_ADD+(WORKER_STOCKS_ADD*player.workers);
}

void add_stocks(Game_data_struct &player1,Game_data_struct &player2){
    add_stock(player1);
    add_stock(player2);
}

int train(Game_data_struct &player,Game_data_struct train_list){

    int train_cost=(LIGHT_INFANTRY_PRICE*train_list.light_infantry)+(HEAVY_INFANTRY_PRICE*train_list.heavy_infantry)+(CAVALRY_PRICE*train_list.cavalry)+(WORKER_PRICE*train_list.workers);
    if(train_cost>player.stocks) return -1;
    else{
        player.stocks-=train_cost;

        player.light_infantry+=train_list.light_infantry;
        player.heavy_infantry+=train_list.heavy_infantry;
        player.cavalry+=train_list.cavalry;
        player.workers+=train_list.workers;

        return 1;
    }
}

/* funkcje walki */
//////////////////////////////////////////////////

int calculate_attack(Game_data_struct units_list){

    int attack;

    attack=(units_list.light_infantry*LIGHT_INFANTRY_DAMAGE)+(units_list.heavy_infantry*HEAVY_INFANTRY_DAMAGE)+(units_list.cavalry*CAVALRY_DAMAGE)+(units_list.workers*WORKER_DAMAGE);

    return attack;
}

int calculate_defense(Game_data_struct units_list)
{
    int defense;

    defense=(units_list.light_infantry*LIGHT_INFANTRY_DEFENSE)+(units_list.heavy_infantry*HEAVY_INFANTRY_DEFENSE)+(units_list.cavalry*CAVALRY_DEFENSE)+(units_list.workers*WORKER_DEFENSE);

    return defense;
}



void calculate_casualties(Game_data_struct &player,int attack,int defense){

    double ratio=double(attack)/double(defense);
    //printf("Ratio: %f",ratio*player.light_infantry);

    player.light_infantry-=floor(player.light_infantry*ratio);
    player.heavy_infantry-=floor(player.heavy_infantry*ratio);
    player.cavalry-=floor(player.cavalry*ratio);

}

void kill_them_all(Game_data_struct &player){
    player.light_infantry=0;
    player.heavy_infantry=0;
    player.cavalry=0;
}

void duel(Game_data_struct player1,Game_data_struct &player2){

    int attack;
    int defense;

    attack=calculate_attack(player1);
    defense=calculate_defense(player2);

    if(attack>defense){
        kill_them_all(player2);
    }
    else{
        calculate_casualties(player2,attack,defense);
    }


    //show_player(player2);

}

void battle(Game_data_struct &player1,Game_data_struct &player2){

    Game_data_struct temp_player1,temp_player2;

    //temp_player1=player1;
    temp_player2=player2;

    duel(player1,player2);

    /*
    show_player(player1);
    show_player(player2);
    */

    duel(temp_player2,player1);

}

/* funkcje inicjalizujace */
///////////////////////////////////////////
void init_clear(Game_data_struct &player){


    player.light_infantry=0;
    player.heavy_infantry=0;
    player.cavalry=0;
    player.workers=0;
    player.stocks=0;
    player.victory_points=0;
    player.winner=0;

}

void init(){

}

int main(int args, char* argv[]){

    /*
    int init_queue=msgget(INIT_QUEUE_KEY,IPC_CREAT|0664);


    Message message;
    message.mtype=ROZPOCZNIJ;
    message.liczba=2137;

    msgsnd(init_queue,&message,4,0);

     */

    Game_data_struct player1,player2,train_list;

    init_clear(player1);
    init_clear(player2);
    init_clear(train_list);

    player1.light_infantry=2;
    player1.heavy_infantry=7;
    player1.cavalry=3;
    player1.workers=2;

    player2.light_infantry=7;
    player2.heavy_infantry=5;
    player2.cavalry=3;



    show_player(player1);
    show_player(player2);

    add_stocks(player1,player2);
    add_stocks(player1,player2);

    show_player(player1);
    show_player(player2);

    train_list.light_infantry=1;
    int t=train(player1,train_list);
    printf("%d",t);


    show_player(player1);
    show_player(player2);




    return 0;
}

/* pytania
Czy mozna skompilowac g++
Czy trzeba zabezpieczac przed atakiem robotnikow czy mozna pominac i nie zwracac zlego ataku
Co gdy 0 ludzi atakuje, zero broni.
*/