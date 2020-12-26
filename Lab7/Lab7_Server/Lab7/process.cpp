#include <iostream>
#include <map>
#include "process.h"
using namespace std;

constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_INT = 0x7fffffff;

bool sendTime(SOCKET sServer, char* sendBuf) {
	SYSTEMTIME time;
	//���ݰ����ݣ��߽���A+����С�������߽��ǣ�B+��Ӧ����A+����A/B+�߽���A
	int packetLength = 1 + 4 + 1 + 2 * 6 + 1;
	int ret;
	char* sendPtr = sendBuf;
	GetLocalTime(&time);
	//����ͷ����Ϣ
	sendBuf[0] = '$';
	sendBuf[packetLength - 1] = '$';
	memcpy((void*)(sendBuf + 1), &packetLength, 4);
	sendBuf[5] = 'T';
	sendPtr += 6;
	//��������
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

	//��������
	ret = send(sServer, sendBuf, BUFFER_SIZE, 0);
	if (ret == SOCKET_ERROR) return false;
	else return true;
}

bool sendHostName(SOCKET sServer, char* sendBuf) {
	const char* hostName = "Asus";
	int hostNameLength = 4;	//����'\0'
	int packetLength;
	int ret;
	char* sendPtr = sendBuf;
	for (int i = 0; i <= 255; i++)
		if (hostName[i] == '\0') {
			hostNameLength = i;
			break;
		}
	//���ݰ����ݣ��߽���A+����С�������߽��ǣ�B+��Ӧ����A+����A/B+�߽���A
	packetLength = 1 + 4 + 1 + hostNameLength + 1;
	//����ͷ����Ϣ
	sendBuf[0] = '$';
	sendBuf[packetLength - 1] = '$';
	memcpy((void*)(sendBuf + 1), &packetLength, 4);
	sendBuf[5] = 'N';
	sendPtr += 6;
	//��������
	memcpy(sendPtr, hostName, hostNameLength);

	//��������
	ret = send(sServer, sendBuf, BUFFER_SIZE, 0);
	if (ret == SOCKET_ERROR) return false;
	else return true;
}

bool sendClientList(SOCKET sServer, char* sendBuf, const map<int, struct sockaddr_in>& clientAddrs) {
	//���ݰ����ݣ��߽���A+����С�������߽��ǣ�B+��Ӧ����A+����A/B+�߽���A
	int packetLength = 1 + 4 + 1 + 4 + 1;
	int clientNum = clientAddrs.size();
	int ret;
	char* sendPtr = sendBuf;
	sendPtr += 6;
	memcpy(sendPtr, &clientNum, 4);
	sendPtr += 4;
	//���ͻ�����Ϣ
	for (pair<int, struct sockaddr_in> pair : clientAddrs) {
		int clientID = pair.first;
		struct sockaddr_in saClient = pair.second;
		char* clientIP = inet_ntoa(saClient.sin_addr);
		int ipLength = cStrLength(clientIP);
		unsigned short port = ntohs(saClient.sin_port);
		//����[2][1]10.214.200.119:[8152]/[2]10.210.85.337:[4439]/
		packetLength += 4 + ipLength + 1 + 2 + 1;
		//���˿ͻ��˵���Ϣ
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
	//���ͷ����Ϣ
	sendBuf[0] = '$';
	sendBuf[packetLength - 1] = '$';
	memcpy(sendBuf + 1, &packetLength, 4);
	sendBuf[5] = 'L';

	//��������
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
	//��������
	memcpy(&recvPackLength, recvBuf + 1, 4);
	for (i = 0; i <= BUFFER_SIZE - 6; i++)
		if (recverIP_cStr[i] == '/') break;
	recverIPLength = i;
	recvPtr += (6 + recverIPLength + 1);
	msgPtr = recvPtr;
	msgLength = recvPackLength - (msgPtr - recvPtr) - 1;
	//���������Ƿ�������
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
	//������δ���ӣ�ֻ�账���ظ������ߵ����ݰ�
	if (!recverConnected) {
		fillFeedbackPacket(toSenderBuf, 2);
		ret = send(sSender, toSenderBuf, BUFFER_SIZE, 0);
		delete[] toSenderBuf;
		if (ret == SOCKET_ERROR) return 3;
		else return 2;
	}
	//�����͸������ߵ����ݰ�
	toRecverBuf = new char[BUFFER_SIZE];
	toRecverPtr = toRecverBuf;
	senderIP_cStr = inet_ntoa(senderAddr.sin_addr);
	senderIPLength = cStrLength(senderIP_cStr);
	toRecverPackLength += senderIPLength;
	toRecverPackLength += (1 + 2 + msgLength);
	senderPort = ntohs(senderAddr.sin_port);
	//���ͷ����Ϣ
	toRecverBuf[0] = '$';
	toRecverBuf[toRecverPackLength - 1] = '$';
	memcpy(toRecverPtr + 1, &toRecverPackLength, 4);
	toRecverPtr += 5;
	toRecverPtr[0] = 'M';
	toRecverPtr++;
	//�������
	memcpy(toRecverPtr, senderIP_cStr, senderIPLength);
	toRecverPtr += senderIPLength;
	toRecverPtr[0] = ':';
	toRecverPtr++;
	memcpy(toRecverPtr, &senderPort, 2);
	toRecverPtr += 2;
	memcpy(toRecverPtr, msgPtr, msgLength);
	toRecverPtr += msgLength;

	//������Ϣ�������ߣ��������������������
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