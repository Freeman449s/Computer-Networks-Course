#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
using namespace std;

enum RequestType { GET, POST };

//在字符串末尾追加整数
void appendInt(string& str, const int i);

//组装HTTP包
void constructPkt(const string& const status, const string& const contentType, fstream& const fs, char* sendBuf);

//将文件内容复制进缓冲区
int copyFile(fstream& const fs, char* const buffer);


//解析相对路径，开头应有斜杠
string parseFilePath(const char* pkt, const int length);

//解析请求类型
RequestType parseRequestType(const char* pkt, const int length);

#endif