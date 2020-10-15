#ifndef _H_BULK_DATABASE_20200722_
#define _H_BULK_DATABASE_20200722_

/*
*/

#if 1

#include <string>
#include <vector>
#include <sqlite3.h>
#include "tool.h"
#include "bulk_common.h"
#include "bulk_config.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace bulk3
{


////////////////////////////////////////////////////////////////////////////////
// 散油宝数据库
class Database
{
public:
	Database();
	virtual ~Database();

	Database(const Database&) = delete;  				
	Database & operator = (const Database&) = delete; 

public:
	bool Init(const DatabaseConfig &config);
	bool Open();
	bool Close();
	bool EnableRW();										// 让数据库文件可读可写
	bool CreateTable();										// 创建数据库表
	bool Read(int readcnt, std::vector<BulkEvent> &events);	// 读实时数据
	bool Write(BulkEvent &event);							// 写实时数据
	int GetDataCount();										// 库中实时数据个数
	
private:
	int SelectDataCount();									// 通过select查询数据库的缓存数据条数
	int dataCount_;											// 数据库模块中缓存数据的个数
	
private:
	DatabaseConfig config_;
	sqlite3 *db_;
	ThreadMutex mutex_;
};


}
#endif

#endif


