//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

#ifndef SHARED_MEM_H
#define SHARED_MEM_H

//Keys
#define SHM_KEY_SEM         0x66666666
#define SHM_KEY_PCB_ARRAY   0x77777777
#define SHM_KEY_MSG         0x88888888
#define SHM_KEY_CLOCK       0x99999999

//----------------------------------------

//Structs
typedef struct {
    unsigned int seconds;
    unsigned int nanoseconds;
} Clock;

typedef struct {
    unsigned int simPID;
    unsigned int priority;
    Clock cpuTimeUsed;
    Clock totalTimeAlive;
    Clock prevBurst;
} PCB;

typedef struct {
    unsigned int simPID;
    unsigned int quantum;
} MSG;

//Struct utility function prototypes
void initClock(Clock* clock);
void initPCB(PCB* pcb, unsigned int sPID, unsigned int prio);
void tickClock(Clock* mainClock, unsigned int sec, unsigned int nanosec);
void resetMSG(MSG* msg);
void setMSG(MSG* msg, unsigned int sPID, unsigned int quant);

#endif