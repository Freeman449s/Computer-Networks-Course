#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
using namespace std;

enum RequestType { GET, POST };

//���ַ���ĩβ׷������
void appendInt(string& str, const int i);

//��װHTTP��
void constructPkt(const string& const status, const string& const contentType, fstream& const fs, char* sendBuf);

//���ļ����ݸ��ƽ�������
int copyFile(fstream& const fs, char* const buffer);


//�������·������ͷӦ��б��
string parseFilePath(const char* pkt, const int length);

//������������
RequestType parseRequestType(const char* pkt, const int length);

#endif