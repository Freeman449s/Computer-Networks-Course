#ifndef __PARSEHTTP_H__
#define __PARSEHTTP_H__

#include <string>
using namespace std;

enum RequestType { GET, POST };

RequestType parseRequestType(const char* pkt, const int length);

//�������·������ͷӦ��б��
string parseFilePath(const char* pkt, const int length);

#endif