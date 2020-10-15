#ifndef _H_BULK_NET_20200722_
#define _H_BULK_NET_20200722_

/*
* 与通讯机通讯
*/

#include <string>
#include <list>
#include "tool.h"
#include "socket.h"
#include "bulk_common.h"
#include "bulk_config.h"

using std::string;
using std::vector;

namespace bulk3
{

enum NetState
{
	NetState_WaitSend,			// 可发数据状态
	NetState_WaitRecv			// 等待数据状态
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 通讯
class Net
{
public:
	Net();
	virtual ~Net();

	Net(const Net&) = delete;  				
	Net & operator = (const Net&) = delete; 
    
	bool Init(const NetConfig &config);

public:
	bool Process();
	bool InsertSendList(const BulkEvent &sendEvent);
	bool GetSendFailedEvents(std::vector<BulkEvent> &sendFailedEvents);	// 取回发送失败的实时数据
	bool GetGunValue();

	bool SendEvent(BulkEvent &baevent);									// 发送事件
	bool ParseCurrencyRsp(std::string &msg, std::string &resp);			// 私有协议通用回复判断
	
private:
	void ReadGunValue();				//通讯机上报提枪状态
	bool RecvValue(int count);

	NetConfig config_;
	TcpSocket tcp_;
	ThreadMutex gunValueMutex_;
	bool gunValue_;

	DataList<BulkEvent> sendEvents_;									// 等待发送的行为事件队列
	//DataList<NetRecvData> recvEvents_;								// 接收到的数据
	BulkEvent curEvent_;
	
	DataList<BulkEvent> sendFailedEvents_;								// 发送失败的行为事件队列
	DataList<BulkEvent> recvEvents_;									// 接收数据

	NetState state_ = NetState_WaitSend;
	time_t stateTime_;													// 修改状态时间
};


}

#endif


