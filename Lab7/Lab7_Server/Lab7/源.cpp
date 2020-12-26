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

constexpr int SERVER_PORT = 1042;	//学号后四位作为监听端口
constexpr int MAX_CONNECTION = 8;
constexpr int BUFFER_SIZE = 1024;	//1KB

//初始化服务器
bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen,
    struct sockaddr_in& saServer);
//成功建立连接，处理请求
void processRequest(int clientID, map<int, struct sockaddr_in>& clientAddrs,
    map<int, SOCKET>& clientSockets);
//输出客户端信息
void printClientAddr(const sockaddr_in* clientAddr, bool upper);
//分配一个尚未被占用的ID
//达到最大连接数时，返回-1
int allocateID(const map<int, struct sockaddr_in>& clientAddrs);
//根据处理成功与否输出提示
void printProcessResultMsg(string requestName, const sockaddr_in* clientAddr,
    int clientID, bool successful);
//根据处理成功与否输出提示，为传递消息请求特别编写
void printMsgReqProcRsMsg(const sockaddr_in* clientAddr, int clientID,
    int code);
//检查接收到的数据包是否完整
//bool checkIntegrity(const char* recvBuf, int nLeft);

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
    else cout << " > Unable to start server. Program terminated." << endl;

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
        thread t(processRequest, clientID, ref(clientAddrs), ref(clientSockets));
        t.detach();
    }

    closesocket(sListen);
    WSACleanup();
}

bool initialize(WORD& wVersionRequested, WSADATA& wsaData, SOCKET& sListen,
    struct sockaddr_in& saServer) {
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
    saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);	//使用INADDR_ANY指示任意地址

    //绑定socket句柄与服务器地址
    ret = ::bind(sListen, (struct sockaddr *)&saServer, sizeof(saServer));
    if (ret == SOCKET_ERROR)
    {
        cout << " > Error: Failed to bind socket. Code:" << WSAGetLastError() << endl;
        closesocket(sListen);	//关闭套接字
        WSACleanup();
        return false;
    }

    //监听socket开始侦听连接请求
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
        //接收数据
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
            //连接中断
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
        ////未能完整接受数据包
        //if (nLeft) {
        //	//客户未发送任何数据
        //	if (nLeft == BUFFER_SIZE) cout << "Thread " << clientID << " terminated." << endl;
        //	//发送过程中中断
        //	else {
        //		cout << "Error: Failed to receive from ";
        //		printClientAddr(clientAddr, false);
        //		cout << ". Thread " << clientID << " terminated." << endl;
        //	}
        //	return;
        //}
        //完整接收数据包，解析类型并处理请求
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
//	//从客户端接收到的数据包至少应有7字节长
//	if (recvSize < 7) return false;
//	memcpy(&sendSize, recvBuf + 1, 4);
//	//接收长度与发送长度不同
//	if (recvSize != sendSize) return false;
//	return true;
//}