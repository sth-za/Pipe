#ifndef PIPE_H
#define PIPE_H


//

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define PIPE_EXPAND_THRESHOLD 1024 //Expand by DEFAULT value if we short more than this
#define PIPE_EXPAND_DEFAULT 1024*1024
#define PIPE_EXPAND_CUTOFF 1024*1024*4 //CUT PIPE if we reach 2MB, prevent slow clients from using excessive memory

#define PIPE_NORMAL 0x0000  //default pipe state
#define PIPE_CLOSED	0x0001	//pipe has been closed
#define PIPE_OPENED	0x0002	//pipe has been opened
#define PIPE_READY	0x0004	//pipe has been opened

//pipeStat() return values
#define PIPE_CLEAN 	0x0000 //nothing to report on the pipe
#define PIPE_READ 	0x0001 //set when data has arrived
#define PIPE_WRITE 	0x0002	//set when pipe has data to deliver
#define PIPE_BROKEN 0x0004  //set when pipe is broken


#define PIPE_DEAD	0x0000 //dead pipe with no links
#define PIPE_NET 	0x0001 //pipe attached to network socket
#define PIPE_LOCAL	0x0002	//local pipe

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
Pipe *pipeCreate();
void pipeAttachNetSocket(Pipe *pipe, int socket);
void pipeExpandInBuffer(Pipe *pipe, int len);
void pipeExpandOutBuffer(Pipe *pipe, int len);
void pipeFlushOutBuffer(Pipe *pipe);
void pipeFlushInBuffer(Pipe *pipe);
void pipeIO(Pipe *pipe);
int pipeStat(Pipe *pipe);
void pipeWriteOutBuffer(Pipe *pipe, void *src, int len);
void pipeWriteInBuffer(Pipe *pipe, void *src, int len);
void pipeReadOperation(Pipe *pipe);
void pipeReadMaxOperation(Pipe *pipe,int Max);
void pipeWriteOperation(Pipe *pipe);
#endif
