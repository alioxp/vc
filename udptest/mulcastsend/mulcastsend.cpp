// mulcastsend.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "winsock.h"
#pragma pack(1) // 将对齐模数设置为1  
//#define MCAST_PORT 30000
#define MCAST_PORT 5353
//#define MCAST_ADDR "224.0.1.88" /*一个局部连接多播地址，路由器不进行转发*/
#define MCAST_ADDR "224.0.0.251"
#define MCAST_DATA "BROADCAST TEST DATA"  // 多播内容
#define MCAST_INTERVAL 5  //发送时间间隔
#define BUFF_SIZE 256   //接收缓冲区大小
//char namebuf[15] = { 0x11,0x44,0x4f,0x4e,
//					0x47,0x4c,0x45,0x2d,
//					0x31,0x42,0x46,0x45,
//					0x45,0x38,0x2d,0x49,
//					0x54,0x56,0xc0,0x36};
char namebuf[15] = { 0x07,0x61,0x70,0x70,
					 0x6c,0x65,0x74,0x76,
					 0x05,0x6c,0x6f,0x63,
					 0x61,0x6c,0x00};
char addbuf[14] = {0x00,0x01,0x80,0x01,
						0x0,0,0,0x78,
						0,0x4,0xc0,0xa8,
						0x1,0x5};
struct dns
{
	short head;
	short flag;
	short qd;
	short an;
	short ns;
	short ar;
	unsigned char nb[15];
	unsigned char ab[14];
	dns()
	{
		head = 0;
		flag = 0x84;
		qd = 0;
		an = 0x100;
		ns = ar = 0;
		int s = sizeof(namebuf);
		memcpy(nb,namebuf,sizeof(namebuf));
		memcpy(ab,addbuf,sizeof(addbuf));
	}
	

};

using namespace std;
bool setTtl(SOCKET s,int nttl)
{
	int nRet = ::setsockopt(s,IPPROTO_IP,IP_MULTICAST_TTL,(char*)&nttl,sizeof(nttl));
	return nRet != SOCKET_ERROR;
}
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

	setTtl(s,1);

	SOCKADDR_IN bcast;
	bcast.sin_family = AF_INET;
	bcast.sin_addr.S_un.S_addr = inet_addr(MCAST_ADDR);
	bcast.sin_port = htons(MCAST_PORT);
	printf("开始发送");
	char sz[] = "test \r\n";

	dns d1;
	//while(1)
	{
		//::sendto(s,sz,strlen(sz),0,(sockaddr*)&bcast,sizeof(bcast));
		::sendto(s,(const char*)&d1,sizeof(d1),0,(sockaddr*)&bcast,sizeof(bcast));
	}

	return 0;
}
