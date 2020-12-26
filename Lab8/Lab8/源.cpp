#include <fstream>
#include <iostream>
#include <map>
#include "ParseHTTP.h"
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
constexpr int BUFFER_SIZE = 1024 * 1024;	//1MB
const string SERVER_ROOT = "../Server Files";	//��������


bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen, struct sockaddr_in& saServer);
void printClientAddr(const sockaddr_in* clientAddr, bool upper);

int main() {
	WORD wVersionRequested;	//typedef unsigned short DWORD;
	WSADATA wsaData;	//����WSAStartupִ�к󷵻ص�Windows Sockets����
	int ret, addrLength;
	SOCKET sListen;	//����socket
					//sListen�ǿ���socket, sServer������socket
					//���̱߳��ʱ����һ������socket���������socket
					//ʹ�ÿ���socket����listen��������������ͨ������socket��ȡ�����ݴ����꼴�ر�����socket
	map<int, SOCKET> clientSockets;	//�ͻ���socket
	struct sockaddr_in saServer;	//�������׽��ֵ�ַ
	map<int, struct sockaddr_in> clientAddrs;	//�ͻ����׽��ֵ�ַ

	if (initialize(wVersionRequested, wsaData, sListen, saServer))
		cout << " > Server startup finished. Awaiting connection." << endl;
	else cout << " > Unable to start server. Program terminated." << endl;

	while (true) {
		//�߳����ﵽ���1�������
		if (clientAddrs.size() == MAX_CONNECTION) {
			Sleep(1000);
			continue;
		}
		sockaddr_in saClient;
		SOCKET sServer;
		addrLength = sizeof(saClient);
		//�������ȴ��ͻ�������
		sServer = accept(sListen, (struct sockaddr*)&saClient, &addrLength);
		if (sServer == INVALID_SOCKET) {
			cout << " > Error: Failed to accept client. Code:" << WSAGetLastError() << endl;
			continue;
		}
		//�ͻ��˽���ɹ����½��̴߳�������
		//inet_nota�������ַתΪ��ָ��ַ�������ʽ
		cout << " > Client " << inet_ntoa(saClient.sin_addr) << ":" << ntohs(saClient.sin_port) << " connected." << endl;
		int clientID = allocateID(clientAddrs);
		clientAddrs.insert(pair<int, struct sockaddr_in>(clientID, saClient));
		clientSockets.insert(pair<int, SOCKET>(clientID, sServer));
		thread t(recvRequest, clientID, ref(clientAddrs), ref(clientSockets));
		t.detach(); //todo
	}

	closesocket(sListen);
	WSACleanup();
}

bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen, struct sockaddr_in& saServer) {
	int ret;
	//WinSock��ʼ��
	wVersionRequested = MAKEWORD(2, 2);	//ϣ��ʹ�õ�WinSock DLL�汾
	ret = WSAStartup(wVersionRequested, &wsaData);	//����socket��
	if (ret != 0) {
		cout << " > Error: Failed to initialize WinSock." << endl;
		return false;
	}

	//ȷ��WinSock DLL֧�ְ汾2.2
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		cout << " > Error: Invalid WinSock Version" << endl;
		WSACleanup();	//�����socket�⣬�ͷ���Դ
		return false;
	}

	//��������socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListen == INVALID_SOCKET) {
		cout << " > Error: Failed to construct socket." << endl;
		WSACleanup();
		return false;
	}

	//�������ص�ַ��Ϣ
	saServer.sin_family = AF_INET;	//��ַ����
	saServer.sin_port = htons(SERVER_PORT);	//�˿ںţ�htons���˿ں�תΪ��˻���ʶ��ĸ�ʽ
											//���ֽڵ���htonl�����ֽڵ���htons
											//int����htons��λ�ضϣ����ضϷ������ܳ���
											//htons: Host TO Network Short
	saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);	//ʹ��INADDR_ANYָʾ�����ַ

	//��socket������������ַ
	ret = ::bind(sListen, (struct sockaddr*)&saServer, sizeof(saServer));
	if (ret == SOCKET_ERROR) {
		cout << " > Error: Failed to bind socket. Code:" << WSAGetLastError() << endl;
		closesocket(sListen);	//�ر��׽���
		WSACleanup();
		return false;
	}

	//����socket��ʼ������������
	ret = listen(sListen, 5);
	if (ret == SOCKET_ERROR) {
		cout << " > Error: Unable to start listening program. Code:" << WSAGetLastError() << endl;
		closesocket(sListen);
		WSACleanup();
		return false;
	}

	return true;
}

int allocateID(const map<int, struct sockaddr_in>& clientAddrs) {
	for (int i = 1; i <= MAX_CONNECTION; i++) {
		if (!clientAddrs.count(i)) return i;
	}
	return -1;
}

//��������
void recvRequest(int clientID, map<int, struct sockaddr_in>& clientAddrs, map<int, SOCKET>& clientSockets) {
	SOCKET& sServer = clientSockets.at(clientID);
	char* const recvBuf = new char[BUFFER_SIZE];
	sockaddr_in* clientAddr = &(clientAddrs.at(clientID));

	//��������
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
	if (recvdLength == 0) { //�����ж�
		printClientAddr(clientAddr, true);
		cout << " disconnected. Thread " << clientID << " terminated." << endl;
		clientAddrs.erase(clientID);
		delete[] recvBuf;
		closesocket(sServer);
		clientSockets.erase(clientID);
		return;
	}

	processRequest(recvBuf, recvdLength);
}

//��������
void processRequest(const char* pkt, const int length) {
	RequestType type = parseRequestType(pkt, length);
	char* const sendBuf = new char[BUFFER_SIZE];
	char* sendPtr = sendBuf;

	if (type == GET) {
		string filePath = SERVER_ROOT + parseFilePath(pkt, length);
		fstream fs;
		fs.open(filePath.c_str(), ios::binary | ios::in);
		if (!fs.is_open()) { //�ļ�������
			fs.open(SERVER_ROOT + "\\NotFound.html", ios::in | ios::binary);
			fs.seekg(0, ios::end);	//g: get, seekg�����Ѿ���Ҫ���ж�ȡ���ļ�
									//������ƫ��������ʼλ��
			int fileLength = fs.tellg();

			string firstLine = "HTTP/1.1 404 Not Found\n";
			string typeLine = "Content-Type: text/html\n";
			string lengthLine = "Content-Length: ";
			appendInt(lengthLine, fileLength);
			//todo
		}
		else {
			//todo
			fs.close();
		}
	}
	else {
		//todo
	}
}

void printClientAddr(const sockaddr_in* clientAddr, bool upper) {
	if (!upper) cout << " > client ";
	else cout << " > Client ";
	cout << inet_ntoa(clientAddr->sin_addr) << ":" << ntohs(clientAddr->sin_port);
}