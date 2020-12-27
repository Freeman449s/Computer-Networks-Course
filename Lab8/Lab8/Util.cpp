#include <cstdlib>
#include <fstream>
#include "Util.h"

void appendInt(string& str, const int i) {
	char* intCStr = new char[11];
	_itoa_s(i, intCStr, 11, 10);
	string intStr(intCStr);
	str.append(intStr);
}

int constructPkt(const string& const status, const string& const contentType, fstream& const fs, char* sendBuf) {
	fs.seekg(0, ios::end);	
	int fileLength = fs.tellg();
	/*fileLength -= countNLines(fs);
	fileLength++;*/
	int pktLength;
	char* sendPtr = sendBuf;

	//���ͷ��
	string firstLine = "HTTP/1.1 ";
	if (status == "404") {
		firstLine += "404 Not Found\n";
	}
	else if (status == "200") {
		firstLine += "200 OK\n";
	}
	string typeLine = "Content-Type: " + contentType + "\n";
	string lengthLine = "Content-Length: ";
	appendInt(lengthLine, fileLength);
	lengthLine += "\n";
	memcpy(sendPtr, firstLine.c_str(), firstLine.size());
	sendPtr += firstLine.size();
	memcpy(sendPtr, typeLine.c_str(), typeLine.size());
	sendPtr += typeLine.size();
	memcpy(sendPtr, lengthLine.c_str(), lengthLine.size());
	sendPtr += lengthLine.size();
	memcpy(sendPtr, "\n", 1);
	sendPtr++;

	//�򻺳���д���ļ�
	copyFile(fs, sendPtr);
	sendPtr += fileLength;
	pktLength = sendPtr - sendBuf;
	return pktLength;
}

int copyFile(fstream& const fs, char* const buffer) {
	int oriPos = fs.tellg();
	fs.seekg(0, ios::end);	//g: get, seekg�����Ѿ���Ҫ���ж�ȡ���ļ�
							//������ƫ��������ʼλ��
	int fileLength = fs.tellg();
	fs.seekg(0);
	fs.read(buffer, fileLength);
	fs.clear();
	/*int nLines = countNLines(fs);
	fileLength -= nLines;
	fileLength++;*/
	fs.seekg(oriPos);
	return fileLength;
}

int countNLines(fstream& fs) {
	int nLines = 0;
	string s;
	int oriPos = fs.tellg();
	fs.seekg(0);
	while (!fs.eof()) {
		getline(fs, s);
		nLines++;
	}
	fs.clear(); //���������λ
	fs.seekg(oriPos);
	return nLines;
}

//todo
vector<string> parseAccountAndPwd(const char* pkt, const int length) {
	
}

ContentType parseContentType(const char* pkt, const int length) {
	string filePath = parseFilePath(pkt, length);
	if (filePath.size() > 5 && filePath.substr(filePath.size() - 5, filePath.size()) == ".html") {
		return ContentType::HTML;
	}
	//todo ͼƬ����
}

string parseFilePath(const char* pkt, const int length) {
	int i = 0;
	string p(pkt);
	string head = p.substr(0, 4);
	if (head == "POST") i = 5;
	else if (head == "GET ") i = 4;
	int count = 0;
	int urlStartIndex = i;
	while (pkt[i] != ' ') {
		i++;
		count++;
	}
	string URL = p.substr(urlStartIndex, count);

	return URL;
}

RequestType parseRequestType(const char* pkt, const int length) {
	string p(pkt);
	string head = p.substr(0, 4);
	if (head == "POST") return RequestType::POST;
	else if (head == "GET ") return RequestType::GET;
	else return RequestType::OTHER;
}

bool sendFile(const SOCKET& sServer, fstream& fs) {
	char* sendBuf = new char[BUFFER_SIZE];
	int pktLength = constructPkt("404", "text/html", fs, sendBuf);
	//�������ͻ���
	int ret = send(sServer, sendBuf, pktLength, 0);
	delete[] sendBuf;
	if (ret == SOCKET_ERROR) return false;
	else return true;
}