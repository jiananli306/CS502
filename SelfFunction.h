//define all the component we need
#include "global.h"

INT32 PID;
INT32 CurrentProcessNumber;

#define         DISK_INTERRUPT_DISK2            (short)7
#define         DISK_INTERRUPT_DISK3            (short)8
#define         DISK_INTERRUPT_DISK4            (short)9
#define         DISK_INTERRUPT_DISK5            (short)10
#define         DISK_INTERRUPT_DISK6            (short)11
#define         DISK_INTERRUPT_DISK7            (short)12


#define         MAX_Process_number                 15//intotal there will be 16 process for max
#define         DO_LOCK                     1
#define         DO_UNLOCK                   0
#define         SUSPEND_UNTIL_LOCKED        TRUE
#define         DO_NOT_SUSPEND              FALSE
#define			TimerQueue_lock				(MEMORY_INTERLOCK_BASE + 1)
#define			ReadyQueue_lock				(MEMORY_INTERLOCK_BASE + 2)
#define			suspendQueue_lock			(MEMORY_INTERLOCK_BASE + 3)
#define			suspendQueue_lock			(MEMORY_INTERLOCK_BASE + 4)

#define			Disk_0_lock			(MEMORY_INTERLOCK_BASE + 5)
#define			Disk_1_lock			(MEMORY_INTERLOCK_BASE + 6)
#define			Disk_2_lock			(MEMORY_INTERLOCK_BASE + 7)
#define			Disk_3_lock			(MEMORY_INTERLOCK_BASE + 8)
#define			Disk_4_lock			(MEMORY_INTERLOCK_BASE + 9)
#define			Disk_5_lock			(MEMORY_INTERLOCK_BASE + 10)
#define			Disk_6_lock			(MEMORY_INTERLOCK_BASE + 11)
#define			Disk_7_lock			(MEMORY_INTERLOCK_BASE + 12)

//char Success[] = "      Action Failed\0        Action Succeeded";
INT32 LockResult_timer;
INT32 LockResult_ready;
INT32 LockResult_suspend;
INT32 LockResult_disk[8];
#define          SPART          22


typedef union {
	char char_data[PGSIZE];
	UINT32 int_data[PGSIZE / sizeof(int)];
} DISK_DATA;

#define         LEGAL_MESSAGE_LENGTH           (INT16)64

typedef struct {
	long    target_pid;
	long    source_pid;
	long    actual_source_pid;
	long    send_length;
	char    msg_buffer[LEGAL_MESSAGE_LENGTH];
} Message_DATA;


//PCB could refer to linux task_struct 
typedef struct ProcessControlBlock {
	//process management
	///1. registers 2. program counter 3.program status word 4. stack pointer 5. process state
	///6. priority 7. scheduling parameters 8. process id 9. parent process 
	///10.process group 11.signals 12. time when process started 13. cpu time used 14. childrens cpu time 15.time to next alarm
	char processName[20]; //related to QueueManager name size
	INT32 PID;
	INT32 priority;
	INT32 timeCreated;
	void* newContext;//mmio.field1
	void* address;//mmio.field2
	void* pageTable;//mmio.field3
	INT32 suspendFlag;

	//memory management
	///1. pointer to text segment 2.pointer to data segment 3. pointer to stack segment

	//file management
	///1. root directory 2. working directory 3. file descriptiors 4. user id 5. group id
	INT32 diskID;
	BOOL  WriteOrRead; //0 as Write, 1 as Read
	INT32 sector;
	DISK_DATA  *DiskData;


}PCB;

PCB *currentPCB;

///PCB queue
///1. ready queue 2.suspend queue 3.block queue
INT32 QID_ready;//ready queue
INT32 QID_timer;//timer queue
INT32 QID_allprocess;//all process generated
INT32 QID_suspend;//suspend queue
INT32 QID_temp;//temp Q for search
INT32 QID_disk[8];
//INT32 QID_terminated;//terminated process
extern void dispatcher();
extern void osCreatProcess(int argc, char* argv[]);
extern void startTimer(int during);
extern void createProcess(PCB* currentPCB1);
extern int checkName(char* name);
extern void dequeueByPid(INT32 PID, INT32 QID);
extern int checkPID(INT32 PID);
extern void suspendByPid(INT32 PID, INT32 QID);
extern void suspendByPid_timer(INT32 PID);
extern int checkPID_suspend(INT32 PID);
extern void resumePID(INT32 PID);
extern void changePriority(INT32 PID,INT32 priority, INT32 QID);
extern void pDisk_write(INT32 disk, INT32 sector, long dataWrite);
extern void pDisk_read(INT32 disk, INT32 sector, long dataRead);
extern void SP_print(char* Action, int targetID);