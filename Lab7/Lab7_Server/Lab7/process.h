#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "WinSock2.h"
using namespace std;

#pragma comment (lib, "WS2_32")
#pragma warning (disable:4996)

bool sendTime(SOCKET sServer, char* sendBuf);
bool sendHostName(SOCKET sServer, char* sendBuf);
bool sendClientList(SOCKET sServer, char* sendBuf, const map<int, struct sockaddr_in>& clientAddrs);
//ͨ������ֵ��ʾ���ܳ��ֵĴ���
//0�����ͳɹ�
//1��������߷��ͳɹ��������߷���ʧ��
//2��������δ���ӣ������߷����ɹ�
//3��������δ���ӣ������߷���ʧ��
//4��������߷���ʧ�ܣ������߷����ɹ�
//5��������߷���ʧ�ܣ������߷���ʧ��
int sendMsg(SOCKET sSender, const sockaddr_in& senderAddr,
	const char* const recvBuf, const map<int, struct sockaddr_in>& clientAddrs,
	const map<int, SOCKET>& clientSockets);
//ͳ��C�ַ����ĳ���
int cStrLength(const char* str);
//����sendMsg��������Բ�ͬ�ķ��ͽ����������в�ͬ��Ϣ�ķ������ݰ�
void fillFeedbackPacket(char* buf, int result);

#endif