// udplisten.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "winsock.h"

#define MCAST_PORT 30000
#define MCAST_ADDR "224.0.1.88" /*一个局部连接多播地址，路由器不进行转发*/
#define MCAST_DATA "BROADCAST TEST DATA"  // 多播内容
#define MCAST_INTERVAL 5  //发送时间间隔
//#define BUFF_SIZE 256   //接收缓冲区大小
#define UDP_BIG_PKG (1024*32)

int _tmain(int argc, _TCHAR* argv[])
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested =MAKEWORD( 1, 1 );
	err = WSAStartup( wVersionRequested, &wsaData );

	SOCKET s = ::socket(AF_INET,SOCK_DGRAM,0);

	bool bReuse = true;
	::setsockopt(s,SOL_SOCKET, SO_REUSEADDR,(char*)&bReuse,sizeof(bool));

	int nRecvBuf=UDP_BIG_PKG;//设置为32K
	setsockopt (s,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
	//发送缓冲区
	int nSendBuf=UDP_BIG_PKG;//设置为32K
	setsockopt (s,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));

	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	sin.sin_port = ::htons(MCAST_PORT);
	if(::bind(s,(sockaddr*)&sin,sizeof(sin)) == SOCKET_ERROR)
	{
		DWORD dwErr = GetLastError();
		printf("bind() failed \n");
		return 0;
	}

	/*设置回环许可*/
	int loop=0;
	err=setsockopt(s,IPPROTO_IP,IP_MULTICAST_LOOP,(const char*)&loop,sizeof(loop));
	//if(err<0)
	//{
	//		printf("set sock error");
	//		return -3;
	//}
	struct ip_mreq mreq;/*加入广播组*/
	mreq.imr_multiaddr.s_addr=inet_addr(MCAST_ADDR);//广播地址
	mreq.imr_interface.s_addr=INADDR_ANY; //网络接口为默认
	/*将本机加入广播组*/
	err=setsockopt(s,IPPROTO_IP,IP_ADD_MEMBERSHIP,(const char*)&mreq,sizeof(mreq));
	if(err<0)
	{
		DWORD dwErr = GetLastError();
		printf("set sock error");
		return -4;
	}

	int count = 0;
	printf("开始接收30000");
	SOCKADDR_IN addrRemote;
	int nLen = sizeof(addrRemote);
	char sz[1024*64];
	
	while(1)
	{
		int nRet = ::recvfrom(s,sz,UDP_BIG_PKG*2,0,(sockaddr*)&addrRemote,&nLen);
		if(nRet>0)
		{
			//sz[nRet]= '\0';
			printf("count:%d src:%d\n",count,sz[0]);
		}

		//char sz[] = "test\0";
		//while(1)
		/*{
			SOCKADDR_IN bcast;
			bcast.sin_family = AF_INET;
			bcast.sin_addr.S_un.S_addr = inet_addr(MCAST_ADDR);
			bcast.sin_port = htons(MCAST_PORT);
			::sendto(s,sz,512,0,(sockaddr*)&bcast,sizeof(bcast));
		}*/
		count ++;
	}
	return 0;
}

