#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "ezipc.h"
#define RAND_TIME (rand() % 2)

int numChairs;
int maxCusts;
int cusIDNum;
int barbShop;
int barbSem;
int barbChairStart;
int barbChairOccupied;
int barbChairFinish;
int cashier;
char* custsInShop;
char* haircutNum;
pid_t barber;

void Customer();

void Usr1Handler(int);

/*
 * Main sets up all the semaphores and shared memory in use, then
 * forks to create a barber and a customer spawner. Barber code
 * is in main, while the customer spawner sends customers to the
 * Customer() function.
 */
int main(int argc, char* argv[]) {
	
	SETUP();
	
	signal(SIGUSR1, Usr1Handler);

	//Semaphore for updating state of barber shop
	barbShop = SEMAPHORE(SEM_BIN, 1);

	//Semaphore that controls if barber sleeps or not
	barbSem = SEMAPHORE(SEM_CNT, 0);

	//Semaphore makes sure customer prints 'getting haircut'
	//before barber prints 'finished haircut'
	barbChairStart = SEMAPHORE(SEM_BIN, 0);

	//Semaphore to ensure only 1 customer in barber chair at a time
	barbChairOccupied = SEMAPHORE(SEM_BIN, 1);

	//Semaphore that tells barber that customer's haircut is
	//finished and barber is allowed to print
	barbChairFinish = SEMAPHORE(SEM_BIN, 0);

	//Binary semaphore represents cashier
	cashier = SEMAPHORE(SEM_BIN, 1);
	
	//Shared Memory for number of customers in shop
	custsInShop = SHARED_MEMORY(5);
	*custsInShop = 0;

	//Shared memory for number of haircuts given by barber
	haircutNum = SHARED_MEMORY(5);
	*haircutNum = 0;

	//Tracks number of customers that enter store
	cusIDNum = 0;

	pid_t barber = getpid();

	//introduction print
	printf("Barber process pid is: %d\n", barber);
	printf("Enter number of chairs and maximum number of customers: ");
	scanf("%d %d", &numChairs, &maxCusts);
	printf("Number of chairs: %d\n", numChairs);
	printf("Maximum number of customers: %d\n", maxCusts);
	printf("\nTo kill barber process when max number of customers generated and served");
	printf(", type the following command into a seperate shell:\n");
	printf("kill -USR1 %d\n", barber);
	
	pid_t pid, cusid;
	
	//Used to update shared memory later on
	int y;

	//Initially forks to barber and customer spawner
	pid = fork();

	if (pid < 0) {
		perror("Error forking: ");
	}

	//Customer spawner process
	else if (pid == 0) {
		for (int i = 1; i <= maxCusts; i++) {
			cusIDNum = i;
			cusid = fork();

			if (cusid < 0) { 
				perror("Error forking: ");
			}
				
			//Sends customer processes to Customer() function
			else if (cusid == 0) {
				Customer();
			}
			if (i != maxCusts) {	
				//Generates random number between
				sleep(RAND_TIME);
			}
		}
		//Kills customer spawner
		kill(getpid(), SIGKILL);
	}

	//Makes barber sleep when not in use
	P(barbSem);
	//Accesses *custsInShop, has to use barbShop
	P(barbShop);
	int x = *custsInShop;
	x = x - 1;
	*custsInShop = x;
	printf("Barber awakened, or there was a customer waiting there are");
	printf("/is %d seats available\n", (numChairs - *custsInShop));
	V(barbShop);


	//Barber code
	while (1) {
		//Waits for customer to enter barber seat and print 'getting haircut', increments # of haircuts
		P(barbChairStart);
		y = *haircutNum;
		y = y + 1;
		*haircutNum = y;
		printf("### Finished giving haircut to customer. That is haircut %d today\n", *haircutNum);

		//Makes barber sleep when not in use
		P(barbSem);
		//Accesses *custsInShop, uses barbShop
		P(barbShop);
		int x = *custsInShop;
		x = x - 1;
		*custsInShop = x;
		printf("Barber awakened, or there was a customer waiting there are");
		printf("/is %d seats available\n", (numChairs - *custsInShop));
		V(barbShop);	
		//Lets customer know they can proceed to cashier
		V(barbChairFinish);

	}
}

//Defines the actions taken by customers upon entering the shop
void Customer() {
	int x;
	//Critical section for updating/accessing custsInShop
	P(barbShop);
	if (cusIDNum == maxCusts) {
		printf("! ! ! Reached maximum number of customers: %d\n", maxCusts);
	}

	if (*custsInShop >= numChairs) {
		printf("OH NO! Customer %d leaves, no chairs available\n", cusIDNum);
		V(barbShop);
		kill(getpid(), SIGKILL);

	}
	else {
		x = *custsInShop;
		x = x + 1;
		*custsInShop = x;
		printf("Customer %d arrived, There are %d seats available\n", cusIDNum, (numChairs - *custsInShop));
		V(barbShop);
	}
	//Frees barber from sleep
	V(barbSem);
	//Ensures two customers can't enter barbers chair at same time
	P(barbChairOccupied);
	printf("**** HAIR CUT! Customer %d is getting a haircut\n", cusIDNum);
	sleep(RAND_TIME);
	//Customer finishes haircut
	V(barbChairStart);
	//Customer waits for barber to print
	if (cusIDNum != maxCusts) {
		P(barbChairFinish);
	}
	V(barbChairOccupied);	
	//Customer pays cashier, takes RAND_TIME seconds
	printf("Customer %d gets in line to pay cashier\n", cusIDNum);
	P(cashier);
	printf("$$ Customer %d now paying cashier\n", cusIDNum);
	sleep(RAND_TIME);
	V(cashier);
	printf("$$$$$$$ Customer %d done paying and leaves, bye!\n", cusIDNum);
	kill(getpid(), SIGKILL);
} 

//SIGUSR1 handler to terminate barber process
void Usr1Handler(int usrSig) {
	printf("\nBarber received USR1 smoke signal\n");
	printf("Total number of haircuts: %d\n", *haircutNum);
	printf("Close shop and go home, bye bye!\n");
	kill(barber, SIGQUIT);
	exit(0);
}

