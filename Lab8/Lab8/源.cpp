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
const string SERVER_ROOT = "../Server Files";	//��������

//Ϊ����ͻ��˷���ID������ͻ��������ﵽ���ֵʱ����-1
int allocateID(const map<int, struct sockaddr_in>& clientAddrs);
//��ʼ������������
bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen, struct sockaddr_in& saServer);
//�ڿ���̨����ͻ��˵�IP��ַ�Ͷ˿���Ϣ
void printClientAddr(const sockaddr_in* clientAddr, bool upper);
//��������
bool processRequest(const char* pkt, const int length, const SOCKET& sClient);
//�������Կͻ��˵�����
void recvRequest(int clientID, map<int, struct sockaddr_in>& clientAddrs, map<int, SOCKET>& clientSockets);

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
		t.detach(); //todo �̲߳���
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

	cout << recvBuf;
	bool ret=processRequest(recvBuf, recvdLength, sServer);
	if (ret == true) {
		//todo �����ʾ��Ϣ
	}
	else {
		//todo
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
			filePath += "/html" + parseFilePath(pkt, length);
			fs.open(filePath.c_str(), ios::binary | ios::in);
		}
		else if (conType == ContentType::IMAGE) {
			filePath += "/img" + parseFilePath(pkt, length);
			fs.open(filePath.c_str(), ios::binary | ios::in);
		} //�����ļ����Ͳ��迼��
		//todo ����ͼ���ļ�
		if (!fs.is_open()) { //�ļ������ڣ�����Not Found�ļ�
			fs.open(SERVER_ROOT + "/html/notFound.html", ios::in | ios::binary);
			bool successful = sendFile(sServer, fs);
			fs.close();
			return successful;
		}
		else { //�ļ����ڣ����������ļ�
			bool successful = sendFile(sServer, fs);
			fs.close();
			return successful;
		}
	}
	else if (reqType == RequestType::POST) {
		string url = parseFilePath(pkt, length);
		if (url != "/dopost") { //����·������ȷ������Not Found
			fs.open(SERVER_ROOT + "/html/notFound.html", ios::in | ios::binary);
			bool successful = sendFile(sServer, fs);
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
			bool successful = sendFile(sServer, fs);
			fs.close();
			return successful;
		}
		else {
			fs.open(SERVER_ROOT + "/html/loginFailure.html", ios::in | ios::binary);
			bool successful = sendFile(sServer, fs);
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