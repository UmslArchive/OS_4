//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#include "sharedMem.h"

//========================GLOBALS========================

typedef enum { OFF, ON } BitState;

//Constants
const int MAX_QUEUABLE_PROCESSES = 18;
const int MAX_LOG_LINES = 10000;
const int SHM_CREATE_FLAGS = IPC_CREAT | IPC_EXCL | 0777;

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
void spawnProcess();
void scheduleProcess();
void dispatchProcess();
void writeLog();
void printSharedMemory(int shmid, void* shmObj);
PCB* selectPCB(PCB* pcbArr, unsigned int sPID);
void setBit(char* arr, int position, BitState setting);
int readBit(char* arr, int position);

//---------------------MAIN-------------------------------

int main(int arg, char* argv[]) {

    //-=-=-=-=-=Initialization-=-=-=--=--=-=-=--=-

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

    sem_t* shmSemPtr = createShmSemaphore(&shmSemKey, &shmSemSize, &shmSemID);                        //pointers
    Clock* shmClockPtr = (Clock*)createSharedMemory(&shmClockKey, &shmClockSize, &shmClockID);
    MSG* shmMsgPtr = (MSG*)createSharedMemory(&shmMsgKey, &shmMsgSize, &shmMsgID);
    PCB* shmPCBArrayPtr = (PCB*)createSharedMemory(&shmPCBArrayKey, &shmPCBArraySize, &shmPCBArrayID);

    //Queues
    unsigned int* queue1;
    unsigned int* queue2;
    unsigned int* queue3;

    //Initalize shared memory
    initClock(shmClockPtr);
    PCB* pcbIterator = shmPCBArrayPtr;
    for(i = 0; i < MAX_QUEUABLE_PROCESSES; ++i) {
        initPCB(pcbIterator, i + 1, 0);
        ++pcbIterator;
    }
    pcbIterator = NULL;
    resetMSG(shmMsgPtr);

    pcbIterator = selectPCB(shmPCBArrayPtr, 3);

    pcbIterator->priority = 100;

    //Bit vector containing active process flags
    char activeProcesses[3];
    memset(activeProcesses, 0, sizeof(int) * 3);
    

    setBit(&activeProcesses[0], 3, ON);
    printf("bit3=%d\n", readBit(&activeProcesses, 3));

    setBit(&activeProcesses[0], 3, OFF);
    printf("bit3=%d\n", readBit(&activeProcesses, 3));

    //-=-==-=-=-=--=Loop=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    // for(k = 0; k < 100; ++k) {
    //     spawnProcess();
    //     scheduleProcess();

    //     //Critical section
    //     sem_wait(shmSemPtr);
        
    //         dispatchProcess();

    //     sem_close(shmSemPtr);
        
    // }

    //-=-=-=--=-==-=Termination-=-=-=-=-==-=-=--=-=-=-=

    cleanupSharedMemory(&shmSemID, &shmSemCtl);
    cleanupSharedMemory(&shmClockID, &shmClockCtl);
    cleanupSharedMemory(&shmMsgID, &shmMsgCtl);
    cleanupSharedMemory(&shmPCBArrayID, &shmPCBArrayCtl);
    
    return 0;
}

//===================FUNCTION=DEFINITIONS============================

sem_t* createShmSemaphore(key_t* key, size_t* size, int* shmid) {
    //Allocate shared memory and get an id
    *shmid = shmget(*key, *size, SHM_CREATE_FLAGS);
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

    //Init semaphore
    if(sem_init(temp, 1, 1) == -1) {
        perror("ERROR:oss:sem_init failed");
        exit(1);
    }

    return temp;
}

void* createSharedMemory(key_t* key, size_t* size, int* shmid) {
    //Allocate shared memory and get an id
    *shmid = shmget(*key, *size, SHM_CREATE_FLAGS);
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

void cleanupSharedMemory(int* shmid, struct shmid_ds* ctl) {
    int cmd = IPC_RMID;
    int rtrn = shmctl(*shmid, cmd, ctl);
    if(rtrn == -1) {
        perror("ERROR:oss:shmctl failed");
        exit(1);
    }
}

void printSharedMemory(int shmid, void* shmPtr) {
    int i;
    Clock* tempClock = NULL;
    MSG* tempMSG = NULL;
    PCB* tempPCB = NULL;

    //Don't print semaphore
    if(shmid == shmSemID)
        return;

    //Print Clock
    if(shmid == shmClockID) {
        tempClock = (Clock*)shmPtr;
        fprintf(stderr, "Clock: %u:%u\n\n", tempClock->seconds, tempClock->nanoseconds);
    }
    
    //Print MSG
    if(shmid == shmMsgID) {
        tempMSG = (MSG*)shmPtr;
        fprintf(stderr, "MSG: simPID=%u quantum=%u\n\n", tempMSG->simPID, tempMSG->quantum);
    }

    //Print PCB array
    if(shmid == shmPCBArrayID) {
        tempPCB = (PCB*)shmPtr;
        for(i = 0; i < MAX_QUEUABLE_PROCESSES; ++i) {
            fprintf(stderr, "PCB#%d:\n  simPID=%u\n  prio=%u\n  alive=%u:%u\n  cpuUseTime=%u:%u\n  pBurst=%u:%u\n\n",
                i + 1, 
                tempPCB->simPID, 
                tempPCB->priority, 
                tempPCB->totalTimeAlive.seconds, tempPCB->totalTimeAlive.nanoseconds,
                tempPCB->cpuTimeUsed.seconds, tempPCB->cpuTimeUsed.nanoseconds, 
                tempPCB->prevBurst.seconds, tempPCB->prevBurst.nanoseconds
            );

            ++tempPCB;
        }
    }
}

//The index of the bit vector has a 1 to 1 correspondence to the simulated pid value of a process.
//This function allows one to select specific PCBs from the array by returning a ptr.
PCB* selectPCB(PCB* pcbArr, unsigned int sPID) {
    int i;

    if(sPID > MAX_QUEUABLE_PROCESSES) {
        fprintf(stderr, "ERROR: Invalid sPID selection\n");
        return NULL;
    }

    PCB* temp = pcbArr;
    for(i = 0; i < sPID - 1; ++i) {
        ++temp;
    }
    
    return temp;
}

void setBit(char* arr, int position, BitState setting) {
    switch(setting) {
        case ON:
            printf("on\n");
        break;

        case OFF:
            printf("off\n");
        break;
    }
}

int readBit(char* arr, int position) {
    return 100;
}