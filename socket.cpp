#include "socket.h"
#include <netinet/tcp.h>
#include <fcntl.h>

#define MAXSIZE     1024
#define FDSIZE        1024
#define EPOLLEVENTS 20



namespace bulk3
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
TcpSocket::TcpSocket()
{
	this->ResetParam();
}


TcpSocket::~TcpSocket()
{
	this->Close(1);
}



bool TcpSocket::Connect(const std::string &ip, unsigned short port)
{
	if (this->IsConnect())
	{
		this->Close(7);
	}
	else
	{
		this->ResetParam();
	}
	
	this->ip_ = ip;
	this->port_ = port;
	
	if ( (ip.empty()) || (port == 0))
	{
		printf("ERROR, TcpSocket::Connect, param error, ip: %s, port: %d\n", ip.c_str(), port);
		return false;
	}
	else
	{
		this->socketfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (this->socketfd_ == -1)
		{
			perror("ERROR TcpLink::Connect, socket() failed: ");
			return false;
		}
		else
		{
			int syncnt = 2;
			struct epoll_event ev;
		
			setsockopt(this->socketfd_, IPPROTO_TCP, TCP_SYNCNT, &syncnt, sizeof(syncnt));

			this->epollfd_ = epoll_create(FDSIZE);
		    ev.events = EPOLLIN|EPOLLET;
		    ev.data.fd = this->socketfd_;
		    epoll_ctl(this->epollfd_,EPOLL_CTL_ADD,this->socketfd_,&ev);
			ev.data.fd=this->socketfd_;
		    ev.events=EPOLLOUT|EPOLLET;
		    epoll_ctl(this->epollfd_,EPOLL_CTL_ADD,this->socketfd_,&ev);
			struct sockaddr_in peerAddr;
			memset(&peerAddr, 0, sizeof(peerAddr));	
			peerAddr.sin_family = AF_INET;
			peerAddr.sin_addr.s_addr = inet_addr(ip.c_str());
			peerAddr.sin_port = htons(port);

			if (connect(this->socketfd_, (struct sockaddr *)&peerAddr, sizeof(peerAddr) ) < 0)
			{
				perror("error, TcpLink::Connect, connect return FAILED ");
				this->Close(6);
				return false;
			}
			else
			{
				printf("tcp connect to %s %d, OK\n", ip.c_str(), port);
				return true;
			}
		}
	}
}


bool TcpSocket::IsConnect()
{
	return (this->socketfd_ == -1)? false: true;
}

bool TcpSocket::Dispatch(uint8_t *Buffer, int bufferLen)
{
	printf("Dispatch :Buffer %s, bufferLen %d\n",Buffer,bufferLen);
	int i;
	int fd;
	int ret = -1; 
	struct epoll_event events[EPOLLEVENTS];

	int nfds = epoll_wait(this->epollfd_,events,EPOLLEVENTS,-1);
	printf("nfd %d\n",nfds);
	for (i = 0; i < nfds; i++)
    {
        fd = events[i].data.fd;
        if (events[i].events & EPOLLIN)
			ret = this->do_read(fd);
        else if (events[i].events & EPOLLOUT)
            ret = this->do_write(fd, Buffer, bufferLen);
    }
	


	return ret;
}

bool TcpSocket::do_read(int fd)
{
	printf("do_read\n");
	int ret = false;
    int nread = read(fd, this->pucRecvBuffer_ , this->recvLen_);
    if (nread == -1)
    {
        printf("read error:");
        close(fd);
		
    }
    else if (nread == 0)
    {
        printf("server close.\n");
        close(fd);
		
    }
    else
    {
    	printf("recv :%s\n",this->pucRecvBuffer_);
        ret = true;
    }
	return ret;
}

bool TcpSocket::do_write(int fd, uint8_t *buf, int bufferLen)
{ 
	printf("do_write\n");

	int ret = false;
    int nwrite = write(fd, buf, bufferLen);
    if (nwrite == -1)
    {
        printf("write error:");
        close(fd);
    }
    else
    {
       struct epoll_event ev;
       ev.data.fd=this->socketfd_;
	   ev.events=EPOLLIN|EPOLLET;
	   epoll_ctl(this->epollfd_,EPOLL_CTL_MOD,this->socketfd_,&ev);
	   memset(buf,0,bufferLen);
	   ret = true;
    }

	return ret;
    
}

bool TcpSocket::Close(int pos)
{
	printf("tcp close, [%d] bye : %s %d\n", pos, this->ip_.c_str(), this->port_);
	if (this->socketfd_ != -1)
	{
		shutdown(this->socketfd_, SHUT_RDWR);
		close(this->socketfd_);
		this->socketfd_ = -1;
		close(this->epollfd_);
		this->epollfd_ = -1;
	}

	this->ResetParam();
	return true;
}

int  TcpSocket::Send(uint8_t *sendBuffer, int sendBufferLen, int timeoutMSec)
{
	fd_set writeSet;
	struct timeval timeout;
	int totalWriteCnt = 0;
	int perWriteCnt = 0;
	do
	{
		printf("send start============\n");
		perWriteCnt = 0;
		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = timeoutMSec / 1000;
		timeout.tv_usec = (timeoutMSec - timeout.tv_sec * 1000)*1000;
		FD_ZERO(&writeSet);
		FD_SET(this->socketfd_, &writeSet);
		//fcntl(this->socketfd_,F_SETFL,fcntl(this->socketfd_,F_GETFL)|O_NONBLOCK);
		if (select(this->socketfd_ + 1, NULL, &writeSet, NULL, &timeout) > 0)
		{
			printf("select\n");
			if (FD_ISSET(this->socketfd_, &writeSet))
			{
				printf("FD_ISSET\n");
				perWriteCnt = write(this->socketfd_, (sendBuffer + totalWriteCnt), sendBufferLen);
				printf("send perWriteCnt: %d\n", perWriteCnt);
				if (perWriteCnt == -1)
				{
					printf("ERROR, write return -1\n");
					this->Close(5);
				}
				else if (perWriteCnt == 0)
				{
					printf("ERROR, write return 0\n");
					this->Close(7);
				}
				totalWriteCnt += perWriteCnt;
				sendBufferLen -= perWriteCnt;
			}
			else
			{
				perror("error, TcpLink::Send, select >0 but FD_ISSET failed");
				this->Close(4);
			}
		}
		else
		{
			printf("TcpLink::Send, select <= 0");
		}

		printf("send perWriteCnt: %d，sendBufferLen %d\n", perWriteCnt,sendBufferLen);
	}
	while ((perWriteCnt > 0) && (sendBufferLen > 0));
	//printf("send: %d\n", totalWriteCnt);
	return totalWriteCnt;
}


int TcpSocket::Recv(uint8_t *buffer, int bufferLen, int timeoutMSec)
{
	if (timeoutMSec<= 0)
		timeoutMSec = 10;

	if (buffer == nullptr)
		return -1;

	if (bufferLen <= 0)
		return -1;
		
	fd_set readSet;
	struct timeval timeout;
	int perRecvRet = 0;
	int recvRet = 0;
	
	do
	{
		perRecvRet = 0;
		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = timeoutMSec / 1000;
		timeout.tv_usec = (timeoutMSec - timeout.tv_sec * 1000)*1000;
		FD_ZERO(&readSet);
		FD_SET(this->socketfd_, &readSet);

		int selectRet = select(this->socketfd_ + 1, &readSet, NULL, NULL, &timeout);
		if (selectRet > 0)
		{
			if (FD_ISSET(this->socketfd_, &readSet))
			{
				perRecvRet = recv(this->socketfd_, buffer+recvRet, bufferLen, 0);
				//printf("perRecvRet:%d\n", perRecvRet);
				//printf("buffer = %s, bufferlen = %d\n",buffer,bufferLen);
				
				if (perRecvRet < 0)
				{
					// 网络已经失去连接
					//this->Close(3);
					recvRet = -1;
					break;
				}
				
				recvRet += perRecvRet;
				bufferLen -= perRecvRet;
			}
			else
			{
				perror("but FD_ISSET isnot socket fd\n");
			}
		}
		else if (selectRet == 0)
		{
			
			recvRet = 0;
			break;
		}
		else
		{
			
		}
	}
	while (0);//while ((perRecvRet > 0) && (bufferLen > 0));

	return recvRet;
}


// 接收平台的回复，放到netdata中
bool TcpSocket::RecvRsp(std::string &baeventRsp, uint8_t &recvtype, int selectcount)
{
	recvtype = (uint8_t)0;//NetHead::
	baeventRsp.clear();

	int timeoutHig = 10000;
	int timeoutLow = 1000;
	
	int selectcur = 0;

	bool isFirst = true;
	while((selectcur++) < selectcount)
	{    
		// -------------------------------------------------------------
		int rcvCount = this->Recv(this->pucRecvBuffer_ + this->recvOff_, this->recvLen_- this->recvOff_, isFirst? timeoutHig : timeoutLow);
		isFirst = false;
		if (rcvCount == -1)
		{
			// 链路异常或者对端关闭
			this->Close(2);
			return false;
		}
		else if (rcvCount == 0)//else if ((0 == rcvCount) && (this->recvLen_ - this->recvOff_ > 0)) 
		{
			// 如果对端关闭链路，关闭本端
			//printf("Recv2, recv 0, close\n");
			// 无数据 不一定要关闭 this->Close();
			//continue;
		}
		//printf("  ====> stream socket receive len %d\n", rcvCount);
		// -------------------------------------------------------------
		if (this->recvOff_ + rcvCount < sizeof(bulk3::NetHead))
		{
			// 消息头不足，缓存消息退出
			//printf("=======> recv data (%d) < sizeof ba::NetHead(%ld), rcvCount: %d\n", (this->recvOff_+rcvCount), sizeof(ba::NetHead), rcvCount);
			//printf("%d < sizeof nethead(%ld)\n", this->recvOff_+rcvCount, sizeof(ba::NetHead));
			this->recvOff_  += rcvCount;
			continue;
		}
		bulk3::NetHead *pHead = (bulk3::NetHead *)(this->pucRecvBuffer_);
		int totalLen = ntohl(pHead->totalLen_);
		printf("NetHead YL:%c %c, totalLen: %d\n", (char)pHead->byteY_, (char)pHead->byteL_, totalLen);

		if (((char)pHead->byteY_ != 'Y') || ((char)pHead->byteL_ != 'L'))
		{
			printf("net head, Y L error\n");
			this->Close(12);
			return false;
		}
		
		if (totalLen > g_max_rcv)
		{
			// clear buffer
			memset(this->pucRecvBuffer_, 0, sizeof(this->pucRecvBuffer_));
			this->recvLen_ = g_max_rcv;
			this->recvOff_ = 0;
			continue;
		}

		// 消息内容不足，缓存消息退出		
		if ((this->recvOff_ + rcvCount) < (totalLen + sizeof(bulk3::NetHead)))
		{
			this->recvOff_  += rcvCount;
			continue;
		}

		std::string jsonResp((char *)this->pucRecvBuffer_+sizeof(bulk3::NetHead), totalLen);
		//printf("resp json: %s\n", jsonResp.c_str());
		baeventRsp = jsonResp;

		recvtype = pHead->type_;
	
		// 派发完成,剩余消息移动到缓存头，下次派发
		this->recvOff_ = this->recvOff_ + rcvCount - totalLen - sizeof(bulk3::NetHead);
		//printf("lk packet socket receive len %d, recvOff:%d\n",totalLen, this->recvOff_);
		if (this->recvOff_ > 0)
		{
			memmove(this->pucRecvBuffer_, this->pucRecvBuffer_ + (totalLen+sizeof(bulk3::NetHead)), this->recvOff_);    
		}        
		rcvCount = 0;
		break;
	}    

	return true;
}


void TcpSocket::ResetParam()
{
	this->socketfd_ = -1;
	this->epollfd_ = -1;
	this->ip_.clear();
	this->port_ = 0;
	
	memset(this->pucRecvBuffer_, 0, sizeof(this->pucRecvBuffer_));
	this->recvLen_ = g_max_rcv;
	this->recvOff_ = 0;

}


}


