#ifndef _H_BULK3_API_20200911_
#define _H_BULK3_API_20200911_
#include <string>
#include <vector>
#include <map>

#include "bulk_common.h"
#include "bulk_config.h"
#include "bulk_database.h"
#include "bulk_net.h"
#include "tool.h"
#include "3rd/CJsonObject.hpp"

namespace bulk3
{


class BulkSystem
{

public:
	static BulkSystem &GetInst();
	BulkSystem(const BulkSystem&) = delete;  				
	BulkSystem & operator = (const BulkSystem&) = delete; 	
	bool Init(const std::string &configFile);		// 初始化
	bool StartWork();								// 开始工作
	bool StopWork();								// 停止工作
	bool GetMainloop();
	bool MakeUpHeartBeat(BulkEvent &hb);	
	bool SendHeartBeat();
	bool process(const std::string &videoFilePath);
	bool MakeUpState(BulkEvent &selfState);
	std::string MakeEventJson(const string &eventUUID);

public:
	int stationID_;									// 油站号
	int boardID_;									// 板号
	std::string boardModel_;						// 板型号
	std::string softVersion_;						// 软件版本号
	Database database_;								// 数据库
	Net net_;
								// 接收数据
private:
	BulkSystem();
	virtual ~BulkSystem();	
	
	pthread_t threadNetID_;
	static void *ThreadNet(void *threadArgv);		// 网络线程

	pthread_t threadWorkID_;						
	static void *ThreadWork(void *threadArgv);		// 从sensor读电平，从网络读取提枪状态
	bool mainloop_;									// 工作标记
	bool upStateFlag_;								// 向服务器平台上报自己参数
	time_t heartBeatTime_;							// 上次心跳时
};

bool StartBulkSystem();
bool PushBulkEvent(const std::string &videoFilePath);

bool StopBulkSystem();



}


#endif

