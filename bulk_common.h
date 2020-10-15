#ifndef _H_BULK_COMMON_20200722_
#define _H_BULK_COMMON_20200722_


/*
* 散油宝通用定义
* 借用大华行为分析项目
*/


#include <string>
#include <vector>
#include <map>
#include <set>
#include <time.h>
#include <cstring>
#include <arpa/inet.h>
#include "tool.h"

namespace bulk3
{


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 算法机与通讯机协议包头
class NetHead
{
public:
	enum Type
	{
		PHT_NULL = 0,			// 未知	
		PHT_GRAB_THE_GUN = 1,    //提枪
		PHT_HANG_UP_THE_GUN = 2,  //挂枪
		PHT_Beat = 5,			// 心跳
		PHT_State = 6,			// 状态
		PHT_Event = 7,			// 报警
		PHT_PIC = 8,			// 图片
		PHT_VIDEO = 9			// 视频
		
	};
	
public:
	NetHead(Type type, int jsonLen, int picLen, int videoLen);
	~NetHead() {}
	int MakeHead(uint8_t *pHead);

	void Host2Net();			// 主机字节序=>网络字节序
	void Net2Host();			// 网络字节序=>主机字节序
	
public:
	uint8_t byteY_;				// 固定为字符Y
	uint8_t byteL_;				// 固定为字符L
	uint8_t type_;				// 对应PrivateHeadType
	uint8_t reserved_;			// 保留，凑对齐的
	uint32_t totalLen_;			// 总长度不含包头
	uint32_t jsonLen_;			// 保留，原定义为json长度
	uint32_t picLen_;			// 保留，原定义为pic长度
	uint32_t videoLen_;			// 保留，原定义为video长度
	
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 散油宝事件
class BulkEvent
{
public:
	BulkEvent();
	virtual ~BulkEvent();

	int GetSendBufferLen();
	bool MakeSendBuffer(uint8_t *sendBuffer);
	std::string MakeSaveSql();									// 生成保存sql语句

public:
	uint8_t type_  = NetHead::PHT_NULL;
	std::string value_;											// 根据type_，可能是json,也可能是照片，录像的文件名
	std::string uuid_;
	time_t time_;												// 散油宝事件条件成立的时间
};


}

#endif

