#include "bulk_database.h"

#if 1
#include <set>

using std::string;
using std::vector;
using std::set;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace bulk3
{


Database::Database()
{
	this->dataCount_ = 0;
	this->db_ = NULL;
}


Database::~Database()
{
	this->Close();
}


bool Database::Init(const DatabaseConfig &config)
{
	this->config_ = config;
	bool ret = this->Open();
	printf("bulk::Database::Init, result: %s\n", ret ? "ok" : "failed");
	return true;
}


// 打开数据库
bool Database::Open()
{
	this->dataCount_ = 0;
	if (this->db_ != NULL)
	{
		printf("warning, Database::Open, db_ is not NULL, close db\n");
		this->Close();
	}

	bool createDBFlag = Tool::AssertFile(this->config_.dbfile_) ? false : true;
	if (createDBFlag)
		printf("ba::Database::Open, createDBFlag: %s\n", createDBFlag ?"true":"false");
	
	ThreadMutexGuard guard(&this->mutex_);
	int ret = sqlite3_open(this->config_.dbfile_.c_str(), &this->db_);
	if (ret != SQLITE_OK)
	{
		printf("ERROR, ba::Database::Open, %s open failed\n", this->config_.dbfile_.c_str());
	}

	if (Tool::AssertFile(this->config_.dbfile_)) 
	{
		this->EnableRW();
	}
	else
	{
		printf("ERROR, bulk::Database::Open, %s database file create failed\n", this->config_.dbfile_.c_str());
	}

	// 数据库是新建的，需要把数据库表建立起来
	if (createDBFlag)
	{
		this->CreateTable();
	}
	else
	{
		this->dataCount_ = this->SelectDataCount();
		printf("database ba count: %d\n", this->dataCount_);
	}

	if ( ret != SQLITE_OK)
	{
		printf("bulk::Database::Open(), %s, failed: %s\n", this->config_.dbfile_.c_str(), sqlite3_errmsg(this->db_));
		return false;
	}
	else
	{
		printf("bulk::Database::Open(), %s, ok\n", this->config_.dbfile_.c_str());
		return true;
	}
}


// 关闭数据库
bool Database::Close()
{
	ThreadMutexGuard guard(&this->mutex_);

	string dbfilename = this->config_.dbfile_;
	
	this->config_.dbfile_.clear();
	this->config_.user_.clear();
	this->config_.passwd_.clear();
	
	if (this->db_ == NULL)
	{
		printf("error, bulk::Database::Close, database file not openned\n");
		return false;
	}
	
	sqlite3_busy_timeout(this->db_, 30*1000); //最长等待30s
	int ret = sqlite3_close(this->db_);
	if ( ret == SQLITE_BUSY )
	{
		printf("error, bulk::Database::Close, close database error\n");
		return false;
	}
	
	printf("bulk::Database::Close, %s close success\n", dbfilename.c_str());
	this->db_ = NULL;
	return true;
}


// 让数据库文件可读可写
bool Database::EnableRW()
{	
	char cmd[1024] = {0};
	sprintf(cmd, "chmod 777 %s", this->config_.dbfile_.c_str());
	system(cmd);

	return true;
}


// 创建行为分析的数据库表
bool Database::CreateTable()
{
	std::string createTableSql;

	createTableSql.append("BEGIN TRANSACTION;");
	createTableSql.append("CREATE TABLE IF NOT EXISTS bulk_event ( "\
							"id integer PRIMARY KEY autoincrement,"\
							"type integer,"\
							"eventuuid TEXT,"\
							"jsonstring TEXT);");

	createTableSql.append("COMMIT TRANSACTION;");	

	char *pszErrMsg = NULL;
	sqlite3_busy_timeout(this->db_, 30 *1000);
	if (SQLITE_OK != sqlite3_exec(this->db_, createTableSql.c_str(), NULL, 0, &pszErrMsg))
	{
		printf("error, bulk::Database::CreateTable, localdb, create database table failed！%s\n", pszErrMsg);
		sqlite3_free(pszErrMsg);
		return false;
	}
	else
	{
		printf("bulk::Database::CreateTable, database create tables success\n");
		return true;
	}
}


bool Database::Read(int readcnt, std::vector<BulkEvent> &events)
{
	ThreadMutexGuard guard(&this->mutex_);
	if (this->dataCount_ <= 0)
	{
		events.clear();
		return false;
	}
	
	set<int> getIDS;	// 读出来的ID 
	std::string readSql = "SELECT id, type, eventuuid, jsonstring FROM bulk_event;";
	sqlite3_busy_timeout(this->db_, 30*1000);
	sqlite3_stmt *stmt = NULL;
	int readCount = 0;
	if (SQLITE_OK == sqlite3_prepare_v2(this->db_, readSql.c_str(), -1, &stmt, NULL))
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			int id = sqlite3_column_int(stmt, 0);
			int type = sqlite3_column_int(stmt, 1);
			std::string uuid = (const char *)sqlite3_column_text(stmt, 2);
			std::string jsonString = (const char *)sqlite3_column_text(stmt, 3);

			BulkEvent ue;
			ue.type_ = type;
			ue.uuid_ = uuid;
			ue.value_ = jsonString;

			if (ue.value_.empty())
			{
				printf("ERROR, Database::Read, PHT_EVENT, ue.json:%s\n", ue.value_.c_str());

			}

			events.push_back(ue);
			getIDS.insert(id);

			char deleteSql[256];
			memset(deleteSql, 0, sizeof(deleteSql));
			sprintf(deleteSql, "DELETE from bulk_event where id=%d", id);
			printf("%s\n", deleteSql);
	
			char *err_msg = NULL;
			sqlite3_busy_timeout(this->db_, 30 *1000);
			bool ret = false;
			if (SQLITE_OK != sqlite3_exec(this->db_, deleteSql, NULL, 0, &err_msg))
			{
				printf("%s\n database failed: %s\n", deleteSql, err_msg);
				sqlite3_free(err_msg);
				ret = false;
			}
			else
			{
				printf("%s\n database ok\n", deleteSql);
				ret = true;
			}
		}
	}
	else
	{
		printf("ERROR, %s\n", readSql.c_str());
	}
	sqlite3_finalize(stmt);

	this->dataCount_ -= (int)events.size();
	return true;
}


// 写实时数据
bool Database::Write(BulkEvent &event)
{
	std::string insertSql = event.MakeSaveSql();
	
	ThreadMutexGuard guard(&this->mutex_);
	
	char *err_msg = NULL;
	sqlite3_busy_timeout(this->db_, 30 *1000);
	bool ret = false;
	if (SQLITE_OK != sqlite3_exec(this->db_, insertSql.c_str(), NULL, 0, &err_msg))
	{
		printf("%s\n database failed: %s\n", insertSql.c_str(), err_msg);
		sqlite3_free(err_msg);
		ret = false;
	}
	else
	{
		this->dataCount_++;
		printf("%s\n database ok, ba data count: %d\n", insertSql.c_str(), this->dataCount_);
		ret = true;
	}	
	return ret;
}


// 取库中缓存的行为数据
int Database::GetDataCount()
{
	return this->dataCount_;
}


// 通过select语句查询数据库中实际缓存的数据条数
int Database::SelectDataCount()
{
	string countSql = "SELECT COUNT(id) FROM bulk_event;";
	sqlite3_busy_timeout(this->db_, 30*1000);

	sqlite3_stmt *stmt = NULL;
	int count = 0;
	if (SQLITE_OK == sqlite3_prepare_v2(this->db_, countSql.c_str(), -1, &stmt, NULL))
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			count = sqlite3_column_int(stmt, 0);
			printf("database select real data count: %d\n", count);
			this->dataCount_ = count;
		}
	}
	else
	{
		printf("error, %s ,database, exec failed\n", countSql.c_str());
	}

	sqlite3_finalize(stmt);
	return count;
}


}

#endif




