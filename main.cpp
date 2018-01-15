
#include <iostream>
#include "Service.h"
#include <string>

using namespace std;

bool ParseCommandArg0(const string& str, const char* cmd)
{
	return str == cmd;
}

bool ParseCommandArg1(const string& str, const char* cmd, string& arg0)
{
	arg0 = "";
	size_t cmdLen = strlen(cmd);
	if (str.size() > cmdLen && memcmp(str.c_str(), cmd, cmdLen) == 0)
	{
		const char* ptr = str.c_str() + cmdLen + 1;
		arg0 = ptr;
		return true;
	}

	return false;
}

bool ParseCommandArg2(const string& str, const char* cmd, string& arg0, string& arg1)
{
	arg0 = "";
	arg1 = "";
	size_t cmdLen = strlen(cmd);
	if (str.size() > cmdLen && memcmp(str.c_str(), cmd, cmdLen) == 0)
	{
		const char* ptr = str.c_str() + cmdLen + 1;
		arg0 = ptr;
		const string::size_type t = arg0.find(' ');
		if (t != string::npos)
		{
			arg1 = arg0.substr(t + 1);
			arg0 = arg0.substr(0, t);
			return true;
		}
	}

	return false;
}

int main()
{
#if _WIN32
	{
		// Initialize Winsock
		WSADATA wsaData = { 0 };
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0)
		{
			wprintf(L"WSAStartup failed: %d\n", result);
			return 1;
		}
	}
#endif

	string arg0;
	string arg1;

	while(true)
	{
		string input;
		getline(cin, input);

		// Handle input
		if(ParseCommandArg0(input, "q" )|| ParseCommandArg0(input, "quit"))
		{
			if (Service::Instance.IsRunning())
				Service::Instance.Stop();
			break;
		}
		else if(ParseCommandArg0(input, "start") || ParseCommandArg1(input, "start", arg0))
		{
			ushort port = DEFAULT_MSG_PORT;
			if (!arg0.empty())
			{
				char *endptr;
				const long val = strtol(arg0.c_str(), &endptr, 10);
				if (*endptr == '\0')
				{
					port = (ushort)val;
				}
				else
				{
					cout << "Invalid port number" << endl;
					continue;
				}
			}
			Service::Instance.Start(port);
		}
		else if(ParseCommandArg0(input, "stop"))
		{
			Service::Instance.Stop();
		}
		else if(ParseCommandArg1(input, "ping", arg0))
		{
			Service::Instance.Ping(arg0);
		}
		else if (ParseCommandArg1(input, "add", arg0))
		{
			Service::Instance.AddFile(arg0);
		}
		else if (ParseCommandArg1(input, "remove", arg0))
		{
			// TODO: implement removing files
			cout << "Not implemented" << endl;
		}
		else if (ParseCommandArg2(input, "get", arg0, arg1) || ParseCommandArg1(input, "get", arg0))
		{
			// TODO: implement getting files
			cout << "Not implemented" << endl;
		}
		else if (ParseCommandArg0(input, "list") || ParseCommandArg1(input, "list", arg0))
		{
			Service::Instance.ListFiles(arg0);
		}
		else if (ParseCommandArg0(input, "nodes"))
		{
			if (Service::Instance.IsRunning())
			{
				std::vector<Node*> nodes;
				Service::Instance.GetNodes(&nodes);
				cout << "P2P network nodes:" << endl;
				for (auto& node : nodes)
					cout << node << endl;
			}
			else
			{
				cout << "Service not running. No nodes." << endl;
			}
		}
		else
		{
			cout << "Unknown command" << endl;
		}
	}

#ifdef WIN32
	WSACleanup();
#endif

    return 0;
}
