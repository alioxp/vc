/**************************************************************************
*   Copyright (C) 2005 by Achal Dhir                                      *
*   achaldhir@gmail.com                                                   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
// TFTPServer.cpp

#ifdef _MSC_VER
   #define strcasecmp _stricmp
   #define _CRT_SECURE_NO_WARNINGS
   #pragma comment(lib, "ws2_32.lib")
   #pragma comment(lib, "iphlpapi.lib")
#endif

//Constants
#define my_inet_addr inet_addr
#define MAX_SERVERS 8

//Structs
struct home
{
	char alias[64];
	char target[MAX_PATH];
};

struct tftpConnType
{
	SOCKET sock;
	SOCKADDR_IN addr;
	DWORD server;
	WORD port;
};

struct acknowledgement
{
	WORD opcode;
	WORD block;
};

struct message
{
	WORD opcode;
	char buffer[514];
};

struct packet
{
	WORD opcode;
	WORD block;
	char buffer;
};

struct tftperror
{
	WORD opcode;
	WORD errorcode;
	char errormessage[508];
};

struct data12
{
	DWORD rangeStart;
	DWORD rangeEnd;
};

struct request
{
	char mapname[24];
	BYTE opcode;
	BYTE attempt;
	BYTE sockInd;
	time_t expiry;
	char path[MAX_PATH];
	FILE *file;
	char *filename;
	char *mode;
	DWORD tsize;
	DWORD blksize;
	DWORD timeout;
	DWORD fblock;
	WORD block;
	WORD tblock;
	int bytesRecd;
	int bytesReady;
	sockaddr_in client;
	socklen_t clientsize;
	packet* pkt[2];
	WORD bytesRead[2];
	union
	{
		acknowledgement acout;
		message mesout;
		tftperror serverError;
	};
};

struct data2
{
	WSADATA wsaData;
	tftpConnType tftpConn[MAX_SERVERS];
	WORD ports[MAX_SERVERS];
	DWORD servers[MAX_SERVERS];
	home homes[8];
	data12 hostRanges[32];
	char fileRead;
	char fileWrite;
	char fileOverwrite;
	FILE *logfile;
	int minport;
	int maxport;
	SOCKET maxFD;
	BYTE logLevel;
};

struct data15
{
	union
	{
		//DWORD ip;
		unsigned ip:32;
		BYTE octate[4];
	};
};

//Functions
void runProg();
void init();
char* myGetToken(char*, BYTE);
bool getSection(char*, char*, BYTE, char*);
char* myLower(char*);
char* myUpper(char*);
char* IP2String(char*, DWORD);
void cleanReq(request*);
void printWindowsError();
void logMess(request*, BYTE);
void logMess(char*, BYTE);
int processNew(request *req);
int processSend(request *req);
int processRecv(request *req);
