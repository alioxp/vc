// udplisten.cpp : 定义控制台应用程序的入口点。
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
	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	sin.sin_port = ::ntohs(30000);
	if(::bind(s,(sockaddr*)&sin,sizeof(sin)) == SOCKET_ERROR)
	{
		DWORD dwErr = GetLastError();
		printf("bind() failed \n");
		return 0;
	}
	printf("开始接收30000");
	SOCKADDR_IN addrRemote;
	int nLen = sizeof(addrRemote);
	char sz[256];
	while(1)
	{
		int nRet = ::recvfrom(s,sz,256,0,(sockaddr*)&addrRemote,&nLen);
		if(nRet>0)
		{
			sz[nRet]= '\0';
			printf(sz);
		}
	}
	return 0;
}

