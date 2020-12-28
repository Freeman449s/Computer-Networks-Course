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
	memcpy(sendPtr, firstLine.c_str(), firstLine.size());
	sendPtr += firstLine.size();
	memcpy(sendPtr, typeLine.c_str(), typeLine.size());
	sendPtr += typeLine.size();
	memcpy(sendPtr, lengthLine.c_str(), lengthLine.size());
	sendPtr += lengthLine.size();
	memcpy(sendPtr, "\n", 1);
	sendPtr++;

	//向缓冲区写入文件
	copyFile(fs, sendPtr);
	sendPtr += fileLength;
	pktLength = sendPtr - sendBuf;
	return pktLength;
}

int copyFile(fstream& const fs, char* const buffer) {
	int oriPos = fs.tellg();
	fs.seekg(0, ios::end);	//g: get, seekg用于已经打开要进行读取的文件
							//参数：偏离量，起始位置
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
	fs.clear(); //清除错误标记位
	fs.seekg(oriPos);
	return nLines;
}

vector<string> parseAccountAndPwd(const char* pkt, const int length) {
	string p(pkt);
	string flag;
	string::size_type positon_account;
	string::size_type positon_password;
	vector<string> res;

	positon_account = p.find("name=\"account\"");
	if (positon_account != p.npos)
	{
		positon_account += 18;
		int account_length = 0;
		while (pkt[positon_account + account_length] != '\r')  account_length++;
		string account = p.substr(positon_account, account_length);
		res.push_back(account);
	}
	else res.push_back("");

	positon_password = p.find("name=\"pwd\"");
	if (positon_password != p.npos)
	{
		positon_password += 14;
		int password_length = 0;
		while (pkt[positon_password + password_length] != '\r')  password_length++;
		string password = p.substr(positon_password, password_length);
		res.push_back(password);
	}
	else res.push_back("");

	return res;
}

ContentType parseContentType(const char* pkt, const int length) {
	string filePath = parseFilePath(pkt, length);
	if (filePath == "/") return ContentType::HTML;
	if (filePath.size() > 5 && filePath.substr(filePath.size() - 5, 5) == ".html") {
		return ContentType::HTML;
	}
	if (filePath.size() > 4 && filePath.substr(filePath.size() - 4, 4) == ".txt") {
		return ContentType::TEXT;
	}
	if (filePath.size() > 4 && filePath.substr(filePath.size() - 4, 4) == ".jpg") {
		return ContentType::JPG;
	}
	return ContentType::OTHER;
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

bool sendFile(const SOCKET& sServer, fstream& fs, string status, string conType) {
	char* sendBuf = new char[BUFFER_SIZE];
	int pktLength = constructPkt(status, conType, fs, sendBuf);
	//发送至客户端
	int ret = send(sServer, sendBuf, pktLength, 0);
	delete[] sendBuf;
	if (ret == SOCKET_ERROR) return false;
	else return true;
}