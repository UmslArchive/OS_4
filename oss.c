//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

#include "sharedMem.h"

//========================GLOBALS========================

typedef enum { OFF, ON } BitState;

//Constants
const int MAX_QUEUABLE_PROCESSES = 3;
const int MAX_LOG_LINES = 10000;
const int SHM_CREATE_FLAGS = IPC_CREAT | IPC_EXCL | 0777;
#define BIT_VEC_SIZE 3

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

//Shared memory pointers
sem_t* shmSemPtr = NULL;
Clock* shmClockPtr = NULL;
MSG* shmMsgPtr = NULL;
PCB* shmPCBArrayPtr = NULL;

unsigned char activeProcesses[BIT_VEC_SIZE];

//====================SIGNAL HANDLERS====================

void interruptSignalHandler(int sig);
void abortSignalHandler(int sig);

//=================FUNCTION=PROTOTYPES===================

//Shm create
sem_t* createShmSemaphore(key_t* key, size_t* size, int* shmid);
void* createSharedMemory(key_t* key, size_t* size, int* shmid);

//Cleanup
void cleanupSharedMemory(int* shmid, struct shmid_ds* ctl);
void cleanupAll();
void terminate(unsigned char activePsArr[], PCB* pcbArr);

//Process handling
int spawnProcess(Clock* mainClock, Clock* sTimes, size_t* sTimesSize, PCB* pcbArr);
void scheduleProcess();
void dispatchProcess();

//Utility
void writeLog();                                                 
void printSharedMemory(int shmid, void* shmObj);
PCB* selectPCB(PCB* pcbArr, unsigned int sPID);
void setBit(unsigned char arr[], int position, BitState setting);
int readBit(unsigned char arr[], int position);
int scanForEmptySlot(unsigned char activePsArr[]);

//========================================================
//---------------------MAIN-------------------------------
//========================================================

int main(int arg, char* argv[]) {

    //-=-=-=-=-=Initialization-=-=-=--=--=-=-=--=-

    srand(time(NULL));

    //Register signal handlers
    signal(SIGINT, interruptSignalHandler);
    signal(SIGABRT, abortSignalHandler);

    //Utility variables
    int i, j, k;
    char convertString[255];
    int exitStatus;
    PCB* pcbIterator = NULL;
    pid_t pid;

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
    Clock* spawnTimes = NULL;
    size_t spawnTimesSize = 0;
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

    //Init bit vector
    memset(activeProcesses, 0, sizeof(int) * 3);

    //-=-==-=-=-=--=Loop=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    while(1) {
        tickClock(shmClockPtr, 1, rand() % 100000000);
        fprintf(stderr, "CLOCK = %d:%d\n", shmClockPtr->seconds, shmClockPtr->nanoseconds);

        //DEBUG print bitvec
        fprintf(stderr, "bitVec: ");
        for(i = 0; i < MAX_QUEUABLE_PROCESSES; ++i) {
            fprintf(stderr, "%d ", readBit(activeProcesses, i));
        }
        fprintf(stderr, "\n");

        //Generate random spawn time
        Clock* randomSpawnTime = malloc(sizeof(Clock));
        initClock(randomSpawnTime);
        tickClock(randomSpawnTime, shmClockPtr->seconds, shmClockPtr->nanoseconds + rand() % 3000000000);

        //Scan for available process slot
        int availableSlot = scanForEmptySlot(activeProcesses);
        printf("bitvecSlot=%d\n", availableSlot);
        if(availableSlot != -1) {
            setBit(activeProcesses, availableSlot, ON);

            //Stick the randomly generated time into the prequeue
            spawnTimes = (Clock*)realloc(spawnTimes, sizeof(Clock) * (spawnTimesSize + 1));
            ++spawnTimesSize;
            spawnTimes[spawnTimesSize - 1].nanoseconds = randomSpawnTime->nanoseconds;
            spawnTimes[spawnTimesSize - 1].seconds = randomSpawnTime->seconds;
            
            //DEBUG
            fprintf(stderr, "spawnTimes: ");
            for(i = 0; i < spawnTimesSize; ++i) {
                fprintf(stderr, "%d:%d ", spawnTimes[i].seconds, spawnTimes[i].nanoseconds);
            } 
            fprintf(stderr, "\n");
            
            //iterate to the corresponding PCB array element
            pcbIterator = shmPCBArrayPtr;
            for(k = 0; k < availableSlot; ++k) {
                ++pcbIterator;
            }

            //If a processes time to spawn has come...
            for(i = 0; i < spawnTimesSize; ++i) {
                if(shmClockPtr->seconds > spawnTimes[i].seconds || 
                (shmClockPtr->seconds == spawnTimes[i].seconds &&
                        shmClockPtr->nanoseconds >= spawnTimes[i].nanoseconds)) 
                {
                    fprintf(stderr, "true\n");
                    //Remove from spawnTimes
                    for(j = i; j < spawnTimesSize - 1; ++j) {
                        spawnTimes[i] = spawnTimes[i + 1];
                    }
                    spawnTimesSize--;
                    spawnTimes = (Clock*)realloc(spawnTimes, sizeof(Clock) * (spawnTimesSize));

                    //"run"
                    setBit(activeProcesses, availableSlot, OFF);
                }
            }
        }
        else {
            fprintf(stderr, "\n");
            //terminate(activeProcesses, shmPCBArrayPtr);
        }
        



        sleep(1);
        fprintf(stderr, "\n");
        free(randomSpawnTime);
        randomSpawnTime = NULL;

    }

    wait(NULL);
    
    //-=-=-=--=-==-=Termination-=-=-=-=-==-=-=--=-=-=-=
    cleanupAll();

    return 0;
}

//===================FUNCTION=DEFINITIONS============================

int spawnProcess(Clock* mainClock, Clock* sTimes, size_t* sTimesSize, PCB* pcbArr) {
    int i, j;
    PCB* iterator = pcbArr;

    //Generate random spawn time
    Clock* randomSpawnTime = malloc(sizeof(Clock));
    initClock(randomSpawnTime);
    tickClock(randomSpawnTime, mainClock->seconds, mainClock->nanoseconds + rand() % 3000000000);
    //fprintf(stderr, "%d:%d\n", randomSpawnTime->seconds, randomSpawnTime->nanoseconds);
    free(randomSpawnTime);

    int stillMore = 1;
    while(stillMore == 1) {
        //Check for available slots
        int processSlot = scanForEmptySlot(activeProcesses);
        if(processSlot < 0) {
            return -1;
        }

        //Stick the randomly generated time into the prequeue
        sTimes = realloc(sTimes, *sTimesSize + 1);
        ++*sTimesSize;
        sTimes[*sTimesSize - 1].nanoseconds = randomSpawnTime->nanoseconds;
        sTimes[*sTimesSize - 1].seconds = randomSpawnTime->seconds;

        randomSpawnTime = NULL;


        //Scan for processes in spawnTimeQueue who need to be forked

        printf("here\n");
        for(i = 0; i < *sTimesSize; ++i) {
            //If a processes time to spawn has come...
            if(mainClock->seconds > sTimes[i].seconds || 
                (mainClock->seconds >= sTimes[i].seconds &&
                        mainClock->nanoseconds >= sTimes[i].nanoseconds)) 
            {
                //Remove from spawnTimes
                for(j = i; j < *sTimesSize - 1; ++j) {
                    sTimes[i] = sTimes[i + 1];
                }
                *sTimesSize--;

                //Iterate to the corresponding PCB
                iterator = selectPCB(pcbArr, i + 1);
                //fork
                pid_t pid = fork();
                if(pid == 0) {
                    execl("./usrPs", "usrPs", (char*) NULL);
                    exit(55);
                }
                if(pid > 0) {
                    iterator->actualPID = pid;
                }
            }
            else {
                stillMore = 0;
            }
        }
    }
   

    return 0;
}

int scanForEmptySlot(unsigned char activePsArr[]) {
    int i;
    for(i = 0; i < MAX_QUEUABLE_PROCESSES; ++i) {
        if(readBit(activePsArr, i) == OFF) {
            setBit(activePsArr, i, ON);
            return i;
        }
    }

    return -1;
}

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
            //kill(iter->actualPID, SIGQUIT);
        }
        ++iter;
    }

    fprintf(stderr, "done\n");
    exit(100);
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
//This function allows selection of specific PCBs from the array by ptr return.
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

void interruptSignalHandler(int sig) {
    fprintf(stderr, "\nOSS caught Ctrl-C interrupt.\nCleaning up...\n");
    terminate(activeProcesses, shmPCBArrayPtr);
}

void abortSignalHandler(int sig) {
    fprintf(stderr, "\nOSS caught abort signal.\nCleaning up...\n");
    terminate(activeProcesses, shmPCBArrayPtr);
}