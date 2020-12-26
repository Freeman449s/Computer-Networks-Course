#include <cstdlib>
#include <fstream>
#include "Util.h"

void appendInt(string& str, const int i) {
	char* intCStr = new char[11];
	itoa(i, intCStr, 10);
	string intStr(intCStr);
	str.append(intStr);
}

void constructPkt(const string& const status, const string& const contentType, fstream& const fs, char* sendBuf) {
	fs.seekg(0, ios::end);	//g: get, seekg用于已经打开要进行读取的文件
							//参数：偏离量，起始位置
	int fileLength = fs.tellg();

	//填充头部
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
	memcpy(sendBuf, firstLine.c_str(), firstLine.size());
	sendBuf += firstLine.size();
	memcpy(sendBuf, typeLine.c_str(), typeLine.size());
	sendBuf += typeLine.size();
	memcpy(sendBuf, lengthLine.c_str(), lengthLine.size());
	sendBuf += lengthLine.size();
	memcpy(sendBuf, "\n", 1);
	sendBuf++;

	//向缓冲区写入文件
	copyFile(fs, sendBuf);
}

int copyFile(fstream& const fs, char* const buffer) {
	int oriPos = fs.tellg();
	fs.seekg(0, ios::end);
	int fileLength = fs.tellg();
	fs.seekg(oriPos);

	fs.read(buffer, fileLength);
	return fileLength;
}

//todo
string parseFilePath(const char* pkt, const int length) {

}

//todo
RequestType parseRequestType(const char* pkt, const int length) {

}