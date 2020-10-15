#include "bulk_common.h"
#include "3rd/tinyxml2.h"
#include "3rd/CJsonObject.hpp"


using std::map;
using std::string;
using std::vector;
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 行为分析系统

namespace bulk3
{



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 

NetHead::NetHead(NetHead::Type type, int jsonLen, int picLen, int videoLen)
{
	this->byteY_ = (uint8_t)'Y';
	this->byteL_ = (uint8_t)'L';
	this->type_ = (uint8_t)type;
	this->reserved_ = (uint8_t)0;

	switch (this->type_)
	{
	case PHT_Beat:
		this->jsonLen_ = jsonLen;
		this->picLen_ = 0;
		this->videoLen_ = 0;
		this->totalLen_ = jsonLen;
	break;
	
	case PHT_Event:
		this->jsonLen_ = jsonLen;
		this->picLen_ = picLen;
		this->videoLen_ = videoLen;
		this->totalLen_ = jsonLen;
	break;
	
	case PHT_PIC:
		this->jsonLen_ = 0;
		this->picLen_ = picLen;
		this->videoLen_ = 0;
		this->totalLen_ = picLen;
	break;
	
	case PHT_State:
		this->jsonLen_ = jsonLen;
		this->picLen_ = 0;
		this->videoLen_ = 0;
		this->totalLen_ = jsonLen;
	break;

	case PHT_VIDEO:
	default:
		this->jsonLen_ = 0;
		this->picLen_ = 0;
		this->videoLen_ = videoLen;
		this->totalLen_ = videoLen;
	break;
	};
}


int NetHead::MakeHead(uint8_t *pBuffer)
{
	this->Host2Net();
	int pos = 0;

	uint8_t *pHead = pBuffer;
	
	memcpy(pHead + pos, &this->byteY_, sizeof(this->byteY_));
	pos += sizeof(this->byteY_);

	memcpy(pHead + pos, &this->byteL_, sizeof(this->byteL_));
	pos += sizeof(this->byteL_);

	memcpy(pHead + pos, &this->type_, sizeof(this->type_));
	pos += sizeof(this->type_);

	memcpy(pHead + pos, &this->reserved_, sizeof(this->reserved_));
	pos += sizeof(this->reserved_);
	
	memcpy(pHead + pos, &this->totalLen_, sizeof(this->totalLen_));
	pos += sizeof(this->totalLen_);

	memcpy(pHead + pos, &this->jsonLen_, sizeof(this->jsonLen_));
	pos += sizeof(this->jsonLen_);

	memcpy(pHead + pos, &this->picLen_, sizeof(this->picLen_));
	pos += sizeof(this->picLen_);

	memcpy(pHead + pos, &this->videoLen_, sizeof(this->videoLen_));
	pos += sizeof(this->videoLen_);
	
	this->Net2Host();
	
	return pos;
}


void NetHead::Host2Net()
{
	this->totalLen_ = htonl(this->totalLen_);
	this->jsonLen_ = htonl(this->jsonLen_);
	this->picLen_ = htonl(this->picLen_);
	this->videoLen_ = htonl(this->videoLen_);
}


void NetHead::Net2Host()
{
	this->totalLen_ = ntohl(this->totalLen_);
	this->jsonLen_ = ntohl(this->jsonLen_);
	this->picLen_ = ntohl(this->picLen_);
	this->videoLen_ = ntohl(this->videoLen_);
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BulkEvent::BulkEvent()
{
	this->time_ = time(NULL);
}


BulkEvent::~BulkEvent()
{
	this->type_ = NetHead::PHT_NULL;
}


// 获取发送buffer的长度
int BulkEvent::GetSendBufferLen()
{
	int len = -1;
	
	switch (this->type_)
	{
		case NetHead::PHT_NULL:
		break;

		case NetHead::PHT_Beat:
			len = sizeof(NetHead) + this->value_.size();
		break;

		case NetHead::PHT_State:
			len = sizeof(NetHead) + this->value_.size();
		break;

		case NetHead::PHT_Event:
			len = sizeof(NetHead) + this->value_.size();
		break;
	
		case NetHead::PHT_PIC:
			len = sizeof(NetHead) + 32 + Tool::GetFileLen(this->value_);
		break;
	
		case NetHead::PHT_VIDEO:
			len = sizeof(NetHead) + 32 + Tool::GetFileLen(this->value_);
		break;

		default:
			len = -1;
		break;
	}

	return len;
}


// 组装发送buffer
bool BulkEvent::MakeSendBuffer(uint8_t *sendBuffer)
{
	bool ret = true;
	
	if (sendBuffer == nullptr)
	{
		printf("ERROR, BulkEvent::MakeSendBuffer, sendBuffer is nullptr.\n");
		return false;
	}
		
	switch (this->type_)
	{
		case NetHead::PHT_NULL:
		break;

		case NetHead::PHT_Beat:			// 心跳：   包头+json
		case NetHead::PHT_State:		// 状态：包头+json
		{
			NetHead head((NetHead::Type)this->type_, this->value_.size(), 0, 0);
			int pos = head.MakeHead(sendBuffer);
			memcpy(sendBuffer + pos, this->value_.c_str(), this->value_.size());
		}
		break;

		case NetHead::PHT_Event:
		{
			NetHead head((NetHead::Type)this->type_, this->value_.size(), 1, 1);
			int pos = head.MakeHead(sendBuffer);
			memcpy(sendBuffer + pos, this->value_.c_str(), this->value_.size());
		}
		break;
	
		case NetHead::PHT_PIC:
		{
			int filelen = Tool::GetFileLen(this->value_);
			NetHead head((NetHead::Type)this->type_, 0, filelen + 32, 0);
			int pos = head.MakeHead(sendBuffer);
			memcpy(sendBuffer + pos, this->uuid_.c_str(), (this->uuid_.size() > 32)?32:this->uuid_.size());
			pos += 32;
			Tool::ReadFile(this->value_, sendBuffer+pos, filelen);
		}
		break;
	
		case NetHead::PHT_VIDEO:
		{
			int filelen = Tool::GetFileLen(this->value_);
			NetHead head((NetHead::Type)this->type_, 0, 0, filelen + 32);
			int pos = head.MakeHead(sendBuffer);
			memcpy(sendBuffer + pos, this->uuid_.c_str(), (this->uuid_.size() > 32)?32:this->uuid_.size());
			Tool::ReadFile(this->value_, sendBuffer + pos, filelen);
			
		}
		break;

		default:
			ret = false;
		break;
	}


	return ret;
}

// 生成保存sql语句
std::string BulkEvent::MakeSaveSql()
{
	int type = this->type_;
	char typeString[32];
	memset(typeString, 0, sizeof(typeString));
	sprintf(typeString, "%d", type);
	
	string insertSql = "INSERT INTO bulk_event VALUES(NULL, ";
	insertSql += typeString;
	insertSql += ", '";
	insertSql += this->uuid_;
	insertSql += "', '";
	insertSql += this->value_;
	insertSql += "');";


	printf("MakeSaveSql  %s\n",insertSql.c_str());
	return insertSql;
}



}



