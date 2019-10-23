//Author:   Colby Ackerman
//Class:    CS4760 Operating Systems
//Assign:   #4
//Date:     10/22/19
//-----------------------------------------------------

#include "sharedMem.h"

void tickClock(Clock* mainClock, unsigned int sec, unsigned int nanosec) {
    //Subtract seconds off of nanoseconds if >= 1,000,000,000 
    while(nanosec >= 1000000000) {
        ++sec;
        nanosec -= 1000000000;
    }

    //Set the clock
    if(mainClock->nanoseconds + nanosec < 1000000000) {
        mainClock->nanoseconds += nanosec;
    }
    else {
        ++sec;
        mainClock->nanoseconds = nanosec - (1000000000 - mainClock->nanoseconds);
    }
    
    mainClock->seconds += sec;
}

void initClock(Clock* clock){
    clock->nanoseconds = 0;
    clock->seconds = 0;
}

void initPCB(PCB* pcb, unsigned int sPID, unsigned int prio){
    pcb->simPID = sPID;     //corresponds to bit vector index (converted to count from 1)
    pcb->priority = prio;
    pcb->cpuTimeUsed.nanoseconds = 0;
    pcb->cpuTimeUsed.seconds = 0;
    pcb->prevBurst.nanoseconds = 0;
    pcb->prevBurst.seconds = 0;
    pcb->totalTimeAlive.nanoseconds = 0;
    pcb->totalTimeAlive.seconds = 0;
}

void resetMSG(MSG* msg) {
    msg->simPID = 0;
    msg->quantum = 0;
}

void setMSG(MSG* msg, unsigned int sPID, unsigned int quant){
    msg->simPID = sPID;
    msg->quantum = quant;
}

void subtractTimes(Clock* newTime, Clock* t1, Clock* t2) {
    *newTime = *t1;
    int nanoDiff = t1->nanoseconds - t2->nanoseconds;

    //Carry
    if(nanoDiff < 0) {
        newTime->seconds--;
        newTime->nanoseconds = 1000000000 + nanoDiff;
    }
    else {
        newTime->nanoseconds = nanoDiff;
    }

    newTime->seconds = t1->seconds - t2->seconds;
}