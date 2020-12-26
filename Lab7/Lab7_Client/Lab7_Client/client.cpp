#include"client.h"
#include"thread.h"
#include"menu.h"

int Connect_flag = 0;

using namespace std;
int main()
{
	Initialize();
	char ch = Input();
	int Socket_flag = 0;
	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;
	SOCKET sClient; //连接套接字
	struct sockaddr_in saServer;//地址信息

	BOOL fSuccess = TRUE;

	//WinSock初始化：
	wVersionRequested = MAKEWORD(2, 2);//希望使用的WinSock DLL的版本
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0)
	{
		printf("WSAStartup() failed!\n");
		return 0;
	}

	//确认WinSock DLL支持版本2.2：
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		printf("Invalid Winsock version!\n");
		return 0;
	}

	//创建socket，使用TCP协议：
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET)
	{
		WSACleanup();
		printf("socket() failed!\n");
		return 0;
	}
	Socket_flag = 1;

	//构建服务器地址信息：
	saServer.sin_family = AF_INET;//地址家族
	saServer.sin_port = htons(1042);//注意转化为网络字节序
	saServer.sin_addr.S_un.S_addr = inet_addr(TMZ_IP);

	bool Already_Thread = false;
	thread* Second_Thread = new thread(Receive, ref(sClient));
	Second_Thread->detach();
	while (ch)
	{
		if (ch == 'a' || ch == 'A')
		{
			if (Connect_flag == 1)
			{
				cout << "Already Connected!" << endl;
				ch = Input();
				continue;
			}
			if (Socket_flag == 0)
			{
				sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (sClient == INVALID_SOCKET)
				{
					WSACleanup();
					printf("socket() failed!\n");
					return 0;
				}
				Socket_flag = 1;
			}

			ret = connect(sClient, (struct sockaddr*) & saServer, sizeof(saServer));
			if (ret == SOCKET_ERROR)
			{
				printf("connect() failed!\n");
				closesocket(sClient);//关闭套接字
				return 0;
			}
			cout << "Connection established." << endl;
			Connect_flag = 1;
		}
		else if (ch == 'b' || ch == 'B')
		{
			if (Connect_flag == 0)
			{
				cout << "Not yet connected!" << endl;
				ch = Input();
				continue;
			}
			Connect_flag = 0;
			closesocket(sClient);//关闭套接字
			Socket_flag = 0;
			cout << "You have disconnected." << endl;
		}
		else if (ch == 'c' || ch == 'C')
		{
			char* RequestTime = new char[LENGTH];
			int length = 7;
			RequestTime[0] = '$';
			memcpy((void*)(RequestTime + 1), &length, 4);
			RequestTime[5] = 'T';
			RequestTime[6] = '$';
			ret = send(sClient, RequestTime, LENGTH, 0);
			if (ret == SOCKET_ERROR) printf("send() failed!\n");
			else printf("Request has been sent.\n");


		}
		else if (ch == 'd' || ch == 'D')
		{
			char* RequestName = new char[LENGTH];
			int length = 7;
			RequestName[0] = '$';
			memcpy((void*)(RequestName + 1), &length, 4);
			RequestName[5] = 'N';
			RequestName[6] = '$';
			ret = send(sClient, RequestName, LENGTH, 0);
			if (ret == SOCKET_ERROR) printf("send() failed!\n");
			else printf("Request has been sent.\n");


		}
		else if (ch == 'e' || ch == 'E')
		{
			if (Connect_flag == 0)
			{
				cout << "Not yet connected. Please connect first!" << endl;
				ch = Input();
				continue;
			}
			char* RequestList = new char[LENGTH];
			int length = 7;
			RequestList[0] = '$';
			memcpy((void*)(RequestList + 1), &length, 4);
			RequestList[5] = 'L';
			RequestList[6] = '$';
			ret = send(sClient, RequestList, LENGTH, 0);
			if (ret == SOCKET_ERROR) printf("send() failed!\n");
			else printf("Request has been sent.\n");


		}

		else if (ch == 'f' || ch == 'F')
		{
			if (Connect_flag == 0)
			{
				cout << "Not yet connected. Please connect first!" << endl;
				ch = Input();
				continue;
			}
			char* RequestList = new char[LENGTH];
			int length = 7;
			RequestList[0] = '$';
			memcpy((void*)(RequestList + 1), &length, 4);
			RequestList[5] = 'L';
			RequestList[6] = '$';
			ret = send(sClient, RequestList, LENGTH, 0);
			if (ret == SOCKET_ERROR) printf("send() failed!\n");
			else printf("Request has been sent.\n");

			Sleep(300);

			char* Send_Message = new char[LENGTH];
			string str_message;
			string str_op_ip;
			cout << "Please enter the IP:" << endl;
			getline(cin, str_op_ip);
			const char* op_ip = new char[str_op_ip.length()];
			op_ip = str_op_ip.c_str();

			cout << "Please enter a message:" << endl;
			getline(cin, str_message);
			const char* message = new char[str_message.length()];
			message = str_message.c_str();

			length = str_op_ip.length() + str_message.length() + 8;
			Send_Message[0] = '$';
			memcpy((void*)(Send_Message + 1), &length, 4);
			Send_Message[5] = 'M';
			memcpy((void*)(Send_Message + 6), (void*)op_ip, str_op_ip.length());
			Send_Message[6 + str_op_ip.length()] = '/';
			memcpy((void*)(Send_Message + 7 + str_op_ip.length()), (void*)message, str_message.length());
			Send_Message[length - 1] = '$';
			ret = send(sClient, Send_Message, LENGTH, 0);
			if (ret == SOCKET_ERROR) printf("send() failed!\n");
			else printf("Request has been sent.\n");

		}

		else if (ch == 'g' || ch == 'G')
		{
			cout << "You are exiting. Confirm ?" << endl;
			cout << "Y or N" << endl;
			char ch;
			cin >> ch;
			while (ch)
			{
				if (ch == 'Y' || ch == 'y')
				{
					closesocket(sClient);//关闭套接字
					WSACleanup();
					exit(0);
				}
				else if (ch == 'N' || ch == 'n') cout << "Please select an operation." << endl;
				else cout << "Invalid Input!" << endl;
				cin >> ch;
			}
		}
		else cout << "Invalid Input: Operation code must between 'a' and 'g'." << endl;
		ch = Input();
	}

	delete Second_Thread;

	return 0;
}