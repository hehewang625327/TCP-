
#include"bulk3.h"
using std::string;
using std::vector;
using std::map;

namespace bulk3
{

BulkSystem &BulkSystem::GetInst()
{
	static BulkSystem bulk;
	return bulk;
}

BulkSystem::BulkSystem()
{
	this->mainloop_ = false;
}


BulkSystem::~BulkSystem()
{
	
}

bool BulkSystem::Init(const std::string &configFile)
{
	BulkSystemConfig bulkSysConf(configFile);
	bulkSysConf.Print();
	this->stationID_ = bulkSysConf.stationID_;
	this->boardID_ = bulkSysConf.boardID_;
	this->boardModel_ = bulkSysConf.boardModel_;
	this->softVersion_ = BULK_VERSION;
	this->heartBeatTime_ = time(NULL);
	this->upStateFlag_ = true;
	bool netInitResult = this->net_.Init(bulkSysConf.netConfig_);
	bool dbInitResult = this->database_.Init(bulkSysConf.dbConfig_);
	bool result = (netInitResult && dbInitResult);
	if (result)
	{
		printf("***BulkSystem::Init ok (db ok, net ok, camera ok)***\n");
	}
	else
	{
		printf("ERROR, BulkSystem::Init, db:%d, tcp;%d\n",dbInitResult,netInitResult);
	}

	return result;
}

// 生成向平台上报自身的配置参数
bool BulkSystem::MakeUpState(BulkEvent &selfState)
{	
	neb::CJsonObject oJson;

	oJson.Add("station_id", this->stationID_);
	oJson.Add("board_index", this->boardID_);
	oJson.Add("board_model", this->boardModel_);
	oJson.Add("software_version", this->softVersion_);
	oJson.Add("camera_num", 1);
	neb::CJsonObject cameraArr;
	neb::CJsonObject cameraItem;

	cameraItem.Add("camera_index",1);
	cameraItem.Add("camera_area", "油桶识别");
	cameraItem.Add("camera_msg", "油桶识别区域");
	cameraItem.Add("camera_model", "BULK3");

	// camera_state
	cameraItem.Add("camera_state", 1);
	cameraItem.Add("open_alg","Barrel identification");

	cameraArr.Add(cameraItem);
	oJson.Add("camera", cameraArr);
	
	selfState.type_ = NetHead::PHT_State;
	selfState.value_ = oJson.ToString();
	selfState.uuid_.clear();
	selfState.time_ = time(NULL);
	
	return true;
}

bool BulkSystem::SendHeartBeat()
{

	//上电发送状态
	if (this->upStateFlag_)
	{
		BulkEvent upstate;
		this->MakeUpState(upstate);
		BulkSystem::GetInst().net_.InsertSendList(upstate);
		this->upStateFlag_ = false;
	}

	// 心跳
	time_t nowTime = time(NULL);
	if (abs(nowTime - this->heartBeatTime_) > 60)
	{
		BulkEvent hbEvent;
		this->MakeUpHeartBeat(hbEvent);
		BulkSystem::GetInst().net_.InsertSendList(hbEvent);
		this->heartBeatTime_ = nowTime;
	}
	return true;

}

string BulkSystem::MakeEventJson(const string &eventUUID)
{
	string json;
	neb::CJsonObject oJson;
	
	oJson.Add("station_id", this->stationID_);
	oJson.Add("msg_type", "RT");

	neb::CJsonObject eventJson;
	eventJson.Add("event_id", eventUUID);
	eventJson.Add("event_type", "BULKOILBAO");
	eventJson.Add("event_state", "once");
	eventJson.Add("board_index", this->boardID_);
	eventJson.Add("board_model", this->boardModel_);
	eventJson.Add("software_version", this->softVersion_);
	eventJson.Add("camera_id", 1);
	eventJson.Add("camera_area", "油桶识别");
	eventJson.Add("camera_msg", "油桶识别区域");
	
	char nowTimestring[255];
	struct tm *ptr;
	time_t nowtime = time(NULL);
	
	ptr = localtime(&nowtime);
	memset(nowTimestring, 0, sizeof(nowTimestring));
	sprintf(nowTimestring, "%4d-%02d-%02d %02d:%02d:%02d", ptr->tm_year+1900, ptr->tm_mon+1, ptr->tm_mday, 
															ptr->tm_hour, ptr->tm_min,ptr->tm_sec);
	eventJson.Add("event_starttime",nowTimestring);
	eventJson.Add("event_currenttime",nowTimestring);

	char eventdesc[256];
	memset(eventdesc, 0, sizeof(eventdesc));
	sprintf(eventdesc, "Barrel identification:%d", 111);
	eventJson.Add("event_describe", eventdesc);

	oJson.Add("event", eventJson);

	json = oJson.ToString();
	printf("BulkSystem::MakeEventJson: %s\n", json.c_str());

	return json;
}

// 生成向平台上报心跳数据
bool BulkSystem::MakeUpHeartBeat(BulkEvent &hb)
{
	neb::CJsonObject oJson;

	oJson.Add("station_id", this->stationID_);
	oJson.Add("board_index", this->boardID_);
	oJson.Add("software_version", this->softVersion_);
	oJson.Add("camera_num", 1);
	
	hb.type_ = NetHead::PHT_Beat;
	hb.value_ = oJson.ToString();
	hb.uuid_.clear();
	hb.time_ = time(NULL);
	
	return true;
}

// 开始工作
bool BulkSystem::StartWork()
{
	this->mainloop_ = true;
	bool workThreadRet = (pthread_create(&this->threadWorkID_, NULL, BulkSystem::ThreadWork, NULL) == 0);
	bool netThreadRet = (pthread_create(&this->threadNetID_, NULL, BulkSystem::ThreadNet, NULL) == 0);

	bool result = ( netThreadRet && workThreadRet);
	if (result)
	{
		printf("** BulkSystem::StartWork, ok. (readCardThreadRet and ThreadNet and ThreadWork ok)\n");
	}
	else
	{
		printf("ERROR, BASystem::StartWork, ThreadNet:%d, ThreadDownFile:%d\n",  netThreadRet, workThreadRet);
	}
	return result;
}


// 网络线程
void *BulkSystem::ThreadNet(void *threadArgv)
{
	while (BulkSystem::GetInst().GetMainloop())
	{
		BulkSystem::GetInst().net_.Process();

		vector<BulkEvent> sendFailedEvents;
		BulkSystem::GetInst().net_.GetSendFailedEvents(sendFailedEvents);
		if (!sendFailedEvents.empty())
		{
			for (auto ite = sendFailedEvents.begin(); ite != sendFailedEvents.end(); ++ite)
			{
				BulkSystem::GetInst().database_.Write(*ite);
			}
			sendFailedEvents.clear();
		}
		
		Tool::MsSleep(10);
	}
	return NULL;
}


// 工作线程
void *BulkSystem::ThreadWork(void * threadArgv)
{
	while (BulkSystem::GetInst().GetMainloop())
	{
		BulkSystem::GetInst().SendHeartBeat();
		
		
		// 网络恢复，从数据库中获取，再发
		// 如果当前网络畅通，又有缓存数据，则3秒发一个，发送太快会有问题
		static time_t lastdbsend = 0;
		time_t nowTime = time(NULL);
		
		if ((gRuninfo.mNetFlag) && (abs(nowTime - lastdbsend) > 1))
		{
			if (BulkSystem::GetInst().database_.GetDataCount() > 0)
			{
				vector<BulkEvent> events;
				BulkSystem::GetInst().database_.Read(1, events);
				for (auto ite = events.begin(); ite != events.end(); ++ite)
				{
					BulkSystem::GetInst().net_.InsertSendList(*ite);
				}
						
				events.clear();
				printf("BulkSystem::ThreadWork, database => net\n");
				lastdbsend = time(NULL);
			}
		}
		
		Tool::MsSleep(10);

	}

	return NULL;
}

// 行为分析系统停止工作
bool BulkSystem::StopWork()
{
	this->mainloop_ = false;
	return true;
}

bool BulkSystem::GetMainloop()
{
	return this->mainloop_;
}



bool BulkSystem::process(const std::string &videoFilePath)
{
	BulkEvent eventVideo;
	BulkEvent eventJson;
	string uuid = Tool::MakeUUID();
	time_t nowTime = time(NULL);

	// 事件event
	eventJson.type_ = NetHead::PHT_Event;
	eventJson.value_ = this->MakeEventJson(uuid);
	eventJson.uuid_ = uuid;
	eventJson.time_ = nowTime;
	BulkSystem::GetInst().net_.InsertSendList(eventJson);	

	eventVideo.type_ = NetHead::PHT_VIDEO;
	eventVideo.value_ = videoFilePath;
	eventVideo.uuid_ = uuid;
	eventVideo.time_ = nowTime;
	BulkSystem::GetInst().net_.InsertSendList(eventVideo);	
	
	return true;
}



bool StartBulkSystem()
{
	string bulkConfigFilePath = Tool::FixFilePath("bulk3.conf");
	printf("bulk config file path: %s\n", bulkConfigFilePath.c_str());
	BulkSystem::GetInst().Init(bulkConfigFilePath);
	return BulkSystem::GetInst().StartWork();
}

bool PushBulkEvent(const std::string &videoFilePath)
{
	BulkSystem::GetInst().process(videoFilePath);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// 停止行为分析系统
bool StopBulkSystem()
{
	BulkSystem::GetInst().StopWork();
	Tool::MsSleep(2000);
	return true;
}


}

