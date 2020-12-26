#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "WinSock2.h"
using namespace std;

#pragma comment (lib, "WS2_32")
#pragma warning (disable:4996)

bool sendTime(SOCKET sServer, char* sendBuf);
bool sendHostName(SOCKET sServer, char* sendBuf);
bool sendClientList(SOCKET sServer, char* sendBuf, const map<int, struct sockaddr_in>& clientAddrs);
//通过返回值表示可能出现的错误：
//0：发送成功
//1：向接收者发送成功，向发送者反馈失败
//2：接收者未连接，向发送者反馈成功
//3：接收者未连接，向发送者反馈失败
//4：向接收者发送失败，向发送者反馈成功
//5：向接收者发送失败，向发送者反馈失败
int sendMsg(SOCKET sSender, const sockaddr_in& senderAddr,
	const char* const recvBuf, const map<int, struct sockaddr_in>& clientAddrs,
	const map<int, SOCKET>& clientSockets);
//统计C字符串的长度
int cStrLength(const char* str);
//用于sendMsg函数，针对不同的发送结果，打包含有不同信息的反馈数据包
void fillFeedbackPacket(char* buf, int result);

#endif