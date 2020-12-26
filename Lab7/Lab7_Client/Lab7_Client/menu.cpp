#include<iostream>
#include<string>
using namespace std;

void Initialize()
{
	cout << "Welcome, what would you like to do now?" << endl;
	cout << "a : Connect              b : Disconnect" << endl;
	cout << "c : Get Present Time     d : Get Server Name" << endl;
	cout << "e : Get Client List      f : Send A Message" << endl;
	cout << "g : Quit" << endl;

	return;
}
char Input()
{
	string str;
	cin >> str;
	char Enter = getchar();
	while (str.length() != 1)
	{
		cout << "Invalid Input : One Character Only!" << endl;
		cin >> str;
	}
	const char* ch_array = str.c_str();
	char ch = ch_array[0];

	return ch;
}
bool BeginQuit()
{
	cout << "You are exiting. Confirm ?" << endl;
	cout << "Y or N" << endl;
	char ch;
	cin >> ch;
	while (ch)
	{
		if (ch == 'Y' || ch == 'y') return true;
		else if (ch == 'N' || ch == 'n') return false;
		else cout << "Invalid Input!" << endl;
		cin >> ch;
	}
}