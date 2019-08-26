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


typedef         int                             INT32;


//osCreateProcess
///once the code goes into onInit, then it will pass all the data into here and create the first process.
///then it will put the process into ready queue
///input:
///output:
void osCreatProcess(int argc, char* argv[]) {
	void* PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	MEMORY_MAPPED_IO mmio;
	//define the pcb
	PCB *pcb;
	//allocate memory for pcb
	pcb = (PCB*)malloc(sizeof(PCB));
	if (pcb == 0)
		printf("We didn't complete the malloc in pcb.");

	INT32 QID_ready;
	//create a ready queue to store pcb
	QID_ready = QCreate("readyQueue");
	//check which test we will run
	if (argc > 1) { 
		if (strcmp(argv[1], "sample") == 0) { mmio.Field2 = (long)SampleCode; pcb->address = mmio.Field2; strncpy(pcb->processName, "sample", sizeof("sample"));}
		else if (strcmp(argv[1], "test0") == 0) { mmio.Field2 = (long)test0; pcb->address = mmio.Field2; strncpy(pcb->processName, "test0", sizeof("test0"));}
		else if (strcmp(argv[1], "test1") == 0) { mmio.Field2 = (long)test1; pcb->address = mmio.Field2; strncpy(pcb->processName, "test01", sizeof("test1"));}
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
	///put the pcb into ready queue
	QInsert(QID_ready, pcb->priority, pcb);

		printf("%s", QGetName(QID_ready));



	mmio.Mode = Z502StartContext;
	// Field1 contains the value of the context returned in the last call
	// Suspends this current thread
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}