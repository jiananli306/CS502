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
	PCB* diskpcb;
	INT32 current_time;
	INT32 diskID_temp;
	char Success[] = "      Action Failed\0        Action Succeeded";
	char lock_disk[20];

	

	//allocate memory for pcb
	timerpcb = calloc(1,sizeof(PCB));
	if (timerpcb == 0)
		printf("We didn't complete the malloc in pcb.");
	diskpcb = calloc(1,sizeof(PCB));
	if (diskpcb == 0)
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
	
	//build a while loop to catch all the interrupt
	while (mmio.Field4 == ERR_SUCCESS) {
		//DO the interrupt
		///handle the timer interrput
		//QPrint(QID_ready);
		//QPrint(QID_timer);
		if (DeviceID == TIMER_INTERRUPT) {
			READ_MODIFY(TimerQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
				&LockResult_timer);
			//printf("%s\n", &(Success[SPART * LockResult_timer]));

			//get current time
			{
				mmio.Mode = Z502ReturnValue;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
				MEM_READ(Z502Clock, &mmio);
				current_time = (INT32)mmio.Field1;
				//printf("Time in interrupt %ld\n", current_time);
			}

			if (QNextItemInfo(QID_timer) != -1) {
				timerpcb = QNextItemInfo(QID_timer);
				while (QNextItemInfo(QID_timer) != -1 && timerpcb->timeCreated - current_time <= 0) {
					timerpcb = QRemoveHead(QID_timer);
					timerpcb->timeCreated = current_time;
					//QInsert(QID_ready, (timerpcb->priority), timerpcb);
					if (timerpcb->suspendFlag != 1) {
						READ_MODIFY(ReadyQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_ready);
						QInsert(QID_ready, (timerpcb->priority), timerpcb);
						READ_MODIFY(ReadyQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_ready);

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

			}
			READ_MODIFY(TimerQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
				&LockResult_timer);
			//printf("%s\n", &(Success[SPART * LockResult_timer]));
		}
		else if (DeviceID > 4 && DeviceID < 13) {
			diskID_temp = DeviceID - 5;
			//get current time
			{
				mmio.Mode = Z502ReturnValue;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
				MEM_READ(Z502Clock, &mmio);
				current_time = (INT32)mmio.Field1;
				//printf("Time in interrupt %ld\n", current_time);
			}
			//(QID_disk[diskID_temp]);
			sprintf(lock_disk, "Disk_%ld_lock", diskID_temp);
			//printf("***********Disk id;;; %ld \n", diskID_temp);
			//if (QNextItemInfo(QID_disk[diskID_temp]) != -1) {
			READ_MODIFY(lock_disk, DO_LOCK, SUSPEND_UNTIL_LOCKED,
				&LockResult_disk[0]);
				diskpcb = QNextItemInfo(QID_disk[diskID_temp]);
				
				if (QNextItemInfo(QID_disk[diskID_temp]) != -1) {
					
					diskpcb = QRemoveHead(QID_disk[diskID_temp]);
					diskpcb->timeCreated = current_time;
					//QInsert(QID_ready, (timerpcb->priority), timerpcb);
					if (timerpcb->suspendFlag != 1) {
						READ_MODIFY(ReadyQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_ready);
						QInsert(QID_ready, (diskpcb->priority), diskpcb);
						READ_MODIFY(ReadyQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_ready);
					}
					else {
						QInsert(QID_suspend, diskpcb->priority, diskpcb);
					}
					

					diskpcb = QNextItemInfo(QID_disk[diskID_temp]);
					if (diskpcb != -1){
						if (diskpcb->WriteOrRead == 0) {
							//printf("write_interrupt: ");
							//QPrint(QID_disk[diskID_temp]);
							//do next disk write
							//printf("***********Disk id; %ld \n", diskpcb->diskID);
							//pDisk_write(diskpcb->diskID, diskpcb->sector, diskpcb->DiskData);
							mmio.Mode = Z502DiskWrite;
							mmio.Field1 = diskpcb->diskID; // Pick same disk location
							mmio.Field2 = diskpcb->sector;
							mmio.Field3 = (long)diskpcb->DiskData;
							MEM_WRITE(Z502Disk, &mmio);
						}
						else {//do next disk read
							//pDisk_read(diskpcb->diskID, diskpcb->sector, diskpcb->DiskData);
							//if disk queue is empty then directly read
							//printf("read_interrupt: ");
							//QPrint(QID_disk[diskID_temp]);
							mmio.Mode = Z502DiskRead;
							mmio.Field1 = diskpcb->diskID; // Pick same disk location
							mmio.Field2 = diskpcb->sector;
							mmio.Field3 = (long)diskpcb->DiskData;
							MEM_WRITE(Z502Disk, &mmio);
						
						}
					}
					
				}
				else {
					//SP_print("aaaaaaa", currentPCB->PID);
					//QPrint(QID_disk[diskID_temp]);
					//SP_print("bbbbbbbbbbbb", currentPCB->PID);
					//QPrint(QID_disk[diskID_temp]);
					printf("********************error disk number: %ld\n", diskpcb);
					printf("***************should not be here!!!!**** no disk in the queue**********\n");
				}
				READ_MODIFY(lock_disk, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
					&LockResult_disk[0]);

		}
		else {
		printf("***************should not be here!!!!**************\n");
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
	short* currentPagetable;
	INT32 frameLocation;
	INT32 tempPID;

	static BOOL remove_this_from_your_fault_code = TRUE;
	static INT32 how_many_fault_entries = 0;
	MemoryStuct* tempmem;
	//allocate memory
	tempmem = calloc(1, sizeof(MemoryStuct));
	MemoryStuct* tempmem_out;
	//allocate memory
	tempmem_out = calloc(1, sizeof(MemoryStuct));

	// Get cause of fault
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;

	// This causes a print of the first few faults - and then stops printing!
	// You can adjust this as you wish.  BUT this code as written here gives
	// an indication of what's happening but then stops printing for long tests
	// thus limiting the output.
	how_many_fault_entries++;
	if (remove_this_from_your_fault_code && (how_many_fault_entries < 10)) {
		aprintf("FaultHandler: Found device ID %d with status %d\n",
			(int)mmio.Field1, (int)mmio.Field2);
	}
	while (mmio.Field4 == ERR_SUCCESS) {
		
		if (DeviceID == INVALID_MEMORY) {
			tempPID = currentPCB->PID;
			//READ_MODIFY(MemoryQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
			//printf("INVALID_MEMORY \n");
			if (Status >= NUMBER_VIRTUAL_PAGES || Status < 0) //status no correct
			{//halt the system
				mmio.Mode = Z502Action;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
				MEM_WRITE(Z502Halt, &mmio);
			}
			//first fault, set up the page table
			//if address never use before
			if ((ShadowPageTable[currentPCB->PID][Status] & PTBL_REFERENCED_BIT) == 0) {
				currentPagetable = currentPCB->pageTable;
				frameLocation = findFirst0Bitmap_mem();
				//enqueue to the memory queue
				tempmem->DiskID = currentPCB->PID;
				tempmem->PID = currentPCB->PID;
				tempmem->PhysicalMemory = frameLocation;
				tempmem->VirtualPageNumber = Status;
				tempmem->pcb = currentPCB;
				
				
				if (frameLocation == -1) {//no frame available
					//FIFO method to choose one into swap area
					int r;
					tempmem_out = QRemoveHead(memoryqueue);
					int rr;
					short* tempPt;
					rr = tempmem_out->VirtualPageNumber;
					r = tempmem_out->PhysicalMemory;
					//put the choose one into swap area
					//set it to the shadow pagetable
					char memory_read[16] = { 0 };
					Z502ReadPhysicalMemory(r, (char*)memory_read);
					//set the swap one to invalid
					//currentPagetable[tempmem_out->VirtualPageNumber] = (short)(~PTBL_VALID_BIT) & (currentPagetable[tempmem_out->VirtualPageNumber] & PTBL_PHYS_PG_NO);
					READ_MODIFY(MemoryQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
					tempPt = tempmem_out->pcb->pageTable;
					tempPt[tempmem_out->VirtualPageNumber]=(short)(~PTBL_VALID_BIT)& (tempPt[tempmem_out->VirtualPageNumber] & PTBL_PHYS_PG_NO);
					//(char)(tempmem_out->pcb->pageTable)[tempmem_out->VirtualPageNumber] = (short)(~PTBL_VALID_BIT) & ((tempmem_out->pcb->pageTable)[tempmem_out->VirtualPageNumber][tempmem_out->VirtualPageNumber] & PTBL_PHYS_PG_NO);;
					ShadowPageTable[tempmem_out->DiskID][tempmem_out->VirtualPageNumber] = (short)PTBL_REFERENCED_BIT | ((short)(~PTBL_VALID_BIT) & (ShadowPageTable[tempmem_out->DiskID][tempmem_out->VirtualPageNumber] & PTBL_PHYS_PG_NO));
					READ_MODIFY(MemoryQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
					pDisk_write(tempmem_out->DiskID, (rr + 0x0600), (long)memory_read);
					//pDisk_read(DeviceID, r, (long)memory_read);
					
					//set the new one 
					tempmem->PhysicalMemory = r;
					QInsertOnTail(memoryqueue, tempmem);
					currentPagetable[Status] = (short)PTBL_VALID_BIT | (r & PTBL_PHYS_PG_NO);
					READ_MODIFY(MemoryQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
					ShadowPageTable[currentPCB->PID][Status] = (short)(PTBL_REFERENCED_BIT | PTBL_VALID_BIT) | (r & PTBL_PHYS_PG_NO);
					READ_MODIFY(MemoryQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
					
					//currentPCB->pageTable = currentPagetable;
				}
				else {
					//printf("***********: frameLocation: %d \n", frameLocation);
					QInsertOnTail(memoryqueue, tempmem);
					currentPagetable[Status] = (short)PTBL_VALID_BIT | (frameLocation & PTBL_PHYS_PG_NO);
					ShadowPageTable[currentPCB->PID][Status] = (short)(PTBL_REFERENCED_BIT | PTBL_VALID_BIT) | (frameLocation & PTBL_PHYS_PG_NO);
					setBitmap_mem(frameLocation);
					//currentPCB->pageTable = currentPagetable;
				}
			}
			else {// memory address used before
				//check page error
				if (tempPID != currentPCB->PID) {
					aprintf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa : panic! PID changed\n");
				}


				if (((short)(ShadowPageTable[currentPCB->PID][Status]|0x7FFF)& (short)(PTBL_VALID_BIT)) == (short)PTBL_VALID_BIT){
					aprintf("**************** %d \n", (short)(ShadowPageTable[currentPCB->PID][Status] | 0x7FFF));
					aprintf("PageOffset not correct, halt the system \n");
					mmio.Mode = Z502Action;
					mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
					MEM_WRITE(Z502Halt, &mmio);
				}
				currentPagetable = currentPCB->pageTable;
				frameLocation = findFirst0Bitmap_mem();
				//enqueue to the memory queue
				tempmem->DiskID = currentPCB->PID;
				tempmem->PID = currentPCB->PID;
				tempmem->PhysicalMemory = frameLocation;
				tempmem->VirtualPageNumber = Status;
				tempmem->pcb = currentPCB;
				
				
				if (frameLocation == -1) {//no frame available
					//FIFO method to choose one into swap area
					int r;
					tempmem_out = QRemoveHead(memoryqueue);
					int rr;
					rr = tempmem_out->VirtualPageNumber;
					r = tempmem_out->PhysicalMemory;
					short* tempPt;
					//put the choose one into swap area
					//set it to the shadow pagetable
					char memory_read[16] = { 0 };
					Z502ReadPhysicalMemory(r, (char*)memory_read);
					//set the swap one to invalid
					//currentPagetable[tempmem_out->VirtualPageNumber] = (short)(~PTBL_VALID_BIT) & (currentPagetable[tempmem_out->VirtualPageNumber] & PTBL_PHYS_PG_NO);
					READ_MODIFY(MemoryQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
					tempPt = tempmem_out->pcb->pageTable;
					tempPt[tempmem_out->VirtualPageNumber] = (short)(~PTBL_VALID_BIT) & (tempPt[tempmem_out->VirtualPageNumber] & PTBL_PHYS_PG_NO);
					ShadowPageTable[tempmem_out->DiskID][tempmem_out->VirtualPageNumber] = (short)PTBL_REFERENCED_BIT | ((short)(~PTBL_VALID_BIT) & (ShadowPageTable[tempmem_out->DiskID][tempmem_out->VirtualPageNumber] & PTBL_PHYS_PG_NO));
					READ_MODIFY(MemoryQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
					pDisk_write(tempmem_out->DiskID, (rr + 0x0600), (long)memory_read);
					/////
					//set the new one 
					tempmem->PhysicalMemory = r;
					//aprintf("framLocation: %d ************\n", r);
					char memory_Write[16] = { 0 };
					pDisk_read(tempmem->DiskID, (tempmem->VirtualPageNumber + 0x0600), (long)memory_Write);
					Z502WritePhysicalMemory(r, (char*)memory_Write);

				
					currentPagetable[Status] = (short)(PTBL_VALID_BIT) | (r & PTBL_PHYS_PG_NO);
					READ_MODIFY(MemoryQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
					ShadowPageTable[currentPCB->PID][Status] = (short)PTBL_REFERENCED_BIT | ((short)(PTBL_VALID_BIT) | (r & PTBL_PHYS_PG_NO));
					READ_MODIFY(MemoryQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);





					QInsertOnTail(memoryqueue, tempmem);
					//currentPCB->pageTable = currentPagetable;
					//currentPagetable[Status] = (short)PTBL_VALID_BIT | (r & PTBL_PHYS_PG_NO);
					//ShadowPageTable[currentPCB->PID][Status] = (short)(PTBL_REFERENCED_BIT | PTBL_VALID_BIT) | (r & PTBL_PHYS_PG_NO);
				}
				else {
					//set to valid
					//copy from disk
					char memory_read[16] = { 0 };
					pDisk_read(DeviceID, Status, (long)memory_read);
					Z502WritePhysicalMemory(frameLocation, (char*)memory_read);

					currentPagetable[Status] = (short)(PTBL_VALID_BIT) | (frameLocation & PTBL_PHYS_PG_NO);
					ShadowPageTable[currentPCB->PID][Status] = (short)PTBL_REFERENCED_BIT | ((short)(PTBL_VALID_BIT) | (frameLocation & PTBL_PHYS_PG_NO));
					
					QInsertOnTail(memoryqueue, tempmem);
					//put into that location
					//currentPagetable[Status] = (short)PTBL_VALID_BIT | (frameLocation & PTBL_PHYS_PG_NO);
					//char memory_read[16] = {0};
					//pDisk_read(DeviceID, 100, (long)memory_read);
					//Z502WritePhysicalMemory(frameLocation, (char*)memory_read);
					//ShadowPageTable[currentPCB->PID][Status] = (short)(PTBL_REFERENCED_BIT | PTBL_VALID_BIT) | (frameLocation & PTBL_PHYS_PG_NO);
					setBitmap_mem(frameLocation);
					//currentPCB->pageTable = currentPagetable;
				}
			}

			//READ_MODIFY(MemoryQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult_memory);
			memory_print();

		}
		else if (DeviceID == INVALID_PHYSICAL_MEMORY)
		{
			aprintf("invalid number %d \n", Status);
			printf("INVALID_PHYSICAL_MEMORY \n");
		}
		else if (DeviceID == CPU_ERROR)
		{
			printf("CPU_ERROR \n");
		}
		else if (DeviceID == PRIVILEGED_INSTRUCTION)
		{
			printf("PRIVILEGED_INSTRUCTION \n");
		}
		//

		mmio.Mode = Z502GetInterruptInfo;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
		MEM_READ(Z502InterruptDevice, &mmio);
		DeviceID = mmio.Field1;
		Status = mmio.Field2;
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
	int Dir;
	INT32 PIDtemp;
	INT32 Time;
	MEMORY_MAPPED_IO mmio;
	PCB* newPCB;
	INT32 diskAll;
	INT32 diskID_temp;
	char lock_disk[20];


	//char dataWrite_temp[16];
	newPCB = calloc(1,sizeof(PCB));
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
		case SYSNUM_TERMINATE_PROCESS:
			if ((long)SystemCallData->Argument[0] == -2) {
				//QPrint(QID_suspend);
				//QPrint(QID_ready);
				//QPrint(QID_timer);
				//QPrint(QID_allprocess);
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
					READ_MODIFY(ReadyQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_ready);
					dequeueByPid(PIDtemp, QID_ready);
					READ_MODIFY(ReadyQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_ready);
					READ_MODIFY(TimerQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_timer);
					dequeueByPid(PIDtemp, QID_timer);
					READ_MODIFY(TimerQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_timer);
					//QPrint(QID_suspend);
					dequeueByPid(PIDtemp, QID_suspend);
					dequeueByPid(PIDtemp, QID_allprocess);
					diskAll = 0;
					
					for (diskID_temp = 0; diskID_temp <= 7; diskID_temp++)//i = 0; i <= 7; i++
					{
						sprintf(lock_disk, "Disk_%ld_lock", diskID_temp);
						READ_MODIFY(lock_disk, DO_LOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_disk[0]);
						//printf("********************%%%%%%%%%%%%%%%%%%%%%%5\n");
						dequeueByPid(PIDtemp, QID_disk[diskID_temp]);
						diskAll = (long)QNextItemInfo(QID_disk[diskID_temp]) + diskAll;
						READ_MODIFY(lock_disk, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_disk[0]);
						//printf("**************************************************************");
					}
					
					if (QNextItemInfo(QID_timer) == -1 && QNextItemInfo(QID_ready) == -1 && diskAll == -8) {
						//terminate whole system
						mmio.Mode = Z502Action;
						mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
						MEM_WRITE(Z502Halt, &mmio);

					}
					//QPrint(QID_timer);
					//QPrint(QID_ready);
					*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
					SP_print("terminate", currentPCB->PID);
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
					READ_MODIFY(ReadyQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_ready);
					dequeueByPid((long)SystemCallData->Argument[0], QID_ready);
					READ_MODIFY(ReadyQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_ready);
					READ_MODIFY(TimerQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_timer);
					dequeueByPid((long)SystemCallData->Argument[0], QID_timer);
					READ_MODIFY(TimerQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_timer);
					dequeueByPid((long)SystemCallData->Argument[0], QID_suspend);
					dequeueByPid((long)SystemCallData->Argument[0], QID_allprocess);
					
					for (diskID_temp = 0; diskID_temp <= 7; diskID_temp++)//i = 0; i <= 7; i++
					{
						sprintf(lock_disk, "Disk_%ld_lock", diskID_temp);
						READ_MODIFY(lock_disk, DO_LOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_disk[0]);
						dequeueByPid((long)SystemCallData->Argument[0], QID_disk[diskID_temp]);
						READ_MODIFY(lock_disk, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
							&LockResult_disk[0]);
						//printf("**************************************************************");
					}
					SP_print("terminate", currentPCB->PID);
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
				////initial Disk part
				newPCB->diskID = 0;
				newPCB->sector = 0;
				newPCB->WriteOrRead = 0;
				//strcpy(newPCB->DiskData, "0");
				newPCB->DiskData = 0;
				//////
				newPCB->target_pid = -2;
				newPCB->source_pid = -2;
				newPCB->actual_source_pid = -2;
				newPCB->send_length = -2;
				newPCB->receive_length = -2;
				strcpy(newPCB->msg_buffer, "0");
				//newPCB->MessageData = 0;
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
			//QPrint(QID_suspend);
			//currnet process direct put into suspend queue
			if ((INT32)SystemCallData->Argument[0] == -1|| (INT32)SystemCallData->Argument[0] == currentPCB->PID) {
				//currentPCB->suspendFlag = 1;
				//QInsert(QID_suspend, currentPCB->priority,currentPCB);
				//*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
				//dispatcher();
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			else {//check process exit or not first
				if (checkPID((INT32)SystemCallData->Argument[0]) == -1|| checkPID_suspend((INT32)SystemCallData->Argument[0]) != -1) {
					*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
				else {//if process in ready queue, put into suspend queue
					//QPrint(QID_ready);
					SP_print("suspendbegin", currentPCB->PID);
					READ_MODIFY(ReadyQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_ready);
					suspendByPid((INT32)SystemCallData->Argument[0], QID_ready);
					READ_MODIFY(ReadyQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
						&LockResult_ready);
					//QPrint(QID_ready);
					//if process in timer queue, set the flag to 1
					suspendByPid_timer((INT32)SystemCallData->Argument[0]);
					SP_print("suspendend", currentPCB->PID);
					*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
				}
			}
			//QPrint(QID_suspend);
			break;
		case SYSNUM_RESUME_PROCESS:
			//check process exit or not first
			if (checkPID_suspend((INT32)SystemCallData->Argument[0]) == -1) {
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			else{//dequeue from suspend queue and put into ready queue
				SP_print("resumebegin", currentPCB->PID);
				resumePID((INT32)SystemCallData->Argument[0]);
				SP_print("resumeend", currentPCB->PID);
				*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
			}
			break;
		//SYSNUM_PHYSICAL_DISK_READ
		case SYSNUM_PHYSICAL_DISK_READ:
			if ((long)SystemCallData->Argument[0] >= 0 && (long)SystemCallData->Argument[0] <= 7) {
				if ((long)SystemCallData->Argument[1] >= 0 && (long)SystemCallData->Argument[1] <= 2047) {
					//char disk_buffer_write[PGSIZE];
					//strncpy(disk_buffer_write, (long)SystemCallData->Argument[2], 15);
					//printf(disk_buffer_write);
					pDisk_read((long)SystemCallData->Argument[0], (long)SystemCallData->Argument[1], (long)(char* )SystemCallData->Argument[2]);
					//SP_print("disk read", currentPCB->PID);
				}
			}
			break;
		case SYSNUM_PHYSICAL_DISK_WRITE:
			if ((long)SystemCallData->Argument[0] >= 0 && (long)SystemCallData->Argument[0] <= 7) {
				if ((long)SystemCallData->Argument[1] >= 0 && (long)SystemCallData->Argument[1] <= 2047) {
					pDisk_write((long)SystemCallData->Argument[0], (long)SystemCallData->Argument[1], (long)(char*)SystemCallData->Argument[2]);
					//SP_print("disk write", currentPCB->PID);
				}
			}
			break;
		case SYSNUM_CHECK_DISK:
			if ((long)SystemCallData->Argument[0] >= 0 && (long)SystemCallData->Argument[0] <= 7) {
				mmio.Mode = Z502CheckDisk;
				mmio.Field1 = (long)SystemCallData->Argument[0];
				mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
				MEM_READ(Z502Disk, &mmio);
				*(long*)SystemCallData->Argument[1] = ERR_SUCCESS;
			}
			else {
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}

			break;
		case SYSNUM_CHANGE_PRIORITY:
			if ((INT32)SystemCallData->Argument[0] == -1) {
				currentPCB->priority = (INT32)SystemCallData->Argument[1];
				changePriority(currentPCB->PID, (INT32)SystemCallData->Argument[1], QID_allprocess);
				*(long*)SystemCallData->Argument[2] = ERR_SUCCESS;
				SP_print("prioritybegin", currentPCB->PID);
				QInsert(QID_ready,currentPCB->priority,currentPCB);
				SP_print("priorityend", currentPCB->PID);
				dispatcher();
			}else if(checkPID((INT32)SystemCallData->Argument[0]) == -1|| (INT32)SystemCallData->Argument[1] < 0) {
				*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
			}
			else {
				SP_print("prioritybegin", currentPCB->PID);
				//change the priority from timer, ready and suspend queue
				changePriority((INT32)SystemCallData->Argument[0], (INT32)SystemCallData->Argument[1], QID_allprocess);
				READ_MODIFY(ReadyQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
					&LockResult_ready);
				changePriority((INT32)SystemCallData->Argument[0], (INT32)SystemCallData->Argument[1],QID_ready);
				READ_MODIFY(ReadyQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
					&LockResult_ready);
				READ_MODIFY(TimerQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,
					&LockResult_timer);
				changePriority((INT32)SystemCallData->Argument[0], (INT32)SystemCallData->Argument[1],QID_timer);
				READ_MODIFY(TimerQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
					&LockResult_timer);
				changePriority((INT32)SystemCallData->Argument[0], (INT32)SystemCallData->Argument[1],QID_suspend);
				*(long*)SystemCallData->Argument[2] = ERR_SUCCESS;
				SP_print("priorityend", currentPCB->PID);
			}
			break;
		case SYSNUM_SEND_MESSAGE:
			if (checkPID((INT32)SystemCallData->Argument[0]) == -1 && (INT32)SystemCallData->Argument[0]!= -1) {
				*(long*)SystemCallData->Argument[3] = ERR_BAD_PARAM;//no PID
			}else if((INT32)SystemCallData->Argument[2] > 100 || (INT32)SystemCallData->Argument[2] < 0) {
				*(long*)SystemCallData->Argument[3] = ERR_BAD_PARAM;//wrong data length
			}
			else {
				int tempSend;
				tempSend = sendMessage(currentPCB->PID,(INT32)SystemCallData->Argument[0], (char* )SystemCallData->Argument[1], (INT32)SystemCallData->Argument[2]);
				if (tempSend == 1){
					SP_print("sendmessage", currentPCB->PID);
					*(long*)SystemCallData->Argument[3] = ERR_SUCCESS;
				}
				else{
					*(long*)SystemCallData->Argument[3] = ERR_BAD_PARAM;
				}
			}
			break;
		case SYSNUM_RECEIVE_MESSAGE:
			if (checkPID((INT32)SystemCallData->Argument[0]) == -1 && ((INT32)SystemCallData->Argument[0]) != -1) {
				*(long*)SystemCallData->Argument[5] = ERR_BAD_PARAM;//no PID
			}
			else if ((INT32)SystemCallData->Argument[2] > 100 || (INT32)SystemCallData->Argument[2] < 0) {
				*(long*)SystemCallData->Argument[5] = ERR_BAD_PARAM;//wrong data length
			}
			else {
				int tempReceive_length;
				
				receiveMessage((INT32)SystemCallData->Argument[0], (char*)SystemCallData->Argument[1], (INT32)SystemCallData->Argument[2]);
				//(char*)SystemCallData->Argument[1] = currentPCB->msg_buffer;
				strncpy((char*)SystemCallData->Argument[1], currentPCB->msg_buffer, currentPCB->send_length);
				*(long*)SystemCallData->Argument[3] = currentPCB->send_length;//PID 
				*(long*)SystemCallData->Argument[4] = currentPCB->actual_source_pid;//length
				if (*(long*)SystemCallData->Argument[4] <= (INT32)SystemCallData->Argument[2]) {
					//SourcePID, MessageBuffer, MessageReceiveLength , &MessageSendLength, &MessageSenderPid ,
					SP_print("recievemessage", currentPCB->PID);
					*(long*)SystemCallData->Argument[5] = ERR_SUCCESS;
				}
				else {
					*(long*)SystemCallData->Argument[5] = ERR_BAD_PARAM;//wrong data length
				}
			}
			break;
		case SYSNUM_FORMAT:
			//check the parameters first

			if ((long)SystemCallData->Argument[0] >= 0 && (long)SystemCallData->Argument[0] <= 7) {
				format_disk((long)SystemCallData->Argument[0]);
			}
			else {
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			break;
		case SYSNUM_CREATE_DIR:
			
				Dir = create_dir( (char*)SystemCallData->Argument[0],1);
				if (Dir == 0) {
					*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
			
			break;
		case SYSNUM_CREATE_FILE:
			Dir = create_dir((char*)SystemCallData->Argument[0], 0);
			if (Dir == 0) {
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}

			break;
		case SYSNUM_OPEN_DIR:
			if ((long)SystemCallData->Argument[0] >= -1 && (long)SystemCallData->Argument[0] <= 7) {
				Dir = open_dir((long)SystemCallData->Argument[0],(char*)SystemCallData->Argument[1],1);
				if (Dir == -1) {
					*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
				}
				else {
					*(long*)SystemCallData->Argument[2] = ERR_SUCCESS;
				}
			}
			else {
				*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
			}
			break;
		case SYSNUM_OPEN_FILE:
			//READ_MODIFY(FileQueue_lock, DO_LOCK, SUSPEND_UNTIL_LOCKED,&LockResult_timer);
			Dir = open_dir(-1, (char*)SystemCallData->Argument[0],0);
			//READ_MODIFY(FileQueue_lock, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,&LockResult_timer);
			*(long*)SystemCallData->Argument[1] = Dir;

			if (Dir == -1) {
				*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
			}
			break;
		case SYSNUM_CLOSE_FILE:
			Dir = close_file((long)SystemCallData->Argument[0]);
			if (Dir == -1) {
				*(long*)SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			break;
		case SYSNUM_WRITE_FILE:
			Dir = readwrite_file((long)SystemCallData->Argument[0], (long)SystemCallData->Argument[1], (long)(char*)SystemCallData->Argument[2],1);
			if (Dir == -1) {
				*(long*)SystemCallData->Argument[3] = ERR_BAD_PARAM;
			}
			break;
		case SYSNUM_READ_FILE:
			Dir = readwrite_file((long)SystemCallData->Argument[0], (long)SystemCallData->Argument[1], (long)(char*)SystemCallData->Argument[2],0);
			if (Dir == -1) {
				*(long*)SystemCallData->Argument[3] = ERR_BAD_PARAM;
			}
			break;
		case SYSNUM_DIR_CONTENTS:
			if (currentPCB->CurrentLocationDisk != 0) {
				dir_content();
				*(long*)SystemCallData->Argument[0] = ERR_SUCCESS;
			}
			else {
				*(long*)SystemCallData->Argument[0] = ERR_BAD_PARAM;
			}
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
