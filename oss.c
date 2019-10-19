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

//========================GLOBALS========================

//Constants
const int MAX_QUEUABLE_PROCESSES = 18;
const int MAX_LOG_LINES = 10000;

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
sem_t* createShmSemaphore(key_t* key, size_t* size, int* shmid);
void* createSharedMemory(key_t* key, size_t* size, int* shmid);
void cleanupSharedMemory(int* shmid, struct shmid_ds* ctl);

//-------------------------------------------------------

int main(int arg, char* argv[]) {

    //Utility variables
    int i, j, k;
    char convertString[255];
    int exitStatus;
    pid_t pid = 0;

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


    //Queues
    unsigned int* queue1;
    unsigned int* queue2;
    unsigned int* queue3;

    
    return 0;
}