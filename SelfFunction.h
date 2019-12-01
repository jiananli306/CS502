//define all the component we need
#include "global.h"

INT32 PID;
INT32 FileID;
INT32 CurrentProcessNumber;
INT32 scheduleprinterFlag; //to enable scheduleprinter 0 == none; 1 == FULL; 2 == limited;
INT32 scheduleprinterCounter; // for partial N <= 50 print

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
#define			suspendbyMessage_lock		(MEMORY_INTERLOCK_BASE + 4)

#define			Disk_0_lock			(MEMORY_INTERLOCK_BASE + 5)
#define			Disk_1_lock			(MEMORY_INTERLOCK_BASE + 6)
#define			Disk_2_lock			(MEMORY_INTERLOCK_BASE + 7)
#define			Disk_3_lock			(MEMORY_INTERLOCK_BASE + 8)
#define			Disk_4_lock			(MEMORY_INTERLOCK_BASE + 9)
#define			Disk_5_lock			(MEMORY_INTERLOCK_BASE + 10)
#define			Disk_6_lock			(MEMORY_INTERLOCK_BASE + 11)
#define			Disk_7_lock			(MEMORY_INTERLOCK_BASE + 12)

#define			tempQueue_lock		(MEMORY_INTERLOCK_BASE + 13)
#define			FileQueue_lock		(MEMORY_INTERLOCK_BASE + 14)

//char Success[] = "      Action Failed\0        Action Succeeded";
INT32 LockResult_timer;
INT32 LockResult_ready;
INT32 LockResult_suspend;
INT32 LockResult_suspendbymessage;
INT32 LockResult_temp;
INT32 LockResult_disk[8];


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
	long    receive_length;
	char    msg_buffer[LEGAL_MESSAGE_LENGTH];
} Message_DATA;

typedef struct {
	long currentProcessID;
	long TargetProcessID;
	char MessageBuffer[LEGAL_MESSAGE_LENGTH];
	long MessageSendLength;
}Message_send;
INT32 sendMessageCount;
#define         MAX_Message_pending_number                 10//intotal there will be 10 message in the message send queue

//define block 0
typedef struct {
	char	FileSystem;
	__int8	DiskId;
	short	DiskLength;
	unsigned __int8	Bitmap;
	unsigned __int8	SwapSize;
	short	BitmapLoca;
	short	RootDirLoca;
	short	SwapLoca;
	__int8	Reserve0;
	__int8	Reserve1;
	__int8	Reserve2;
	__int8	Revision;
}Block0;
//define the header of directry/file
typedef struct {
	__int8	Inode;
	char	Name[7];
	INT32	CreateTime; //we only use 3 bytes here
	__int8	FileDiscrip;
	short	IndexLoca;
	short	FileSize;
}Header;

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
	INT32	CurrentLocationDisk;
	INT32   CurrentFileDisk;
	//message
	//Message_DATA *MessageData;
	long    target_pid;
	long    source_pid;
	long    actual_source_pid;
	long    send_length;
	long    receive_length;
	char    msg_buffer[LEGAL_MESSAGE_LENGTH];

}PCB;

PCB *currentPCB;

///PCB queue
///1. ready queue 2.suspend queue 3.block queue
INT32 QID_ready;//ready queue
INT32 QID_timer;//timer queue
INT32 QID_allprocess;//all process generated
INT32 QID_suspend;//suspend queue
INT32 QID_suspendbyMessage;//recieve message but is not there
INT32 QID_temp;//temp Q for search
INT32 QID_disk[8];
INT32 message_sendqueue;//send message and wait for recieve queue
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
extern int resumePIDMessgage(INT32 PID, Message_send* message);
extern int sendMessage(INT32 currentProcessID, INT32 ProcessID, char* MessageBuffer, INT32 MessageSendLength);//ProcessID, MessageBuffer, MessageSendLength
extern void receiveMessage(INT32 ProcessID, char* MessageBuffer, INT32 MessageReceiveLength);//receiveMessage
extern void format_disk(long disk);//format the disk
extern void setHeader(long disk, long HeaderLoca, char* HeaderNameHeader, int file_direc, int indexLevel, int parentNode);
extern void setSwap(long disk, long swapLocation, long swapSize);
extern void setBitmap(long disk, long BitmapLocation, long sectorLocation);
extern void setBitmap_0(long disk, long BitmapLocation, long Bitmap);
extern int findFirst0Bitmap(long disk);
extern int create_dir( char* name,int fileOrDir);
extern int open_dir(long disk,char* name, int fileOrDir);
extern int close_file(long Inode);
extern int readwrite_file(long Inode, long Index, long dataWrite,long read_write);
extern void write_file_level0(long disk,long sector,long dataWrite);
extern void read_file_level0(long disk, long sector,long Index, long dataWrite);
extern void dir_content();