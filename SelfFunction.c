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
//extern INT32 PID;
//extern INT32 CurrentProcessNumber;
//waste time
void WasteTime()
{
	printf("");
	//QPrint(QID_ready);
	//QPrint(QID_timer);
	//QPrint(QID_allprocess);
	//Waste Time
}

//dispatcher deal with the pcb within the ready queue.
///if there is no pcb with in ready queue, z502 idle.
///otherwise run the next pcb.
void dispatcher() {
	//define the pcb
	PCB* pcb;
	//allocate memory for pcb
	pcb = (PCB*)malloc(sizeof(PCB));
	if (pcb == 0)
		printf("We didn't complete the malloc in pcb.");
	MEMORY_MAPPED_IO mmio;
	//QRemoveHead(QID_ready);
	//check the ready queue
	///if ready queue is empty then idle, if not,process next context
	//QPrint(QID_ready);
	//QPrint(QID_timer);
	//QPrint(QID_allprocess);
	while (QNextItemInfo(QID_ready) == -1)
	{ //watetime here
		//mmio.Mode = Z502Action;
		//mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		//MEM_WRITE(Z502Idle, &mmio);
		CALL(WasteTime());
	}
	//printf("dispatcher!!!!!!\n");
	//QPrint(QID_ready);
	//QPrint(QID_timer);
	//start the next context
	{
		pcb = QRemoveHead(QID_ready);
		currentPCB = pcb;
		mmio.Mode = Z502StartContext;
		mmio.Field1 = pcb->newContext;
		//printf("%s", pcb->processName);
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		mmio.Field3 = 0;
		MEM_WRITE(Z502Context, &mmio);
	}

}



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
	printf("%s\n", QGetName(QID_ready));
	//create a timer quue to store pcb
	QID_timer = QCreate("timerQueue");
	printf("%s\n", QGetName(QID_timer));
	//create a all process quue to store pcb
	QID_allprocess = QCreate("allProcessQueue");
	printf("%s\n", QGetName(QID_allprocess));
	//create a suspend queue QID_suspend
	QID_suspend = QCreate("suspendQueue");
	printf("%s\n", QGetName(QID_suspend));
	//create a temp queue QID_temp
	QID_temp = QCreate("tempQueue");
	printf("%s\n", QGetName(QID_temp));
	//get the current time
	
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	pcb->timeCreated = (INT32)mmio.Field1;
	

	//check which test we will run
	if (argc > 1) { 
		if (strcmp(argv[1], "sample") == 0) { mmio.Field2 = (long)SampleCode; pcb->address = mmio.Field2; strcpy(pcb->processName, "sample");}
		else if (strcmp(argv[1], "test0") == 0) { mmio.Field2 = (long)test0; pcb->address = mmio.Field2; strcpy(pcb->processName, "test0");}
		else if (strcmp(argv[1], "test1") == 0) { mmio.Field2 = (long)test1; pcb->address = mmio.Field2; strcpy(pcb->processName, "test01");}
		else if (strcmp(argv[1], "test2") == 0) { mmio.Field2 = (long)test2; pcb->address = mmio.Field2; strcpy(pcb->processName, "test02"); }
		else if (strcmp(argv[1], "test3") == 0) { mmio.Field2 = (long)test3; pcb->address = mmio.Field2; strcpy(pcb->processName, "test03"); }
		else if (strcmp(argv[1], "test4") == 0) { mmio.Field2 = (long)test4; pcb->address = mmio.Field2; strcpy(pcb->processName, "test04"); }
		else if (strcmp(argv[1], "test5") == 0) { mmio.Field2 = (long)test5; pcb->address = mmio.Field2; strcpy(pcb->processName, "test05"); }
		else if (strcmp(argv[1], "test6") == 0) { mmio.Field2 = (long)test6; pcb->address = mmio.Field2; strcpy(pcb->processName, "test06"); }
		else if (strcmp(argv[1], "test7") == 0) { mmio.Field2 = (long)test7; pcb->address = mmio.Field2; strcpy(pcb->processName, "test07"); }
		else if (strcmp(argv[1], "test8") == 0) { mmio.Field2 = (long)test8; pcb->address = mmio.Field2; strcpy(pcb->processName, "test08"); }
		//else if (strcmp(argv[1], "testX") == 0) { mmio.Field2 = (long)testX; pcb->address = mmio.Field2; strncpy(pcb->processName, "testX", sizeof("testX")); }

	}

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	//mmio.Field2 = (long)test1;
	mmio.Field3 = (long)PageTable;
	MEM_WRITE(Z502Context, &mmio);   // Start this new Context Sequence
	//put the infor into pcb node
	pcb->PID = PID;
	PID++;
	//printf("%d\n", PID);
	pcb->newContext = mmio.Field1;
	pcb->pageTable = PageTable;
	pcb->priority = 1;
	pcb->suspendFlag = 0;
	CurrentProcessNumber ++;
	//pcb->timeCreated = 0;
	// 
	///put the pcb into ready queue
	//QInsert(QID_ready, pcb->priority, pcb);
	currentPCB = pcb;
	QInsert(QID_allprocess, pcb->priority, pcb);

	////testing
	//QInsert(QID_ready, 3, pcb);
	//QInsert(QID_ready, 4, pcb);
	//QInsert(QID_ready, 2, pcb);
	//ttttt1 = QRemoveHead(QID_ready);
	//ttttt = QNextItemInfo(QID_ready);
	//QPrint(QID_ready);
	//printf("%s\n", QGetName(QID_ready));
	//QPrint(QID_ready);
	//QPrint(QID_timer);
	//QPrint(QID_allprocess);

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
	//printf("druing  :%d\n", during);
	MEMORY_MAPPED_IO mmio;
	INT32 Status;
	INT32 current_time;
	PCB *timerpcb;
	PCB *nextpcb;
	char Success[] = "      Action Failed\0        Action Succeeded";
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
		current_time = (INT32)mmio.Field1;
	}
	READ_MODIFY(TimerQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_timer);
	//printf("%s\n", &(Success[SPART * LockResult_timer]));


	timerpcb = currentPCB;
	timerpcb->timeCreated = current_time + during;
	QInsert(QID_timer, (timerpcb->timeCreated), timerpcb);
	//check timer first
	//mmio.Mode = Z502Status;
	//mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	//MEM_READ(Z502Timer, &mmio);
	//Status = mmio.Field1;
	//if (Status == DEVICE_FREE)//no timer is inuse now and start the timer

	
	timerpcb = QNextItemInfo(QID_timer);
	while (QNextItemInfo(QID_timer) != -1 && timerpcb->timeCreated - current_time <= 0) {
			timerpcb = QRemoveHead(QID_timer);
			timerpcb->timeCreated = current_time;
			//QInsert(QID_ready, (timerpcb->priority), timerpcb);
			if (timerpcb->suspendFlag != 1) {
				QInsert(QID_ready, (timerpcb->priority), timerpcb);
			}
			else {
				QInsert(QID_suspend, timerpcb->priority, timerpcb);
			}
			timerpcb = QNextItemInfo(QID_timer);
			if (timerpcb == -1) {
				break;
			}
	}

	if (QNextItemInfo(QID_timer) != -1) {
			mmio.Mode = Z502Start;
			//printf("time for timer : %d \n", timerpcb->timeCreated - current_time);
			mmio.Field1 = timerpcb->timeCreated - current_time;   // set the time of timer
			mmio.Field2 = mmio.Field3 = 0;
			MEM_WRITE(Z502Timer, &mmio);
	}

	READ_MODIFY(TimerQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_timer);
	//printf("%s\n", &(Success[SPART * LockResult_timer]));




/*	{
		//QPrint(QID_timer);
		timerpcb = QNextItemInfo(QID_timer);
		//printf("timer time:%d\n", (timerpcb->timeCreated - current_time));
		//startTimer(timerpcb->timeCreated - current_time);
		// Start the timer - here's the sequence to use
		mmio.Mode = Z502Start;
		printf("time for timer : %d \n", timerpcb->timeCreated - current_time);
		mmio.Field1 = timerpcb->timeCreated - current_time;   // set the time 
		//printf("Sleeptime : %d \n", mmio.Field1);
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
	}
	//QPrint(QID_timer);*/
	


	/*
	
	//check timer first
	mmio.Mode = Z502Status;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Timer, &mmio);
	Status = mmio.Field1;
	if (Status == DEVICE_FREE)//no timer is inuse now and start the first timer
	{
		timerpcb = currentPCB;
		timerpcb->timeCreated = current_time + during;
		///enqueue to timer queue by time order
		QInsert(QID_timer, (timerpcb->timeCreated), timerpcb);
		timerpcb = QNextItemInfo(QID_timer);


		QPrint(QID_timer);
		// Start the timer - here's the sequence to use
		mmio.Mode = Z502Start;
		mmio.Field1 = timerpcb->timeCreated - current_time; // set the time of timer
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
		//enqueue the current context into timer queue
		///dequeue from ready queue
		//timerpcb = QRemoveHead(QID_ready);
		//timerpcb = currentPCB;
		//timerpcb->timeCreated = current_time + during;
		///enqueue to timer queue by time order
		//QInsert(QID_timer, (timerpcb->timeCreated), timerpcb);
		//QPrint(QID_ready);
		//QPrint(QID_timer);
		//QPrint(QID_allprocess);
	}//printf("Got expected (DEVICE_FREE) result for Status of Timer\n");//and do the next
	else//timer is inuse now.put the current context into timer queue
	{
		//enqueue the current context into timer queue
		///dequeue from ready queue
		//timerpcb = QRemoveHead(QID_ready);
		timerpcb = currentPCB;
		timerpcb->timeCreated = current_time + during;
		//printf("teetetetetete : %d", timerpcb->timeCreated);
		///enqueue to timer queue by time order
		QInsert(QID_timer, (timerpcb->timeCreated), timerpcb);
		//
		timerpcb = QNextItemInfo(QID_timer);
		//printf("timer time:%d\n", (timerpcb->timeCreated - current_time));
		//startTimer(timerpcb->timeCreated - current_time);
		// Start the timer - here's the sequence to use
		mmio.Mode = Z502Start;
		mmio.Field1 = timerpcb->timeCreated - current_time;   // set the time of timer
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
	}*/
	dispatcher();
}
///create process and put it into ready queue
void createProcess(PCB* currentPCB1) {
	MEMORY_MAPPED_IO mmio;
	void* PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)currentPCB1->address;
	mmio.Field3 = (long)PageTable;
	MEM_WRITE(Z502Context, &mmio);
	currentPCB1->newContext = (long*)mmio.Field1;
	currentPCB1->pageTable = PageTable;
	//put it into ready queue
	QInsert(QID_ready, currentPCB1->priority, currentPCB1);
	QInsert(QID_allprocess, currentPCB1->priority, currentPCB1);
	//printf("%s", currentPCB->processName);
	//QPrint(QID_ready);
	//QPrint(QID_timer);
	//QPrint(QID_allprocess);

}
/////check by name so see it exit or not
int checkName(char* name) {
	PCB* temppcb;
	int temppcb1 = -1;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID_allprocess, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID_allprocess);
		QInsert(QID_temp, temppcb->priority, temppcb);
		if (strcmp(name, temppcb->processName) == 0) {
			temppcb1 = temppcb->PID;
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		QInsert(QID_allprocess, temppcb->priority, temppcb);
	}
	//QPrint(QID_temp);
	return temppcb1;
}



//check pid from allprocess
int checkPID(INT32 PID) {
	PCB* temppcb;
	int temppcb1 = -1;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID_allprocess, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID_allprocess);
		QInsert(QID_temp, temppcb->priority, temppcb);
		if (PID == temppcb->PID) {
			temppcb1 = temppcb->PID;
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		QInsert(QID_allprocess, temppcb->priority, temppcb);
	}
	return temppcb1;
}


///delete a pcd from spcific queue
void dequeueByPid(INT32 PID, INT32 QID) {
	PCB* temppcb;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID);
		QInsert(QID_temp, temppcb->priority, temppcb);
		if (PID == temppcb->PID) {
			QRemoveItem(QID_temp, temppcb);
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		if (QID == QID_timer) {
			QInsert(QID, temppcb->timeCreated, temppcb);
		}else{

			QInsert(QID, temppcb->priority, temppcb);
		}
				
				

	}
	//QPrint(QID_temp);
}


//////suspend pid
void suspendByPid(INT32 PID, INT32 QID) {
	PCB* temppcb;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID);
		
		if (PID == temppcb->PID) {
			temppcb->suspendFlag = 1;
			QInsert(QID_suspend, temppcb->priority, temppcb);
		}
		else {
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		if (QID == QID_timer) {
			QInsert(QID, temppcb->timeCreated, temppcb);
		}
		else {

			QInsert(QID, temppcb->priority, temppcb);
		}
	}
	////QPrint(QID_temp);
}


//suspend pcb to timer queue
void suspendByPid_timer(INT32 PID) {
	PCB* temppcb;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID_timer, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID_timer);

		if (PID == temppcb->PID) {
			temppcb->suspendFlag = 1;
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
		else {
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		QInsert(QID_timer, temppcb->timeCreated, temppcb);
	}
	//QPrint(QID_temp);
}


///check a pid is suspend or not
int checkPID_suspend(INT32 PID) {
	PCB* temppcb;
	int temppcb1 = -1;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID_suspend, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID_suspend);
		QInsert(QID_temp, temppcb->priority, temppcb);
		if (PID == temppcb->PID) {
			temppcb1 = temppcb->PID;
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		QInsert(QID_suspend, temppcb->priority, temppcb);
	}
	return temppcb1;
}


//resume a pcd
void resumePID(INT32 PID) {
	PCB* temppcb;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID_suspend, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID_suspend);

		if (PID == temppcb->PID) {
			temppcb->suspendFlag = 0;
			QInsert(QID_ready, temppcb->priority, temppcb);
		}
		else {
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		QInsert(QID_suspend, temppcb->priority, temppcb);
	}
	//QPrint(QID_temp);
}
