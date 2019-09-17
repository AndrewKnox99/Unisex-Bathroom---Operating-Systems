/*					EZIPC.h				*/
#include <sys/types.h>	
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#define IPC_MAX  20
#define MAX_CHAR 100


/* NOTICE: programs compiled using different values of IPC_MAX are not
	fully compatable, since IPC_MAX is used in calculating the key
	used to request the semaphores and shared memory. */
/* GLOBAL DEFINITIONS */

int semid;
int msgid;

#define IPC_KEY		getuid()
#define SEM_COUNTING         0
#define SEM_CNT              0
#define SEM_BINARY           1
#define SEM_BIN              1


/*
SEM_TRANS converts the semaphore id to the form used by the ipc library.
*/

int EZIPC_SEM_TRANS(int sid)

{
int ipcid;
        ipcid=semget(((IPC_KEY*IPC_MAX)+sid),1,0666|IPC_CREAT);
        return(ipcid);
}

/*
SHM_TRANS converts the memory id to the form used by the ipc library.
*/

int EZIPC_SHM_TRANS(int mid)

{
int ipcid;
        ipcid=shmget(((IPC_KEY*IPC_MAX)+mid),1,0666|IPC_CREAT);
        return(ipcid);
}



/*
MSG_TRANS converts the message queue id to the form used by the ipc library
 */

int EZIPC_MSG_TRANS(int qid) {
    int ipcid;
    ipcid = msgget(((IPC_KEY * IPC_MAX) + qid), 0666 | IPC_CREAT);
    // printf("********   ipcid  %d\n",ipcid);
    return (ipcid);
}



/*
ERROR reports errors and exit
*/

int EZIPC_ERROR(char *string)

{
	printf("\nEZIPC Error Encountered:\n");
	perror(string);
	exit(0);
}


/* MESSAGES  */

void SEND(int type, char* message) {

    struct msgbuf {
        long ltype;
        char msg[MAX_CHAR];
    } toSend;

    toSend.ltype = (long) type;
    strcpy(toSend.msg, message);
    int size = sizeof (message);
    if ( msgsnd(msgid, &toSend, strlen(toSend.msg) + 1, IPC_NOWAIT) == -1) {
        printf( " msgid : %d\n", msgid);
        EZIPC_ERROR("SEND: Error sending message");
    }

}

int RECEIVE(int type, char* message) {

    struct msgbuf {
        long int ltype;
        char msg[MAX_CHAR];
    } toReceive;

    int pulled;
    if ((pulled = msgrcv(msgid, &toReceive, MAX_CHAR, (long) type, 0)) == -1) {
        EZIPC_ERROR("RECEIVE: Error getting message");
    }

    memcpy(message, toReceive.msg, pulled);
    return pulled - 1;
}



/* END MESSAGES   */


/*
SHOW returns the current value of a semaphore, this is really just for debugging
purposes.
*/

int SHOW(int sid)

{
int value;
	value=semctl(EZIPC_SEM_TRANS(semid),sid,GETVAL,0);
	return (value);
}

/*
SEM_CALL performs a designated operation on a semaphore in the
block choosen by SEM_TRANS.
	sid is the id of the semaphore to be operated on.
 	op is a number to be added to the current value of
         the semaphore. 
*/

int EZIPC_SEM_CALL(int sid,int op)


{
int x;
        struct sembuf sb;
        sb.sem_num = sid;
        sb.sem_op = op;
        sb.sem_flg = 0;
/* 	printf("sem_call: sid:%d val:%d op:%d\n",sid,SHOW(sid),op);  */
        if(( x=(semop(EZIPC_SEM_TRANS(semid),&sb,1)) )==-1)  
             { printf("TRACE SEM_CALL ERROR\n");
                EZIPC_ERROR("SEM_CALL: Semaphore ID Error");
             }
        return(x);
}

/*
SHM_ADDR attachs and returns a pointer to the shared memory block mid.
*/

char *EZIPC_SHM_ADDR(int mid)

{
char *addr;
// extern char *shmat();
	addr=shmat(EZIPC_SHM_TRANS(mid),0,0);
if ( (int)addr == -1 )
    { 
       EZIPC_ERROR("EZIPC_SHM_ADDR Error:");
   }
	return (addr);
}



/*
SEM_MAKE creates a semaphore and releases it for use.
*/

int EZIPC_SEM_MAKE(int sid,int numsems)


{
int i;
i=0;
      if((semid=semget(((IPC_KEY*IPC_MAX)+i),IPC_MAX,0666|IPC_CREAT)==-1 ))
                EZIPC_ERROR("SEM_MAKE: Semaphore Creation Error");
/*   printf("SEM_MAKE semid %d\n",semid); */
      return(semid);
}

/*
SHM_MAKE creates a shared memory block and releases it for use.
*/

void EZIPC_SHM_MAKE(int mid,int size)


{
	if(shmget(((IPC_KEY*IPC_MAX)+mid),size,0777|IPC_CREAT)==-1)
		EZIPC_ERROR("Shared Memory Creation Error");
}


/*
EZIPC_SEM_REMOVE removes all the semaphores.
*/

void EZIPC_SEM_REMOVE()
{
int x;
	for(x=0; x<=IPC_MAX; x++)
		semctl(EZIPC_SEM_TRANS(semid),x,IPC_RMID);
}

/*
SHM_REMOVE removes all the blocks of shared memory.
*/

void EZIPC_SHM_REMOVE()
{
int x;
	for(x=0;x<=IPC_MAX; x++)
		shmctl(EZIPC_SHM_TRANS(x),IPC_RMID,0);
}

/*
EZIPC_SHM_DET detach a shared memory segment
SV has a default limit of 6 attached segments
*/

int EZIPC_SHM_DET( char *addr)
{
	shmdt( addr);
        return(0);
}


/*
P and V  follow the text-book standard for the P and V operations.
sid is the number of the semaphore indicated by SEM_TRANS.
*/

void P(int sid)

{
        EZIPC_SEM_CALL(sid,-1);
}


void V(int sid)

{
char *addr;
        EZIPC_SEM_CALL(0,-1);
        addr = EZIPC_SHM_ADDR(0);
        if ( ((*(addr+1+sid))==SEM_BIN) && (SHOW(sid)==1))
                {
                /* do not increment binary semaphore past 1 */
                }
        else
                {
                EZIPC_SEM_CALL(sid,1);
                }
        EZIPC_SEM_CALL(0,1);
        EZIPC_SHM_DET(addr);
}



/*
SETUP should be the very first thing executed in your program.
it will set up all block of shared memory that the EZIPC library
uses to keep track of all the requests for shared memory and semaphores
It also begins by forking off a process to execute the program while
the parent waits for the program to terminate so that it can remove all
the IPC structures, which otherwise will not be erased.
*/

void SETUP()
{
char *Maint_Block; 
int Child;

	EZIPC_SHM_MAKE(0,2+IPC_MAX);
	Maint_Block=EZIPC_SHM_ADDR(0);
	*Maint_Block=1;
	*(Maint_Block+1)=1;

 msgid = EZIPC_MSG_TRANS(0);


	semid = EZIPC_SEM_MAKE(0,1);
	EZIPC_SEM_CALL(semid,1);
	if((Child=fork())!=0)
		{
		wait(&Child);
		EZIPC_SEM_REMOVE();
		EZIPC_SHM_REMOVE();
		exit(0);
		}
	EZIPC_SHM_DET(Maint_Block);
 		/* else continue running the program */
}

/*
SEMAPHORE creates a new semaphore and returns it's sid , it also sets the initial
value and the type of semaphore to be used, the types can be SEM_BIN, SEM_CNT, 
SEM_BINARY, SEM_COUNTING. the value of a Binary Semaphore will not get above 1.
*/

int SEMAPHORE(int type, int value)


{
int sid, x;
union sem_num {
 int	val;
 struct  semid_ds *buf;
 ushort	*array;
 } semctl_arg;

char *Maint_Block; 

	EZIPC_SEM_CALL(0,-1);
 	Maint_Block=EZIPC_SHM_ADDR(0);  
	if(*Maint_Block < IPC_MAX)
		{
		sid=(*Maint_Block)++;
		EZIPC_SEM_CALL(0,1);
		*(Maint_Block+1+sid)=type;
		if ( (type == SEM_BIN) && ( value > 1 || value < 0 ) )
		  EZIPC_ERROR("SEMAPHORE: Binary semaphore not initialized to 1 or 0");
		if (value < 0 )
		  EZIPC_ERROR("SEMAPHORE:Semaphore initialized to negative value");
		  semctl_arg.val = value;
	          semctl(EZIPC_SEM_TRANS(semid),sid,SETVAL,semctl_arg);
						  
/*		EZIPC_SEM_CALL(sid,value);   */
		}
	else
		{
		EZIPC_SEM_CALL(0,1);
		EZIPC_ERROR("SEMAPHORE: Too Many Semaphores Requested");
		}
	EZIPC_SHM_DET(Maint_Block);
	return(sid);
}

/*
SHARED_MEMORY creates a block of shared memory, and returns a pointer to the
block of memory. 
*/

char *SHARED_MEMORY(int size)

{
int mid;
char *addr;
char *Maint_Block; 

	EZIPC_SEM_CALL(0,-1);
 	Maint_Block=EZIPC_SHM_ADDR(0); 
	if(*(Maint_Block+1) < IPC_MAX)
		{
		mid=(*(Maint_Block+1))++;
		EZIPC_SEM_CALL(0,1);
		EZIPC_SHM_MAKE(mid,size);
		}
	else
		{
		EZIPC_SEM_CALL(0,1);
		EZIPC_ERROR("Too Many Shared Memory Blocks Requested");
		}
	
	EZIPC_SHM_DET(Maint_Block);
	addr=EZIPC_SHM_ADDR(mid);
	return(addr);
}


/*

EZIPC was written by Chris Argenta for Dr. Danial A. Canas
                  at The Math and Computer Science Department of
                      Wake Forest University, Winston-Salem, NC, 27109

E-mail concerning EZIPC can be sent to argenta@mthcsc.wfu.edu or
                                         canas@mthcsc.wfu.edu

EZIPC was created to help simplify the use of semaphores and shared memory
on the System V UNIX operating system, especially for teaching purposes.

EZIPC may be used, modified, and distributed freely.

*/


/*			COBEGIN/COEND construct

COBEGIN spawns "X" number of processes. the returned value for each process
will be it's relative number, the parent being 0, the children going from 
1 to "X".								*/

int COBEGIN(int X)

{
int i;
	for (i=1;i<=X;i++)
		if(fork()==0)		/* creates a new process */
			break;
	if (i>X)
		i=0;			/* assigns parent 0 */
	return(i);			/* returns relative number */
}

/*
COEND requires that the process's relative number (see COBEGIN) be sent.
it will kill and children processes that finish and suspend execution 
for the parent until all the children have been killed (either from the coend
or some other by some other means).					*/

void COEND(int X)

{
int signal;
	if(X==0)
		while (wait(&signal) != -1); /*parent waits for all children*/
	else 
		exit(0);		/* children are killed */
}
/***************************************************************************
This source code was designed and written by Chris Argenta and Ed Mallozzi
for the purpose of simplifing semaphores and concurrency on a System V UNIX
based operating system.  Any questions or comments should be addressed through
email to 'argenta@mthcsc.wfu.edu'.		 		MAY 1, 1991	
***************************************************************************/