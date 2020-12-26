/*
 *	本程序的主要目的在于说明socket编程的基本过程，所以服务器/客户端的交互过程非常简单，
 *  只是由客户端向服务器传送一个学生信息的结构。
 */
 //informWinClient.cpp：参数为serverIP name age
#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_PORT	6666 //侦听端口


//客户端向服务器传送的结构：
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
	SOCKET sClient; //连接套接字
	struct sockaddr_in saServer; //地址信息
	struct student stu;
	char *ptr = (char *)&stu;
	BOOL fSuccess = TRUE;

	argc = 4;
	const char* serverIP = "10.181.213.167";
	const char* name = "Serena";
	const char* age = "14";


	if (argc != 4)
	{
		printf("usage: informWinClient serverIP name age\n"); //输出用法提示
		return;
	}

	//WinSock初始化：
	wVersionRequested = MAKEWORD(2, 2); //希望使用的WinSock DLL的版本
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0)
	{
		printf("WSAStartup() failed!\n");
		return;
	}
	//确认WinSock DLL支持版本2.2：
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		printf("Invalid Winsock version!\n");
		return;
	}

	//创建socket，使用TCP协议：
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		return;
	}

	//构建服务器地址信息：
	saServer.sin_family = AF_INET; //地址家族
	saServer.sin_port = htons(SERVER_PORT); //注意转化为网络字节序
	saServer.sin_addr.S_un.S_addr = inet_addr(serverIP);


	//连接服务器：
	ret = connect(sClient, (struct sockaddr *)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR)
	{
		printf("connect() failed!\n");
		closesocket(sClient); //关闭套接字
		WSACleanup();
		return;
	}

	//按照预定协议，客户端将发送一个学生的信息：
	strcpy(stu.name, name);
	stu.age = atoi(age);
	ret = send(sClient, (char *)&stu, sizeof(stu), 0);
	if (ret == SOCKET_ERROR)
	{
		printf("send() failed!\n");
	}
	else
		printf("student info has been sent!\n");

	closesocket(sClient); //关闭套接字
	WSACleanup();
}