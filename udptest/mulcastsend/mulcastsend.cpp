// mulcastsend.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "winsock.h"

#define MCAST_PORT 30000
#define MCAST_ADDR "224.0.1.88" /*һ���ֲ����Ӷಥ��ַ��·����������ת��*/
#define MCAST_DATA "BROADCAST TEST DATA"  // �ಥ����
#define MCAST_INTERVAL 5  //����ʱ����
#define BUFF_SIZE 256   //���ջ�������С

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
	printf("��ʼ����");
	char sz[] = "test \r\n";
	//while(1)
	{
		::sendto(s,sz,strlen(sz),0,(sockaddr*)&bcast,sizeof(bcast));
	}

	return 0;
}
