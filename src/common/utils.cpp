#include "utils.h"
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <stdarg.h>
#endif
#include <chrono>
#include <thread>
#include <sstream>

using namespace std;

uint32_t ip_string_to_int(const string& in, bool* const success)
{
	uint32_t ret = 0;
	const bool _success = (1 == inet_pton(AF_INET, in.c_str(), &ret));

	if (success)
	{
		*success = _success;
	}
	else if (!_success)
	{
		char buf[200] = { 0 };
	}

	return ret;
}

string ip_to_string(uint32_t in, bool* const success)
{
	string ret(INET_ADDRSTRLEN, '\0');
	const bool _success = (NULL != inet_ntop(AF_INET, &in, &ret[0], ret.size()));

	if (success)
	{
		*success = _success;
	}

	if (_success)
	{
		ret.pop_back(); // remove null-terminator required by inet_ntop
	}
	else if (!success)
	{
		char buf[200] = { 0 };
	}

	return ret;
}

bool isNumber(const string& str)
{
	char* ptr;
	strtol(str.c_str(), &ptr, 10);
	return *ptr == '\0';
}

bool yesOrNo(float probabilityOfYes)
{
	return rand() % 100 < probabilityOfYes;
}

bool yesOrNo(int probabilityOfYes)
{
	return rand() % 100 < probabilityOfYes;
}

const char* FormatSeconds(int seconds)
{
	static char string[64];

	int hours = 0;
	int minutes = seconds / 60;

	if (minutes > 0)
	{
		seconds -= (minutes * 60);
		hours = minutes / 60;

		if (hours > 0)
		{
			minutes -= (hours * 60);
		}
	}

	if (hours > 0)
	{
		snprintf(string, sizeof(string), "%2i:%02i:%02i", hours, minutes, seconds);
	}
	else
	{
		snprintf(string, sizeof(string), "%02i:%02i", minutes, seconds);
	}

	return string;
}

char* va(const char* format, ...)
{
	va_list argptr;
	static char string[8][2048];
	static int curstring = 0;

	curstring = (curstring + 1) % 8;

	va_start(argptr, format);
	vsnprintf(string[curstring], sizeof(string[curstring]), format, argptr);
	va_end(argptr);

	return string[curstring];
}

const char* WSAGetLastErrorString()
{
#ifdef WIN32
	int error = WSAGetLastError();
	static char msgbuf[256];	// for a message up to 255 bytes.
	msgbuf[0] = '\0';	// Microsoft doesn't guarantee this on man page.

	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,	// flags
		NULL,			// lpsource
		error,			// message id
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),	// languageid
		msgbuf,			// output buffer
		sizeof(msgbuf),	// size of msgbuf, bytes
		NULL);			// va_list of arguments

	if (!*msgbuf)
		sprintf(msgbuf, "%d", error);  // provide error # if no string available

	return msgbuf;
#else
	return strerror(errno);
#endif
}

vector<string> deserialize_array_str(string const& csv)
{
	istringstream parse(csv);
	vector<string> ret;
	for (string token; getline(parse, token, ','); ret.push_back(token));
	return ret;
}

vector<int> deserialize_array_int(string const& csv)
{
	istringstream parse(csv);
	vector<int> ret;
	for (string token; getline(parse, token, ','); ret.push_back(stoi(token)));
	return ret;
}

vector<unsigned char> deserialize_array_uchar(string const& csv)
{
	istringstream parse(csv);
	vector<unsigned char> ret;
	for (string token; getline(parse, token, ','); ret.push_back(stoi(token)));
	return ret;
}

string serialize_array_str(const vector<string>& arr)
{
	ostringstream cat;
	for (size_t index = 0; index < arr.size(); ++index)
		cat << arr[index] << ',';
	string ret = cat.str();
	return ret.substr(0, ret.size() - 1);
}

string serialize_array_int(const vector<int>& arr)
{
	ostringstream cat;
	for (size_t index = 0; index < arr.size(); ++index)
		cat << arr[index] << ',';
	string ret = cat.str();
	return ret.substr(0, ret.size() - 1);
}

string serialize_array_uchar(const vector<unsigned char>& arr)
{
	ostringstream cat;
	for (size_t index = 0; index < arr.size(); ++index)
		cat << arr[index] << ',';
	string ret = cat.str();
	return ret.substr(0, ret.size() - 1);
}

size_t findCaseInsensitive(string data, string toSearch, size_t pos)
{
	// Convert complete given String to lower case
	transform(data.begin(), data.end(), data.begin(), ::tolower);
	// Convert complete given Sub String to lower case
	transform(toSearch.begin(), toSearch.end(), toSearch.begin(), ::tolower);
	// Find sub string in given string
	return data.find(toSearch, pos);
}

size_t findCaseInsensitive(string data, const vector<string>& toSearch, size_t pos)
{
	for (auto str : toSearch)
	{
		if (findCaseInsensitive(data, str) != string::npos)
			return 1;
	}

	return 0;
}

vector<string> ParseArguments(const string& str)
{
	stringstream iss(str);
	vector<string> args((istream_iterator<string>(iss)), istream_iterator<string>());

	return args;
}

char* build_number(void);
inline static char* ServerBuild()
{
	return va("Server build: %s", build_number());
}

void SleepMS(unsigned int ms)
{
	this_thread::sleep_for(chrono::milliseconds(ms));
}

int GetNetworkError()
{
#ifdef WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}
