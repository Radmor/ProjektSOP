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
#define SEMAPHORE_QUANTITY 5

#define SHARED_MEMORY_SEMAPHORE_NUM 0
#define SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM 1
#define SHARED_MEMORY_PLAYER2_SEMAPHORE_NUM 2
#define SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM 3
#define SHARED_MEMORY_WINNER_SEMAPHORE_NUM 4

#define SHARED_MEMORY_SEMAPHORE_INIT 1
#define SHARED_MEMORY_PLAYER1_SEMAPHORE_INIT 1
#define SHARED_MEMORY_PLAYER2_SEMAPHORE_INIT 1
#define SHARED_MEMORY_ENDGAME_SEMAPHORE_INIT 1
#define SHARED_MEMORY_WINNER_SEMAPHORE_INIT 1

/* key pamieci wspoldzielonej */
#define SHARED_MEMORY_PLAYER1_KEY 21
#define SHARED_MEMORY_PLAYER2_KEY 37
#define SHARED_MEMORY_ENDGAME_KEY 87
#define SHARED_MEMORY_PIDS_KEY 665
#define SHARED_MEMORY_WINNER_KEY 78

/* consty dotycace dodawania surowcow */

#define INIT_STOCKS_VALUE 300
#define INIT_STOCKS_ADD 50
#define WORKER_STOCKS_ADD 5

/* inne consty */

#define ATTACK_DURATION 5
#define ATTACK_CODE 1
#define DEFENSE_CODE 1
#define INFO_FREQUENCY 4
#define ENDGAME_CHECK_FREQUENCY 2
#define SIGKILL 9


/* consty dotyczace pidow */
#define PIDS_QUANTITY 8
#define STOCKS_PID 0
#define PLAYER1_ATTACK_PID 1
#define PLAYER2_ATTACK_PID 2
#define PLAYER1_TRAIN_PID 3
#define PLAYER2_TRAIN_PID 4
#define STATE_PID 5
#define PLAYER1_SURRENDER_PID 6
#define PLAYER2_SURRENDER_PID 7



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


int gr1_queue_id;
int gr2_queue_id;
int *pids;

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


void interrupt(){
    int i;
    printf("\nDZIALANIE SERWERA ZOSTALO NAGLE PRZERWANE\n");

    Game_message message;
    message.mtype=KONIEC;

    /* Wyslanie wiadomosci KONIEC */

    msgsnd(gr1_queue_id,&message, sizeof(message.game_data),0);
    msgsnd(gr2_queue_id,&message, sizeof(message.game_data),0);


    for(i=0;i<PIDS_QUANTITY;i++){
        kill(pids[i],SIGKILL);
    }

    exit(0);

}


void end_of_game(){
    int i;
    printf("GRA ZOSTALA ZAKONCZONA\n");

    for(i=0;i<PIDS_QUANTITY;i++){
        kill(pids[i],SIGKILL);
    }

    exit(0);

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
    if(train_cost>player->stocks || train_list.light_infantry<0 || train_list.heavy_infantry<0  || train_list.cavalry<0 || train_list.workers<0)
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


/////////////////////////////////////////////////////

void clear_list(Game_data_struct *player){

    player->light_infantry=0;
    player->heavy_infantry=0;
    player->cavalry=0;
    player->workers=0;
    player->stocks=0;
    player->victory_points=0;
    player->winner=0;
}

int casualties_empty(Game_data_struct *casualties_list){

    int sum=casualties_list->light_infantry+casualties_list->heavy_infantry+casualties_list->cavalry;

    if(sum==0)
        return 1;
    else
        return 0;
}

/* funkcje walki */
//////////////////////////////////////////////////

double calculate_attack(Game_data_struct *units_list){

    double attack;

    attack=(units_list->light_infantry*LIGHT_INFANTRY_DAMAGE)+(units_list->heavy_infantry*HEAVY_INFANTRY_DAMAGE)+(units_list->cavalry*CAVALRY_DAMAGE)+(units_list->workers*WORKER_DAMAGE);

    return attack;
}

double calculate_defense(Game_data_struct *units_list)
{
    double defense;

    defense=(units_list->light_infantry*LIGHT_INFANTRY_DEFENSE)+(units_list->heavy_infantry*HEAVY_INFANTRY_DEFENSE)+(units_list->cavalry*CAVALRY_DEFENSE)+(units_list->workers*WORKER_DEFENSE);

    return defense;
}



void calculate_casualties(Game_data_struct *player,double attack,double defense,Game_data_struct *casualties_list){

    double ratio=attack/defense;

    casualties_list->light_infantry=floor(player->light_infantry*ratio);
    casualties_list->heavy_infantry=floor(player->heavy_infantry*ratio);
    casualties_list->cavalry=floor(player->cavalry*ratio);


    player->light_infantry-=floor(player->light_infantry*ratio);
    player->heavy_infantry-=floor(player->heavy_infantry*ratio);
    player->cavalry-=floor(player->cavalry*ratio);

}

void kill_them_all(Game_data_struct *player){
    player->light_infantry=0;
    player->heavy_infantry=0;
    player->cavalry=0;
}

int duel(Game_data_struct *player1,Game_data_struct *player2,Game_data_struct *casualties_list){

    double attack;
    double defense;

    attack=calculate_attack(player1);
    defense=calculate_defense(player2);

    if(attack>defense){
        kill_them_all(player2);
        return 1;
    }
    else{
        calculate_casualties(player2,attack,defense,casualties_list);
        return 0;
    }

}

int battle(Game_data_struct *player1,Game_data_struct *player2,int semaphore_id,int semnum,Game_data_struct *attack_casualties_list,Game_data_struct *defense_casualties_list){

    sleep(ATTACK_DURATION);

    clear_list(attack_casualties_list);
    clear_list(defense_casualties_list);
    int wynik=0;


    semaphore_down(semaphore_id,semnum);

    Game_data_struct temp_player2;

    temp_player2=*player2;

    Game_data_struct *wsk_temp_player2=&temp_player2;

    temp_player2=*player2;

    int duel1=duel(player1,player2,defense_casualties_list);

    if(duel1==1){
        wynik=1;

        player1->victory_points+=1;
    }

    int duel2=duel(wsk_temp_player2,player1,attack_casualties_list);

    if(duel2==1){
        wynik=-1;
    }

    semaphore_up(semaphore_id,semnum);

    return wynik;


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
    if(battle_list->light_infantry<=player->light_infantry && battle_list->heavy_infantry<=player->heavy_infantry && battle_list->cavalry<=player->cavalry && battle_list->light_infantry>=0 && battle_list->heavy_infantry>=0 && battle_list->cavalry>=0 && (battle_list->light_infantry+battle_list->heavy_infantry+battle_list->cavalry)>0)
        return 1;
    else
        return -1;

}

int war(Game_data_struct *attacker,Game_data_struct *defenser,Game_data_struct *battle_list,Game_data_struct *attack_casualties_list,Game_data_struct *defense_casualties_list,int semaphore_id,int semnum){

    int wynik=0;
    semaphore_down(semaphore_id,semnum);
    if(check_army(attacker,battle_list)==-1){

        semaphore_up(semaphore_id,semnum);
        return -2;
    }
    else{
        sendtroops(attacker,battle_list);
        semaphore_up(semaphore_id,semnum);

        wynik=battle(battle_list,defenser,semaphore_id,semnum,attack_casualties_list,defense_casualties_list);


        semaphore_down(semaphore_id,semnum);
        bringboysbackhome(attacker,battle_list);
        semaphore_up(semaphore_id,semnum);
        return wynik;

    }
}


void conflict(Game_data_struct *attacker,Game_data_struct *defenser,Game_data_struct *battle_list,int attacker_queue_id,int defenser_queue_id,int semaphore_id,int semnum,int *endgame,int semaphore_id_endgame,int semnum_endgame){


    Game_message attack_error_message;
    attack_error_message.mtype=BLEDNY_ATAK;

    Game_message attack_failure_message;
    attack_failure_message.mtype=PORAZKA_ATAK;

    Game_message defense_failure_message;
    defense_failure_message.mtype=PORAZKA_OBRONA;

    Game_message attack_loss_message;
    attack_loss_message.mtype=STRATY_ATAK;

    Game_message defense_loss_message;
    defense_loss_message.mtype=STRATY_OBRONA;

    Game_message attack_victory_message;
    attack_victory_message.mtype=SKUTECZNY_ATAK;

    Game_message defense_victory_message;
    defense_victory_message.mtype=SKUTECZNA_OBRONA;

    Game_data_struct attacker_casualties_list;
    Game_data_struct *attacker_casualties_list_pointer=&attacker_casualties_list;
    Game_data_struct defenser_casualties_list;
    Game_data_struct *defenser_casualties_list_pointer=&defenser_casualties_list;

    clear_list(attacker_casualties_list_pointer);
    clear_list(defenser_casualties_list_pointer);


    int war_result;

    war_result=war(attacker,defenser,battle_list,attacker_casualties_list_pointer,defenser_casualties_list_pointer,semaphore_id,semnum);

    if(war_result==-2){
        msgsnd(attacker_queue_id,&attack_error_message,sizeof(attack_error_message.game_data),0);
    }
    else if(war_result==-1){
        msgsnd(attacker_queue_id,&attack_failure_message,sizeof(attack_failure_message.game_data),0);
        msgsnd(defenser_queue_id,&defense_victory_message, sizeof(defense_victory_message.game_data),0);
    }
    else if(war_result==1){
        attacker->victory_points++;
        if(attacker->victory_points==5){
            semaphore_down(semaphore_id_endgame,semnum_endgame);
            *endgame=1;
            semaphore_up(semaphore_id_endgame,semnum_endgame);
        }

        msgsnd(attacker_queue_id,&attack_victory_message, sizeof(attack_victory_message.game_data),0);
        msgsnd(defenser_queue_id,&defense_failure_message,sizeof(defense_failure_message.game_data),0);
    }

    int attack_casualties=casualties_empty(attacker_casualties_list_pointer);
    if(attack_casualties==0){
        attack_loss_message.game_data=attacker_casualties_list;
        msgsnd(attacker_queue_id,&attack_loss_message,sizeof(attack_loss_message.game_data),0);
    }

    int defense_casualties=casualties_empty(defenser_casualties_list_pointer);
    if(defense_casualties==0){
        defense_loss_message.game_data=defenser_casualties_list;
        msgsnd(defenser_queue_id,&defense_loss_message,sizeof(defense_loss_message.game_data),0);
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

    signal(SIGINT,interrupt);

    if(args<2){
        printf("NIEWLASCIWA LICZBA ARGUMENTOW\n");
        exit(1);
    }

    int init_queue_key=atoi(argv[1]);


    /* SEMAFORY */

    /* utworzenie i inicjalizacja semaforow */

    int shared_memory_semaphore_id=semget(SEMAPHORE_KEY,SEMAPHORE_QUANTITY,IPC_CREAT|0664);
    if(shared_memory_semaphore_id==-1){
        perror("Blad przy tworzeniu semafora");
        exit(1);
    }

    /* Inicjalizacja semafora 0 */

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 0 pamieci wspoldzielonej");
        exit(1);
    }

    /* Inicjalizacja semafora 1 */

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_PLAYER1_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_PLAYER1_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 1 pamieci wspoldzielonej");
        exit(1);
    }

    /* Inicjalizacja semafora 2 */

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_PLAYER2_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_PLAYER2_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 2 pamieci wspoldzielonej");
        exit(1);
    }

    /* Inicjalizacja semafora 3 */

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_ENDGAME_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 3 pamieci wspoldzielonej");
        exit(1);
    }

    /* Inicjalizacja semafora 4 */

    if(semctl(shared_memory_semaphore_id,SHARED_MEMORY_WINNER_SEMAPHORE_NUM,SETVAL,SHARED_MEMORY_WINNER_SEMAPHORE_INIT)==-1){
        perror("Blad przy inicjalizacji semafora 4 pamieci wspoldzielonej");
        exit(1);
    }


    /* PAMIECI WSPOLDZIELONE */

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

    /* Utworzenie i przylaczenie pamieci wspoldzielonej endgame */


    int shared_memory_endgame_id=shmget(SHARED_MEMORY_ENDGAME_KEY,sizeof(int),IPC_CREAT|0664);
    if(shared_memory_endgame_id==-1){
        perror("Blad przy tworzeniu pamieci wspoldzielonej endgame");
        exit(1);
    }

    int *endgame=(int*)shmat(shared_memory_endgame_id,NULL,0);
    if(endgame==NULL){
        perror("Blad przy przylaczeniu pamieci wspoldzielonej endgame");
        exit(1);
    }

    /*Inicjalizacja wartości endgame */

    *endgame=0;

    /* Utworzenie i przylaczenie pamieci wspoldzielonej pids */

    int shared_memory_pids_id=shmget(SHARED_MEMORY_PIDS_KEY,PIDS_QUANTITY*sizeof(int),IPC_CREAT|0664);
    if(shared_memory_pids_id==-1){
        perror("Blad przy tworzeniu pamieci wspoldzielonej pids");
        exit(1);
    }
    pids=(int*)shmat(shared_memory_pids_id,NULL,0);
    if(pids==NULL){
        perror("Blad przy przylaczeniu pamieci wspoldzielonej pids");
        exit(1);
    }

    /* Utworzenie i przylaczenie pamieci wspoldzielonej winner */

    int shared_memory_winner_id=shmget(SHARED_MEMORY_WINNER_KEY, sizeof(int),IPC_CREAT|0664);
    if(shared_memory_winner_id==-1){
        perror("Blad przy tworzeniu pamieci wspoldzielonej winner");
        exit(1);
    }

    int *winner=(int*) shmat(shared_memory_winner_id,NULL,0);
    if(winner==NULL){
        perror("Blad przy przylaczeniu pamieci wspoldzielonej winner");
        exit(1);
    }


    /* Utworzenie kolejki komunikatow init */

    int init_queue_id=msgget(init_queue_key,IPC_CREAT|0664);
    if(init_queue_id==-1){
        perror("Blad przy tworzeniu poczatkowej kolejki komunikatow");
        exit(1);
    }


    /* INICJALIZACJA */

    Init_message init_message1;
    Init_message init_message2;


    printf("\033[2J\033[1;1H");
    printf("OCZEKIWANIE NA GRACZY\n");

    msgrcv(init_queue_id,&init_message1,sizeof(init_message1.init_data),ROZPOCZNIJ,0);
    int player1_id=init_message1.init_data.id_gracza;
    printf("POJAWIENIE SIE GRACZA 1\n");



    /* utworzenie kolejki komunikatow gracza1 */

    gr1_queue_id=msgget(GR1_QUEUE_KEY,IPC_CREAT|0664);
    if(gr1_queue_id==-1){
        perror("Blad przy tworzeniu kolejki gracza 1");
        exit(1);
    }


    init_message1.mtype=AKCEPTUJ;
    init_message1.init_data.id_kolejki_kom=GR1_QUEUE_KEY;

    msgrcv(init_queue_id,&init_message2,sizeof(init_message1.init_data),ROZPOCZNIJ,0);
    int player2_id=init_message2.init_data.id_gracza;

    /* utworzenie kolejki komunikatow gracza2 */

    gr2_queue_id=msgget(GR2_QUEUE_KEY,IPC_CREAT|0664);
    if(gr2_queue_id==-1){
        perror("Blad przy tworzeniu kolejki gracza 2");
        exit(1);
    }


    init_message2.mtype=AKCEPTUJ;
    init_message2.init_data.id_kolejki_kom=GR2_QUEUE_KEY;

    //wyslij wiadomosci

    msgsnd(init_queue_id,&init_message1,sizeof(init_message1.init_data),0);
    msgsnd(init_queue_id,&init_message2,sizeof(init_message2.init_data),0);

    /* KONIEC INICJALIZACJI */

    printf("\033[2J\033[1;1H");
    printf("GRA ROZPOCZETA\n");

    Game_data_struct train_list;
    Game_data_struct *train_list_pointer=&train_list;

    init_player(player1,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM);
    init_player(player2,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM);
    init_clear(train_list_pointer,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM);


    if(fork()==0){  //pobieranie surowcow
        int pid=getpid();
        if(pid==-1){
            perror("Blad przy pobieraniu STOCKS_PID");
        }
        pids[STOCKS_PID]=pid;

        while(1){
            add_stocks(player1,player2,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM);
            sleep(1);
        }

    }
    else{

        if(fork()==0){
            if(fork()==0){/* ATAKI */
                if(fork()==0){ /* ATAKI GRACZA 1 */
                    int pid=getpid();
                    if(pid==-1){
                        perror("Blad przy pobieraniu PLAYER1_ATTACK_PID");
                    }
                    pids[PLAYER1_ATTACK_PID]=pid;

                    Game_message message;
                    Game_data_struct battle_list;
                    Game_data_struct *battle_list_pointer=&battle_list;

                    while(1){
                        msgrcv(gr1_queue_id,&message,sizeof(message.game_data),ATAK,0);
                        battle_list=message.game_data;

                        conflict(player1,player2,battle_list_pointer,gr1_queue_id,gr2_queue_id,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM,endgame,shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);

                    }
                }
                else{ /* ATAKI GRACZA 2 */
                    int pid=getpid();
                    if(pid==-1){
                        perror("Blad przy pobieraniu PLAYER2_ATTACK_PID");
                    }
                    pids[PLAYER2_ATTACK_PID]=pid;

                    Game_message message;
                    Game_data_struct battle_list;
                    Game_data_struct *battle_list_pointer=&battle_list;

                    while(1){

                        msgrcv(gr2_queue_id,&message,sizeof(message.game_data),ATAK,0);
                        battle_list=message.game_data;

                        conflict(player2,player1,battle_list_pointer,gr2_queue_id,gr1_queue_id,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM,endgame,shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);

                    }
                }

            }
            else{
                if(fork()==0){ /*Treningi jednostek */
                    if(fork()==0){ /* TRENING GRACZA 1 */
                        int pid=getpid();
                        if(pid==-1){
                            perror("Blad przy pobieraniu PLAYER1_TRAIN_PID");
                        }
                        pids[PLAYER1_TRAIN_PID]=pid;
                        Game_message train_message;
                        Game_message train_failure_message;
                        train_failure_message.mtype=NIEDOST_SUROWCE;

                        Game_data_struct train_list;

                        while(1){
                            msgrcv(gr1_queue_id,&train_message, sizeof(train_message.game_data),TWORZ,0);
                            train_list=train_message.game_data;
                            if(train(player1,train_list,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM)==-1){
                                msgsnd(gr1_queue_id,&train_failure_message, sizeof(train_failure_message.game_data),0);
                            }

                        }
                    }
                    else{ /*TRENING GRACZA 2 */
                        int pid=getpid();
                        if(pid==-1){
                            perror("Blad przy pobieraniu PLAYER2_TRAIN_PID");
                        }
                        pids[PLAYER2_TRAIN_PID]=pid;
                        Game_message train_message;
                        Game_message train_failure_message;
                        train_failure_message.mtype=NIEDOST_SUROWCE;

                        Game_data_struct train_list;

                        while(1){
                            msgrcv(gr2_queue_id,&train_message, sizeof(train_message.game_data),TWORZ,0);
                            train_list=train_message.game_data;
                            if(train(player2,train_list,shared_memory_semaphore_id,SHARED_MEMORY_SEMAPHORE_NUM)==-1){
                                msgsnd(gr2_queue_id,&train_failure_message, sizeof(train_failure_message.game_data),0);
                            }

                        }
                    }

                }
                else{
                    int pid=getpid();
                    if(pid==-1){
                        perror("Blad przy pobieraniu STATE_PID");
                    }
                    pids[STATE_PID]=pid;

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


        }
        else {
                if(fork()==0){ /* poddawanie sie */

                    if(fork()==0){ /* player1 poddaj sie */
                        int pid=getpid();
                        if(pid==-1){
                            perror("Blad przy pobieraniu PLAYER1_SURRENDER_PID");
                        }
                        pids[PLAYER1_SURRENDER_PID]=pid;

                        Game_message game_message;


                        msgrcv(gr1_queue_id,&game_message, sizeof(game_message.game_data),PODDAJSIE,0);

                        semaphore_down(shared_memory_semaphore_id,SHARED_MEMORY_WINNER_SEMAPHORE_NUM);
                        *winner=player2_id;
                        semaphore_up(shared_memory_semaphore_id,SHARED_MEMORY_WINNER_SEMAPHORE_NUM);

                        semaphore_down(shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);
                        *endgame=1;
                        semaphore_up(shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);


                    }
                    else{/* player2 poddaj sie */
                        int pid=getpid();
                        if(pid==-1){
                            perror("Blad przy pobieraniu PLAYER2_SURRENDER_PID");
                        }
                        pids[PLAYER2_SURRENDER_PID]=pid;


                        Game_message game_message;

                        msgrcv(gr2_queue_id,&game_message, sizeof(game_message.game_data),PODDAJSIE,0);

                        semaphore_down(shared_memory_semaphore_id,SHARED_MEMORY_WINNER_SEMAPHORE_NUM);
                        *winner=player1_id;
                        semaphore_up(shared_memory_semaphore_id,SHARED_MEMORY_WINNER_SEMAPHORE_NUM);


                        semaphore_down(shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);
                        *endgame=1;
                        semaphore_up(shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);

                    }
                }
            else{  /*Konczenie gry*/
                    int koniecgry=0;

                    Game_message game_message;
                    game_message.mtype=ZAKONCZ;


                    do{
                        sleep(ENDGAME_CHECK_FREQUENCY);
                        semaphore_down(shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);
                        koniecgry=*endgame;
                        semaphore_up(shared_memory_semaphore_id,SHARED_MEMORY_ENDGAME_SEMAPHORE_NUM);
                    }while(koniecgry==0);


                    if(player1->victory_points==5)
                        game_message.game_data.winner=player1_id;
                    else if(player2->victory_points==5)
                        game_message.game_data.winner=player2_id;
                    else{
                        semaphore_down(shared_memory_semaphore_id,SHARED_MEMORY_WINNER_SEMAPHORE_NUM);
                        game_message.game_data.winner=*winner;
                        semaphore_up(shared_memory_semaphore_id,SHARED_MEMORY_WINNER_SEMAPHORE_NUM);
                    }


                    msgsnd(gr1_queue_id,&game_message, sizeof(game_message.game_data),0);
                    msgsnd(gr2_queue_id,&game_message, sizeof(game_message.game_data),0);


                    end_of_game();


                }
        }

    }


    return 0;
}



/* UWAGI
 * istnieje ryzyko, że komendy ataku i treningu będą nie pokolei
 * dodaj czyszczenie kolejki na poczatku
 * */

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