#include <iostream>
#include <map>
#include "process.h"
#include <set>
#include <string>
#include <thread>
#include <WinSock2.h>
using namespace std;

#pragma comment (lib, "WS2_32")
#pragma warning (disable:4996)

constexpr int SERVER_PORT = 1042;	//ѧ�ź���λ��Ϊ�����˿�
constexpr int MAX_CONNECTION = 8;
constexpr int BUFFER_SIZE = 1024;	//1KB

//��ʼ��������
bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen,
    struct sockaddr_in& saServer);
//�ɹ��������ӣ���������
void processRequest(int clientID, map<int, struct sockaddr_in>& clientAddrs,
    map<int, SOCKET>& clientSockets);
//����ͻ�����Ϣ
void printClientAddr(const sockaddr_in* clientAddr, bool upper);
//����һ����δ��ռ�õ�ID
//�ﵽ���������ʱ������-1
int allocateID(const map<int, struct sockaddr_in>& clientAddrs);
//���ݴ���ɹ���������ʾ
void printProcessResultMsg(string requestName, const sockaddr_in* clientAddr,
    int clientID, bool successful);
//���ݴ���ɹ���������ʾ��Ϊ������Ϣ�����ر��д
void printMsgReqProcRsMsg(const sockaddr_in* clientAddr, int clientID,
    int code);
//�����յ������ݰ��Ƿ�����
//bool checkIntegrity(const char* recvBuf, int nLeft);

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
        thread t(processRequest, clientID, ref(clientAddrs), ref(clientSockets));
        t.detach();
    }

    closesocket(sListen);
    WSACleanup();
}

bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen,
    struct sockaddr_in& saServer) {
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
    ret = ::bind(sListen, (struct sockaddr *)&saServer, sizeof(saServer));
    if (ret == SOCKET_ERROR)
    {
        cout << " > Error: Failed to bind socket. Code:" << WSAGetLastError() << endl;
        closesocket(sListen);	//�ر��׽���
        WSACleanup();
        return false;
    }

    //����socket��ʼ������������
    ret = listen(sListen, 5);
    if (ret == SOCKET_ERROR)
    {
        cout << " > Error: Unable to start listening program. Code:" << WSAGetLastError() << endl;
        closesocket(sListen);
        WSACleanup();
        return false;
    }

    return true;
}

void processRequest(int clientID, map<int, struct sockaddr_in>& clientAddrs,
    map<int, SOCKET>& clientSockets) {
    SOCKET& sServer = clientSockets.at(clientID);
    char* const recvBuf = new char[BUFFER_SIZE];
    char* recvPtr = recvBuf;
    char* const sendBuf = new char[BUFFER_SIZE];
    //char* sendPtr = sendBuf;
    int nLeft;
    int ret;
    bool connected = true;
    sockaddr_in* clientAddr = &(clientAddrs.at(clientID));
    while (true) {
        nLeft = BUFFER_SIZE;
        recvPtr = recvBuf;
        //��������
        while (nLeft > 0) {
            ret = recv(sServer, recvPtr, nLeft, 0);
            if (ret == SOCKET_ERROR) {
                cout << " > Failed to receive from client " << inet_ntoa(clientAddr->sin_addr)
                    << ":" << ntohs(clientAddr->sin_port)
                    << ": remote host might have disconnected." << endl;
                cout << "   Thread " << clientID << " terminated." << endl;
                clientAddrs.erase(clientID);
                delete[] recvBuf;
                delete[] sendBuf;
                closesocket(sServer);
                clientSockets.erase(clientID);
                return;
            }
            //�����ж�
            if (ret == 0) {
                printClientAddr(clientAddr, true);
                cout << " disconnected. Thread " << clientID << " terminated." << endl;
                clientAddrs.erase(clientID);
                delete[] recvBuf;
                delete[] sendBuf;
                closesocket(sServer);
                clientSockets.erase(clientID);
                return;
            }
            nLeft -= ret;
            recvPtr += ret;
        }
        ////δ�������������ݰ�
        //if (nLeft) {
        //	//�ͻ�δ�����κ�����
        //	if (nLeft == BUFFER_SIZE) cout << "Thread " << clientID << " terminated." << endl;
        //	//���͹������ж�
        //	else {
        //		cout << "Error: Failed to receive from ";
        //		printClientAddr(clientAddr, false);
        //		cout << ". Thread " << clientID << " terminated." << endl;
        //	}
        //	return;
        //}
        //�����������ݰ����������Ͳ���������
        char requestType = recvBuf[5];
        bool procResult;
        if (requestType == 't' || requestType == 'T') {
            procResult = sendTime(sServer, sendBuf);
            printProcessResultMsg("SendTime", clientAddr, clientID, procResult);
        }
        else if (requestType == 'n' || requestType == 'N') {
            procResult = sendHostName(sServer, sendBuf);
            printProcessResultMsg("SendHostName", clientAddr, clientID, procResult);
        }
        else if (requestType == 'l' || requestType == 'L') {
            procResult = sendClientList(sServer, sendBuf, clientAddrs);
            printProcessResultMsg("SendClientList", clientAddr, clientID, procResult);
        }
        else {
            ret = sendMsg(sServer, *clientAddr, recvBuf, clientAddrs, clientSockets);
            printMsgReqProcRsMsg(clientAddr, clientID, ret);
        }
    }
}

void printClientAddr(const sockaddr_in* clientAddr, bool upper) {
    if (!upper) cout << "client ";
    else cout << " > Client ";
    cout << inet_ntoa(clientAddr->sin_addr) << ":" << ntohs(clientAddr->sin_port);
}

int allocateID(const map<int, struct sockaddr_in>& clientAddrs) {
    for (int i = 1; i <= MAX_CONNECTION; i++) {
        if (!clientAddrs.count(i)) return i;
    }
    return -1;
}

void printProcessResultMsg(string requestName, const sockaddr_in* clientAddr,
    int clientID, bool successful) {
    if (successful) {
        cout << " > Request \"" << requestName << "\" by ";
        printClientAddr(clientAddr, false);
        cout << " has been processed properly." << endl;
    }
    else {
        cout << " > Warning: Failed to process request \""
            << requestName << "\" by ";
        printClientAddr(clientAddr, false);
        cout << ". Thread " << clientID << " standing by." << endl;
    }
}
void printMsgReqProcRsMsg(const sockaddr_in* clientAddr, int clientID,
    int code) {
    string requestName = "SendMessage";
    if (code == 0) {
        cout << " > Request \"" << requestName << "\" by ";
        printClientAddr(clientAddr, false);
        cout << " has been processed properly." << endl;
    }
    else {
        cout << " > Warning: Failed to process request \""
            << requestName << "\" by ";
        printClientAddr(clientAddr, false);
        cout << " properly. Code:" << code << endl;
        cout << "   Thread " << clientID << " standing by." << endl;
    }
}

//bool checkIntegrity(const char* recvBuf, int nLeft) {
//	int recvSize = BUFFER_SIZE - nLeft;
//	int sendSize;
//	//�ӿͻ��˽��յ������ݰ�����Ӧ��7�ֽڳ�
//	if (recvSize < 7) return false;
//	memcpy(&sendSize, recvBuf + 1, 4);
//	//���ճ����뷢�ͳ��Ȳ�ͬ
//	if (recvSize != sendSize) return false;
//	return true;
//}