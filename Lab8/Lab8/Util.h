#ifndef __UTIL_H__
#define __UTIL_H__

#include <fstream>
#include <string>
#include <vector>
#include <WinSock2.h>
using namespace std;

enum class RequestType { GET, POST, OTHER };
enum class ContentType { HTML, IMAGE };
constexpr int BUFFER_SIZE = 2 * 1024;	//2MB

//在字符串末尾追加整数
void appendInt(string& str, const int i);

//组装HTTP包
int constructPkt(const string& const status, const string& const contentType, fstream& const fs, char* sendBuf);

//将文件内容复制进缓冲区
int copyFile(fstream& const fs, char* const buffer);

//统计文件行数
int countNLines(fstream& fs);

//解析账号和密码
vector<string> parseAccountAndPwd(const char* pkt, const int length);

//解析请求的文件类型
ContentType parseContentType(const char* pkt, const int length);

//解析相对路径，开头应有斜杠
string parseFilePath(const char* pkt, const int length);

//解析请求类型
RequestType parseRequestType(const char* pkt, const int length);

//向客户端发送文件
bool sendFile(const SOCKET& sServer, fstream& fs);

#endif