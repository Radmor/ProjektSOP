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

/* key semaforow */
#define SEMAPHORE_KEY 67
#define SEMAPHORE_QUANTITY 3

#define SHARED_MEMORY_SEMAPHORE_NUM 0
#define SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM 1
#define SHARED_MEMORY_PLAYER2_SEMAPHORE_NUM 2

#define SHARED_MEMORY_SEMAPHORE_INIT 1
#define SHARED_MEMORY_PLAYER1_SEMAPHORE_INIT 1
#define SHARED_MEMORY_PLAYER2_SEMAPHORE_INIT 1

/* key pamieci wspoldzielonej */
#define SHARED_MEMORY_PLAYER1_KEY 21
#define SHARED_MEMORY_PLAYER2_KEY 37

/* consty dotycace dodawania surowcow */

#define INIT_STOCKS_VALUE 300
#define INIT_STOCKS_ADD 50
#define WORKER_STOCKS_ADD 5

/* inne consty */

#define ATTACK_DURATION 5
#define ATTACK_CODE 1
#define DEFENSE_CODE 1
#define INFO_FREQUENCY 2



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

static struct sembuf buf;

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

typedef struct Shared_memory_struct{
    Game_data_struct player1;
    Game_data_struct player2;
}Shared_memory_struct;

/* funkcje */
///////////////////////////////////////////////////////////////////////////////

/* funkcje pomocnicze */
///////////////////////////////////////
void show_player(Game_data_struct *player){

    printf("\n");
    printf("Statystyki gracza\n\n");
    printf("Lekka piechota: %d\n",player->light_infantry);
    printf("Ciezka piechota: %d\n",player->heavy_infantry);
    printf("Jazda: %d\n",player->cavalry);
    printf("Robotnicy: %d\n",player->workers);
    printf("Surowce: %d\n",player->stocks);
    printf("Punkty zwyciestwa: %d\n",player->victory_points);
    printf("\n");
}


void semaphore_up(int semid,int semnum){
    buf.sem_num=semnum;
    buf.sem_op=1;
    buf.sem_flg=0;
    if(semop(semid,&buf,1)==-1){
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void semaphore_down(int semid,int semnum){
    buf.sem_num=semnum;
    buf.sem_op=-1;
    buf.sem_flg=0;
    if(semop(semid,&buf,1)==-1){
        perror("Opuszczenie semafora");
        exit(1);
    }
}


/* funkcje rozgrywki */
///////////////////////////////////////


void add_stock(Game_data_struct *player){
    player->stocks+=INIT_STOCKS_ADD+(WORKER_STOCKS_ADD*(player->workers));
}

void add_stocks(Game_data_struct *player1,Game_data_struct *player2,int semaphore_id,int semnum){
    semaphore_down(semaphore_id,semnum);
    add_stock(player1);
    add_stock(player2);
    semaphore_up(semaphore_id,semnum);
}

int train(Game_data_struct *player,Game_data_struct train_list,int semaphore_id,int semaphore_num){

    int i;
    int train_cost=(LIGHT_INFANTRY_PRICE*train_list.light_infantry)+(HEAVY_INFANTRY_PRICE*train_list.heavy_infantry)+(CAVALRY_PRICE*train_list.cavalry)+(WORKER_PRICE*train_list.workers);
    if(train_cost>player->stocks)
    {

        return -1;
    }
    else{

        player->stocks-=train_cost;

        for(i=0;i<train_list.light_infantry;i++){
            sleep(LIGHT_INFANTRY_PRODUCTION_TIME);

            semaphore_down(semaphore_id,semaphore_num);
            player->light_infantry++;
            semaphore_up(semaphore_id,semaphore_num);
        }

        for(i=0;i<train_list.heavy_infantry;i++){
            sleep(HEAVY_INFANTRY_PRODUCTION_TIME);

            semaphore_down(semaphore_id,semaphore_num);
            player->heavy_infantry++;
            semaphore_up(semaphore_id,semaphore_num);
        }

        for(i=0;i<train_list.cavalry;i++){
            sleep(CAVALRY_PRODUCTION_TIME);

            semaphore_down(semaphore_id,semaphore_num);
            player->cavalry++;
            semaphore_up(semaphore_id,semaphore_num);
        }

        for(i=0;i<train_list.workers;i++){
            sleep(WORKER_PRODUCTION_TIME);

            semaphore_down(semaphore_id,semaphore_num);
            player->workers++;
            semaphore_up(semaphore_id,semaphore_num);

        }
        return 1;
    }
}

/* funkcje walki */
//////////////////////////////////////////////////

int calculate_attack(Game_data_struct *units_list){

    int attack;

    attack=(units_list->light_infantry*LIGHT_INFANTRY_DAMAGE)+(units_list->heavy_infantry*HEAVY_INFANTRY_DAMAGE)+(units_list->cavalry*CAVALRY_DAMAGE)+(units_list->workers*WORKER_DAMAGE);

    return attack;
}

int calculate_defense(Game_data_struct *units_list)
{
    int defense;

    defense=(units_list->light_infantry*LIGHT_INFANTRY_DEFENSE)+(units_list->heavy_infantry*HEAVY_INFANTRY_DEFENSE)+(units_list->cavalry*CAVALRY_DEFENSE)+(units_list->workers*WORKER_DEFENSE);

    return defense;
}



void calculate_casualties(Game_data_struct *player,int attack,int defense){

    double ratio=(double)attack/(double)defense;


    player->light_infantry-=floor(player->light_infantry*ratio);
    player->heavy_infantry-=floor(player->heavy_infantry*ratio);
    player->cavalry-=floor(player->cavalry*ratio);

}

void kill_them_all(Game_data_struct *player,int code){
    player->light_infantry=0;
    player->heavy_infantry=0;
    player->cavalry=0;
    if(code==ATTACK_CODE)
        player->victory_points++;
}

void duel(Game_data_struct *player1,Game_data_struct *player2,int code){

    int attack;
    int defense;

    attack=calculate_attack(player1);
    defense=calculate_defense(player2);

    if(attack>defense){
        kill_them_all(player2,code);
    }
    else{
        calculate_casualties(player2,attack,defense);
    }


    //show_player(player2);

}

void battle(Game_data_struct *player1,Game_data_struct *player2,int semaphore_id,int semnum){

    sleep(ATTACK_DURATION);

    semaphore_down(semaphore_id,semnum);

    Game_data_struct temp_player1,temp_player2;
    Game_data_struct *wsk_temp_player2=&temp_player2;

    temp_player2=*player2;


    duel(player1,player2,ATTACK_CODE);

    duel(wsk_temp_player2,player1,DEFENSE_CODE);

    semaphore_up(semaphore_id,semnum);

}


void sendtroops(Game_data_struct *attacker,Game_data_struct *battle_list){
    attacker->light_infantry-=battle_list->light_infantry;
    attacker->heavy_infantry-=battle_list->heavy_infantry;
    attacker->cavalry-=battle_list->cavalry;
}

void bringboysbackhome(Game_data_struct *attacker,Game_data_struct *battle_list){
    attacker->light_infantry+=battle_list->light_infantry;
    attacker->heavy_infantry+=battle_list->heavy_infantry;
    attacker->cavalry+=battle_list->cavalry;
}

int check_army(Game_data_struct *player,Game_data_struct *battle_list){
    if(battle_list->light_infantry<=player->light_infantry && battle_list->heavy_infantry<=player->heavy_infantry && battle_list->cavalry<=player->cavalry && battle_list->light_infantry+battle_list->heavy_infantry+battle_list->cavalry>0)
        return 1;
    else
        return -1;

}

int war(Game_data_struct *attacker,Game_data_struct *defenser,Game_data_struct *battle_list,int semaphore_id,int semnum){

    semaphore_down(semaphore_id,semnum);
    if(check_army(attacker,battle_list)==-1){
        semaphore_up(semaphore_id,semnum);
        return -1;
    }
    else{

        sendtroops(attacker,battle_list);
        semaphore_up(semaphore_id,semnum);

        battle(battle_list,defenser,semaphore_id,semnum);

        semaphore_down(semaphore_id,semnum);
        bringboysbackhome(attacker,battle_list);
        semaphore_up(semaphore_id,semnum);

        return 1;
    }
}

/* funkcje inicjalizujace */
///////////////////////////////////////////
void init_clear(Game_data_struct *player,int semaphore_id,int semnum){

    semaphore_down(semaphore_id,semnum);

    player->light_infantry=0;
    player->heavy_infantry=0;
    player->cavalry=0;
    player->workers=0;
    player->stocks=0;
    player->victory_points=0;
    player->winner=0;

    semaphore_up(semaphore_id,semnum);

}

void setval(Game_data_struct *player,int semaphore_id,int semnum,int light_infantry,int heavy_infantry,int cavalry,int workers, int stocks, int victory_points,int winner){

    semaphore_down(semaphore_id,semnum);

    player->light_infantry=light_infantry;
    player->heavy_infantry=heavy_infantry;
    player->cavalry=cavalry;
    player->workers=workers;
    player->stocks=stocks;
    player->victory_points=victory_points;
    player->winner=winner;

    semaphore_up(semaphore_id,semnum);

}
void init_player(Game_data_struct *player,int semaphore_id,int semnum){

    semaphore_down(semaphore_id,semnum);

    player->light_infantry=0;
    player->heavy_infantry=0;
    player->cavalry=0;
    player->workers=0;
    player->stocks=INIT_STOCKS_VALUE;
    player->victory_points=0;
    player->winner=0;

    semaphore_up(semaphore_id,semnum);

}


int main(int args, char* argv[]){



    /* utworzenie i inicjalizacja semaforow */

    int shared_memory_semaphore_id=semget(SEMAPHORE_KEY,SEMAPHORE_QUANTITY,IPC_CREAT|0664);
    if(shared_memory_semaphore_id==-1){
        perror("Blad przy tworzeniu semafora");
        exit(1);
    }

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 0 pamieci wspoldzielonej");
        exit(1);
    }

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_PLAYER1_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 0 pamieci wspoldzielonej");
        exit(1);
    }

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_PLAYER2_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_PLAYER2_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 1 pamieci wspoldzielonej");
        exit(1);
    }

    /* utworzenie i przylaczenie pamieci wspoldzielonej player1 */

    int SHARED_MEMORY_PLAYER1_ID=shmget(SHARED_MEMORY_PLAYER1_KEY,sizeof(Game_data_struct),IPC_CREAT|0664);
    if(SHARED_MEMORY_PLAYER1_ID==-1){
        perror("Blad przy tworzeniu pamieci wspoldzielonej player1");
        exit(1);
    }

    Game_data_struct *player1=(Game_data_struct*)shmat(SHARED_MEMORY_PLAYER1_ID,NULL,0);
    if(player1==NULL){
        perror("Blad przy przylaczeniu pamieci wspoldzielonej player1");
        exit(1);
    }

    /* utworzenie i przylaczenie pamieci wspoldzielonej player2 */

    int SHARED_MEMORY_PLAYER2_ID=shmget(SHARED_MEMORY_PLAYER2_KEY,sizeof(Game_data_struct),IPC_CREAT|0664);
    if(SHARED_MEMORY_PLAYER2_ID==-1){
        perror("Blad przy tworzeniu pamieci wspoldzielonej player2");
        exit(1);
    }

    Game_data_struct *player2=(Game_data_struct*)shmat(SHARED_MEMORY_PLAYER2_ID,NULL,0);
    if(player2==NULL){
        perror("Blad przy przylaczeniu pamieci wspoldzielonej player2");
        exit(1);
    }

    /* utworzenie kolejki komunikatow init */

    int init_queue_id=msgget(INIT_QUEUE_KEY,IPC_CREAT|0664);
    if(init_queue_id==-1){
        perror("Blad przy tworzeniu poczatkowej kolejki komunikatow");
        exit(1);
    }


    // INICJALIZACJA

    Init_message init_message1;
    Init_message init_message2;

    msgrcv(init_queue_id,&init_message1,sizeof(init_message1.init_data),ROZPOCZNIJ,0);
    msgrcv(init_queue_id,&init_message2,sizeof(init_message1.init_data),ROZPOCZNIJ,0);


    /* utworzenie kolejki komunikatow gracza1 */

    int gr1_queue_id=msgget(GR1_QUEUE_KEY,IPC_CREAT|0664);
    if(gr1_queue_id==-1){
        perror("Blad przy tworzeniu kolejki gracza 1");
        exit(1);
    }

    /* utworzenie kolejki komunikatow gracza2 */

    int gr2_queue_id=msgget(GR2_QUEUE_KEY,IPC_CREAT|0664);
    if(gr2_queue_id==-1){
        perror("Blad przy tworzeniu kolejki gracza 2");
        exit(1);
    }

    init_message1.mtype=AKCEPTUJ;
    init_message1.init_data.id_kolejki_kom=gr1_queue_id;

    init_message2.mtype=AKCEPTUJ;
    init_message2.init_data.id_kolejki_kom=gr2_queue_id;

    //wyslij wiadomosci

    msgsnd(init_queue_id,&init_message1,sizeof(init_message1.init_data),0);
    msgsnd(init_queue_id,&init_message2,sizeof(init_message2.init_data),0);



    Game_data_struct train_list;
    Game_data_struct *train_list_pointer=&train_list;

    init_clear(player1,shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM);
    init_clear(player2,shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM);
    init_clear(train_list_pointer,shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM);

    //setval(player1,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM,2,7,3,2,5000,0,0);
    //setval(player2,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM,7,5,3,0,0,0,0);

    printf("PO INICJALIZACJI\n");

    show_player(player1);
    show_player(player2);


    /*train_list.light_infantry=1;
    train_list.cavalry=1;

        int t=train(player1,train_list,shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM);
        printf("%d",t);

    printf("PO WYPRODUKOWANIU JEDNOSTEK\n");

    show_player(player1);
    show_player(player2);
     */





    if(fork()==0){  //pobieranie surowcow
        while(1){
            semaphore_down(shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM);
            add_stocks(player1,player2,shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM);
            semaphore_up(shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM);
            sleep(1);
            //printf("Wyprodukowano surowce\n");
        }

    }
    else{

        if(fork()==0){
            if(fork()==0){/* ATAKI OD GRACZA 1 */
                Game_message message;
                Game_message attack_failure_message;
                attack_failure_message.mtype=BLEDNY_ATAK;

                Game_data_struct battle_list;
                Game_data_struct *battle_list_pointer=&battle_list;

                //setval(battle_list_pointer,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM,2,7,3,0,0,0,0);


                while(1){

                    msgrcv(gr1_queue_id,&message,sizeof(message.game_data),ATAK,0);
                    printf("TUTAJ");
                    battle_list=message.game_data;

                    if(war(player1,player2,battle_list_pointer,shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM)==-1){
                        msgsnd(gr1_queue_id,&attack_failure_message,sizeof(attack_failure_message.game_data),0);
                    }

                }
            }
            else{
                Game_message game_message;
                game_message.mtype=STAN;

                while(1){
                    sleep(INFO_FREQUENCY);
                    game_message.game_data=*player1;
                    msgsnd(gr1_queue_id,&game_message, sizeof(game_message.game_data),0);
                    game_message.game_data=*player2;
                    msgsnd(gr2_queue_id,&game_message, sizeof(game_message.game_data),0);
                }


            }


        }
        else {
                wait(NULL);

                printf("PO BITWIE\n");

                show_player(player1);
                show_player(player2);

        }

    }


    return 0;
}

/* pytania
 * czy mozna pominac i nie zwracac zlego ataku
Co gdy 0 ludzi atakuje, zero broni.
*/


/* smietnisko */

/*
   int init_queue=msgget(INIT_QUEUE_KEY,IPC_CREAT|0664);


   Message message;
   message.mtype=ROZPOCZNIJ;
   message.liczba=2137;

   msgsnd(init_queue,&message,4,0);

    */

/*
Game_data_struct train_list;

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
 */

/*
    show_player(player1);
    show_player(player2);

    player1->light_infantry=2;
    player1->heavy_infantry=7;
    player1->cavalry=3;
    player1->workers=2;

    player2->light_infantry=7;
    player2->heavy_infantry=5;
    player2->cavalry=3;



    show_player(player1);
    show_player(player2);

    add_stocks(player1,player2);

    show_player(player1);
    show_player(player2);

    train_list.light_infantry=1;
    int t=train(player1,train_list);
    printf("%d",t);


    show_player(player1);
    show_player(player2);
     */