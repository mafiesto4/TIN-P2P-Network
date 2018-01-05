
#include <iostream>
#include "Service.h"
#include <string>

using namespace std;

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

	while(true)
	{
		string input;
		cin >> input;

		// Handle input
		if(input == "q" || input == "quit")
		{
			if (Service::Instance.IsRunning())
				Service::Instance.Stop();
			break;
		}
		else if(input == "start")
		{
			Service::Instance.Start();
		}
		else if(input == "stop")
		{
			Service::Instance.Stop();
		}
	}

    return 0;
}
