//define all the component we need


INT32 PID;
INT32 CurrentProcessNumber;
#define         MAX_Process_number                 15

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

	//memory management
	///1. pointer to text segment 2.pointer to data segment 3. pointer to stack segment

	//file management
	///1. root directory 2. working directory 3. file descriptiors 4. user id 5. group id


}PCB;



///PCB queue
///1. ready queue 2.suspend queue 3.block queue
INT32 QID_ready;//ready queue
INT32 QID_timer;//timer queue
INT32 QID_allprocess;//all process generated
extern void osCreatProcess(int argc, char* argv[]);
extern void startTimer(int during);
extern void createProcess(PCB* currentPCB);