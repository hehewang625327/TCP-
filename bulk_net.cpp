#include "bulk_net.h"
#include "3rd/CJsonObject.hpp"

using std::vector;
using std::string;

namespace bulk3
{


Net::Net()
{
	this->gunValue_ = false;
}


Net::~Net()
{
	
}

bool Net::Init(const NetConfig &config)
{
	this->config_ = config;

	this->tcp_.Connect(this->config_.ip_, this->config_.port_);
	if (this->tcp_.IsConnect())
	{
		printf("NetModule::Init, ok, ip:%s, port: %d\n", this->config_.ip_.c_str(), this->config_.port_);
		return true;
	}
	else
	{
		// tcp_ connect失败，自己会关闭
		printf("error, NetModule::Init failed, ip:%s, port: %d\n", this->config_.ip_.c_str(), this->config_.port_);
		return false;
	}
}

bool Net::Process()
{
	// 处理发送队列
	if (this->state_ == NetState::NetState_WaitSend)
	{
		// 弹出一个sendEvent
		if (this->sendEvents_.Pop(NULL, this->curEvent_))
		{
			bool sendEventRet = this->SendEvent(this->curEvent_);
			this->stateTime_ = time(NULL);
			if (sendEventRet)
			{
				this->state_ = NetState::NetState_WaitRecv;
			}
			else
			{
				this->state_ = NetState::NetState_WaitSend;
				if ((this->curEvent_.type_ == NetHead::PHT_Event) || (this->curEvent_.type_ == NetHead::PHT_VIDEO) ||
					(this->curEvent_.type_ == NetHead::PHT_PIC))
				{
					// 只收报警事件图片与录像
					this->sendFailedEvents_.Push(this->curEvent_);
				}
		
			}
			
			printf("sendEventRet: %s\n", sendEventRet?"ok":"fail");
		}
	}

	// 收数据
	bool recvRet = this->RecvValue(10);

	// 处理超时时间
	// 当前是 NetState_WaitRecv，时间超时也算失败
	if (this->state_ == NetState::NetState_WaitRecv)
	{
		time_t nowTime = time(NULL);
		if (abs(nowTime - this->stateTime_) > 30)
		{
			if ((this->curEvent_.type_ == NetHead::PHT_Event) || (this->curEvent_.type_ == NetHead::PHT_VIDEO) ||
				(this->curEvent_.type_ == NetHead::PHT_PIC))
			{
				// 只收报警事件图片与录像
				this->sendFailedEvents_.Push(this->curEvent_);
			}
			this->state_ = NetState::NetState_WaitSend;
			printf("overtime :this state is NetState_WaitSend\n");
		}
	}



	return true;
}



bool Net::InsertSendList(const BulkEvent &sendEvent)
{
	
	this->sendEvents_.Push(sendEvent);
	return true;
}


// 取回发送失败的实时数据
bool Net::GetSendFailedEvents(vector<BulkEvent> &sendFailedEvents)
{
	BulkEvent failEvent;
	while (this->sendFailedEvents_.Pop(NULL, failEvent))
	{
		sendFailedEvents.push_back(failEvent);
	}
	
	return true;
}


// 发送事件
bool Net::SendEvent(BulkEvent &baevent)
{
	bool ret = false;
	
	int sendBufferLen = baevent.GetSendBufferLen();
	if (sendBufferLen <= 0)
	{
		// todo 这种情况下，应当处理这个baevent
		return false;
	}

	if (!this->tcp_.IsConnect())
	{
		this->tcp_.Connect(this->config_.ip_, this->config_.port_);
		if (!this->tcp_.IsConnect())
		{
			printf("ERROR, Net::SendEvent, IsConnect failed\n");
			return false;
		}
	}

	uint8_t *sendBuffer = new uint8_t[sendBufferLen];
	if (sendBuffer)
	{
		baevent.MakeSendBuffer(sendBuffer);
		int realSend = this->tcp_.Send(sendBuffer, sendBufferLen, 500);
	
		delete [] sendBuffer;
		#if 1
		if (realSend == sendBufferLen)
		{
			printf("%s\n send json ok: %d\n", baevent.value_.c_str(), realSend);
			ret = true;
			
		}
		else
		{
			ret = false;
			gRuninfo.mNetFlag = false;
		}
		#endif
	}
	else
	{
		ret = false;
	}
	return ret;	
}


// 私有协议通用回复判断
bool Net::ParseCurrencyRsp(std::string &msg, std::string &resp)
{
	neb::CJsonObject respJson;
	if (respJson.Parse(resp))
	{
		std::string dateTime;
		respJson.Get("datetime", dateTime);
		printf("%s, resp json: %s\n datetime: %s\n", msg.c_str(), resp.c_str(), dateTime.c_str());
		return true;
	}
	else
	{
		printf("ERROR, %s, NetModule::ParseCurrencyRsp, parse failed, %s\n", msg.c_str(), resp.c_str());
		return false;
	}
}


bool Net::GetGunValue()
{
	ThreadMutexGuard guard(&this->gunValueMutex_);
	bool ret = this->gunValue_;
	return ret;
}


bool Net::RecvValue(int count)
{
	printf("NetModule::RecvValue, start......\n");
	if (!this->tcp_.IsConnect())
	{
		this->tcp_.Connect(this->config_.ip_, this->config_.port_);
		if (!this->tcp_.IsConnect())
		{
			printf("ERROR, NetModule::SendEvent, IsConnect failed\n");
			return false;
		}
	}

	uint8_t recvtype;
	bool sendResult = false;
	std::string respString;
	bool recvRet = this->tcp_.RecvRsp(respString, recvtype, count);
	switch ((NetHead::Type)recvtype)
	{
		case NetHead::PHT_GRAB_THE_GUN:
		{
			ThreadMutexGuard guard(&this->gunValueMutex_);
			this->gunValue_ = true;
			std::string msg = "grap the gun";
			sendResult = this->ParseCurrencyRsp(msg, respString);

		}
		break;
		
		case NetHead::PHT_HANG_UP_THE_GUN:
		{
			ThreadMutexGuard guard(&this->gunValueMutex_);
			this->gunValue_ = false;
			std::string msg = "hand up the gun";
			sendResult = this->ParseCurrencyRsp(msg, respString);
		}
		
		break;
		
		case NetHead::PHT_State:
		case NetHead::PHT_Beat:
		case NetHead::PHT_Event:
		{
			std::string msg = "json";
			sendResult = this->ParseCurrencyRsp(msg, respString);
			this->state_ = NetState_WaitSend;
			gRuninfo.mNetFlag = true;
		}
		
		break;
		
		case NetHead::PHT_PIC:
		{
			std::string msg = "jpg";
			sendResult = this->ParseCurrencyRsp(msg, respString);
			this->state_ = NetState_WaitSend;
		}
		
		break;
		case NetHead::PHT_VIDEO:
		{
			std::string msg = "video";
			sendResult = this->ParseCurrencyRsp(msg, respString);
			this->state_ = NetState_WaitSend;
		}		
		break;
		
		default:
			sendResult = false;
			printf("recevied type invaild ,type %d\n",recvtype);
		break;
	}

	return sendResult;
}



}


