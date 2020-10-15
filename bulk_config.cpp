#include "bulk_config.h"
#include "tool.h"
#include "3rd/tinyxml2.h"

using namespace tinyxml2;
using std::string;

namespace bulk3
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 散油宝系统配置
BulkSystemConfig::BulkSystemConfig()
{
	
}


// 读取散油宝系统配置
BulkSystemConfig::BulkSystemConfig(const std::string &configFileName)
{
	this->ReadConfig(configFileName);
}

// 
BulkSystemConfig::~BulkSystemConfig()
{
	this->isok_ = false;
	this->configFile_.clear();
}


// 
void ToolXmlGetString(XMLElement *pNode, const std::string &key, const std::string &defaultValue, std::string &value)
{
	XMLElement *pele = pNode->FirstChildElement(key.c_str());
	if (pele)
	{
		const char *p = pele->GetText();
		value = p?p:defaultValue;
	}
	else
	{
		value = defaultValue;
	}
}


//
void ToolXmlGetInt(XMLElement *pNode, const std::string &key, const int defaultValue, int &value)
{
	XMLElement *pele = pNode->FirstChildElement(key.c_str());
	if (pele)
	{
		value = pele->IntText(defaultValue);
	}
	else
	{
		printf("ERROR, key: %s, not found\n", key.c_str());
		value = defaultValue;
	}
}


// 读配置文件
bool BulkSystemConfig::ReadConfig(const std::string &configFileName)
{
	this->configFile_ = configFileName;

	XMLDocument confDoc;
	if (confDoc.LoadFile(configFileName.c_str()) != XML_SUCCESS)
	{
		printf("BulkSystemConfig, %s load failed: %s\n", configFileName.c_str(), confDoc.ErrorStr());
		this->isok_ = false;
		return false;
	}

	XMLElement *bulkRoot = confDoc.RootElement();
	if (bulkRoot)
	{
		ToolXmlGetInt(bulkRoot, "station_id", -1, this->stationID_);
		ToolXmlGetInt(bulkRoot, "board_id", -1, this->boardID_);
		ToolXmlGetString(bulkRoot, "board_model", "x86", this->boardModel_);
		string modelString;
		ToolXmlGetString(bulkRoot, "model", "", modelString);

		this->softVersion_ = BULK_VERSION;
		// 数据库配置
		XMLElement *pdbconfig = bulkRoot->FirstChildElement("database");
		if (pdbconfig)
		{
			ToolXmlGetString(pdbconfig, "dbfile", "bulk3.db", this->dbConfig_.dbfile_);

			if (this->dbConfig_.dbfile_[0] != '/')
			{
				std::string dbfilePath;
				Tool::GetSelfPath(dbfilePath);
				dbfilePath += this->dbConfig_.dbfile_;

				this->dbConfig_.dbfile_ = dbfilePath;
			}

			ToolXmlGetString(pdbconfig, "user", "root", this->dbConfig_.user_);
			ToolXmlGetString(pdbconfig, "passwd", "", this->dbConfig_.passwd_);
		}

		// 通讯机网络配置
		XMLElement *pNetConfig = bulkRoot->FirstChildElement("net");
		if (pNetConfig)
		{				
			ToolXmlGetString(pNetConfig, "ip", "", this->netConfig_.ip_);
			ToolXmlGetInt(pNetConfig, "port", 0, this->netConfig_.port_);
		}
		else
		{
			printf("ERROR, BulkSystemConfig::ReadConfig, net not found\n");
		}
	
		this->isok_ = true;
	}
	else
	{
		printf("bulk root is NULL\n");
		this->isok_ = false;
	}
	
	printf("bulk system config, file: %s, read: %s\n", configFileName.c_str(), this->isok_ ? "true" : "false");
	return true;
}


// 显示配置
bool BulkSystemConfig::Print()
{
	printf("\n--------------------------------------------------------\n");
	printf("Bulk system config, %s :\n", this->configFile_.c_str());

	printf("database:\n");
	printf("\t dbfile: %s, user: %s, passwd: %s\n", this->dbConfig_.dbfile_.c_str(), this->dbConfig_.user_.c_str(), this->dbConfig_.passwd_.c_str());

	printf("net:\n");
	printf("\t ip: %s, port: %d\n", this->netConfig_.ip_.c_str(), this->netConfig_.port_);

	printf("BulkSystemConfig, end.\n");
	printf("\n--------------------------------------------------------\n");
	
	return true;
}


// 获取配置成功标志
bool BulkSystemConfig::IsOK()
{
	return this->isok_;
}




}

