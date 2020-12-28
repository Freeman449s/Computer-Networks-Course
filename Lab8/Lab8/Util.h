#ifndef __UTIL_H__
#define __UTIL_H__

#include <fstream>
#include <string>
#include <vector>
#include <WinSock2.h>
using namespace std;

enum class RequestType { GET, POST, OTHER };
enum class ContentType { HTML, JPG, TEXT, OTHER };
constexpr int BUFFER_SIZE = 1024 * 1024;	//1MB

//���ַ���ĩβ׷������
void appendInt(string& str, const int i);

//��װHTTP��
int constructPkt(const string& const status, const string& const contentType, fstream& const fs, char* sendBuf);

//���ļ����ݸ��ƽ�������
int copyFile(fstream& const fs, char* const buffer);

//ͳ���ļ�����
int countNLines(fstream& fs);

//�����˺ź�����
vector<string> parseAccountAndPwd(const char* pkt, const int length);

//����������ļ�����
ContentType parseContentType(const char* pkt, const int length);

//�������·������ͷӦ��б��
string parseFilePath(const char* pkt, const int length);

//������������
RequestType parseRequestType(const char* pkt, const int length);

//��ͻ��˷����ļ�
bool sendFile(const SOCKET& sServer, fstream& fs, string status, string conType);

#endif