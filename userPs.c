//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

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

void quitSignalHandler(int sig);

//=================FUNCTION=PROTOTYPES===================
sem_t* attachShmSemaphore(key_t* key, size_t* size, int* shmid);
void* attachSharedMemory(key_t* key, size_t* size, int* shmid);
int checkForMSG();
void detachAll();

void run();

//-------------------------------------------------------
int main(int arg, char* argv[]) {

    //-=-==--=-=-==-=-Initialization-==-=-=-=-=-=--==-=-

    //Register signal handler
    signal(SIGQUIT, quitSignalHandler);

    //Utility variables
    int i, j, k;
    char convertString[255];

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

    //Shared mem ptrs
    sem_t* shmSemPtr = 
        attachShmSemaphore(&shmSemKey, &shmSemSize, &shmSemID);

    Clock* shmClockPtr = 
        (Clock*)attachSharedMemory(&shmClockKey, &shmClockSize, &shmClockID);

    MSG* shmMsgPtr = 
        (MSG*)attachSharedMemory(&shmMsgKey, &shmMsgSize, &shmMsgID);

    PCB* shmPCBArrayPtr = 
        (PCB*)attachSharedMemory
            (&shmPCBArrayKey, &shmPCBArraySize, &shmPCBArrayID);

    //Set the iterator
    PCB* pcbIterator = shmPCBArrayPtr;
    for(i = 0; i < atoi(argv[1]) - 1; ++i) {
        pcbIterator++;
    }

    //-==-=-=-=-=-=-=-=-Loop-==--=-=-=-=-=-=-==-=-=-=-=--==

    fprintf(stderr, "Child: simPID=%d\n", pcbIterator->simPID);

    //-=-=-==-=-=-=-Finalization/Termination--==--==-=--==-

    return 100;
}

//=================================================================
//-----------------------Function Defs----------------------------------
//=================================================================

sem_t* attachShmSemaphore(key_t* key, size_t* size, int* shmid) {
    //Retrieve shmid
    *shmid = shmget(*key, *size, SHM_ATTACH_FLAGS);
    if(*shmid < 0) {
        perror("ERROR:usrPs:shmget failed(semaphore)");
        exit(1);
    }

    //Assign pointer
    void* temp = (void*)shmat(*shmid, NULL, 0);
    if(temp == (void*) -1) {
        perror("ERROR:usrPs:shmat failed(semaphore)");
        exit(1);
    }

    return (sem_t*)temp;
}

void* attachSharedMemory(key_t* key, size_t* size, int* shmid) {
    //Allocate shared memory and get an id
    *shmid = shmget(*key, *size, SHM_ATTACH_FLAGS);
    if(*shmid < 0) {
        switch(*key) {
            case SHM_KEY_CLOCK:
            perror("ERROR:usrPs:shmid failed(clock)");
            break;

            case SHM_KEY_MSG:
            perror("ERROR:usrPs:shmid failed(msg)");
            break;

            case SHM_KEY_PCB_ARRAY:
            perror("ERROR:usrPs:shmid failed(pcbArray)");
            break;
        }
        detachAll();
        exit(10);
    }

    //Assign pointer
    void* temp = shmat(*shmid, NULL, 0);
    if(temp == (void*) -1) {
        switch(*key) {
            case SHM_KEY_CLOCK:
            perror("ERROR:usrPs:shmat failed(clock)");
            break;

            case SHM_KEY_MSG:
            perror("ERROR:usrPs:shmat failed(msg)");
            break;

            case SHM_KEY_PCB_ARRAY:
            perror("ERROR:usrPs:shmat failed(pcbArray)");
            break;
        }
        detachAll();
        exit(20);
    }

    return temp;
}

void detachAll() {
    if(shmClockID > 0)
        shmdt(&shmClockID);
    if(shmMsgID > 0)
        shmdt(&shmMsgID);
    if(shmPCBArrayID > 0)
        shmdt(&shmPCBArrayID);
    if(shmSemID > 0)
        shmdt(&shmSemID);

    fprintf(stderr, "Child PID(%d), simPID(%d) detached\n", getpid(), 5);
}

void quitSignalHandler(int sig) {
    detachAll();
    exit(99);
}