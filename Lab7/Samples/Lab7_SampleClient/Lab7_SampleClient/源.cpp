/*
 *	���������ҪĿ������˵��socket��̵Ļ������̣����Է�����/�ͻ��˵Ľ������̷ǳ��򵥣�
 *  ֻ���ɿͻ��������������һ��ѧ����Ϣ�Ľṹ��
 */
 //informWinClient.cpp������ΪserverIP name age
#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_PORT	6666 //�����˿�


//�ͻ�������������͵Ľṹ��
struct student
{
	char name[32];
	int age;
};

void main(int argc, char *argv[])
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;
	SOCKET sClient; //�����׽���
	struct sockaddr_in saServer; //��ַ��Ϣ
	struct student stu;
	char *ptr = (char *)&stu;
	BOOL fSuccess = TRUE;

	argc = 4;
	const char* serverIP = "10.181.213.167";
	const char* name = "Serena";
	const char* age = "14";


	if (argc != 4)
	{
		printf("usage: informWinClient serverIP name age\n"); //����÷���ʾ
		return;
	}

	//WinSock��ʼ����
	wVersionRequested = MAKEWORD(2, 2); //ϣ��ʹ�õ�WinSock DLL�İ汾
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0)
	{
		printf("WSAStartup() failed!\n");
		return;
	}
	//ȷ��WinSock DLL֧�ְ汾2.2��
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		printf("Invalid Winsock version!\n");
		return;
	}

	//����socket��ʹ��TCPЭ�飺
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		return;
	}

	//������������ַ��Ϣ��
	saServer.sin_family = AF_INET; //��ַ����
	saServer.sin_port = htons(SERVER_PORT); //ע��ת��Ϊ�����ֽ���
	saServer.sin_addr.S_un.S_addr = inet_addr(serverIP);


	//���ӷ�������
	ret = connect(sClient, (struct sockaddr *)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR)
	{
		printf("connect() failed!\n");
		closesocket(sClient); //�ر��׽���
		WSACleanup();
		return;
	}

	//����Ԥ��Э�飬�ͻ��˽�����һ��ѧ������Ϣ��
	strcpy(stu.name, name);
	stu.age = atoi(age);
	ret = send(sClient, (char *)&stu, sizeof(stu), 0);
	if (ret == SOCKET_ERROR)
	{
		printf("send() failed!\n");
	}
	else
		printf("student info has been sent!\n");

	closesocket(sClient); //�ر��׽���
	WSACleanup();
}