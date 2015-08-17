# Pipe
A Hybrid Stream &amp; Dynamic Buffer for C, which attaches to Socket file descriptors. (Tested on Linux / FreeBSD / QNX) 

	typedef struct Pipe
	{
		int id;
		int type;
		int socket;
	
		void *in;
		int offset_in;
		int len_in;
	
		/* Parser */
		int rx_offset; //Actual Receive offset of the Packet Filter	
		int rx_status;
		int TypeID;
		int PacketLen;
		int PacketOffset;
	
		void *out;
		int offset_out;
		int len_out;
		int tx_offset; //Transmit offset
	
		int status;
	} Pipe;

void pipeDebug(Pipe *pipe);
void pipeDestroy(Pipe *pipe);

**Allocate a new pipe** 
Pipe *pipeCreate();

**Attach to a File Descriptor** 
void pipeAttachNetSocket(Pipe *pipe, int socket);

**Manually expand buffers** 
void pipeExpandInBuffer(Pipe *pipe, int len);
void pipeExpandOutBuffer(Pipe *pipe, int len);


**Flush buffers** 


void pipeFlushOutBuffer(Pipe *pipe);

void pipeFlushInBuffer(Pipe *pipe);


**Do Rx/Tx IO** 
void pipeIO(Pipe *pipe);


**Status Check Pipe**
int pipeStat(Pipe *pipe);


**Write some data into a Pipe's Out Buffer** 

void pipeWriteOutBuffer(Pipe *pipe, void *src, int len);



** Write some data into a Pipe's In Buffer ** 
void pipeWriteInBuffer(Pipe *pipe, void *src, int len);

** Read Data From Pipe **
void pipeReadOperation(Pipe *pipe);
void pipeReadMaxOperation(Pipe *pipe,int Max);

** Attempt to write any data that is stuck in the pipe's Out Buffer **
void pipeWriteOperation(Pipe *pipe);
