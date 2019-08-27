#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
//#include			"QueueManager.c"
//include the data we created
#include			"SelfFunction.h"
//define the variables




//osCreateProcess
///once the code goes into onInit, then it will pass all the data into here and create the first process.
///then it will put the process into ready queue
///input:argc argv
///output:
void osCreatProcess(int argc, char* argv[]) {
	//INT32 PID = 0;
	//INT32 QID_ready = 0;
	void* PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	MEMORY_MAPPED_IO mmio;
	//define the pcb
	PCB *pcb;
	//allocate memory for pcb
	pcb = (PCB*)malloc(sizeof(PCB));
	if (pcb == 0)
		printf("We didn't complete the malloc in pcb.");

	
	//create a ready queue to store pcb
	QID_ready = QCreate("readyQueue");
	//create a timer quue to store pcb
	QID_timer = QCreate("timerQueue");
	//get the current time
	{
		mmio.Mode = Z502ReturnValue;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_READ(Z502Clock, &mmio);
		pcb->timeCreated = mmio.Field1;
	}

	//check which test we will run
	if (argc > 1) { 
		if (strcmp(argv[1], "sample") == 0) { mmio.Field2 = (long)SampleCode; pcb->address = mmio.Field2; strncpy(pcb->processName, "sample", sizeof("sample"));}
		else if (strcmp(argv[1], "test0") == 0) { mmio.Field2 = (long)test0; pcb->address = mmio.Field2; strncpy(pcb->processName, "test0", sizeof("test0"));}
		else if (strcmp(argv[1], "test1") == 0) { mmio.Field2 = (long)test1; pcb->address = mmio.Field2; strncpy(pcb->processName, "test01", sizeof("test1"));}
		else if (strcmp(argv[1], "test2") == 0) { mmio.Field2 = (long)test2; pcb->address = mmio.Field2; strncpy(pcb->processName, "test02", sizeof("test2")); }
	}

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	//mmio.Field2 = (long)test1;
	mmio.Field3 = (long)PageTable;
	MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence
	//put the infor into pcb node
	pcb->PID = PID + 1;
	pcb->newContext = mmio.Field1;
	pcb->pageTable = PageTable;
	pcb->priority = 1;
	//pcb->timeCreated = 0;
	// 
	///put the pcb into ready queue
	QInsert(QID_ready, pcb->priority, pcb);

	////testing
	//QInsert(QID_ready, 3, pcb);
	//QInsert(QID_ready, 4, pcb);
	//QInsert(QID_ready, 2, pcb);
	//ttttt1 = QRemoveHead(QID_ready);	//ttttt = QNextItemInfo(QID_ready);	//QPrint(QID_ready);	//printf("%s\n", QGetName(QID_ready));	

	mmio.Mode = Z502StartContext;
	// Field1 contains the value of the context returned in the last call
	// Suspends this current thread
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}

//start timer function
////check timer status first and then start a timer with given time.
//input: time integar number
//output:
void startTimer(int during) {
	MEMORY_MAPPED_IO mmio;
	INT32 Status;
	INT32 current_time;
	PCB *timerpcb;
	PCB *nextpcb;
	//allocate memory for pcb
	timerpcb = (PCB*)malloc(sizeof(PCB));
	if (timerpcb == 0)
		printf("We didn't complete the malloc in pcb.");
	nextpcb = (PCB*)malloc(sizeof(PCB));
	if (nextpcb == 0)
		printf("We didn't complete the malloc in pcb.");
	//get the current time
	{
		mmio.Mode = Z502ReturnValue;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_READ(Z502Clock, &mmio);
		current_time = mmio.Field1;
	}
	//check timer first
	mmio.Mode = Z502Status;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Timer, &mmio);
	Status = mmio.Field1;
	if (Status == DEVICE_FREE)//no timer is inuse now and start the first timer
	{
		// Start the timer - here's the sequence to use
		mmio.Mode = Z502Start;
		mmio.Field1 = during;   // set the time of timer
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
		//enqueue the current context into timer queue
		///dequeue from ready queue
		timerpcb = QRemoveHead(QID_ready);
		timerpcb->timeCreated = current_time;
		///enqueue to timer queue by time order
		QInsert(QID_timer, (timerpcb->timeCreated + during), timerpcb);
	}//printf("Got expected (DEVICE_FREE) result for Status of Timer\n");//and do the next
	else//timer is inuse now.put the current context into timer queue
	{
		//enqueue the current context into timer queue
		///dequeue from ready queue
		timerpcb = QRemoveHead(QID_ready);
		timerpcb->timeCreated = current_time;
		///enqueue to timer queue by time order
		QInsert(QID_timer, (timerpcb->timeCreated + during), timerpcb);
	}
		//printf("Got erroneous result for Status of Timer\n");
	//check the ready queue
	///if ready queue is empty then idle, if not,process next context
	if (QNextItemInfo(QID_ready) == -1)
	{ //idle the current context and wait for the interrupt
		mmio.Mode = Z502Action;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Idle, &mmio);
	}
	else//start the next context
	{
		nextpcb = QNextItemInfo(QID_ready);
		mmio.Mode = Z502StartContext;
		mmio.Field1 = nextpcb->newContext;
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		mmio.Field3 = 0;
		MEM_WRITE(Z502Context, &mmio);
	}
}

