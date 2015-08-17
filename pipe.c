#include "pipe.h"


void *reallocf(void *ptr, size_t size)
{
	void *nptr;

	nptr = realloc(ptr, size);
	if (!nptr && ptr)
		free(ptr);
	return (nptr);
}


int pipeId=0;


void pipeDestroy(Pipe *pipe)
{
	printf("pipe %d: destroying buffers and pipe..\n", pipe->id);

	close(pipe->socket);

	memset(pipe->in, '\0', pipe->len_in);
	free(pipe->in);

	memset(pipe->out, '\0', pipe->len_out);
	free(pipe->out);
	
	memset(pipe, '\0', sizeof(Pipe));
	free(pipe);
}


//Create new Pipe
Pipe *pipeCreate()
{
	Pipe *ptr = NULL;

	ptr = (Pipe*)malloc(sizeof(Pipe));
	memset(ptr, 0, sizeof(Pipe));
    pipeId++;
	ptr->id = pipeId;
	//ptr->socket = socket;

	pipeExpandInBuffer(ptr, PIPE_EXPAND_DEFAULT);
	pipeExpandOutBuffer(ptr, PIPE_EXPAND_DEFAULT);

	return ptr;
}

//Attach pipe to socket and do some TCP optimization for small data transfers
void pipeAttachNetSocket(Pipe *pipe, int socket)
{
	int flag=1;

	pipe->socket = socket;
	pipe->status = PIPE_OPENED;
	pipe->type = PIPE_NET;

 	/*if (setsockopt(pipe->socket,SOL_SOCKET,SO_SNDLOWAT,&flag,sizeof(int)) == -1)
	{
		perror("setsockopt SO_SNDLOWAT");
    }*/	
    
    if (setsockopt(pipe->socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int)) == -1)
    {
    	perror("setsockopt TCP_NODELAY");
    } 

	printf("pipe %d: attached socket=%d\n", pipe->id, socket);
}

void pipeDetach(Pipe *pipe)
{
	printf("pipe %d: Detaching pipe..\n", pipe->id);
	pipe->socket = 0;
	pipe->status = PIPE_NORMAL;
	pipe->type = PIPE_DEAD;
}

void pipeDebug(Pipe *pipe)
{
	int status = pipeStat(pipe);

	printf("pipe %d:\tIn Buffer [ Size: %d Offset: %d Status:", pipe->id, pipe->len_in, pipe->offset_in);
	
	if(status & PIPE_READ)
	{
		printf(" READ");
	}

	if(status & PIPE_WRITE)
	{
		printf(" WRITE");
	}

	if(status & PIPE_BROKEN)
	{
		printf(" BROKEN");
	}
	
	if(status & PIPE_CLEAN)
	{
		printf(" CLEAN");
	}

	printf("]\n");
}

//perform IO tick operation
void pipeIO(Pipe *pipe)
{
	pipeWriteOperation(pipe);
	pipeReadOperation(pipe);
}

//get status information
int pipeStat(Pipe *pipe)
{
	int status = 0;

	if(((pipe->offset_in>0) && ((pipe->rx_offset+1) < (pipe->offset_in))) || ((pipe->rx_offset==0) && (pipe->offset_in==1)) )
	{ 
		status |= PIPE_READ;
	}	

	if(pipe->tx_offset < pipe->offset_out)
	{
		status |= PIPE_WRITE;
	}

	if(pipe->status == PIPE_CLOSED)
	{
		status |= PIPE_BROKEN;
	}
	
	return status;
}


void pipeReadMaxOperation(Pipe *pipe,int Max)
{
	struct timeval tv;
    fd_set readfds;
	int ret = 0, selectret=0;
	

	//we can only read if the pipe is open
	if ((pipe->type == PIPE_NET) && (pipe->status == PIPE_OPENED)) 
	{
		/*
		tv.tv_sec = 0;	
		tv.tv_usec = 1;
		FD_ZERO(&readfds);
		FD_SET(pipe->socket, &readfds);

		selectret = select(pipe->socket+1, &readfds, NULL, NULL, &tv);

		if((selectret>0) && FD_ISSET(pipe->socket, &readfds))
		{	*/
			//Expand buffer if its full
			if(pipe->offset_in == pipe->len_in)
			{	
				pipeExpandInBuffer(pipe, PIPE_EXPAND_DEFAULT);
			}

			ret = recv(pipe->socket, pipe->in+ pipe->offset_in, Max, 0); 

			if(ret >0)
			{
				pipe->offset_in += ret;
			//	printf("pipe %d: read %d bytes offset=%d buffersize=%d\n", pipe->id, ret, pipe->offset_in, pipe->len_in);
			}
			else if (ret == 0)
			{
				printf("pipe %d: Connection closed\n", pipe->id);
				close(pipe->socket);
				pipe->status = PIPE_CLOSED;
			}

			else
			{
				printf("pipe %d: %s\n", pipe->id, strerror(errno));
				close(pipe->socket);
				pipe->status = PIPE_CLOSED;
			}
		/*}
		else if(selectret == 0)
		{	
			//printf("pipe %d: no data ready to be read.\n", pipe->id);
		}*/
	}
}

//Read data from socket into In buffer
void pipeReadOperation(Pipe *pipe)
{
	struct timeval tv;
    fd_set readfds;
	int ret = 0, selectret=0;
	

	//we can only read if the pipe is open
	if ((pipe->type == PIPE_NET) && (pipe->status == PIPE_OPENED)) 
	{
		/*
		tv.tv_sec = 0;	
		tv.tv_usec = 1;
		FD_ZERO(&readfds);
		FD_SET(pipe->socket, &readfds);

		selectret = select(pipe->socket+1, &readfds, NULL, NULL, &tv);

		if((selectret>0) && FD_ISSET(pipe->socket, &readfds))
		{	*/
			//Expand buffer if its full
			if(pipe->offset_in == pipe->len_in)
			{	
				pipeExpandInBuffer(pipe, PIPE_EXPAND_DEFAULT);
			}

			ret = recv(pipe->socket, pipe->in+ pipe->offset_in, pipe->len_in - pipe->offset_in, 0); 

			if(ret >0)
			{
				pipe->offset_in += ret;
			//	printf("pipe %d: read %d bytes offset=%d buffersize=%d\n", pipe->id, ret, pipe->offset_in, pipe->len_in);
			}
			else if (ret == 0)
			{
				printf("pipe %d: Connection closed\n", pipe->id);
				close(pipe->socket);
				pipe->status = PIPE_CLOSED;
			}

			else
			{
				printf("pipe %d: %s\n", pipe->id, strerror(errno));
				close(pipe->socket);
				pipe->status = PIPE_CLOSED;
			}
		/*}
		else if(selectret == 0)
		{	
			//printf("pipe %d: no data ready to be read.\n", pipe->id);
		}*/
	}
}

//Write data from Out buffer to socket
void pipeWriteOperation(Pipe *pipe)
{
	struct timeval tv;
    fd_set writefds;
	int ret = 0, selectret=0;
	
	
 
	//only do this if we have data waiting to be written
	if(/*(pipe->type == PIPE_NET) && (pipe->status == PIPE_OPENED) &&  */(pipe->tx_offset < pipe->offset_out))
	{
		//printf(" ******** tx offset=%d  offset out= %d\n",pipe->tx_offset, pipe->offset_out);
		tv.tv_sec = 0;	
		tv.tv_usec = 1;
		FD_ZERO(&writefds);
		FD_SET(pipe->socket, &writefds);

		selectret = select(pipe->socket+1, NULL, &writefds, NULL, &tv);

		if((selectret>0) && (FD_ISSET(pipe->socket, &writefds)))
		{	
			ret = send(pipe->socket, pipe->out + pipe->tx_offset, pipe->offset_out- pipe->tx_offset , 0);
			if(ret>0)
			{
				//printf("pipe %d: Wrote %d bytes\n", pipe->id, ret);
				pipe->tx_offset+=ret;
				if(pipe->offset_out == pipe->tx_offset)
				{
					pipeFlushOutBuffer(pipe);
				}
			}
		}
	}

	
}

void pipeWriteInBuffer(Pipe *pipe, void *src, int len)
{
	//see if we have enough space inside
	int size = pipe->offset_in+len;

	if(size > pipe->len_in)
	{	
		int shortage = size-pipe->len_in;
		//Prevent lots of small memory allocations
		if(shortage > PIPE_EXPAND_THRESHOLD)
		{
			pipeExpandInBuffer(pipe, shortage + PIPE_EXPAND_DEFAULT);
		}
		else
		{
			pipeExpandInBuffer(pipe, PIPE_EXPAND_DEFAULT);
		}
		//attempt to write again
		pipeWriteInBuffer(pipe,src,len);
	}
	else
	{
		memcpy(pipe->in+pipe->offset_in, src, len);
		pipe->offset_in += len;
		//printf("pipe %d: Wrote %d bytes to In buffer\n", pipe->id, len);
	}
}

void pipeAllocInBuffer(Pipe *pipe, int size)
{
		void *ptr = NULL;

		pipeFlushInBuffer(pipe);
		free(pipe->in);
		ptr = (void*)malloc(size);

		if(ptr)
		{
			memset(ptr, 0, size);
			pipe->len_in = size;
			pipe->in = ptr;
		}
		else
		{
			printf("pipe %d: Failed to Alloc In buffer\n", pipe->id);
		}
}



//Expand pipe In Buffer
void pipeExpandInBuffer(Pipe *pipe, int len)
{
	void *ptr = NULL;

	printf("Expand...\n");
	
	if(pipe->in == NULL)
	{
		printf("pipe %d: Allocating In buffer %d bytes\n", pipe->id, len);
		pipe->in = (void*)malloc(len);
		memset(pipe->in, '\0', len);
		pipe->len_in=len;
	}
	else
	{
		printf("pipe %d: Expanding In buffer from %d to %d bytes\n", pipe->id, pipe->len_in, pipe->len_in+len);
		
		ptr = (void*)reallocf(pipe->in, pipe->len_in+len);
		if(ptr)
		{
			//clean the new portion of the memory
			memset(ptr+pipe->len_in+len, 0, len);
			pipe->len_in+= len;
			pipe->in = ptr;
		}
		else
		{
			printf("pipe %d: Failed to expand In buffer\n", pipe->id);

		}
	}
}


//Flush everything in the transmit queue
void pipeFlushOutBuffer(Pipe *pipe)
{
	//printf("pipe %d: Flushing Out buffer, len=%d\n", pipe->id, pipe->len_out);
	memset(pipe->out, '\0', pipe->len_out);
	pipe->tx_offset=0;
	pipe->offset_out=0;
	
}

//Flush everything in receive queue
void pipeFlushInBuffer(Pipe *pipe)
{
	//printf("pipe %d: Flushing In buffer, len=%d\n", pipe->id, pipe->len_in);
	memset(pipe->in, '\0', pipe->len_in);
	pipe->rx_offset=0;
	pipe->offset_in=0;
	
}



void pipeWriteOutBuffer(Pipe *pipe, void *src, int len)
{
	int size = pipe->offset_out+len;
	if(pipe->status == PIPE_OPENED)
	{
		//see if we have enough space inside
		if(size > (1024*1024*4))
		{
			close(pipe->socket);
			pipe->status = PIPE_CLOSED;
			printf("Closing pipe after 8k buffer reached.\n");

		}	
		
		else if(size > pipe->len_out)
		{	
			int shortage = size-pipe->len_out;
			//Prevent lots of small memory allocations
			if(shortage > PIPE_EXPAND_THRESHOLD)
			{
				pipeExpandOutBuffer(pipe, shortage + PIPE_EXPAND_DEFAULT);
				//printf("pipe %d: Expanding Out buffer from %d to %d bytes\n", pipe->id, pipe->len_out, pipe->len_out+len);
		
			}
			else
			{
				pipeExpandOutBuffer(pipe, PIPE_EXPAND_DEFAULT);
			    //printf("pipe %d: Expanding Out buffer from %d to %d bytes\n", pipe->id, pipe->len_out, pipe->len_out+PIPE_EXPAND_DEFAULT);
		
			}
			//attempt to write again
			pipeWriteOutBuffer(pipe,src,len);
		}
		else
		{
			memcpy(pipe->out+pipe->offset_out, src, len);
			pipe->offset_out += len;
		  // printf("pipe %d: Wrote %d bytes to Out buffer\n", pipe->id, len);
		}
	}
}


void pipeExpandOutBuffer(Pipe *pipe, int len)
{
	void *ptr = NULL;
	
	if(pipe->out == NULL)
	{
		printf("pipe %d: Allocating Out buffer %d bytes\n", pipe->id, len);
		pipe->out = (void*)malloc(len);
		memset(pipe->out, '\0', len);
		pipe->len_out=len;
	}
	else
	{
		printf("pipe %d: Expanding Out buffer from %d to %d bytes\n", pipe->id, pipe->len_out, pipe->len_out+len);
		
		ptr = (void*)reallocf(pipe->out, pipe->len_out+len);
		if(ptr)
		{
			//clean the new portion of the memory
			//printf("Addr of new Memory: %X Old Memory: %X\n", ptr, pipe->out);
			memset(ptr+pipe->len_out, 0, len);
			pipe->len_out+= len;
			pipe->out = ptr;
		}
		else
		{
			printf("pipe %d: Failed to expand Out buffer\n", pipe->id);
		}
	}
}
