#include "tool.h"
#include "bulk_common.h"
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
/* CRC 高位字节值表*/
static const uint8_t auchCRCHi[] = {
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,
0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,
0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,
0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40
} ;
/* CRC低位字节值表*/
static const uint8_t auchCRCLo[] = {
0x00,0xC0,0xC1,0x01,0xC3,0x03,0x02,0xC2,0xC6,0x06,
0x07,0xC7,0x05,0xC5,0xC4,0x04,0xCC,0x0C,0x0D,0xCD,
0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,
0x08,0xC8,0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,
0x1E,0xDE,0xDF,0x1F,0xDD,0x1D,0x1C,0xDC,0x14,0xD4,
0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,
0x11,0xD1,0xD0,0x10,0xF0,0x30,0x31,0xF1,0x33,0xF3,
0xF2,0x32,0x36,0xF6,0xF7,0x37,0xF5,0x35,0x34,0xF4,
0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,
0x3B,0xFB,0x39,0xF9,0xF8,0x38,0x28,0xE8,0xE9,0x29,
0xEB,0x2B,0x2A,0xEA,0xEE,0x2E,0x2F,0xEF,0x2D,0xED,
0xEC,0x2C,0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,
0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,0xA0,0x60,
0x61,0xA1,0x63,0xA3,0xA2,0x62,0x66,0xA6,0xA7,0x67,
0xA5,0x65,0x64,0xA4,0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,
0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,
0x78,0xB8,0xB9,0x79,0xBB,0x7B,0x7A,0xBA,0xBE,0x7E,
0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,0xB4,0x74,0x75,0xB5,
0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,
0x70,0xB0,0x50,0x90,0x91,0x51,0x93,0x53,0x52,0x92,
0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9C,0x5C,
0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,
0x99,0x59,0x58,0x98,0x88,0x48,0x49,0x89,0x4B,0x8B,
0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,
0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,
0x43,0x83,0x41,0x81,0x80,0x40
} ;


uint16_t Tool::Crc16(uint8_t *buffer, int bufferLen)
{
	uint8_t *puchMsg = buffer; 				/* 要进行CRC校验的消息*/
	uint16_t usDataLen = (uint16_t) bufferLen; /* 消息中字节数*/
	
	uint8_t uchCRCHi = 0xFF ; /* 高CRC字节初始化*/
	uint8_t uchCRCLo = 0xFF ; /* 低CRC 字节初始化*/
	int uIndex ; 	   /* CRC循环中的索引*/
	while (usDataLen--)    /* 传输消息缓冲区*/
	{
		uIndex = uchCRCHi ^ *puchMsg++ ; /* 计算CRC */
		uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
		uchCRCLo = auchCRCLo[uIndex] ;
	}
	return ((uchCRCHi << 8) | uchCRCLo);
}


bool Tool::DisplayBufferBin(const char *msg, const uint8_t *buffer, int bufferLen)
{
	if ((buffer == NULL) || (bufferLen <= 0))
		return false;
		
	printf("%s: ", msg);
	for (int i = 0; i < bufferLen; ++i)
	{
		printf("0x%x ", buffer[i]);
	}
	printf("\n");
	return true;
}

void Tool::MsSleep(int msSec)
{
    struct timespec delay;
    struct timespec rem;
    int             n = -1;
    
    delay.tv_sec    = msSec/1000;
    delay.tv_nsec   = (msSec - delay.tv_sec*1000)*1000*1000;
    if (msSec == 0)
    {
        delay.tv_nsec  = 1;
    }
    do
    {
        n     = nanosleep(&delay,&rem);
        delay = rem;
    }
    while ((n < 0) && (errno == EINTR)); 

}

// 构建modbus协议，读取0x03功能的请求buffer
int Tool::MakeModbusSendBuffer(uint8_t addr, uint8_t funcode, uint16_t startRegAddr, uint16_t regCount, uint8_t *sendBuffer)
{
	sendBuffer[0] = addr;
	sendBuffer[1] = funcode;
	uint16_t nStartAddr = htons(startRegAddr);
	memcpy(sendBuffer+2, &nStartAddr, 2);
	uint16_t nRegisterCount = htons(regCount);
	memcpy(sendBuffer+4, &nRegisterCount, 2);
	uint16_t crc16 = Tool::Crc16(sendBuffer, 6);
	uint16_t nCrc16 = htons(crc16);
	memcpy(sendBuffer+6, &nCrc16, 2);

	const int thisLen = 8;		// 这种modbus固定是8字节
	return thisLen;
}

// 分隔字符串
std::vector<std::string> Tool::SplitString(const std::string &str, const char delimiter)
{
	std::vector<std::string> splited;
	std::string s(str);
	size_t pos;

	while ((pos = s.find(delimiter)) != std::string::npos) 
	{
		std::string sec = s.substr(0, pos);
		if (!sec.empty()) 
		{
			splited.push_back(s.substr(0, pos));
		}
		s = s.substr(pos + 1);
	}

	splited.push_back(s);
	return splited;
}

// B900原始数据转换为实时数值
double Tool::ConvertRealdataForB900(uint16_t origData)
{
	uint16_t tickData = origData & 0xc000;
	origData = origData & 0x3fff;

	int st = tickData >> 14;
	st = st & 0x0003;

	double ret = 0.0;
	switch (st)
	{
		case 0:
			ret = origData;
		break;
		case 1:
			ret = (double)origData/10.0;
		break;
		case 2:
			ret = (double)origData/100.0;
		break;
		case 3:
			ret = (double)origData/1000.0;
		break;
		default:
			printf("error, Tool::ConvertRealdataForB900, st failed\n");
			ret = -1.0;
		break;
	}
	return ret;
}

// 检查文件是否存在
bool Tool::AssertFile(std::string &filename)
{
	if (access(filename.c_str(), F_OK) == 0)
		return true;
	else
		return false;
}

uint32_t Tool::MakeMsgID()
{
	static ThreadMutex msgidTheradMutex;
	ThreadMutexGuard guard(&msgidTheradMutex);
	static uint32_t msgid = 0;
	msgid++;
	return msgid;
}


// 取程序所在目录，不含执行文件名
bool Tool::GetSelfPath(std::string& selfPath)
{
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	int ret = readlink("/proc/self/exe", buf, sizeof(buf));
	if (ret == -1)
		return false;

	for (int i = ret; i >=0; --i)
	{
    	if (buf[i] == '/')
    	{
        	buf[i+1] = '\0';
        	break;
    	}
	}
	selfPath = buf;
	return true; 
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 取本地ip
bool Tool::GetLocalIP(const std::string &eth, std::string &ip)
{
	if (eth.empty())
	{
		printf("ERROR, Tool::GetLocalIP, eth is empty\n");
		return false;
	}
	else
	{		
		int sock_get_ip;
		if ((sock_get_ip = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
		{  
			 printf("ERROR Tool::GetLocalIP, socket create failse...GetLocalIp!/n");  
			 return false;  
		}  

		struct	 ifreq ifr_ip;
		memset(&ifr_ip, 0, sizeof(ifr_ip)); 	
		strncpy(ifr_ip.ifr_name, eth.c_str(), sizeof(ifr_ip.ifr_name) - 1);	   
		   
		if (ioctl(sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 ) 	
		{	  
			 return false; 	
		}		
		struct sockaddr_in *sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;
		//char localAddr[sizeof(struct sockaddr_in)*2];
		//memset(localAddr, 0, sizeof(localAddr));
		//memcpy(pLocalAddr, sin, sizeof(struct sockaddr_in));
		char ipaddr[50]; 	  
		strcpy(ipaddr,inet_ntoa(sin->sin_addr));		 
		ip = ipaddr;  
		printf("local ip:%s\n", ipaddr);	  
		close( sock_get_ip );  

		return true;
	}
}


void Tool::GetFormatTime(std::string &formatTime)
{
	time_t timestamp = time(NULL);
	struct tm *tm_timestamp = localtime(&timestamp);
	char currentTime[128];
	memset(currentTime, 0, sizeof(currentTime));
	strftime(currentTime, 32, "%Y-%m-%d %H:%M:%S", tm_timestamp);
	formatTime = currentTime;

    return;
}


void Tool::getCurrentTime(std::string &currentTime)
{
	struct timeval tNowTimeval;
	gettimeofday(&tNowTimeval, 0);

	time_t wTime;
	struct tm *pNowTime;

	time(&wTime);
	pNowTime = localtime(&wTime);

	char timeBuffer[128];
	memset(timeBuffer, 0, sizeof(timeBuffer));
	sprintf(timeBuffer, "%4d-%02d-%02d %02d:%02d:%02d.%03d", 
			pNowTime->tm_year + 1900, 
			pNowTime->tm_mon + 1,
			pNowTime->tm_mday,
			pNowTime->tm_hour,
			pNowTime->tm_min,
			pNowTime->tm_sec,
			(int)tNowTimeval.tv_usec/1000);

	currentTime = timeBuffer;
	return;
}


bool Tool::GetMacAddr(const std::string &eth, std::string &mac)
{	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if( sockfd < 0)
	{
		perror("ERROR Tool::GetMacAddr, create socket fail.\n");
		return false;
	}

	struct ifreq tmp;	
	memset(&tmp,0,sizeof(struct ifreq));

	// 不同的linux配置、使用的网口可能不一样，通常为eth0
	strncpy(tmp.ifr_name, eth.c_str(), sizeof(tmp.ifr_name)-1);
	int ioctlRet = ioctl(sockfd,SIOCGIFHWADDR,&tmp);
	close(sockfd);
	
	if( (ioctlRet) < 0 )
	{
		printf("ERROR Tool::GetMacAddr, mac ioctl error.\n");
		return false;
	}

	const int MACADDRSTRSIZE = 64;
	char macBuffer[MACADDRSTRSIZE];
	memset(macBuffer, 0, sizeof(macBuffer));
	sprintf(macBuffer, "%02x%02x%02x%02x%02x%02x",
				(uint8_t)tmp.ifr_hwaddr.sa_data[0],
				(uint8_t)tmp.ifr_hwaddr.sa_data[1],
				(uint8_t)tmp.ifr_hwaddr.sa_data[2],
				(uint8_t)tmp.ifr_hwaddr.sa_data[3],
				(uint8_t)tmp.ifr_hwaddr.sa_data[4],
				(uint8_t)tmp.ifr_hwaddr.sa_data[5]);

	mac = macBuffer;
	printf("Tool::GetMacAddr SUCCESS, local mac is: %s.\n", macBuffer);
	return true;
}


// 重定向输出流
void Tool::ResetStdout()
{
	string reoutFile;
	Tool::GetSelfPath(reoutFile);
	reoutFile += "ba_stdout.conf";

	FILE *fConf = fopen(reoutFile.c_str(), "r");
	if (fConf)
	{
		char stdoutFile[256];
		memset(stdoutFile, 0, sizeof(stdoutFile));
		fscanf(fConf, "%s", stdoutFile);
		freopen(stdoutFile,"w",stdout);
		fclose(fConf);
		fConf = NULL;
	}
	else
	{
		printf("ERROR, %s open failed\n", reoutFile.c_str());
	}
}


// 输出fileorpath的绝对路径
std::string Tool::FixFilePath(std::string fileorpath)
{
	if (fileorpath.empty())
		return fileorpath;
		
	if (fileorpath[0] != '/')
	{
		std::string fixfilePath;
		Tool::GetSelfPath(fixfilePath);
		fixfilePath += fileorpath;

		return fixfilePath;
	}
	else
	{
		return fileorpath;
	}
}


#if 0
使用更灵活的SaveBufferToFile
string Tool::SaveJpgFile(std::string &eventID, unsigned char *buffer, int bufferLen)
{
	std::string filePath;
	Tool::GetSelfPath(filePath);
	filePath += "alarmpic/";
	filePath += eventID;
	filePath += ".jpg";

	if (buffer)
	{
		FILE *f = fopen(filePath.c_str(), "w");
		if (f)
		{
			fwrite(buffer, bufferLen, 1, f);
			fclose(f);
			f = NULL;
		}
		else
		{
			printf("ERROR, %s open failed\n", filePath.c_str());
			filePath.clear();
		}
		return filePath;
	}
	else
	{
		printf("Tool::SavePic, buffer is NULL\n");
		filePath.clear();
		return filePath;
	}
}
#endif


// 保存picBuffer到pic文件中，文件只有部分内容是jpg文件 
string Tool::SaveBufferToFile(std::string &uuid, unsigned char *buffer, int bufferLen, const char *suffix)
{
	std::string filePath;
	Tool::GetSelfPath(filePath);
	filePath += "alarmpic/";
	filePath += uuid;
	filePath += suffix;//".pic";

	if (buffer)
	{
		FILE *f = fopen(filePath.c_str(), "w");
		if (f)
		{
			fwrite(buffer, bufferLen, 1, f);
			fclose(f);
			f = NULL;
		}
		else
		{
			printf("ERROR, %s open failed\n", filePath.c_str());
			filePath.clear();
		}
		return filePath;
	}
	else
	{
		printf("Tool::SavePicBuffer2File, buffer is NULL\n");
		filePath.clear();
		return filePath;
	}
}



unsigned char *Tool::ReadBufferFromFile(std::string &uuid, const char *suffix, int &bufferLen)
{
	std::string filePath;
	Tool::GetSelfPath(filePath);
	filePath += "alarmpic/";
	filePath += uuid;
	filePath += suffix;//".pic";

	long flen = Tool::GetFileLen(filePath);
	if (flen <= 0)
	{
		printf("ERROR, ReadPicBuffer, file: %s, len: %ld\n", filePath.c_str(), flen);
		bufferLen = 0;
		return nullptr;
	}

	printf("ReadPicBuffer, file: %s, len: %ld\n", filePath.c_str(), flen);

	FILE *f = fopen(filePath.c_str(), "r");
	if (f)
	{
		unsigned char *pBuffer = new unsigned char[flen];
		if (pBuffer)
		{
			fread(pBuffer, 1, flen, f);
		}
		else
		{
			printf("ERROR, ReadPicBuffer, new pBuffer is nullptr\n");
			flen = 0;
		}
		fclose(f);
		f = nullptr;
		unlink(filePath.c_str());
		
		bufferLen = flen;
		return pBuffer;
	}
	else
	{
		printf("ERROR, ReadPicBuffer, file: %s, open fail\n", filePath.c_str());
		bufferLen = 0;
		return nullptr;
	}
}



long Tool::GetFileLen(std::string &filename)
{
	FILE *f = fopen(filename.c_str(), "r");
 	if (f == nullptr)
 	{
 		printf("ERROR, Tool::GetFileLen:%s , open failed\n", filename.c_str());
 		return -1;
 	}
 	
	fseek(f, 0, SEEK_END);
	long length  = ftell(f);
	fclose(f);
	f = nullptr;
	return length;
}

#if 0
unsigned char * Tool::ReadDavFileToBuffer(std::string &uuid, int &bufferLen)
{
	std::string filePath = "/tmp/";
	filePath += uuid;
	filePath += ".dav";

	long flen = Tool::GetFileLen(filePath);
	if (flen <= 0)
	{
		printf("ERROR, Tool::ReadDavFileToBuffer, file: %s, len: %ld\n", filePath.c_str(), flen);
		bufferLen = 0;
		return nullptr;
	}

	printf("Tool::ReadDavFileToBuffer, file: %s, len: %ld\n", filePath.c_str(), flen);

	FILE *f = fopen(filePath.c_str(), "r");
	if (f)
	{
		unsigned char *pBuffer = new unsigned char[flen + sizeof(bulk::NetHead) + 32];
		if (pBuffer)
		{
			memset(pBuffer, 0, sizeof(pBuffer));
			fread(pBuffer + sizeof(bulk::NetHead) + 32, 1, flen, f);
		}
		else
		{
			printf("ERROR, ReadPicBuffer, new pBuffer is nullptr\n");
			flen = 0;
		}
		fclose(f);
		f = nullptr;
		unlink(filePath.c_str());
		
		bufferLen = flen;
		return pBuffer;
	}
	else
	{
		printf("ERROR, ReadPicBuffer, file: %s, open fail\n", filePath.c_str());
		bufferLen = 0;
		return nullptr;
	}
}
#endif

unsigned char * Tool::ReadFileToBuffer(std::string &phototpath, int &bufferLen)
{
	long flen = Tool::GetFileLen(phototpath);
	if (flen <= 0)
	{
		printf("ERROR, Tool::ReadDavFileToBuffer, file: %s, len: %ld\n", phototpath.c_str(), flen);
		bufferLen = 0;
		return nullptr;
	}

	printf("Tool::ReadDavFileToBuffer, file: %s, len: %ld\n", phototpath.c_str(), flen);

	FILE *f = fopen(phototpath.c_str(), "r");
	if (f)
	{
		unsigned char *pBuffer = new unsigned char[flen + sizeof(bulk3::NetHead) + 32];
		if (pBuffer)
		{
			memset(pBuffer, 0, sizeof(pBuffer));
			fread(pBuffer + sizeof(bulk3::NetHead) + 32, 1, flen, f);
		}
		else
		{
			printf("ERROR, ReadPicBuffer, new pBuffer is nullptr\n");
			flen = 0;
		}
		fclose(f);
		f = nullptr;
		//unlink(phototpath.c_str());
		
		bufferLen = flen;
		return pBuffer;
	}
	else
	{
		printf("ERROR, ReadPicBuffer, file: %s, open fail\n", phototpath.c_str());
		bufferLen = 0;
		return nullptr;
	}

}


// 调用方自行保证buffer有效，并且可靠（通常在之前调用 GetFileLen()确认文件长度） 
bool Tool::ReadFile(const std::string &filepath, uint8_t *readBuffer, int readlen)
{
	if (readlen <= 0)
	{
		printf("ERROR, Tool::ReadFile, file: %s, len: %d\n", filepath.c_str(), readlen);
		return false;
	}

	if (readBuffer == nullptr)
	{
		printf("ERROR, Tool::ReadFile, buffer is nullptr\n");
		return false;
	}

	FILE *f = fopen(filepath.c_str(), "r");
	if (f)
	{
		memset(readBuffer, 0, readlen);
		fread(readBuffer, 1, readlen, f);
		fclose(f);
		f = nullptr;
		return true;
	}
	else
	{
		printf("ERROR, Tool::ReadFile, file: %s, open fail\n", filepath.c_str());
		return false;
	}
}

// 调用方自行保证buffer有效，并且可靠（通常在之前调用 GetFileLen()确认文件长度） 
bool Tool::TestFileIsNull(const std::string &filepath)
{
	char buf[1024];

	FILE *f = fopen(filepath.c_str(), "r");
	if (f)
	{
		fread(buf, 1, 100, f);
		fclose(f);
		f = nullptr;
		if (*buf == '\0')
		{
			return false;
		}
		return true;
	}
	else
	{
		printf("ERROR, Tool::ReadFile, file: %s, open fail\n", filepath.c_str());
		return false;
	}
}


// 生成32个字节的UUID 
std::string Tool::MakeUUID()
{
	uuid_t uu;
	uuid_generate(uu);

	char buffer[64];
	memset(buffer, 0, sizeof(buffer));
	
	for (int i = 0; i < 16; i++)
	{
		sprintf(buffer+2*i, "%02x", uu[i]);
	}

	#if 0
	printf("uuid: ");
	for (int i = 0; i < 32; i++)
	{
		printf("[%02x]", buffer[i]);
	}
	printf("\n");
	#endif

	std::string ret = buffer;
	return ret;
}


// 重定向输出流
void Tool::ReOut()
{
	string reoutFile;
	Tool::GetSelfPath(reoutFile);
	reoutFile += "ba_stdout.conf";

	FILE *fConf = fopen(reoutFile.c_str(), "r");
	if (fConf)
	{
		char stdoutFile[256];
		memset(stdoutFile, 0, sizeof(stdoutFile));
		fscanf(fConf, "%s", stdoutFile);
		freopen(stdoutFile,"w",stdout);
		fclose(fConf);
		fConf = NULL;
	}
	else
	{
		printf("ERROR, %s open failed\n", reoutFile.c_str());
	}
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Runinfo gRuninfo;




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 线程互斥
ThreadMutex::ThreadMutex()
{
	pthread_mutex_init(&this->mMutex, NULL);
}

ThreadMutex::~ThreadMutex()
{
	pthread_mutex_destroy(&this->mMutex);
}

void ThreadMutex::Lock()
{
	pthread_mutex_lock(&this->mMutex);
}

void ThreadMutex::UnLock()
{
	pthread_mutex_unlock(&this->mMutex);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 线程互斥保护
ThreadMutexGuard::ThreadMutexGuard(ThreadMutex *pMutex)
{
	pMutex->Lock();
	this->mpMutex = pMutex;
}

ThreadMutexGuard::~ThreadMutexGuard()
{
	this->mpMutex->UnLock();
}





