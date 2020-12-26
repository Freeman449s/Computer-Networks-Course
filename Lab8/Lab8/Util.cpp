#include <cstdlib>
#include "Util.h"

void appendInt(string& str, const int i) {
	char* intCStr = new char[11];
	itoa(i, intCStr, 10);
	string intStr(intCStr);
	str.append(intStr);
}