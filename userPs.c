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
sem_t* getShmSemaphore(key_t* key, size_t* size, int* shmid);
void* getSharedMemory(key_t* key, size_t* size, int* shmid);

//-------------------------------------------------------
int main(int arg, char* argv[]) {

    return 100;
}