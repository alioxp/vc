// udpsend.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "winsock.h"

int _tmain(int argc, _TCHAR* argv[])
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested =MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );

	SOCKET s = ::socket(AF_INET,SOCK_DGRAM,0);
	bool bBroadcast = true;
	::setsockopt(s,SOL_SOCKET,SO_BROADCAST,(char*)&bBroadcast,sizeof(bool));
	SOCKADDR_IN bcast;
	bcast.sin_family = AF_INET;
	bcast.sin_addr.S_un.S_addr = INADDR_BROADCAST;
	bcast.sin_port = htons(30000);
	printf("开始发送");
	char sz[] = "test \r\n";
	//while(1)
	{
		::sendto(s,sz,strlen(sz),0,(sockaddr*)&bcast,sizeof(bcast));
	}

	return 0;
}

