#include <iostream>
#include <map>
#include "process.h"
using namespace std;

constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_INT = 0x7fffffff;

bool sendTime(SOCKET sServer, char* sendBuf) {
	SYSTEMTIME time;
	//数据包内容：边界标记A+包大小（包括边界标记）B+响应类型A+数据A/B+边界标记A
	int packetLength = 1 + 4 + 1 + 2 * 6 + 1;
	int ret;
	char* sendPtr = sendBuf;
	GetLocalTime(&time);
	//处理头部信息
	sendBuf[0] = '$';
	sendBuf[packetLength - 1] = '$';
	memcpy((void*)(sendBuf + 1), &packetLength, 4);
	sendBuf[5] = 'T';
	sendPtr += 6;
	//拷贝数据
	memcpy((void*)sendPtr, &time.wYear, 2);
	sendPtr += 2;
	memcpy((void*)sendPtr, &time.wMonth, 2);
	sendPtr += 2;
	memcpy((void*)sendPtr, &time.wDay, 2);
	sendPtr += 2;
	memcpy((void*)sendPtr, &time.wHour, 2);
	sendPtr += 2;
	memcpy((void*)sendPtr, &time.wMinute, 2);
	sendPtr += 2;
	memcpy((void*)sendPtr, &time.wSecond, 2);

	//发送数据
	ret = send(sServer, sendBuf, BUFFER_SIZE, 0);
	if (ret == SOCKET_ERROR) return false;
	else return true;
}

bool sendHostName(SOCKET sServer, char* sendBuf) {
	const char* hostName = "Asus";
	int hostNameLength = 4;	//不含'\0'
	int packetLength;
	int ret;
	char* sendPtr = sendBuf;
	for (int i = 0; i <= 255; i++)
		if (hostName[i] == '\0') {
			hostNameLength = i;
			break;
		}
	//数据包内容：边界标记A+包大小（包括边界标记）B+响应类型A+数据A/B+边界标记A
	packetLength = 1 + 4 + 1 + hostNameLength + 1;
	//处理头部信息
	sendBuf[0] = '$';
	sendBuf[packetLength - 1] = '$';
	memcpy((void*)(sendBuf + 1), &packetLength, 4);
	sendBuf[5] = 'N';
	sendPtr += 6;
	//拷贝数据
	memcpy(sendPtr, hostName, hostNameLength);

	//发送数据
	ret = send(sServer, sendBuf, BUFFER_SIZE, 0);
	if (ret == SOCKET_ERROR) return false;
	else return true;
}

bool sendClientList(SOCKET sServer, char* sendBuf, const map<int, struct sockaddr_in>& clientAddrs) {
	//数据包内容：边界标记A+包大小（包括边界标记）B+响应类型A+数据A/B+边界标记A
	int packetLength = 1 + 4 + 1 + 4 + 1;
	int clientNum = clientAddrs.size();
	int ret;
	char* sendPtr = sendBuf;
	sendPtr += 6;
	memcpy(sendPtr, &clientNum, 4);
	sendPtr += 4;
	//填充客户端信息
	for (pair<int, struct sockaddr_in> pair : clientAddrs) {
		int clientID = pair.first;
		struct sockaddr_in saClient = pair.second;
		char* clientIP = inet_ntoa(saClient.sin_addr);
		int ipLength = cStrLength(clientIP);
		unsigned short port = ntohs(saClient.sin_port);
		//例：[2][1]10.214.200.119:[8152]/[2]10.210.85.337:[4439]/
		packetLength += 4 + ipLength + 1 + 2 + 1;
		//填充此客户端的信息
		memcpy(sendPtr, &clientID, 4);
		sendPtr += 4;
		memcpy(sendPtr, clientIP, ipLength);
		sendPtr += ipLength;
		*sendPtr = ':';
		sendPtr++;
		memcpy(sendPtr, &port, 2);
		sendPtr += 2;
		*sendPtr = '/';
		sendPtr++;
	}
	//填充头部信息
	sendBuf[0] = '$';
	sendBuf[packetLength - 1] = '$';
	memcpy(sendBuf + 1, &packetLength, 4);
	sendBuf[5] = 'L';

	//发送数据
	ret = send(sServer, sendBuf, BUFFER_SIZE, 0);
	if (ret == SOCKET_ERROR) return false;
	else return true;
}

int sendMsg(SOCKET sSender, const sockaddr_in& senderAddr,
	const char* const recvBuf, const map<int, struct sockaddr_in>& clientAddrs,
	const map<int, SOCKET>& clientSockets) {
	const char* recvPtr = recvBuf;
	const char* recverIP_cStr = recvPtr + 6;
	const char* senderIP_cStr;
	const char* msgPtr;
	char* toSenderBuf = new char[BUFFER_SIZE];
	char* toRecverBuf;
	char* toRecverPtr;
	char* toSenderPtr = toSenderBuf;
	unsigned short senderPort;
	unsigned short recverPort;
	const int toSenderPackLength = 1 + 4 + 1 + 1 + 1;
	int msgLength;
	int recverIPLength;
	int senderIPLength;
	int i;
	int recvPackLength;
	int toRecverPackLength = 1 + 4 + 1 + 1;
	int ret;
	//sockaddr_in recverAddr;
	//解析数据
	memcpy(&recvPackLength, recvBuf + 1, 4);
	for (i = 0; i <= BUFFER_SIZE - 6; i++)
		if (recverIP_cStr[i] == '/') break;
	recverIPLength = i;
	recvPtr += (6 + recverIPLength + 1);
	msgPtr = recvPtr;
	msgLength = recvPackLength - (msgPtr - recvPtr) - 1;
	//检查接收者是否已连接
	string recverIP_str;
	bool recverConnected = false;
	const char* clientIP_cStr;
	int recverID;
	for (int j = 1; j <= recverIPLength; j++) {
		recverIP_str += recverIP_cStr[j - 1];
	}
	for (pair<int, struct sockaddr_in> pair : clientAddrs) {
		struct sockaddr_in clientAddr = pair.second;
		clientIP_cStr = inet_ntoa(clientAddr.sin_addr);
		string clientIP_str(clientIP_cStr);
		if (clientIP_str == recverIP_str) {
			recverConnected = true;
			recverID = pair.first;
			break;
		}
	}
	//接收者未连接，只需处理返回给发送者的数据包
	if (!recverConnected) {
		fillFeedbackPacket(toSenderBuf, 2);
		ret = send(sSender, toSenderBuf, BUFFER_SIZE, 0);
		delete[] toSenderBuf;
		if (ret == SOCKET_ERROR) return 3;
		else return 2;
	}
	//处理发送给接收者的数据包
	toRecverBuf = new char[BUFFER_SIZE];
	toRecverPtr = toRecverBuf;
	senderIP_cStr = inet_ntoa(senderAddr.sin_addr);
	senderIPLength = cStrLength(senderIP_cStr);
	toRecverPackLength += senderIPLength;
	toRecverPackLength += (1 + 2 + msgLength);
	senderPort = ntohs(senderAddr.sin_port);
	//填充头部信息
	toRecverBuf[0] = '$';
	toRecverBuf[toRecverPackLength - 1] = '$';
	memcpy(toRecverPtr + 1, &toRecverPackLength, 4);
	toRecverPtr += 5;
	toRecverPtr[0] = 'M';
	toRecverPtr++;
	//填充数据
	memcpy(toRecverPtr, senderIP_cStr, senderIPLength);
	toRecverPtr += senderIPLength;
	toRecverPtr[0] = ':';
	toRecverPtr++;
	memcpy(toRecverPtr, &senderPort, 2);
	toRecverPtr += 2;
	memcpy(toRecverPtr, msgPtr, msgLength);
	toRecverPtr += msgLength;

	//发送消息给接收者，并将结果反馈给发送者
	const SOCKET& sRecver = clientSockets.at(recverID);
	ret = send(sRecver, toRecverBuf, BUFFER_SIZE, 0);
	delete[] toRecverBuf;
	if (ret == SOCKET_ERROR) {
		fillFeedbackPacket(toSenderBuf, 1);
		ret = send(sSender, toSenderBuf, BUFFER_SIZE, 0);
		delete[] toSenderBuf;
		if (ret == SOCKET_ERROR) return 5;
		else return 4;
	}
	fillFeedbackPacket(toSenderBuf, 0);
	ret = send(sSender, toSenderBuf, BUFFER_SIZE, 0);
	delete[] toSenderBuf;
	if (ret == SOCKET_ERROR) return 1;
	else return 0;
}

int cStrLength(const char* str) {
	int i;
	for (i = 0; i <= MAX_INT - 1; i++)
		if (str[i] == '\0') break;
	return i;
}

void fillFeedbackPacket(char* buf, int result) {
	int packetLength = 1 + 4 + 1 + 4 + 1;
	buf[0] = '$';
	buf[packetLength - 1] = '$';
	memcpy(buf + 1, &packetLength, 4);
	buf += 5;
	buf[0] = 'F';
	buf++;
	memcpy(buf, &result, 4);
	buf += 4;
}