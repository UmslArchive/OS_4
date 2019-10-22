//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

#include "sharedMem.h"

//========================GLOBALS========================

typedef enum { OFF, ON } BitState;

//Constants
const int MAX_QUEUABLE_PROCESSES = 18;
const int MAX_LOG_LINES = 10000;
const int SHM_CREATE_FLAGS = IPC_CREAT | IPC_EXCL | 0777;
const int BIT_VEC_SIZE = 3;

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

//Shm create
sem_t* createShmSemaphore(key_t* key, size_t* size, int* shmid);
void* createSharedMemory(key_t* key, size_t* size, int* shmid);

//Cleanup
void cleanupSharedMemory(int* shmid, struct shmid_ds* ctl);
void cleanupAll();
void terminate(unsigned char activePsArr[], PCB* pcbArr);

//Process handling
void spawnProcess();
void scheduleProcess();
void dispatchProcess();

//Utility
void writeLog();                                                 
void printSharedMemory(int shmid, void* shmObj);
PCB* selectPCB(PCB* pcbArr, unsigned int sPID);
void setBit(unsigned char arr[], int position, BitState setting);
int readBit(unsigned char arr[], int position);

//========================================================
//---------------------MAIN-------------------------------
//========================================================

int main(int arg, char* argv[]) {

    //-=-=-=-=-=Initialization-=-=-=--=--=-=-=--=-

    //Utility variables
    int i, j, k;
    char convertString[255];
    int exitStatus;
    pid_t pid = 0;
    PCB* pcbIterator = NULL;

    //Shared mem keys
    key_t shmSemKey = SHM_KEY_SEM;
    key_t shmClockKey = SHM_KEY_CLOCK;
    key_t shmMsgKey = SHM_KEY_MSG;
    key_t shmPCBArrayKey = SHM_KEY_PCB_ARRAY;

    //Shared mem sizes
    size_t shmSemSize = sizeof(sem_t);
    size_t shmClockSize = sizeof(Clock);
    size_t shmMsgSize = sizeof(MSG);
    size_t shmPCBArraySize = 18 * sizeof(PCB);

    //Shared mem pointers
    sem_t* shmSemPtr = 
        createShmSemaphore(&shmSemKey, &shmSemSize, &shmSemID);
    
    Clock* shmClockPtr = 
        (Clock*)createSharedMemory(&shmClockKey, &shmClockSize, &shmClockID);
    
    MSG* shmMsgPtr = 
        (MSG*)createSharedMemory(&shmMsgKey, &shmMsgSize, &shmMsgID);
    
    PCB* shmPCBArrayPtr = 
        (PCB*)createSharedMemory
            (&shmPCBArrayKey, &shmPCBArraySize, &shmPCBArrayID);

    //Queues
    unsigned int* queue1 = NULL;
    unsigned int* queue2 = NULL;
    unsigned int* queue3 = NULL;

    //Initalize shared memory
    initClock(shmClockPtr);
    pcbIterator = shmPCBArrayPtr;
    for(i = 0; i < MAX_QUEUABLE_PROCESSES; ++i) {
        initPCB(pcbIterator, i + 1, 0);
        ++pcbIterator;
    }
    pcbIterator = NULL;
    resetMSG(shmMsgPtr);

    //Bit vector containing active process flags
    unsigned char activeProcesses[BIT_VEC_SIZE];
    memset(activeProcesses, 0, sizeof(int) * 3);

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
    cleanupAll();

    return 0;
}

//===================FUNCTION=DEFINITIONS============================

sem_t* createShmSemaphore(key_t* key, size_t* size, int* shmid) {
    //Allocate shared memory and get an id
    *shmid = shmget(*key, *size, SHM_CREATE_FLAGS);
    if(*shmid < 0) {
        perror("ERROR:oss:shmget failed(semaphore)");
        cleanupAll(); //In case someone mixes up creation order
        exit(1);
    }

    //Assign pointer
    void* temp = shmat(*shmid, NULL, 0);
    if(temp == (void*) -1) {
        perror("ERROR:oss:shmat failed(semaphore)");
        cleanupAll();
        exit(1);
    }

    //Init semaphore
    if(sem_init(temp, 1, 1) == -1) {
        perror("ERROR:oss:sem_init failed");
        cleanupAll();
        exit(1);
    }

    return (sem_t*)temp;
}

void* createSharedMemory(key_t* key, size_t* size, int* shmid) {
    //Allocate shared memory and get an id
    *shmid = shmget(*key, *size, SHM_CREATE_FLAGS);
    if(*shmid < 0) {
        switch(*key) {
            case SHM_KEY_CLOCK:
            perror("ERROR:oss:shmid failed(clock)");
            break;

            case SHM_KEY_MSG:
            perror("ERROR:oss:shmid failed(msg)");
            break;

            case SHM_KEY_PCB_ARRAY:
            perror("ERROR:oss:shmid failed(pcbArray)");
            break;
        }
        cleanupAll();
        exit(10);
    }

    //Assign pointer
    void* temp = shmat(*shmid, NULL, 0);
    if(temp == (void*) -1) {
        switch(*key) {
            case SHM_KEY_CLOCK:
            perror("ERROR:oss:shmat failed(clock)");
            break;

            case SHM_KEY_MSG:
            perror("ERROR:oss:shmat failed(msg)");
            break;

            case SHM_KEY_PCB_ARRAY:
            perror("ERROR:oss:shmat failed(pcbArray)");
            break;
        }
        cleanupAll();
        exit(20);
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

void cleanupAll() {
    if(shmSemID > 0)
        cleanupSharedMemory(&shmSemID, &shmSemCtl);

    if(shmClockID > 0)   
        cleanupSharedMemory(&shmClockID, &shmClockCtl);
    
    if(shmMsgID > 0)
        cleanupSharedMemory(&shmMsgID, &shmMsgCtl);

    if(shmPCBArrayID > 0)
        cleanupSharedMemory(&shmPCBArrayID, &shmPCBArrayCtl);
}

//Frees all shared memory, and sends the signal to all living child processes to
//detach from shared memory.
void terminate(unsigned char activePsArr[], PCB* pcbArr) {
    int i;
    const PCB* iter = pcbArr;

    //Set all shared memory to deallocate upon total detachment
    cleanupAll();

    //Send Kill signals to every child process in the system for detachment
    for(i = 0; i < MAX_QUEUABLE_PROCESSES; ++i) {
        if(readBit(activePsArr, i) == ON) {
            kill(iter->actualPID, SIGILL);
        }
        ++iter;
    }
}

void printSharedMemory(int shmid, void* shmPtr) {
    int i;
    Clock* tempClock = NULL;
    MSG* tempMSG = NULL;
    PCB* tempPCB = NULL;
    unsigned int tempSec, tempNano;

    //Don't print semaphore
    if(shmid == shmSemID)
        return;

    //Print Clock
    if(shmid == shmClockID) {
        tempClock = (Clock*)shmPtr;
        fprintf(stderr, "Clock: %u:%u\n\n",
                tempClock->seconds, tempClock->nanoseconds);
    }
    
    //Print MSG
    if(shmid == shmMsgID) {
        tempMSG = (MSG*)shmPtr;
        fprintf(stderr, "MSG: simPID=%u quantum=%u\n\n",
                tempMSG->simPID, tempMSG->quantum);
    }

    //Print PCB array
    if(shmid == shmPCBArrayID) {
        tempPCB = (PCB*)shmPtr;
        for(i = 0; i < MAX_QUEUABLE_PROCESSES; ++i) {
            fprintf(stderr, "PCB#%d:\n  ", i + 1);
            fprintf(stderr, "simPID=%u\n  ", tempPCB->simPID);
            fprintf(stderr, "prio=%u\n  ", tempPCB->priority);

            tempSec = tempPCB->totalTimeAlive.seconds;
            tempNano = tempPCB->totalTimeAlive.nanoseconds;
            fprintf(stderr, "alive=%u:%u\n  ", tempSec, tempNano);

            tempSec = tempPCB->cpuTimeUsed.seconds;
            tempNano = tempPCB->cpuTimeUsed.nanoseconds;
            fprintf(stderr, "cpuUseTime=%u:%u\n  ", tempSec, tempNano);

            tempSec = tempPCB->prevBurst.seconds;
            tempNano = tempPCB->prevBurst.nanoseconds;
            fprintf(stderr, "pBurst=%u:%u\n\n", tempSec, tempNano);
        
            ++tempPCB;
        }
    }
}

//The index of the bit vector has a 1 to 1 correspondence to the simulated pid.
//This function allows selection ofspecific PCBs from the array by ptr return.
PCB* selectPCB(PCB* pcbArr, unsigned int sPID) {
    int i;

    if(sPID >= MAX_QUEUABLE_PROCESSES || sPID < 0) {
        fprintf(stderr, "ERROR: Invalid sPID selection in selectPCB()\n");
        return NULL;
    }

    PCB* temp = pcbArr;
    for(i = 0; i < sPID - 1; ++i) {
        ++temp;
    }
    
    return temp;
}

void setBit(unsigned char arr[], int position, BitState setting) {
    if(position >= MAX_QUEUABLE_PROCESSES || position < 0) {
        fprintf(stderr, "Invalid position selected in setBit()\n");
        return;
    }

    int arrIndex = position / sizeof(char);
    int bitPos = position % sizeof(char);

    unsigned char flag = 1;

    flag = flag << bitPos;
    
    switch(setting) {
        case ON:
            arr[arrIndex] |= flag;
            break;

        case OFF:
            flag = ~flag;
            arr[arrIndex] &= flag;
            break;

        default:
            fprintf(stderr, "Invalid bitvector setting selected in setBit()\n");
    }
}

int readBit(unsigned char arr[], int position) {
    if(position >= MAX_QUEUABLE_PROCESSES || position < 0) {
        fprintf(stderr, "Invalid position selected in setBit()\n");
        return -1;
    }

    int arrIndex = position / sizeof(char);
    int bitPos = position % sizeof(char);

    unsigned char flag = 1;
    flag = flag << bitPos;

    if(arr[arrIndex] & flag)
        return 1;
        
    return 0;
}