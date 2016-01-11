#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>

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

typedef struct Message{
    long mtype;
    int liczba;

}Message;

/* struktury */

typedef struct game_data_struct {
    int lek_piech;
    int cie_piech;
    int jazda;
    int robot;
    int surowce;
    int skut_ataki;
    int wygral;
}game_data_struct;

typedef struct init_data_struct {
    int id_kolejki_kom;
    int id_gracza;
}init_data_struct;

typedef struct game_message {
    long mtype;
    struct game_data_struct game_data;
}game_message;

typedef struct init_message {
    long mtype;
    struct init_data_struct init_data;
}init_message;

int main(int args, char argv[]){

    int init_queue=msgget(INIT_QUEUE_KEY,IPC_CREAT|0664);


    Message message;
    message.mtype=ROZPOCZNIJ;
    message.liczba=2137;

    msgsnd(init_queue,&message,4,0);


    return 0;
}