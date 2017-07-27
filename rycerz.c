#include "def.h"

bool dead=0;

int myRank; //my process number
int knightNum; //number of started processses
int windmillNum; //number of available windmills
Windmill *windmills; //array of available windmills

int globalClock = 0; 

sem_t sem_confirmCount; 
int confirmCount = 0; //counter of received confirmations

void shutdown(){
	printf("%sShutting down process %d\n", KWHT, myRank);
	dead = true;
}

//new thread of knight
void *knight(){
	int random;
	while(!dead){

	//choose windmill to fight
	random = rand() % windmillNum;

	//request critical section for this windmill
	requestCritcalSection(myRank, &globalClock, &windmills[random], random, knightNum);

	//check if can access crital section
	waitToEnter(&confirmCount, knightNum, &sem_confirmCount, myRank, &windmills[random]);
	sem_wait(&sem_confirmCount);

	//go into critical section
	printf("%sKNIGHT %d IS FIGHTING WINDMILL %d\n\n", KRED, myRank, random);
	sleep(rand() % 20 + 10);

	//release critical section
	printf("%sKNIGHT %d IS RESTING\n\n", KGRN, myRank);
	releaseCriticalSection(&globalClock, myRank, knightNum, random, &windmills[random]);

	//wait some time
	sleep(rand() % 10);
	}
}

void initArrays(){
	for(int i=0; i<windmillNum; i++){
		windmills[i].queueSize = 0;
	}
}

int main( int argc, char **argv ){

	sem_init(&sem_confirmCount, 0, 1); 
	sem_wait(&sem_confirmCount);

	signal(SIGTERM, shutdown);

	srand(getpid()); 

	windmillNum = atoi(argv[1]); 
	windmills = (Windmill*)malloc(sizeof(Windmill) * windmillNum);
	initArrays();

	MPI_Status status; //info about received message

	int msg[MSG_SIZE];

	MPI_Init(&argc, &argv);

	MPI_Comm_rank( MPI_COMM_WORLD, &myRank );
    MPI_Comm_size( MPI_COMM_WORLD, &knightNum );

    MPI_Barrier(MPI_COMM_WORLD);
    
    //start new knight thread to request critical section
    pthread_t thread;
    pthread_create(&thread, NULL, knight, NULL);
    pthread_detach(thread);

    //receive messages from other knights
    while(!dead){

    	MPI_Recv(msg, MSG_SIZE, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); 

    	if(status.MPI_TAG == MSG_REQUEST){
    		receiveCriticalSectionRequest(myRank, msg[1], msg[2], &windmills[msg[0]], &globalClock, msg[0]);
    	} 
    	else if(status.MPI_TAG == MSG_RELEASE){
    		receiveCriticalSectionRelease(&windmills[msg[0]], msg[1], &globalClock);
    	}
    	else if(status.MPI_TAG == MSG_CONFIRM){
    		confirmCount++;
    	}
    	else{
    		printf("%d: UNKNOWN MESSAGE\n", myRank);
    	}
    }
	
	free(windmills);
	MPI_Finalize();
}
