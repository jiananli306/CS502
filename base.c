/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 4.51 August  2018: Minor bug fixes
 4.60 February2019: Test for the ability to alloc large amounts of memory
 ************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
//include the data we created
#include			"SelfFunction.h"

//define the variables
//extern INT32 PID;
//extern INT32 CurrentProcessNumber;



//  This is a mapping of system call nmemonics with definitions

char *call_names[] = {"MemRead  ", "MemWrite ", "ReadMod  ", "GetTime  ",
        "Sleep    ", "GetPid   ", "Create   ", "TermProc ", "Suspend  ",
        "Resume   ", "ChPrior  ", "Send     ", "Receive  ", "PhyDskRd ",
        "PhyDskWrt", "DefShArea", "Format   ", "CheckDisk", "OpenDir  ",
        "OpenFile ", "CreaDir  ", "CreaFile ", "ReadFile ", "WriteFile",
        "CloseFile", "DirContnt", "DelDirect", "DelFile  " };

/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the Operating System.
 NOTE WELL:  Just because the timer or the disk has interrupted, and
         therefore this code is executing, it does NOT mean the 
         action you requested was successful.
         For instance, if you give the timer a NEGATIVE time - it 
         doesn't know what to do.  It can only cause an interrupt
         here with an error.
         If you try to read a sector from a disk but that sector
         hasn't been written to, that disk will interrupt - the
         data isn't valid and it's telling you it was confused.
         YOU MUST READ THE ERROR STATUS ON THE INTERRUPT
 ************************************************************************/
void InterruptHandler(void) {
    INT32 DeviceID;
    INT32 Status;
	PCB* timerpcb;
	INT32 current_time;
	//allocate memory for pcb
	timerpcb = (PCB*)malloc(sizeof(PCB));
	if (timerpcb == 0)
		printf("We didn't complete the malloc in pcb.");

    MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

    //static BOOL  remove_this_from_your_interrupt_code = TRUE; /** TEMP **/
    //static INT32 how_many_interrupt_entries = 0;              /** TEMP **/

    // Get cause of interrupt
    mmio.Mode = Z502GetInterruptInfo;
    mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
    MEM_READ(Z502InterruptDevice, &mmio);
    DeviceID = mmio.Field1;
    Status = mmio.Field2;
	//QPrint(QID_ready);
	//QPrint(QID_timer);
	//build a while loop to catch all the interrupt
	while (mmio.Field4 == ERR_SUCCESS) {
		//DO the interrupt
		///handle the timer interrput
		//QPrint(QID_ready);
		//QPrint(QID_timer);
		if (DeviceID == TIMER_INTERRUPT) {
			//get current time
			{
				mmio.Mode = Z502ReturnValue;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
				MEM_READ(Z502Clock, &mmio);
				current_time = (INT32)mmio.Field1;
				//printf("Time in interrupt %ld\n", current_time);
			}
			//dequeue the first timer queue and put it into ready queue.
			timerpcb = QRemoveHead(QID_timer);
			timerpcb->timeCreated = current_time;
			//printf("%d", timerpcb->PID);
			//printf("%d", currentPCB->PID);
			if (timerpcb->suspendFlag != 1) {
				QInsert(QID_ready, (timerpcb->priority), timerpcb);
			}
			else {
				QInsertOnTail(QID_suspend, timerpcb);
			}
			//chech next timer queue context
			//QPrint(QID_timer);
			if (QNextItemInfo(QID_timer) != -1) {
				timerpcb = QNextItemInfo(QID_timer);
				while (QNextItemInfo(QID_timer) != -1 && timerpcb->timeCreated <= current_time)
				{
					timerpcb = QRemoveHead(QID_timer);
					timerpcb->timeCreated = current_time;
					//QInsert(QID_ready, (timerpcb->priority), timerpcb);
					if (timerpcb->suspendFlag != 1) {
						QInsert(QID_ready, (timerpcb->priority), timerpcb);
					}
					else {
						QInsertOnTail(QID_suspend, timerpcb);
					}
					timerpcb = QNextItemInfo(QID_timer);

				}
				//printf("timer time handleer:%d\n", (timerpcb->timeCreated - current_time));
				//startTimer(timerpcb->timeCreated - current_time);
				// Start the timer - here's the sequence to use
				
				if (QNextItemInfo(QID_timer) != -1) {
					mmio.Mode = Z502Start;
					mmio.Field1 = timerpcb->timeCreated - current_time;   // set the time of timer
					mmio.Field2 = mmio.Field3 = 0;
					MEM_WRITE(Z502Timer, &mmio);
				}
			}
		}


		//catch next interrput
			// Get cause of interrupt
		mmio.Mode = Z502GetInterruptInfo;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
		MEM_READ(Z502InterruptDevice, &mmio);
		DeviceID = mmio.Field1;
		Status = mmio.Field2;
	}
	//dispatcher();


   /* if (mmio.Field4 != ERR_SUCCESS) {
        aprintf( "The InterruptDevice call in the InterruptHandler has failed.\n");
        aprintf("The DeviceId and Status that were returned are not valid.\n");
    }*/
    // HAVE YOU CHECKED THAT THE INTERRUPTING DEVICE FINISHED WITHOUT ERROR?
	//if it is a timer interrupt

    ///** REMOVE THESE SIX LINES **/
    //how_many_interrupt_entries++; /** TEMP **/
    //if (remove_this_from_your_interrupt_code && (how_many_interrupt_entries < 10)) {
    //    aprintf("InterruptHandler: Found device ID %d with status %d\n",
    //            DeviceID, Status);
    //}

}           // End of InterruptHandler

/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void FaultHandler(void) {
    INT32 DeviceID;
    INT32 Status;
    MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

    static BOOL remove_this_from_your_fault_code = TRUE; 
    static INT32 how_many_fault_entries = 0; 

    // Get cause of fault
    mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
    mmio.Mode = Z502GetInterruptInfo;
    MEM_READ(Z502InterruptDevice, &mmio);
    DeviceID = mmio.Field1;
    Status   = mmio.Field2;

    // This causes a print of the first few faults - and then stops printing!
    // You can adjust this as you wish.  BUT this code as written here gives
    // an indication of what's happening but then stops printing for long tests
    // thus limiting the output.
    how_many_fault_entries++; 
    if (remove_this_from_your_fault_code && (how_many_fault_entries < 10)) {
            aprintf("FaultHandler: Found device ID %d with status %d\n",
                            (int) mmio.Field1, (int) mmio.Field2);
    }

} // End of FaultHandler

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
    short call_type;
    static short do_print = 10;
    short i;
	INT32 PIDtemp;
	INT32 Time;
	MEMORY_MAPPED_IO mmio;
	PCB* newPCB;
	newPCB = (PCB*)malloc(sizeof(PCB));
	if (newPCB == 0)
		printf("We didn't complete the malloc in pcb.");
	void* PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	

	///write the code here. testing

    call_type = (short) SystemCallData->SystemCallNumber;
    if (do_print > 0) {
        aprintf("SVC handler: %s\n", call_names[call_type]);
        for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
            //Value = (long)*SystemCallData->Argument[i];
            aprintf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
                    (unsigned long) SystemCallData->Argument[i],
                    (unsigned long) SystemCallData->Argument[i]);
        }
        do_print--;
    }
	switch (call_type) {
		//Get time sevice call
		case SYSNUM_GET_TIME_OF_DAY: 
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Clock, &mmio);
			*(long*)SystemCallData->Argument[0] = mmio.Field1;
			break;
		//terminate system call 
		//for now it terminate the whole system which is not correct
		case SYSNUM_TERMINATE_PROCESS:
			if ((long)SystemCallData->Argument[0] == -2) {
				*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
				//terminate whole system
				mmio.Mode = Z502Action;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
				MEM_WRITE(Z502Halt, &mmio);
			}
			else if ((long)SystemCallData->Argument[0] == -1 || currentPCB->PID==(long)SystemCallData->Argument[0]) {
				//terminate the current context
				PIDtemp = checkName(currentPCB->processName);
				if (PIDtemp >= 0)
				{
					dequeueByPid(PIDtemp, QID_ready);
					dequeueByPid(PIDtemp, QID_timer);
					dequeueByPid(PIDtemp, QID_suspend);
					dequeueByPid(PIDtemp, QID_allprocess);
					if (QNextItemInfo(QID_timer) == -1 && QNextItemInfo(QID_ready) == -1) {
						//terminate whole system
						mmio.Mode = Z502Action;
						mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
						MEM_WRITE(Z502Halt, &mmio);

					}
					//QPrint(QID_timer);
					//QPrint(QID_ready);
					*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
					dispatcher();
				}
				else {
					*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
			}
			else {
				PIDtemp = checkPID((long)SystemCallData->Argument[0]);
				if (PIDtemp >= 0)
				{
					dequeueByPid((long)SystemCallData->Argument[0], QID_ready);
					dequeueByPid((long)SystemCallData->Argument[0], QID_timer);
					dequeueByPid((long)SystemCallData->Argument[0], QID_suspend);
					dequeueByPid((long)SystemCallData->Argument[0], QID_allprocess);
					*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
				}
				else {
					*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
			}
		
			
			break;
		//Get Process ID
		case SYSNUM_GET_PROCESS_ID:
			mmio.Mode = Z502GetCurrentContext;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Context, &mmio);
			//printf("%s", currentPCB->processName);
			//check which process ID you need 
			if (strcmp((long*)SystemCallData->Argument[0], "") == 0) {
				//currentPCB = QNextItemInfo(QID_ready);
				*(long*)SystemCallData->Argument[1] = currentPCB->PID;
				*(long*)SystemCallData->Argument[2] = mmio.Field3;
			}
			else {
				PIDtemp = checkName((char*)SystemCallData->Argument[0]);
				if (PIDtemp == -1){
					*(long*)SystemCallData->Argument[1] = PIDtemp;
					*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
				}
				else {
					*(long*)SystemCallData->Argument[1] = PIDtemp;
					*(long*)SystemCallData->Argument[2] = mmio.Field3;
				}
				
			}
			break;
		//sleep for specific time
		case SYSNUM_SLEEP:
			//check error first
			if ((INT32)SystemCallData->Argument[0] > 0) {
				startTimer((INT32)SystemCallData->Argument[0]);
			}
			break;
		//create a new process
		case SYSNUM_CREATE_PROCESS:
			//check priority first
			if ((INT32)SystemCallData->Argument[2] < 1) {
				*(long*)SystemCallData->Argument[3] = -1;
				*(long*)SystemCallData->Argument[4] = ERR_BAD_PARAM;// "illegal priority";
			}
			else if (CurrentProcessNumber > MAX_Process_number) {
				*(long*)SystemCallData->Argument[3] = -1;
				*(long*)SystemCallData->Argument[4] = ERR_BAD_PARAM;// "too many process";
			}
			else if (checkName((char*)SystemCallData->Argument[0])!=-1) {
				*(long*)SystemCallData->Argument[3] = -1;
				*(long*)SystemCallData->Argument[4] = ERR_BAD_PARAM;// "same name";
			}
			else{//create process here
				newPCB->PID = PID;
				PID++;
				//printf("%d\n", PID);
				//printf("%s", (char*)SystemCallData->Argument[0]);
				strcpy(newPCB->processName, (char*)SystemCallData->Argument[0]);
				newPCB->address = (long)SystemCallData->Argument[1];
				newPCB->priority = (long)SystemCallData->Argument[2];
				newPCB->pageTable = PageTable;
				newPCB->suspendFlag = 0;
				CurrentProcessNumber++;
				{
					mmio.Mode = Z502ReturnValue;
					mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
					MEM_READ(Z502Clock, &mmio);
					newPCB->timeCreated = (INT32)mmio.Field1;
				}
				createProcess(newPCB);
				*(long*)SystemCallData->Argument[3] = newPCB->PID;
				*(long*)SystemCallData->Argument[4] = ERR_SUCCESS;// "correct";
			}
			break;
		//suspend and resume process
		case SYSNUM_SUSPEND_PROCESS:
			//currnet process direct put into suspend queue
			if ((INT32)SystemCallData->Argument[0] == -1|| (INT32)SystemCallData->Argument[0] == currentPCB->PID) {
				//currentPCB->suspendFlag = 1;
				//QInsertOnTail(QID_suspend, currentPCB);
				//*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
				//dispatcher();
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			else {//check process exit or not first
				if (checkPID((INT32)SystemCallData->Argument[0]) == -1|| checkPID_suspend((INT32)SystemCallData->Argument[0]) != -1) {
					*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
				else {//if process in ready queue, put into suspend queue
					suspendByPid((INT32)SystemCallData->Argument[0], QID_ready);
					//if process in timer queue, set the flag to 1
					suspendByPid_timer((INT32)SystemCallData->Argument[0]);
					*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
				}
			}
			break;
		case SYSNUM_RESUME_PROCESS:
			//check process exit or not first
			if (checkPID_suspend((INT32)SystemCallData->Argument[0]) == -1) {
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			else{//dequeue from suspend queue and put into ready queue
				resumePID((INT32)SystemCallData->Argument[0]);
				*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
			}
			break;
		//SYSNUM_PHYSICAL_DISK_READ
		case SYSNUM_PHYSICAL_DISK_READ:
			break;
		case SYSNUM_PHYSICAL_DISK_WRITE:
			break;
		case SYSNUM_CHECK_DISK:
			break;
		default:
			printf("ERROR. Unrecognized call type");
			printf("Call_type: %i\n",call_type);
	
	}
}                                               // End of svc

/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[]) {
    // Every process will have a page table.  This will be used in
    // the second half of the project.  
    void *PageTable = (void *) calloc(2, NUMBER_VIRTUAL_PAGES);
    INT32 i;
    MEMORY_MAPPED_IO mmio;
	PID = 0;
	
	
    // Demonstrates how calling arguments are passed thru to here

    aprintf("Program called with %d arguments:", argc);
    for (i = 0; i < argc; i++)
        aprintf(" %s", argv[i]);
    aprintf("\n");
    aprintf("Calling with argument 'sample' executes the sample program.\n");

    // Here we check if a second argument is present on the command line.
    // If so, run in multiprocessor mode.  Note - sometimes people change
    // around where the "M" should go.  Allow for both possibilities
    if (argc > 2) {
        if ((strcmp(argv[1], "M") ==0) || (strcmp(argv[1], "m")==0)) {
            strcpy(argv[1], argv[2]);
            strcpy(argv[2],"M\0");
        }
        if ((strcmp(argv[2], "M") ==0) || (strcmp(argv[2], "m")==0)) {
            aprintf("Simulation is running as a MultProcessor\n\n");
            mmio.Mode = Z502SetProcessorNumber;
            mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
            mmio.Field2 = (long) 0;
            mmio.Field3 = (long) 0;
            mmio.Field4 = (long) 0;
            MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
        }
    } else {
        aprintf("Simulation is running as a UniProcessor\n");
        aprintf("Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
    }

    //  Some students have complained that their code is unable to allocate
    //  memory.  Who knows what's going on, other than the compiler has some
    //  wacky switch being used.  We try to allocate memory here and stop
    //  dead if we're unable to do so.
    //  We're allocating and freeing 8 Meg - that should be more than
    //  enough to see if it works.
    void *Temporary = (void *) calloc( 8, 1024 * 1024);
    if ( Temporary == NULL )  {  // Not allocated
	    printf( "Unable to allocate memory in osInit.  Terminating simulation\n");
	    exit(0);
    }
    free(Temporary);
	//call os creat process
	osCreatProcess(argc,argv);
}  // End of osInit
