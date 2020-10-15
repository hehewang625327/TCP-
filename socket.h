#ifndef _H_SOCKET_20200911_
#define _H_SOCKET_20200911_

/*
* tcp连接封装
*/
#include "tool.h"
#include "bulk_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <sys/epoll.h>

const unsigned int  g_max_rcv = (16*4096);
namespace bulk3
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// tcp socket
class TcpSocket
{
public:
	TcpSocket();
	virtual ~TcpSocket();

public:

	bool Connect(const std::string &ip, unsigned short port);
	bool Dispatch(uint8_t *Buffer, int bufferLen);
	bool do_read(int fd);
	bool do_write(int fd, uint8_t *buf, int bufferLen);
	int Send(uint8_t *pSendBuffer, int sendBufferLen, int timeoutMSec = 100);
	int Recv(uint8_t *pRecvbuffer, int recvBufferLen, int timeoutMSec = 100);
	bool RecvRsp(std::string &baeventRsp, uint8_t &recvtype, int selectcount);
	bool IsConnect();
	bool Close(int pos);

protected:
	void ResetParam();

protected:
	int socketfd_;
	int epollfd_;
	std::string ip_;
	unsigned short port_;

protected:
	uint8_t pucRecvBuffer_[g_max_rcv];
	int recvLen_;						
	int recvOff_;						
};


}
#endif

