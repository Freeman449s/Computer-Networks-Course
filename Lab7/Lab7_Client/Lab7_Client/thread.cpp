#include"thread.h"

using namespace std;

void Receive(SOCKET& socket)
{
	int ret;
	char* Recv = new char[LENGTH];
	while (true)
	{
		if (Connect_flag == 1) {
			ret = recv(socket, Recv, LENGTH, 0);
			if (ret > 0)
			{
				char tmp = Recv[5];
				if (tmp == 'T' || tmp == 't')
				{
					char* RecvTime = Recv;
					int year = (int)*(unsigned short*)(RecvTime + 6);
					int month = (int)*(unsigned short*)(RecvTime + 8);
					int day = (int)*(unsigned short*)(RecvTime + 10);
					int hour = (int)*(unsigned short*)(RecvTime + 12);
					int minute = (int)*(unsigned short*)(RecvTime + 14);
					int second = (int)*(unsigned short*)(RecvTime + 16);
					cout << "* Present time: " << year << "/" << month << "/" << day << " ";
					if (hour < 10) cout << "0";
					cout << hour << ":";
					if (minute < 10)cout << "0";
					cout << minute << ":";
					if (second < 10)cout << "0";
					cout << second << endl;
				}

				else if (tmp == 'N' || tmp == 'n')
				{
					char* RecvName = Recv;
					int Pack_Length = *(RecvName + 1);
					char* Name = new char[Pack_Length - 6];
					memcpy(Name, RecvName + 6, Pack_Length - 7);
					Name[Pack_Length - 7] = '\0';
					cout << "* Server name: " << Name << "." << endl;
				}

				else if (tmp == 'L' || tmp == 'l')
				{
					char* RecvList = Recv;
					int count = 1;
					char* ptr = RecvList + 6;
					int Total = (int)(*ptr);
					int Untreated_Num = Total;
					if (Total == 1) cout << "* Only one client has connected:" << endl;
					else if (Total > 1) cout <<"* " << Total << " clients has connected:" << endl;
					else cout << "ERROR : Number of clients less than 0!" << endl;
					ptr += 4;
					while (Total > 0)
					{
						char* IP_Begin = ptr + 4;
						char* IP_End = IP_Begin;
						while (*IP_End != ':') IP_End++;
						char* IP_NUM = new char[IP_End - IP_Begin + 1];
						memcpy(IP_NUM, IP_Begin, IP_End - IP_Begin);
						IP_NUM[IP_End - IP_Begin] = '\0';
						ptr = IP_End + 1;
						int PORT_NUM = (int)*(unsigned short*)ptr;
						cout << "* [" << count << "] IP:" << IP_NUM << " PORT:" << PORT_NUM << endl;
						ptr += 3;
						count++;
						Total--;
					}
				}
				else if (tmp == 'F' || tmp == 'f')
				{
					char* RecvSMsg = Recv;
					int Re_flag = (int)*(unsigned short*)(RecvSMsg + 6);
					if (Re_flag == 0) cout << "Message sent successfully!" << endl;
					else if (Re_flag == 1) cout << "Failed to send the message. The opposite host might have connection problems." << endl;
					else if (Re_flag == 2) cout << "The opposite client is not connected." << endl;
					else cout << "ERROR : Re_flag parameter error!" << endl;
				}
				else if (tmp == 'M' || tmp == 'm')
				{
					char* RecvMessage = Recv;
					char* ptr = RecvMessage + 6;
					char* IP_Begin = ptr;
					char* IP_End = IP_Begin;
					while (*IP_End != ':') IP_End++;
					char* IP_NUM = new char[IP_End - IP_Begin + 1];
					memcpy(IP_NUM, IP_Begin, IP_End - IP_Begin);
					IP_NUM[IP_End - IP_Begin] = '\0';
					ptr = IP_End + 1;
					int PORT_NUM = (int)*(unsigned short*)ptr;
					ptr += 2;
					char* MSG_Begin = ptr;
					char* MSG_End = MSG_Begin;
					while (*MSG_End != '$') MSG_End++;
					char* MSG = new char[MSG_End - MSG_Begin + 1];
					memcpy(MSG, MSG_Begin, MSG_End - MSG_Begin);
					MSG[MSG_End - MSG_Begin] = '\0';
					cout << "***************************************************" << endl;
					cout << "* Received message from " << IP_NUM << ":" << endl;
					cout << "* " << MSG << endl;
					cout << "***************************************************" << endl;
				}
				tmp = 'P';
			}
		}
	}
}