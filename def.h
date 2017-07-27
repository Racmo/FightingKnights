#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>       
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#include <pthread.h>
#include <semaphore.h>

#include <signal.h>

#define KGRN  "\x1B[32m"
#define KRED  "\x1B[31m"
#define KWHT  "\x1B[37m"

#define MSG_REQUEST 1 //request critical section
#define MSG_RELEASE 2 //release critical section
#define MSG_CONFIRM 3 //confirm that you got a request

#define MSG_SIZE 3 //size of msg buff 

#define MAX_QUEUE_SIZE 50//max size of windmill queue array 

typedef struct Knight
{
	int rank;
	int clock;
}Knight;

typedef struct Windmill
{
	Knight queue[MAX_QUEUE_SIZE];
	int queueSize;
}Windmill;

//send data to all other processes from 0 - processNum without yourself (myRank) (MPI_Bcast cant receive msg when sender is not known)
void myBcast(const void* msg, int msg_size, MPI_Datatype datatype,int msgTag, MPI_Comm comm, int processNum, int myRank){
	for(int i=0; i<processNum; i++){
		if(i!=myRank){
			MPI_Send(msg, msg_size, datatype, i, msgTag, comm);
		}	
	}
}

void insertSort(Knight knight, Windmill *windmill){
	int index = -1;
	for(int i=0; i<windmill->queueSize; i++){
		if(knight.clock < windmill->queue[i].clock){	
			index = i;
			break;
		}
		//if clocks are the same and there is a conflict sort by knights' rank
		else if(knight.clock == windmill->queue[i].clock){
			if(knight.rank < windmill->queue[i].rank){
				index = i;
				break;
			}
			else continue;
		}
	}
	if(index<0){
		windmill->queue[windmill->queueSize] = knight;
	}
	else{
		int j = windmill->queueSize;
		while(j > index){
			windmill->queue[j] = windmill->queue[j-1];
			j--;
		}

		windmill->queue[j] = knight;
	}

	windmill->queueSize++;
}

void removeSort(int knightRank, Windmill *windmill){
	int index = -1;
	for(int i=0; i<windmill->queueSize; i++){
		if(windmill->queue[i].rank == knightRank){
			index = i;
			break;
		}
	}
	if(index >= 0){
		for(int i=index; i<windmill->queueSize-1; i++){
			windmill->queue[i] = windmill->queue[i+1];
		}
		windmill->queueSize--;
	}
}

void requestCritcalSection(int myRank, int *globalClock, Windmill *windmill, int windmillNum, int processesNum){
	int msg[MSG_SIZE];

	*globalClock += 1;

	Knight k;
	k.rank = myRank;
	k.clock = *globalClock;

	msg[0] = windmillNum;
	msg[1] = myRank;
	msg[2] = *globalClock;

	myBcast(msg, MSG_SIZE, MPI_INT, MSG_REQUEST, MPI_COMM_WORLD, processesNum, myRank);
	insertSort(k, windmill); 
}

void receiveCriticalSectionRequest(int myRank, int whoRank, int whoClock, Windmill *windmill, int *globalClock, int windmillNum){
	if(whoClock > *globalClock){
		*globalClock = whoClock + 1;
	}
	else{
		*globalClock += 1;
	}

	Knight k;
	k.rank = whoRank;
	k.clock = whoClock; 

	insertSort(k, windmill);

	//send confirmation 
	int msg[MSG_SIZE];
	msg[0] = windmillNum;
	msg[1] = myRank;
	msg[2] = *globalClock;

	MPI_Send(msg, MSG_SIZE, MPI_INT, whoRank, MSG_CONFIRM, MPI_COMM_WORLD);	

}

bool isFirstN(int rank, Windmill *windmill, int n){
	for(int i=0; i<n; i++){
		if(windmill->queue[i].rank == rank)
			return true;
	}
	return false;
}

void waitToEnter(int *counter, int processesNum, sem_t *semaphore, int myRank, Windmill *windmill){
	//wait for confirmation from other knights
	while(1){
		if(*counter == processesNum -1){
			break;
		}
	}
	*counter = 0;
	//wait till I'm one of the first 4 in queue
	while(1){
		if(isFirstN(myRank, windmill, 4)){
			sem_post(semaphore); //let knight into the critical section
			break;
		}
	}
}

void releaseCriticalSection(int *globalClock, int myRank, int processesNum, int windmillNum, Windmill *windmill){
	*globalClock += 1;

	int msg[MSG_SIZE];
	msg[0] = windmillNum;
	msg[1] = myRank;
	msg[2] = *globalClock;

	removeSort(myRank, windmill);
	myBcast(msg, MSG_SIZE, MPI_INT, MSG_RELEASE, MPI_COMM_WORLD, processesNum, myRank);
}

void receiveCriticalSectionRelease(Windmill *windmill, int whoRank, int *globalClock){
	*globalClock += 1;
	removeSort(whoRank, windmill);
}