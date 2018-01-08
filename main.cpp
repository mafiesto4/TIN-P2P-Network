
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
			if (arg0.size() > 0)
			{
				port = std::stoi(arg0);
			}
			Service::Instance.Start(port);
		}
		else if(ParseCommandArg0(input, "stop"))
		{
			Service::Instance.Stop();
		}
		else if (ParseCommandArg0(input, "nodes"))
		{
			if (Service::Instance.IsRunning())
			{
				std::vector<Node*> nodes;
				Service::Instance.GetNodes(&nodes);
				cout << "P2P network nodes:" << endl;
				for (auto& node : nodes)
					cout << node->GetName() << " - " << node->GetAddressName() << ":" << ntohs(node->GetAddress().sin_port) << endl;
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
