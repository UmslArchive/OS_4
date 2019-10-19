//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#include "sharedMem.h"

//=======================GLOBALS========================

//Constants
const int SHM_ATTACH_FLAGS = 0777;

//Shared memory IDs
int shmSemID = 0;
int shmMsgID = 0;
int shmClockID = 0;
int shmPCBArrayID = 0;

//Shared memory control structs
struct shmid_ds shmSemCtl;
struct shmid_ds shmMsgCtl;
struct shmid_ds shmClockCtl;
struct shmid_ds shmPCBArrayCtl;

//====================SIGNAL HANDLERS====================

//=================FUNCTION=PROTOTYPES===================
sem_t* attachShmSemaphore(key_t* key, size_t* size, int* shmid);
void* attachSharedMemory(key_t* key, size_t* size, int* shmid);
int checkForMSG();

void run();

//-------------------------------------------------------
int main(int arg, char* argv[]) {

    //-=-==--=-=-==-=-Initialization-==-=-=-=-=-=--==-=-

    //Utility variables
    int i, j, k;
    char convertString[255];

    //Shared memory variables
    key_t shmSemKey = SHM_KEY_SEM;                  //keys
    key_t shmClockKey = SHM_KEY_CLOCK;
    key_t shmMsgKey = SHM_KEY_MSG;
    key_t shmPCBArrayKey = SHM_KEY_PCB_ARRAY;

    size_t shmSemSize = sizeof(sem_t);              //sizes
    size_t shmClockSize = sizeof(Clock);
    size_t shmMsgSize = sizeof(MSG);
    size_t shmPCBArraySize = 18 * sizeof(PCB);

    sem_t* shmSemPtr = NULL;                        //pointers
    Clock* shmClockPtr = NULL;
    MSG* shmMsgPtr = NULL;
    PCB* shmPCBArrayPtr = NULL;

    //-==-=-=-=-=-=-=-=-Loop-==--=-=-=-=-=-=-==-=-=-=-=--==

    while(1) {
        sem_wait(shmSemPtr);
            while(checkForMSG == 0);
        sem_close(shmSemPtr);
        run();
    }

    //-=-=-==-=-=-=-Finalization/Termination--==--==-=--==-

    return 100;
}

sem_t* attachShmSemaphore(key_t* key, size_t* size, int* shmid) {
    //Retrieve shmid
    *shmid = shmget(*key, *size, SHM_ATTACH_FLAGS);
    if(*shmid < 0) {
        perror("ERROR:oss:shmget failed(semaphore)");
        exit(1);
    }

    //Assign pointer
    sem_t* temp = (sem_t*)shmat(*shmid, NULL, 0);
    if(temp == (sem_t*) -1) {
        perror("ERROR:oss:shmat failed(semaphore)");
        exit(1);
    }

    return temp;
}

void* attachSharedMemory(key_t* key, size_t* size, int* shmid) {
    //Retrieve shmid
    *shmid = shmget(*key, *size, SHM_ATTACH_FLAGS);
    if(*shmid < 0) {
        perror("ERROR:oss:shmget failed");
        exit(1);
    }

    //Assign pointer
    void* temp = shmat(*shmid, NULL, 0);

    switch(*key) {
        case SHM_KEY_CLOCK:
        if(temp == (Clock*)-1) {
            perror("ERROR:oss:shmat failed(clock)");
            exit(1);
        }
        break;

        case SHM_KEY_MSG:
        if(temp == (MSG*)-1) {
            perror("ERROR:oss:shmat failed(msg)");
            exit(1);
        }
        break;

        case SHM_KEY_PCB_ARRAY:
        if(temp == (PCB*)-1) {
            perror("ERROR:oss:shmat failed(pcbArray)");
            exit(1);
        }
        break;
    }

    return temp;
}