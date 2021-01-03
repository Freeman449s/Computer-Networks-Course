#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include "Util.h"
#include <WinSock2.h>
using namespace std;

#pragma comment (lib, "WS2_32")
#pragma warning (disable:4996)

constexpr int SERVER_PORT = 1042;
constexpr int MAX_CONNECTION = 8;
const string SERVER_ROOT = "../Server Files";	//服务器根

//为接入客户端分配ID，接入客户端数量达到最大值时返回-1
int allocateID(const map<int, struct sockaddr_in>& clientAddrs);
//返回客户端的套接字号
string getClientAddr(const sockaddr_in* clientAddr, bool upper);
//初始化服务器程序
bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen, struct sockaddr_in& saServer);
//在控制台输出客户端的IP地址和端口信息
void printClientAddr(const sockaddr_in* clientAddr, bool upper);
//处理请求
bool processRequest(const char* pkt, const int length, const SOCKET& sClient);
//接收来自客户端的数据
void recvRequest(int clientID, map<int, struct sockaddr_in>& clientAddrs, map<int, SOCKET>& clientSockets);

int main() {
	WORD wVersionRequested;	//typedef unsigned short DWORD;
	WSADATA wsaData;	//包含WSAStartup执行后返回的Windows Sockets数据
	int ret, addrLength;
	SOCKET sListen;	//侦听socket
					//sListen是控制socket, sServer是数据socket
					//多线程编程时，有一个控制socket，多个数据socket
					//使用控制socket调用listen，监听到的数据通过数据socket获取，数据传输完即关闭数据socket
	map<int, SOCKET> clientSockets;	//客户端socket
	struct sockaddr_in saServer;	//服务器套接字地址
	map<int, struct sockaddr_in> clientAddrs;	//客户端套接字地址

	if (initialize(wVersionRequested, wsaData, sListen, saServer))
		cout << " > Server startup finished. Awaiting connection." << endl;
	else {
		cout << " > Unable to start server. Program terminated." << endl;
		return 0;
	}

	while (true) {
		//线程数达到最大，1秒后重试
		if (clientAddrs.size() == MAX_CONNECTION) {
			Sleep(1000);
			continue;
		}
		sockaddr_in saClient;
		SOCKET sServer;
		addrLength = sizeof(saClient);
		//阻塞，等待客户端连接
		sServer = accept(sListen, (struct sockaddr*)&saClient, &addrLength);
		if (sServer == INVALID_SOCKET) {
			cout << " > Error: Failed to accept client. Code:" << WSAGetLastError() << endl;
			continue;
		}
		//客户端接入成功，新建线程处理请求
		//inet_nota将网络地址转为点分隔字符串的形式
		cout << " > Client " << inet_ntoa(saClient.sin_addr) << ":" << ntohs(saClient.sin_port) << " connected." << endl;
		int clientID = allocateID(clientAddrs);
		clientAddrs.insert(pair<int, struct sockaddr_in>(clientID, saClient));
		clientSockets.insert(pair<int, SOCKET>(clientID, sServer));
		thread t(recvRequest, clientID, ref(clientAddrs), ref(clientSockets));
		t.detach();
	}

	closesocket(sListen);
	WSACleanup();
}

int allocateID(const map<int, struct sockaddr_in>& clientAddrs) {
	for (int i = 1; i <= MAX_CONNECTION; i++) {
		if (!clientAddrs.count(i)) return i;
	}
	return -1;
}

string getClientAddr(const sockaddr_in* clientAddr, bool upper) {
	string ret = "";
	if (!upper) ret += "client ";
	else ret += "Client ";
	ret += inet_ntoa(clientAddr->sin_addr);
	ret += ":";
	appendInt(ret, ntohs(clientAddr->sin_port));
	return ret;
}

bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen, struct sockaddr_in& saServer) {
	int ret;
	//WinSock初始化
	wVersionRequested = MAKEWORD(2, 2);	//希望使用的WinSock DLL版本
	ret = WSAStartup(wVersionRequested, &wsaData);	//启动socket库
	if (ret != 0) {
		cout << " > Error: Failed to initialize WinSock." << endl;
		return false;
	}

	//确认WinSock DLL支持版本2.2
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		cout << " > Error: Invalid WinSock Version" << endl;
		WSACleanup();	//解除绑定socket库，释放资源
		return false;
	}

	//创建监听socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET) {
		cout << " > Error: Failed to construct socket." << endl;
		WSACleanup();
		return false;
	}

	//构建本地地址信息
	saServer.sin_family = AF_INET;	//地址家族
	saServer.sin_port = htons(SERVER_PORT);	//端口号，htons将端口号转为大端机可识别的格式
											//四字节调用htonl，两字节调用htons
											//int传入htons高位截断，不截断反而可能出错
											//htons: Host TO Network Short
	saServer.sin_addr.s_addr = inet_addr("0.0.0.0");	//使用INADDR_ANY指示任意地址

	//绑定socket句柄与服务器地址
	ret = ::bind(sListen, (struct sockaddr*)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR) {
		cout << " > Error: Failed to bind socket. Code:" << WSAGetLastError() << endl;
		closesocket(sListen);	//关闭套接字
		WSACleanup();
		return false;
	}

	//监听socket开始侦听连接请求
	ret = listen(sListen, 5);
	if (ret == SOCKET_ERROR) {
		cout << " > Error: Unable to start listening program. Code:" << WSAGetLastError() << endl;
		closesocket(sListen);
		WSACleanup();
		return false;
	}

	return true;
}

void recvRequest(int clientID, map<int, struct sockaddr_in>& clientAddrs, map<int, SOCKET>& clientSockets) {
	SOCKET& sServer = clientSockets.at(clientID);
	char* const recvBuf = new char[BUFFER_SIZE];
	sockaddr_in* clientAddr = &(clientAddrs.at(clientID));

	//接收数据
	int recvdLength = recv(sServer, recvBuf, BUFFER_SIZE, 0);
	if (recvdLength == SOCKET_ERROR) {
		cout << " > Failed to receive from client " << inet_ntoa(clientAddr->sin_addr)
			<< ":" << ntohs(clientAddr->sin_port)
			<< ": remote host might have disconnected." << endl;
		cout << "   Thread " << clientID << " terminated." << endl;
		clientAddrs.erase(clientID);
		delete[] recvBuf;
		closesocket(sServer);
		clientSockets.erase(clientID);
		return;
	}
	if (recvdLength == 0) { //连接中断
		printClientAddr(clientAddr, true);
		cout << " disconnected. Thread " << clientID << " terminated." << endl;
		clientAddrs.erase(clientID);
		delete[] recvBuf;
		closesocket(sServer);
		clientSockets.erase(clientID);
		return;
	}

	bool ret = processRequest(recvBuf, recvdLength, sServer);
	if (ret == true) {
		cout << " > Request from " << getClientAddr(clientAddr, false) << " has been processed properly. Thread " << clientID << " terminated." << endl;
	}
	else {
		cout << " > Warning: Failed to process request from " << getClientAddr(clientAddr, false) << " properly. Thread " << clientID << " terminated." << endl;
	}
	closesocket(sServer);
	clientSockets.erase(clientID);
	clientAddrs.erase(clientID);
	delete[] recvBuf;
}

bool processRequest(const char* pkt, const int length, const SOCKET& sServer) {
	RequestType reqType = parseRequestType(pkt, length);
	fstream fs;
	if (reqType == RequestType::GET) {
		ContentType conType = parseContentType(pkt, length);
		string filePath = SERVER_ROOT;
		if (conType == ContentType::HTML) {
			if (parseFilePath(pkt, length) == "/") filePath += "/index.html";
			else filePath += "/html" + parseFilePath(pkt, length);
			fs.open(filePath.c_str(), ios::binary | ios::in);
		}
		else if (conType == ContentType::JPG) {
			filePath += parseFilePath(pkt, length);
			fs.open(filePath.c_str(), ios::binary | ios::in);
		}
		else if (conType == ContentType::TEXT) {
			filePath += "/txt" + parseFilePath(pkt, length);
			fs.open(filePath.c_str(), ios::binary | ios::in);
		} //其他文件类型不予考虑
		if (!fs.is_open()) { //文件不存在，返回Not Found文件
			fs.open(SERVER_ROOT + "/html/notFound.html", ios::in | ios::binary);
			bool successful = sendFile(sServer, fs, "404", "text/html");
			fs.close();
			return successful;
		}
		else { //文件存在，返回所需文件
			bool successful;
			if (conType == ContentType::HTML) successful = sendFile(sServer, fs, "200", "text/html");
			else if (conType == ContentType::TEXT) successful = sendFile(sServer, fs, "200", "text/plain");
			else successful = sendFile(sServer, fs, "200", "image/jpeg"); //外层if确保类型只可能是TEXT, HTML和JPG
			fs.close();
			return successful;
		}
	}
	else if (reqType == RequestType::POST) {
		string url = parseFilePath(pkt, length);
		if (url != "/dopost") { //请求路径不正确，返回Not Found
			fs.open(SERVER_ROOT + "/html/notFound.html", ios::in | ios::binary);
			bool successful = sendFile(sServer, fs, "404", "text/html");
			fs.close();
			return successful;
		}
		vector<string> loginInfo = parseAccountAndPwd(pkt, length);
		string account = loginInfo[0];
		string pwd = loginInfo[1];
		bool loginSuccess = false;
		if (account == "3180101042") {
			if (pwd == "1042") loginSuccess = true;
		}
		else if (account == "3180105058") {
			if (pwd == "5058") loginSuccess = true;
		}
		if (loginSuccess) {
			fs.open(SERVER_ROOT + "/html/loginSuccess.html", ios::in | ios::binary);
			bool successful = sendFile(sServer, fs, "200", "text/html");
			fs.close();
			return successful;
		}
		else {
			fs.open(SERVER_ROOT + "/html/loginFailure.html", ios::in | ios::binary);
			bool successful = sendFile(sServer, fs, "200", "text/html");
			fs.close();
			return successful;
		}
	}
}

void printClientAddr(const sockaddr_in* clientAddr, bool upper) {
	if (!upper) cout << " > client ";
	else cout << " > Client ";
	cout << inet_ntoa(clientAddr->sin_addr) << ":" << ntohs(clientAddr->sin_port);
}