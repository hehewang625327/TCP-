#ifndef _H_BULK_CONFIG_20200722_
#define _H_BULK_CONFIG_20200722_

#include <string>
#include <vector>
const std::string BULK_VERSION = "1.0.0";

namespace bulk3
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 数据库配置
class DatabaseConfig
{
public:
	std::string dbfile_;				// sqlite3文件名
	std::string user_;					// 用户名(暂时未用)
	std::string passwd_;				// 口令(暂时未用)
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 网络通讯配置
class NetConfig
{
public:
	std::string ip_;					// 服务器ip
	int port_;							// 服务器端口
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 散油宝系统配置
class BulkSystemConfig
{
public:
	BulkSystemConfig();
	BulkSystemConfig(const std::string &confFilename);
	virtual ~BulkSystemConfig();

	bool ReadConfig(const std::string &confFilename);
	bool Print();
	bool IsOK();						// 获取配置成功标志
	
public:
	int stationID_ = -1;				// 油站ID
	int boardID_ = -1;					// 算法机ID
	std::string boardModel_;			// 算法机型号 esm6800, rk3288
	std::string softVersion_;			// 软件版本号 编译常量 BAVERSION
	std::string configFile_;			// 配置文件名
	bool isok_ = false;					// 配置读取成功标志
	DatabaseConfig dbConfig_;			// 数据库配置
	NetConfig netConfig_;				// 网络配置
};


}


#endif

