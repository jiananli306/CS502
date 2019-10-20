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
///if there is no pcb with in ready queue, waste time.
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
	//scheduler printer
	
	//start the next context
	{
		pcb = QRemoveHead(QID_ready);
		currentPCB = pcb;
		//SP_print("aaaaaaa", currentPCB->PID);
		//SP_print("dispacher", currentPCB->PID);
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
	//create a suspend message queue to waite recieve data
	QID_suspendbyMessage = QCreate("suspendmessageQueue");
	printf("%s\n", QGetName(QID_suspendbyMessage));
	//create a message_sendqueue for sending message to stay
	message_sendqueue = QCreate("message_sendqueue");
	printf("%s\n", QGetName(message_sendqueue));

	//create 8 disk queue
	int i = 0;
	char diskName[20];
	for (i = 0; i <= 7; i++)
	{
		sprintf(diskName, "Disk_%ld", i);
		QID_disk[i] = QCreate(diskName);
		printf("%s\n", QGetName(QID_disk[i]));
	}
	//get the current time
	
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	pcb->timeCreated = (INT32)mmio.Field1;
	

	//check which test we will run
	if (argc > 1) { 
		if (strcmp(argv[1], "sample") == 0) { mmio.Field2 = (long)SampleCode; pcb->address = mmio.Field2; strcpy(pcb->processName, "sample"); scheduleprinterFlag = 0;}
		else if (strcmp(argv[1], "test0") == 0) { mmio.Field2 = (long)test0; pcb->address = mmio.Field2; strcpy(pcb->processName, "test0"); scheduleprinterFlag = 0;}
		else if (strcmp(argv[1], "test1") == 0) { mmio.Field2 = (long)test1; pcb->address = mmio.Field2; strcpy(pcb->processName, "test01"); scheduleprinterFlag = 0;}
		else if (strcmp(argv[1], "test2") == 0) { mmio.Field2 = (long)test2; pcb->address = mmio.Field2; strcpy(pcb->processName, "test02"); scheduleprinterFlag = 0;}
		else if (strcmp(argv[1], "test3") == 0) { mmio.Field2 = (long)test3; pcb->address = mmio.Field2; strcpy(pcb->processName, "test03"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test4") == 0) { mmio.Field2 = (long)test4; pcb->address = mmio.Field2; strcpy(pcb->processName, "test04"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test5") == 0) { mmio.Field2 = (long)test5; pcb->address = mmio.Field2; strcpy(pcb->processName, "test05"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test6") == 0) { mmio.Field2 = (long)test6; pcb->address = mmio.Field2; strcpy(pcb->processName, "test06"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test7") == 0) { mmio.Field2 = (long)test7; pcb->address = mmio.Field2; strcpy(pcb->processName, "test07"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test8") == 0) { mmio.Field2 = (long)test8; pcb->address = mmio.Field2; strcpy(pcb->processName, "test08"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test9") == 0) { mmio.Field2 = (long)test9; pcb->address = mmio.Field2; strcpy(pcb->processName, "test09"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test10") == 0) { mmio.Field2 = (long)test10; pcb->address = mmio.Field2; strcpy(pcb->processName, "test10"); scheduleprinterFlag = 1;}
		else if (strcmp(argv[1], "test11") == 0) { mmio.Field2 = (long)test11; pcb->address = mmio.Field2; strcpy(pcb->processName, "test11"); scheduleprinterFlag = 2;}
		else if (strcmp(argv[1], "test12") == 0) { mmio.Field2 = (long)test12; pcb->address = mmio.Field2; strcpy(pcb->processName, "test12"); scheduleprinterFlag = 2;}
		else if (strcmp(argv[1], "test13") == 0) { mmio.Field2 = (long)test13; pcb->address = mmio.Field2; strcpy(pcb->processName, "test13"); scheduleprinterFlag = 2;}
		else if (strcmp(argv[1], "test14") == 0) { mmio.Field2 = (long)test14; pcb->address = mmio.Field2; strcpy(pcb->processName, "test14"); scheduleprinterFlag = 2;}
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
	//disk initial
	pcb->diskID = 0;
	pcb->sector = 0;
	pcb->WriteOrRead = 0;
	//strcpy(pcb->DiskData, "0");
	pcb->DiskData = 0;
	pcb->target_pid = -2;
	pcb->source_pid = -2;
	pcb->actual_source_pid = -2;
	pcb->send_length = -2;
	pcb->receive_length = -2;
	strcpy(pcb->msg_buffer, "0");
	//pcb->MessageData = 0;
	//pcb->timeCreated = 0;
	// 
	///put the pcb into ready queue
	//QInsert(QID_ready, pcb->priority, pcb);
	currentPCB = pcb;
	QInsert(QID_allprocess, pcb->priority, pcb);


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
/////check by name in allprocess so see it exit or not
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
	//QPrint(QID_temp);
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
	//QPrint(QID_temp);
}


//suspend pcb to timer queue
void suspendByPid_timer(INT32 PID) {
	PCB* temppcb;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	READ_MODIFY(TimerQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_timer);
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
	READ_MODIFY(TimerQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_timer);
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
	//QPrint(QID_temp);
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


void changePriority(INT32 PID,INT32 priority, INT32 QID) {
	PCB* temppcb;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	//QPrint(QID);
	while (QWalk(QID, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID);

		if (PID == temppcb->PID) {
			temppcb->priority = priority;
			QInsert(QID_temp, temppcb->priority, temppcb);
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
	
	//QPrint(QID);
	//QPrint(QID_temp);
}


void pDisk_write(INT32 disk, INT32 sector, long dataWrite) {
	char lock_disk[20];
	MEMORY_MAPPED_IO mmio;
	sprintf(lock_disk, "Disk_%ld_lock", disk);
	//printf(lock_write);

	READ_MODIFY(lock_disk, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_disk[0]);

	

	if (QNextItemInfo(QID_disk[disk]) == -1) {
		currentPCB->diskID = disk;
		currentPCB->sector = sector;
		currentPCB->WriteOrRead = 0;
		//strcpy(currentPCB->DiskData, dataWrite);
		currentPCB->DiskData = dataWrite;
		//QInsert(QID_disk[disk], (currentPCB->priority), currentPCB);
		//QPrint(QID_disk[disk]);
		QInsertOnTail(QID_disk[disk], currentPCB);

		//if disk queue is empty then directly write
		mmio.Mode = Z502DiskWrite;
		mmio.Field1 = disk; // Pick same disk location
		mmio.Field2 = sector;
		mmio.Field3 = (long)dataWrite;
		MEM_WRITE(Z502Disk, &mmio);
	}else {
		//printf("write_start: ");
		//else enqueue the quest to disk queue
		currentPCB->diskID = disk;
		currentPCB->sector = sector;
		currentPCB->WriteOrRead = 0;
		//strcpy(currentPCB->DiskData, dataWrite);
		currentPCB->DiskData = dataWrite;
		//QInsert(QID_disk[disk], (currentPCB->priority), currentPCB);
		//QPrint(QID_disk[disk]);
		QInsertOnTail(QID_disk[disk], currentPCB);
		//QPrint(QID_disk[disk]);
	}
	

	

	READ_MODIFY(lock_disk, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_disk[0]);

	//mmio.Mode = Z502Action;
	//mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	//MEM_WRITE(Z502Idle, &mmio);
	dispatcher();
}
void pDisk_read(INT32 disk, INT32 sector, long dataRead) {
	char lock_disk[20];
	
	MEMORY_MAPPED_IO mmio;
	sprintf(lock_disk, "Disk_%ld_lock", disk);
	

	
		READ_MODIFY(lock_disk, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult_disk[0]);
	
	if (QNextItemInfo(QID_disk[disk]) == -1) {

		currentPCB->diskID = disk;
		currentPCB->sector = sector;
		currentPCB->WriteOrRead = 1;
		//strcpy(currentPCB->DiskData, dataRead);
		currentPCB->DiskData = dataRead;
		//QInsert(QID_disk[disk], (currentPCB->priority), currentPCB);
		QInsertOnTail(QID_disk[disk], currentPCB);
		//if disk queue is empty then directly read
		mmio.Mode = Z502DiskRead;
		mmio.Field1 = disk; // Pick same disk location
		mmio.Field2 = sector;
		mmio.Field3 = (long)dataRead;
		MEM_WRITE(Z502Disk, &mmio);
	}
	else{
		//else enqueue the quest to disk queue
		//printf("read_start: ");
		currentPCB->diskID = disk;
		currentPCB->sector = sector;
		currentPCB->WriteOrRead = 1;
		//strcpy(currentPCB->DiskData, dataRead);
		currentPCB->DiskData = dataRead;
		//QInsert(QID_disk[disk], (currentPCB->priority), currentPCB);
		//QPrint(QID_disk[disk]);
		QInsertOnTail(QID_disk[disk], currentPCB);
		//QPrint(QID_disk[disk]);
	}
	
		READ_MODIFY(lock_disk, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult_disk[0]);
	
	//mmio.Mode = Z502Action;
	//mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	//MEM_WRITE(Z502Idle, &mmio);
	dispatcher();
}

///Scheduler Printer
void SP_print(char* Action,int targetID) {
	int i;
	int j;
	SP_INPUT_DATA SPData;
	PCB* temppcb;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	memset(&SPData, 0, sizeof(SP_INPUT_DATA));
	strcpy(SPData.TargetAction, Action);
	SPData.CurrentlyRunningPID = currentPCB->PID;
	SPData.TargetPID = targetID;




	// The NumberOfRunningProcesses as used here is for a future implementation
	// when we are running multiple processors.  For right now, set this to 0
	// so it won't be printed out.
	SPData.NumberOfRunningProcesses = 0;
	{
		j = 0;
		i = 0;
		while (QWalk(QID_ready, 0) != -1) {
			//get the n th process
			j++;
			temppcb = QRemoveHead(QID_ready);
			SPData.ReadyProcessPIDs[i] = temppcb->PID;
			i++;
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
		while (QWalk(QID_temp, 0) != -1) {
			temppcb = QRemoveHead(QID_temp);
			QInsert(QID_ready, temppcb->priority, temppcb);
		}
		SPData.NumberOfReadyProcesses = j;
	}// Processes ready to run



	{
		j = 0;
		i = 0;
		while (QWalk(QID_suspend, 0) != -1) {
			//get the n th process
			j++;
			temppcb = QRemoveHead(QID_suspend);
			SPData.ProcSuspendedProcessPIDs[i] = temppcb->PID;
			i++;
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
		while (QWalk(QID_temp, 0) != -1) {
			temppcb = QRemoveHead(QID_temp);
			QInsert(QID_suspend, temppcb->priority, temppcb);
		}
		SPData.NumberOfProcSuspendedProcesses = j;
	}// Processes suspend Queue
	//SPData.NumberOfProcSuspendedProcesses = 0;
	//for (i = 0; i <= SPData.NumberOfProcSuspendedProcesses; i++) {
	//	SPData.ProcSuspendedProcessPIDs[i] = i + 3;
	//}




	{
		j = 0;
		i = 0;
		while (QWalk(QID_suspendbyMessage, 0) != -1) {
			//get the n th process
			j++;
			temppcb = QRemoveHead(QID_suspendbyMessage);
			SPData.MessageSuspendedProcessPIDs[i] = temppcb->PID;
			i++;
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
		while (QWalk(QID_temp, 0) != -1) {
			temppcb = QRemoveHead(QID_temp);
			QInsert(QID_suspendbyMessage, temppcb->priority, temppcb);
		}
		SPData.NumberOfMessageSuspendedProcesses = j;
	}// Processes suspend message Queue
	//SPData.NumberOfMessageSuspendedProcesses = 0;
	//for (i = 0; i <= SPData.NumberOfMessageSuspendedProcesses; i++) {
	//	SPData.MessageSuspendedProcessPIDs[i] = i + 16;
	//}


	READ_MODIFY(TimerQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_timer);

	{
		j = 0;
		i = 0;
		while (QWalk(QID_timer, 0) != -1) {
			//get the n th process
			j++;
			temppcb = QRemoveHead(QID_timer);
			SPData.TimerSuspendedProcessPIDs[i] = temppcb->PID;
			i++;
			QInsert(QID_temp, temppcb->timeCreated, temppcb);
		}
		while (QWalk(QID_temp, 0) != -1) {
			temppcb = QRemoveHead(QID_temp);
			QInsert(QID_timer, temppcb->timeCreated, temppcb);
		}
		SPData.NumberOfTimerSuspendedProcesses = j;
	}// Processes ready to run

	//SPData.NumberOfTimerSuspendedProcesses = 6;
	//for (i = 0; i <= SPData.NumberOfTimerSuspendedProcesses; i++) {
	//	SPData.TimerSuspendedProcessPIDs[i] = i + 8;
	//}
	READ_MODIFY(TimerQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult_timer);

	char lock_disk[20];
	j = 0;
	i = 0;
	for (int k = 0; k <= 7; k++) {
		
		sprintf(lock_disk, "Disk_%ld_lock", k);
		READ_MODIFY(lock_disk, DO_LOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult_disk);

		while (QWalk(QID_disk[k], 0) != -1) {
			//get the n th process
			j++;
			temppcb = QRemoveHead(QID_disk[k]);
			SPData.DiskSuspendedProcessPIDs[i] = temppcb->PID;
			i++;
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
		while (QWalk(QID_temp, 0) != -1) {
			temppcb = QRemoveHead(QID_temp);
			QInsert(QID_disk[k], temppcb->priority, temppcb);
		}
		SPData.NumberOfDiskSuspendedProcesses = j;

		READ_MODIFY(lock_disk, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
			&LockResult_disk);
	}// Processes ready to run
	//SPData.NumberOfDiskSuspendedProcesses = 0;
	//for (i = 0; i <= SPData.NumberOfDiskSuspendedProcesses; i++) {
	//	SPData.DiskSuspendedProcessPIDs[i] = i + 15;
	//}



	SPData.NumberOfTerminatedProcesses = 0;   // Not used at this time
	if (scheduleprinterFlag == 1 || (scheduleprinterFlag == 2 && scheduleprinterCounter <= 50)) {
		scheduleprinterCounter++;
		CALL(SPPrintLine(&SPData));
	}
	
}


//resume a pcd from message queue
int resumePIDMessgage(INT32 PID, Message_send *message) {
	PCB* temppcb;
	int flag = 0;
	//allocate memory for pcb
	temppcb = (PCB*)malloc(sizeof(PCB));
	while (QWalk(QID_suspendbyMessage, 0) != -1) {
		//get the n th process
		temppcb = QRemoveHead(QID_suspendbyMessage);

		if ((PID == temppcb->source_pid || temppcb->source_pid == -1) && flag == 0) {
			flag++;
			temppcb->actual_source_pid = message->currentProcessID;
			temppcb->send_length = message->MessageSendLength;
			strncpy(temppcb->msg_buffer, message->MessageBuffer, message->MessageSendLength);
			QInsert(QID_ready, temppcb->priority, temppcb);
		}
		else {
			QInsert(QID_temp, temppcb->priority, temppcb);
		}
	}
	while (QWalk(QID_temp, 0) != -1) {
		temppcb = QRemoveHead(QID_temp);
		QInsert(QID_suspendbyMessage, temppcb->priority, temppcb);
	}
	return flag;
}

////send message form a current PCB
int sendMessage(INT32 currentProcessID,INT32 ProcessID, char* MessageBuffer, INT32 MessageSendLength) {
	Message_send* tempmessage;
	int flag = 0;
	//allocate memory for message send
	tempmessage = (Message_send*)malloc(sizeof(Message_send));

	tempmessage->currentProcessID = currentProcessID;
	tempmessage->TargetProcessID = ProcessID;
	tempmessage->MessageSendLength = MessageSendLength;
	//tempmessage->MessageBuffer = MessageBuffer;
	strncpy(tempmessage->MessageBuffer, MessageBuffer, MessageSendLength);
	//printf(tempmessage->MessageBuffer);
	if (QNextItemInfo(QID_suspendbyMessage) != -1) {//if there is some message pending
		// find the id exist or not
		flag = resumePIDMessgage(ProcessID, tempmessage);
	}
	if (flag == 0  && sendMessageCount < 10) {
		QInsertOnTail(message_sendqueue, tempmessage);
		sendMessageCount++;
		return 1;
	}
	else {
		if (flag == 1){
			return 1;
		}
		else {
			return 0;
		}
		
	}

}

//receive message
void receiveMessage(INT32 ProcessID, char* MessageBuffer, INT32 MessageReceiveLength) {
	Message_send* tempmessage;
	Message_send* tempmessage_out;
	
	//allocate memory for message send
	tempmessage = (Message_send*)malloc(sizeof(Message_send));
	tempmessage_out = (Message_send*)malloc(sizeof(Message_send));
	int counter;
	
	//check the process id if it is -1, recieve the broadcasting message
	if (ProcessID == -1) {
			counter = 0;
			while (QWalk(message_sendqueue, 0) != -1) {
				//QPrint(message_sendqueue);
				tempmessage = QRemoveHead(message_sendqueue);
				if (tempmessage->TargetProcessID == currentPCB->PID || tempmessage->TargetProcessID == -1) {
					if (counter == 0){
						tempmessage_out = tempmessage;
						counter++;
					}
					else {
						QInsertOnTail(QID_temp, tempmessage);
					}
				}else{
					QInsertOnTail(QID_temp, tempmessage);
				}
			}
			while (QWalk(QID_temp, 0) != -1) {
				tempmessage = QRemoveHead(QID_temp);
				QInsertOnTail(message_sendqueue, tempmessage);
			}
			if (counter == 0) {//send to message suspend queue
				currentPCB->target_pid = currentPCB->PID;
				currentPCB->source_pid = ProcessID;
				currentPCB->receive_length = MessageReceiveLength;
				QInsertOnTail(QID_suspendbyMessage, currentPCB);
				strncpy(MessageBuffer, currentPCB->msg_buffer, MessageReceiveLength);
				
				dispatcher();
			}
			else {
				strncpy(currentPCB->msg_buffer, tempmessage_out->MessageBuffer, MessageReceiveLength);  //tempmessage->MessageBuffer, MessageBuffer, MessageSendLength tempmessage_out->MessageBuffer
				currentPCB->actual_source_pid = tempmessage_out->currentProcessID;
				currentPCB->send_length = tempmessage_out->MessageSendLength;
			}



	}
	else {
		counter = 0;
		while (QWalk(message_sendqueue, 0) != -1) {
			tempmessage = QRemoveHead(message_sendqueue);
			if (tempmessage->TargetProcessID == ProcessID && counter == 0) {
				tempmessage_out = tempmessage;
				counter++;
			}
			else {
				QInsertOnTail(QID_temp, tempmessage);
			}

		}
		while (QWalk(QID_temp, 0) != -1) {
			tempmessage = QRemoveHead(QID_temp);
			QInsertOnTail(message_sendqueue, tempmessage);
		}
		if (counter == 0) {//send to message suspend queue
			currentPCB->target_pid = currentPCB->PID;
			currentPCB->source_pid = ProcessID;
			currentPCB->receive_length = MessageReceiveLength;
			QInsertOnTail(QID_suspendbyMessage, currentPCB);
			//dispatcher();
			strncpy(MessageBuffer, currentPCB->msg_buffer, MessageReceiveLength);
			dispatcher();
		}
		else {
			strncpy(currentPCB->msg_buffer, tempmessage_out->MessageBuffer, MessageReceiveLength);  //tempmessage->MessageBuffer, MessageBuffer, MessageSendLength tempmessage_out->MessageBuffer
			currentPCB->actual_source_pid = tempmessage_out->currentProcessID;
			currentPCB->send_length = tempmessage_out->MessageSendLength;
		}
	}

	
}