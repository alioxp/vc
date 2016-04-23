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

#include <stdio.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <time.h>
#include <tchar.h>
#include <limits.h>
#include <iphlpapi.h>
#include <string>
#include <map>
using namespace std;
#include "TFTPServerSP.h"
#pragma warning(disable:4996) 

#define USE_BROADCAST_UDP true

#define MCAST_ADDR "224.0.1.88"
const int UDP_PORT = 30000;
//types
typedef map<string, request*> myMap;
typedef multimap<long, request*> myMultiMap;

//Global Variables
myMap tftpCache;
myMultiMap tftpAge;
bool verbatim = false;
char serviceName[] = "TFTPServer";
char displayName[] = "TFTP Server Single Port";
char sVersion[] = "TFTP Server SinglePort Version 1.60 Windows Built 1601";
char iniFile[MAX_PATH];
char logFile[MAX_PATH];
char fileSep = '\\';
char notFileSep = '/';
DWORD blksize = 65464;
DWORD timeout = 3;
data2 cfig;
packet* datain;
char tempbuff[256];
char logBuff[512];


//Service Variables
SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
HANDLE stopServiceEvent = 0;

void WINAPI ServiceControlHandler(DWORD controlCode)
{
	switch (controlCode)
	{
		case SERVICE_CONTROL_INTERROGATE:
			break;

		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus(serviceStatusHandle, &serviceStatus);
			SetEvent(stopServiceEvent);
			return ;

		case SERVICE_CONTROL_PAUSE:
			break;

		case SERVICE_CONTROL_CONTINUE:
			break;

		default:
			if (controlCode >= 128 && controlCode <= UCHAR_MAX)
				break;
			else
				break;
	}

	SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

void WINAPI ServiceMain(DWORD /*argc*/, TCHAR* /*argv*/[])
{
	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_STOPPED;
	serviceStatus.dwControlsAccepted = 0;
	serviceStatus.dwWin32ExitCode = NO_ERROR;
	serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;
	serviceStatusHandle = RegisterServiceCtrlHandler(serviceName, ServiceControlHandler);

	if (serviceStatusHandle)
	{
		serviceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);

		//Initialize
		verbatim = false;
		init();
		datain = (packet*)calloc(1, blksize + 4);

		if (!datain)
		{
			sprintf(logBuff, "Memory Error");
			logMess(logBuff, 0);
			exit(1);
		}

		timeval tv;
		fd_set readfds;
		request req;
		int fdsReady = 0;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		stopServiceEvent = CreateEvent(0, FALSE, FALSE, 0);
		serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		serviceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);

		do
		{
			FD_ZERO(&readfds);

			for (int i = 0; i < MAX_SERVERS && cfig.tftpConn[i].port; i++)
				FD_SET(cfig.tftpConn[i].sock, &readfds);

			fdsReady = select(cfig.maxFD, &readfds, NULL, NULL, &tv);

			//if (errno)
			//	printf("%s\n", strerror(errno));

			for (int i = 0; fdsReady > 0 && i < MAX_SERVERS && cfig.tftpConn[i].port; i++)
			{
				if (FD_ISSET(cfig.tftpConn[i].sock, &readfds))
				{
					fdsReady--;
					memset(&req, 0, sizeof(request));
					memset(datain, 0, blksize + 4);
					req.clientsize = sizeof(req.client);
					req.sockInd = i;
					errno = 0;
					req.bytesRecd = recvfrom(cfig.tftpConn[req.sockInd].sock, (char*)datain, blksize + 4, 0, (sockaddr*)&req.client, &req.clientsize);

					if (req.bytesRecd < 4 || errno)
						continue;

					//printf("%u=%u\n", req.bytesRecd, blksize + 4);
					sprintf(req.mapname, "%s:%u", inet_ntoa(req.client.sin_addr), ntohs(req.client.sin_port));
					request *req1 = tftpCache[req.mapname];

					if (!req1)
						tftpCache.erase(req.mapname);

					//printf("%u\n",req1);

					if (req1)
					{
						req1->bytesRecd = req.bytesRecd;

						if (ntohs(datain->opcode) == 3 && req1->opcode == 2)
						{
							if ((WORD)req1->bytesRecd  <= req1->blksize + 4)
							{
								if (req1->attempt <= 3)
								{
									req1->tblock = req1->block + 1;

									if (ntohs(datain->block) == req1->tblock)
									{
										req1->block = req1->tblock;
										req1->fblock++;
										req1->attempt = 0;
										req1->acout.opcode = htons(4);
										req1->acout.block = ntohs(req1->block);
										processRecv(req1);
									}
								}
							}
							else
							{
								req1->serverError.opcode = htons(5);
								req1->serverError.errorcode = htons(4);
								sendto(cfig.tftpConn[req1->sockInd].sock, (const char*)&req1->serverError, strlen(req1->serverError.errormessage) + 5, 0, (sockaddr*)&req1->client, req1->clientsize);
								sprintf(req1->serverError.errormessage, "Error: Incoming Packet too large");
								logMess(req1, 1);
								cleanReq(req1);
							}
						}
						else if (ntohs(datain->opcode) == 4 && req1->opcode == 1)
						{
							if (req1->bytesRecd == 4)
							{
								if (req1->attempt <= 3)
								{
									if (ntohs(datain->block) == req1->block)
									{
										req1->block++;
										req1->fblock++;
										req1->attempt = 0;
										processSend(req1);
									}
								}
							}
						}
						else if (req1->bytesRecd > (int)sizeof(message))
						{
							req1->serverError.opcode = htons(5);
							req1->serverError.errorcode = htons(4);
							sendto(cfig.tftpConn[req1->sockInd].sock, (const char*)&req1->serverError, strlen(req1->serverError.errormessage) + 5, 0, (sockaddr*)&req1->client, req1->clientsize);
							sprintf(req1->serverError.errormessage, "Error: Incoming Packet too large");
							logMess(req1, 1);
							cleanReq(req1);
						}
						else if (ntohs(datain->opcode) == 1 || ntohs(datain->opcode) == 2)
						{
							cleanReq(req1);
							if (!processNew(&req))
							{
								memcpy(req1, &req, sizeof(request));
							}
						}
						else if (ntohs(datain->opcode) == 5)
						{
							sprintf(req1->serverError.errormessage, "Error %i at Client, %s", ntohs(datain->block), &datain->buffer);
							logMess(req1, 1);
							cleanReq(req1);
						}
						else
						{
							req1->serverError.opcode = htons(5);
							req1->serverError.errorcode = htons(4);
							sprintf(req1->serverError.errormessage, "Unexpected Option Code %u", ntohs(datain->opcode));
							sendto(cfig.tftpConn[req1->sockInd].sock, (const char*)&req1->serverError, strlen(req1->serverError.errormessage) + 5, 0, (sockaddr*)&req1->client, req1->clientsize);
							logMess(req1, 1);
							cleanReq(req1);
						}
					}
					else if (req.bytesRecd > (int)sizeof(message))
					{
						req.serverError.opcode = htons(5);
						req.serverError.errorcode = htons(4);
						sprintf(req.serverError.errormessage, "Error: Incoming Packet too large");
						sendto(cfig.tftpConn[i].sock, (const char*)&req.serverError, strlen(req.serverError.errormessage) + 5, 0, (sockaddr*)&req.client, req.clientsize);
						logMess(&req, 1);
					}
					else if (ntohs(datain->opcode) == 5)
					{
						sprintf(req.serverError.errormessage, "Error %i at Client, %s", ntohs(datain->block), &datain->buffer);
						logMess(&req, 1);
					}
					else if (ntohs(datain->opcode) != 1 && ntohs(datain->opcode) != 2)
					{
						req.serverError.opcode = htons(5);
						req.serverError.errorcode = htons(5);
						sprintf(req.serverError.errormessage, "Unknown transfer ID");
						sendto(cfig.tftpConn[i].sock, (const char*)&req.serverError, strlen(req.serverError.errormessage) + 5, 0, (sockaddr*)&req.client, req.clientsize);
						logMess(&req, 1);
					}
					else if (!processNew(&req))
					{
						request *req1 = (request*)calloc(1, sizeof(request));

						if (!req1)
						{
							sprintf(logBuff,"Memory Error");
							logMess(logBuff, 1);
							continue;
						}

						memcpy(req1, &req, sizeof(request));
						tftpCache[req1->mapname] = req1;
						tftpAge.insert(pair<long, request*>(req1->expiry, req1));
					}
				}
			}

			myMultiMap::iterator p = tftpAge.begin();
			myMultiMap::iterator q;
			time_t currentTime = time(NULL);
			request *req;

			while (p != tftpAge.end())
			{
				req = p->second;

				if (p->first > currentTime)
					break;
				else if (p->first < req->expiry && req->expiry > currentTime)
				{
					q = p;
					p++;
					tftpAge.erase(q);
					tftpAge.insert(pair<long, request*>(req->expiry, req));
				}
				else if (req->expiry <= currentTime && req->attempt >= 3)
				{
					if (req->attempt < UCHAR_MAX)
					{
						req->serverError.opcode = htons(5);
						req->serverError.errorcode = htons(0);

						if (req->fblock && !req->block)
							strcpy(req->serverError.errormessage, "Large File, Block# Rollover not supported by Client");
						else
							strcpy(req->serverError.errormessage, "Timeout");

						sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
						logMess(req, 1);
					}

					q = p;
					p++;
					tftpAge.erase(q);
					tftpCache.erase(req->mapname);
					cleanReq(req);
					free(req);
				}
				else if (req->expiry <= currentTime)
				{
					if (ntohs(req->acout.opcode) == 3)
					{
						if (processSend(req))
							cleanReq(req);
						else
						{
							req->attempt++;
							req->expiry = currentTime + req->timeout;
						}
					}
					else
					{
						errno = 0;
						sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->acout, req->bytesReady, 0, (sockaddr*)&req->client, req->clientsize);
						errno = WSAGetLastError();

						if (errno)
							cleanReq(req);
						else
						{
							req->attempt++;
							req->expiry = currentTime + req->timeout;
						}
					}
					p++;
				}
				else
					p++;
			}
		}
		while (WaitForSingleObject(stopServiceEvent, 0) == WAIT_TIMEOUT);

		if (cfig.logfile)
			fclose(cfig.logfile);

		serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);

		sprintf(logBuff, "Closing Network Connections...");
		logMess(logBuff, 1);

		for (int i = 0; i < MAX_SERVERS && cfig.tftpConn[i].port; i++)
			closesocket(cfig.tftpConn[i].sock);

		WSACleanup();

		sprintf(logBuff, "TFTP Server Stopped !");
		logMess(logBuff, 1);

		serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(serviceStatusHandle, &serviceStatus);

		CloseHandle(stopServiceEvent);
		stopServiceEvent = 0;
	}
}

void runService()
{
	SERVICE_TABLE_ENTRY serviceTable[] =
	    {
	        {serviceName, ServiceMain},
	        {0, 0}
	    };

	StartServiceCtrlDispatcher(serviceTable);
}

bool stopService(SC_HANDLE service)
{
	if (service)
	{
		SERVICE_STATUS serviceStatus;
		QueryServiceStatus(service, &serviceStatus);
		if (serviceStatus.dwCurrentState != SERVICE_STOPPED)
		{
			ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus);
			printf("Stopping Service.");
			for (int i = 0; i < 100; i++)
			{
				QueryServiceStatus(service, &serviceStatus);
				if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
				{
					printf("Stopped\n");
					return true;
				}
				else
				{
					Sleep(500);
					printf(".");
				}
			}
			printf("Failed\n");
			return false;
		}
	}
	return true;
}

void installService()
{
	SC_HANDLE serviceControlManager = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);

	if (serviceControlManager)
	{
		SC_HANDLE service = OpenService(serviceControlManager,
		                                serviceName, SERVICE_QUERY_STATUS);
		if (service)
		{
			printf("Service Already Exists..\n");
			StartService(service,0,NULL);
			CloseServiceHandle(service);
		}
		else
		{
			TCHAR path[ _MAX_PATH + 1 ];
			if (GetModuleFileName(0, path, sizeof(path) / sizeof(path[0])) > 0)
			{
				SC_HANDLE service = CreateService(serviceControlManager,
				                                  serviceName, displayName,
				                                  SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
				                                  SERVICE_AUTO_START, SERVICE_ERROR_IGNORE, path,
				                                  0, 0, 0, 0, 0);
				if (service)
				{
					printf("Successfully installed.. !\n");
					StartService(service,0,NULL);
					CloseServiceHandle(service);
				}
				else
					printf("Installation Failed..\n");
			}
		}
		CloseServiceHandle(serviceControlManager);
	}
	else
		printWindowsError();
}

void uninstallService()
{
	SC_HANDLE serviceControlManager = OpenSCManager(0, 0, SC_MANAGER_CONNECT);

	if (serviceControlManager)
	{
		SC_HANDLE service = OpenService(serviceControlManager,
		                                serviceName, SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE);
		if (service)
		{
			if (stopService(service))
			{
				DeleteService(service);
				printf("Successfully Removed !\n");
			}
			else
				printf("Failed to Stop Service..\n");

			CloseServiceHandle(service);
		}

		CloseServiceHandle(serviceControlManager);
	}
	else
		printWindowsError();
}

void printWindowsError()
{
	DWORD dw = GetLastError();

	if (dw)
	{
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

		printf("Error: %s\nPress Enter..\n", lpMsgBuf);
		getchar();
	}
}

int main(int argc, TCHAR* argv[])
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	bool result = GetVersionEx(&osvi);

	if (result && osvi.dwPlatformId >= VER_PLATFORM_WIN32_NT)
	{
		if (argc > 1 && lstrcmpi(argv[1], TEXT("-i")) == 0)
			installService();
		else if (argc > 1 && lstrcmpi(argv[1], TEXT("-u")) == 0)
			uninstallService();
		else if (argc > 1 && lstrcmpi(argv[1], TEXT("-v")) == 0)
		{
			SC_HANDLE serviceControlManager = OpenSCManager(0, 0, SC_MANAGER_CONNECT);
			bool serviceStopped = true;

			if (serviceControlManager)
			{
				SC_HANDLE service = OpenService(serviceControlManager,
				                                serviceName, SERVICE_QUERY_STATUS | SERVICE_STOP);
				if (service)
				{
					serviceStopped = stopService(service);
					CloseServiceHandle(service);
				}
				CloseServiceHandle(serviceControlManager);
			}
			else
				printWindowsError();

			if (serviceStopped)
				runProg();
			else
				printf("Failed to Stop Service\n");
		}
		else
			runService();
	}
	else if (argc == 1 || lstrcmpi(argv[1], TEXT("-v")) == 0)
		runProg();
	else
		printf("This option is not available on Windows95/98/ME\n");

	return 0;
}

void runProg()
{
	//Initialize
	verbatim = true;
	init();

	datain = (packet*)calloc(1, blksize + 4);

	if (!datain)
	{
		sprintf(logBuff, "Memory Error");
		logMess(logBuff, 0);
		exit(1);
	}

	timeval tv;
	fd_set readfds;
	request req;
	int fdsReady = 0;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	printf("\nAccepting requests..\n");

	do
	{
		FD_ZERO(&readfds);

		for (int i = 0; i < MAX_SERVERS && cfig.tftpConn[i].port; i++)
			FD_SET(cfig.tftpConn[i].sock, &readfds);

		fdsReady = select(cfig.maxFD, &readfds, NULL, NULL, &tv);

		//if (errno)
		//	printf("%s\n", strerror(errno));

		for (int i = 0; fdsReady > 0 && i < MAX_SERVERS && cfig.tftpConn[i].port; i++)
		{
			if (FD_ISSET(cfig.tftpConn[i].sock, &readfds))
			{
				fdsReady--;
				memset(&req, 0, sizeof(request));
				memset(datain, 0, blksize + 4);
				req.clientsize = sizeof(req.client);
				req.sockInd = i;
				errno = 0;
				req.bytesRecd = recvfrom(cfig.tftpConn[req.sockInd].sock, (char*)datain, blksize + 4, 0, (sockaddr*)&req.client, &req.clientsize);

#ifdef USE_BROADCAST_UDP
				req.client.sin_addr.S_un.S_addr = inet_addr(MCAST_ADDR); 
				req.client.sin_port = htons(UDP_PORT);
#endif
				if (req.bytesRecd < 4 || errno)
					continue;

				//printf("%u=%u\n", req.bytesRecd, blksize + 4);
				sprintf(req.mapname, "%s:%u", inet_ntoa(req.client.sin_addr), ntohs(req.client.sin_port));
				request *req1 = tftpCache[req.mapname];

				if (!req1)
					tftpCache.erase(req.mapname);

				//printf("%u\n",req1);

				if (req1)
				{
					req1->bytesRecd = req.bytesRecd;

					if (ntohs(datain->opcode) == 3 && req1->opcode == 2)
					{
						if ((WORD)req1->bytesRecd  <= req1->blksize + 4)
						{
							if (req1->attempt <= 3)
							{
								req1->tblock = req1->block + 1;

								if (ntohs(datain->block) == req1->tblock)
								{
									req1->block = req1->tblock;
									req1->fblock++;
									req1->attempt = 0;
									req1->acout.opcode = htons(4);
									req1->acout.block = ntohs(req1->block);
									processRecv(req1);
								}
							}
						}
						else
						{
							req1->serverError.opcode = htons(5);
							req1->serverError.errorcode = htons(4);
							sendto(cfig.tftpConn[req1->sockInd].sock, (const char*)&req1->serverError, strlen(req1->serverError.errormessage) + 5, 0, (sockaddr*)&req1->client, req1->clientsize);
							sprintf(req1->serverError.errormessage, "Error: Incoming Packet too large");
							logMess(req1, 1);
							cleanReq(req1);
						}
					}
					else if (ntohs(datain->opcode) == 4 && req1->opcode == 1)
					{
						if (req1->bytesRecd == 4)
						{
							if (req1->attempt <= 3)
							{
								if (ntohs(datain->block) == req1->block)
								{
									req1->block++;
									req1->fblock++;
									req1->attempt = 0;
									processSend(req1);
								}
							}
						}
					}
					else if (req1->bytesRecd > (int)sizeof(message))
					{
						req1->serverError.opcode = htons(5);
						req1->serverError.errorcode = htons(4);
						sendto(cfig.tftpConn[req1->sockInd].sock, (const char*)&req1->serverError, strlen(req1->serverError.errormessage) + 5, 0, (sockaddr*)&req1->client, req1->clientsize);
						sprintf(req1->serverError.errormessage, "Error: Incoming Packet too large");
						logMess(req1, 1);
						cleanReq(req1);
					}
					else if (ntohs(datain->opcode) == 1 || ntohs(datain->opcode) == 2)
					{
						cleanReq(req1);
						if (!processNew(&req))
						{
							memcpy(req1, &req, sizeof(request));
						}
					}
					else if (ntohs(datain->opcode) == 5)
					{
						sprintf(req1->serverError.errormessage, "Error %i at Client, %s", ntohs(datain->block), &datain->buffer);
						logMess(req1, 1);
						cleanReq(req1);
					}
					else
					{
						req1->serverError.opcode = htons(5);
						req1->serverError.errorcode = htons(4);
						sprintf(req1->serverError.errormessage, "Unexpected Option Code %u", ntohs(datain->opcode));
						sendto(cfig.tftpConn[req1->sockInd].sock, (const char*)&req1->serverError, strlen(req1->serverError.errormessage) + 5, 0, (sockaddr*)&req1->client, req1->clientsize);
						logMess(req1, 1);
						cleanReq(req1);
					}
				}
				else if (req.bytesRecd > (int)sizeof(message))
				{
					req.serverError.opcode = htons(5);
					req.serverError.errorcode = htons(4);
					sprintf(req.serverError.errormessage, "Error: Incoming Packet too large");
					sendto(cfig.tftpConn[i].sock, (const char*)&req.serverError, strlen(req.serverError.errormessage) + 5, 0, (sockaddr*)&req.client, req.clientsize);
					logMess(&req, 1);
				}
				else if (ntohs(datain->opcode) == 5)
				{
					sprintf(req.serverError.errormessage, "Error %i at Client, %s", ntohs(datain->block), &datain->buffer);
					logMess(&req, 1);
				}
				else if (ntohs(datain->opcode) != 1 && ntohs(datain->opcode) != 2)
				{
					req.serverError.opcode = htons(5);
					req.serverError.errorcode = htons(5);
					sprintf(req.serverError.errormessage, "Unknown transfer ID");
					sendto(cfig.tftpConn[i].sock, (const char*)&req.serverError, strlen(req.serverError.errormessage) + 5, 0, (sockaddr*)&req.client, req.clientsize);
					logMess(&req, 1);
				}
				else if (!processNew(&req))
				{
					request *req1 = (request*)calloc(1, sizeof(request));

					if (!req1)
					{
						sprintf(logBuff,"Memory Error");
						logMess(logBuff, 1);
						continue;
					}

					memcpy(req1, &req, sizeof(request));
					tftpCache[req1->mapname] = req1;
					tftpAge.insert(pair<long, request*>(req1->expiry, req1));
				}
			}
		}

		myMultiMap::iterator p = tftpAge.begin();
		myMultiMap::iterator q;
		time_t currentTime = time(NULL);
		request *req;

		while (p != tftpAge.end())
		{
			req = p->second;

			if (p->first > currentTime)
				break;
			else if (p->first < req->expiry && req->expiry > currentTime)
			{
				q = p;
				p++;
				tftpAge.erase(q);
				tftpAge.insert(pair<long, request*>(req->expiry, req));
			}
			else if (req->expiry <= currentTime && req->attempt >= 3)
			{
				if (req->attempt < UCHAR_MAX)
				{
					req->serverError.opcode = htons(5);
					req->serverError.errorcode = htons(0);

					if (req->fblock && !req->block)
						strcpy(req->serverError.errormessage, "Large File, Block# Rollover not supported by Client");
					else
						strcpy(req->serverError.errormessage, "Timeout");

					sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
					logMess(req, 1);
				}

				q = p;
				p++;
				tftpAge.erase(q);
				tftpCache.erase(req->mapname);
				cleanReq(req);
				free(req);
			}
			else if (req->expiry <= currentTime)
			{
				if (ntohs(req->acout.opcode) == 3)
				{
					if (processSend(req))
						cleanReq(req);
					else
					{
						req->attempt++;
						req->expiry = currentTime + req->timeout;
					}
				}
				else
				{
					errno = 0;
					sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->acout, req->bytesReady, 0, (sockaddr*)&req->client, req->clientsize);
					errno = WSAGetLastError();

					if (errno)
						cleanReq(req);
					else
					{
						req->attempt++;
						req->expiry = currentTime + req->timeout;
					}
				}
				p++;
			}
			else
				p++;
		}
	}
	while (true);

	for (int i = 0; i < MAX_SERVERS && cfig.tftpConn[i].port; i++)
		closesocket(cfig.tftpConn[i].sock);

	WSACleanup();
}

int processNew(request *req)
{
	if (cfig.hostRanges[0].rangeStart)
	{
		DWORD iip = ntohl(req->client.sin_addr.s_addr);
		BYTE allowed = false;

		for (WORD j = 0; j <= sizeof(cfig.hostRanges) && cfig.hostRanges[j].rangeStart; j++)
		{
			if (iip >= cfig.hostRanges[j].rangeStart && iip <= cfig.hostRanges[j].rangeEnd)
			{
				allowed = true;
				break;
			}
		}

		if (!allowed)
		{
			req->serverError.opcode = htons(5);
			req->serverError.errorcode = htons(2);
			strcpy(req->serverError.errormessage, "Access Denied");
			sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
			logMess(req, 1);
			cleanReq(req);
			return 2;
		}
	}

	req->block = 0;
	req->blksize = 512;
	req->timeout = timeout;
	req->expiry = time(NULL) + req->timeout;
	req->opcode = ntohs(datain->opcode);
	char *inPtr = (char*)datain;
	*(inPtr + (req->bytesRecd - 1)) = 0;
	inPtr += 2;
	req->filename = inPtr;

	if (!strlen(req->filename) || strlen(req->filename) > UCHAR_MAX)
	{
		req->serverError.opcode = htons(5);
		req->serverError.errorcode = htons(0);
		strcpy(req->serverError.errormessage, "Malformed Request, Invalid/Missing Filename");
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
		logMess(req, 1);
		cleanReq(req);
		return 1;
	}

	inPtr += strlen(inPtr) + 1;
	req->mode = inPtr;

	if (!strlen(req->mode) || strlen(req->mode) > 25)
	{
		req->serverError.opcode = htons(5);
		req->serverError.errorcode = htons(0);
		strcpy(req->serverError.errormessage, "Malformed Request, Invalid/Missing Mode");
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
		logMess(req, 1);
		cleanReq(req);
		return 1;
	}

	inPtr += strlen(inPtr) + 1;

	for (WORD i = 0; i < strlen(req->filename); i++)
		if (req->filename[i] == notFileSep)
			req->filename[i] = fileSep;

	tempbuff[0] = '.';
	tempbuff[1] = '.';
	tempbuff[2] = fileSep;
	tempbuff[3] = 0;

	if (strstr(req->filename, tempbuff))
	{
		req->serverError.opcode = htons(5);
		req->serverError.errorcode = htons(2);
		strcpy(req->serverError.errormessage, "Access violation");
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
		logMess(req, 1);
		cleanReq(req);
		return 1;
	}

	if (req->filename[0] == fileSep)
		req->filename++;

	if (!cfig.homes[0].alias[0])
	{
		if (strlen(cfig.homes[0].target) + strlen(req->filename) >= sizeof(req->path))
		{
			req->serverError.opcode = htons(5);
			req->serverError.errorcode = htons(2);
			strcpy(req->serverError.errormessage, "Filename too large");
			sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
			logMess(req, 1);
			cleanReq(req);
			return 1;
		}
		strcpy(req->path, cfig.homes[0].target);
		strcat(req->path, req->filename);
	}
	else
	{
		char *bname = strchr(req->filename, fileSep);

		if (bname)
		{
			*bname = 0;
			bname++;
		}
		else
		{
			req->serverError.opcode = htons(5);
			req->serverError.errorcode = htons(2);
			sprintf(req->serverError.errormessage, "Missing directory/alias");
			sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
			logMess(req, 1);
			cleanReq(req);
			return 1;
		}

		for (int i = 0; i < 8; i++)
		{
			if (cfig.homes[i].alias[0] && !strcasecmp(req->filename, cfig.homes[i].alias))
			{
				if (strlen(cfig.homes[i].target) + strlen(bname) >= sizeof(req->path))
				{
					req->serverError.opcode = htons(5);
					req->serverError.errorcode = htons(2);
					strcpy(req->serverError.errormessage, "Filename too large");
					sendto(cfig.tftpConn[req->sockInd].sock, (const char*) &req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
					logMess(req, 1);
					cleanReq(req);
					return 1;
				}

				strcpy(req->path, cfig.homes[i].target);
				strcat(req->path, bname);
				break;
			}
			else if (i == 7 || !cfig.homes[i].alias[0])
			{
				req->serverError.opcode = htons(5);
				req->serverError.errorcode = htons(2);
				sprintf(req->serverError.errormessage, "No such directory/alias %s", req->filename);
				sendto(cfig.tftpConn[req->sockInd].sock, (const char*) &req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
				logMess(req, 1);
				cleanReq(req);
				return 1;
			}
		}
	}

    if (ntohs(datain->opcode) == 1)
    {
		if (!cfig.fileRead)
		{
            req->serverError.opcode = htons(5);
            req->serverError.errorcode = htons(2);
            strcpy(req->serverError.errormessage, "GET Access Denied");
            sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
            logMess(req, 1);
            cleanReq(req);
            return 1;
        }

        errno = 0;

		if (*inPtr)
		{
			char *tmp = inPtr;

			while (*tmp)
			{
				if (!strcasecmp(tmp, "blksize"))
				{
					tmp += strlen(tmp) + 1;
					DWORD val = atol(tmp);

					if (val < 512)
						val = 512;
					else if (val > blksize)
						val = blksize;

					req->blksize = val;
					break;
				}

				tmp += strlen(tmp) + 1;
			}
		}

		if (!strcasecmp(req->mode, "netascii") || !strcasecmp(req->mode, "ascii"))
			req->file = fopen(req->path, "rt");
		else
			req->file = fopen(req->path, "rb");

		if (errno || !req->file)
		{
            req->serverError.opcode = htons(5);
            req->serverError.errorcode = htons(1);
            strcpy(req->serverError.errormessage, "No Such File/No Access");
            sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
            logMess(req, 1);
            cleanReq(req);
            return 1;
        }
		setvbuf(req->file, NULL, _IOFBF, req->blksize);
    }
    else
    {
		if (!cfig.fileWrite && !cfig.fileOverwrite)
		{
            req->serverError.opcode = htons(5);
            req->serverError.errorcode = htons(2);
            strcpy(req->serverError.errormessage, "PUT Access Denied");
            sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
            logMess(req, 1);
            cleanReq(req);
            return 1;
        }

		req->file = fopen(req->path, "rb");

		if (req->file)
		{
			fclose(req->file);
			req->file = NULL;

			if (!cfig.fileOverwrite)
			{
                req->serverError.opcode = htons(5);
                req->serverError.errorcode = htons(6);
                strcpy(req->serverError.errormessage, "File already exists");
                sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
                logMess(req, 1);
                cleanReq(req);
                return 1;
            }
        }
		else if (!cfig.fileWrite)
		{
			req->serverError.opcode = htons(5);
			req->serverError.errorcode = htons(2);
			strcpy(req->serverError.errormessage, "Create File Access Denied");
			sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
			logMess(req, 1);
			cleanReq(req);
			return 1;
		}

		errno = 0;

		if (strcasecmp(req->mode, "netascii") && strcasecmp(req->mode, "ascii"))
			req->file = fopen(req->path, "wb");
		else
			req->file = fopen(req->path, "wt");

		if (errno || !req->file)
		{
			req->serverError.opcode = htons(5);
			req->serverError.errorcode = htons(1);
			strcpy(req->serverError.errormessage, "Invalid Path or No Access");
			sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
			logMess(req, 1);
			cleanReq(req);
			return 1;
		}
	}

	if (*inPtr)
	{
		char *outPtr = req->mesout.buffer;
		req->mesout.opcode = htons(6);
		DWORD val;
		while (*inPtr)
		{
			//printf("%s", inPtr);
			if (!strcasecmp(inPtr, "blksize"))
			{
				strcpy(outPtr, inPtr);
				outPtr += strlen(outPtr) + 1;
				inPtr += strlen(inPtr) + 1;
				val = atol(inPtr);

				if (val < 512)
					val = 512;
				else if (val > blksize)
					val = blksize;

				req->blksize = val;
				sprintf(outPtr, "%u", val);
				outPtr += strlen(outPtr) + 1;
			}
			else if (!strcasecmp(inPtr, "tsize"))
			{
				strcpy(outPtr, inPtr);
				outPtr += strlen(outPtr) + 1;
				inPtr += strlen(inPtr) + 1;

				if (ntohs(datain->opcode) == 1)
				{
					if (!fseek(req->file, 0, SEEK_END))
					{
						if (ftell(req->file) >= 0)
						{
							req->tsize = ftell(req->file);
							sprintf(outPtr, "%u", req->tsize);
							outPtr += strlen(outPtr) + 1;
						}
						else
						{
							req->serverError.opcode = htons(5);
							req->serverError.errorcode = htons(2);
							strcpy(req->serverError.errormessage, "Invalid Path or No Access");
							sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
							logMess(req, 1);
							req->attempt = UCHAR_MAX;
							break;
						}
					}
					else
					{
						req->serverError.opcode = htons(5);
						req->serverError.errorcode = htons(2);
						strcpy(req->serverError.errormessage, "Invalid Path or No Access");
						sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
						logMess(req, 1);
						req->attempt = UCHAR_MAX;
						break;
					}
				}
				else
				{
					req->tsize = 0;
					sprintf(outPtr, "%u", req->tsize);
					outPtr += strlen(outPtr) + 1;
				}
			}
			else if (!strcasecmp(inPtr, "timeout"))
			{
				strcpy(outPtr, inPtr);
				outPtr += strlen(outPtr) + 1;
				inPtr += strlen(inPtr) + 1;
				val = atoi(inPtr);

				if (val < 1)
					val = 1;
				else if (val > UCHAR_MAX)
					val = UCHAR_MAX;

				req->timeout = val;
				req->expiry = time(NULL) + req->timeout;
				sprintf(outPtr, "%u", val);
				outPtr += strlen(outPtr) + 1;
			}

			inPtr += strlen(inPtr) + 1;

			//printf("=%u\n", val);
		}

		errno = 0;
		req->bytesReady = (DWORD)outPtr - (DWORD)&req->acout;
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->acout, req->bytesReady, 0, (sockaddr*)&req->client, req->clientsize);
		errno = WSAGetLastError();
	}
	else if (htons(datain->opcode) == 2)
	{
		req->acout.opcode = htons(4);
		req->acout.block = htons(0);
		errno = 0;
		req->bytesReady = 4;
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->acout, req->bytesReady, 0, (sockaddr*)&req->client, req->clientsize);
		errno = WSAGetLastError();
	}

	if (errno)
	{
		sprintf(req->serverError.errormessage, "Communication Error");
		logMess(req, 1);
		cleanReq(req);
		return errno;
	}

	if (ntohs(datain->opcode) == 1)
	{
		errno = 0;
		req->pkt[0] = (packet*)calloc(1, req->blksize + 4);
		req->pkt[1] = (packet*)calloc(1, req->blksize + 4);

		if (errno || !req->pkt[0] || !req->pkt[1])
		{
			strcpy(req->serverError.errormessage, "Memory Error");
			logMess(req, 1);
			cleanReq(req);
			return 1;
		}

		long ftellLoc = ftell(req->file);

		if (ftellLoc > 0)
		{
			if (fseek(req->file, 0, SEEK_SET))
			{
				req->serverError.opcode = htons(5);
				req->serverError.errorcode = htons(4);
				strcpy(req->serverError.errormessage, "Invalid Path or No Access");
				sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
				logMess(req, 1);
				cleanReq(req);
				return errno;
			}
		}
		else if (ftellLoc < 0)
		{
			req->serverError.opcode = htons(5);
			req->serverError.errorcode = htons(4);
			strcpy(req->serverError.errormessage, "Invalid Path or No Access");
			sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
			logMess(req, 1);
			cleanReq(req);
			return errno;
		}

		req->pkt[0]->opcode = htons(3);
		req->pkt[0]->block = htons(1);
		req->bytesRead[0] = fread(&req->pkt[0]->buffer, 1, req->blksize, req->file);

		if (errno)
		{
			req->serverError.opcode = htons(5);
			req->serverError.errorcode = htons(4);
			strcpy(req->serverError.errormessage, "Invalid Path or No Access");
			sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
			logMess(req, 1);
			cleanReq(req);
			return errno;
		}

		if (req->bytesRead[0] == req->blksize)
		{
			req->pkt[1]->opcode = htons(3);
			req->pkt[1]->block = htons(2);
			req->bytesRead[1] = fread(&req->pkt[1]->buffer, 1, req->blksize, req->file);

			if (errno)
			{
				req->serverError.opcode = htons(5);
				req->serverError.errorcode = htons(4);
				strcpy(req->serverError.errormessage, "Invalid Path or No Access");
				sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
				logMess(req, 1);
				cleanReq(req);
				return errno;
			}

			if (req->bytesRead[1] < req->blksize)
			{
				fclose(req->file);
				req->file = 0;
			}
		}
		else
		{
			fclose(req->file);
			req->file = 0;
		}

		if (!req->bytesReady)
		{
			req->block = 1;
			return processSend(req);
		}
	}
	return 0;
}

int processSend(request *req)
{
	errno = 0;
	req->expiry = time(NULL) + req->timeout;

	if (ntohs(req->pkt[0]->block) == req->block)
	{
		errno = 0;
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)req->pkt[0], req->bytesRead[0] + 4, 0, (sockaddr*)&req->client, req->clientsize);
		memcpy(&req->acout, req->pkt[0], 4);
		errno = WSAGetLastError();

		if (errno)
		{
			sprintf(req->serverError.errormessage, "Communication Error");
			logMess(req, 1);
			cleanReq(req);
			return errno;
		}

		if (req->file)
		{
			req->tblock = ntohs(req->pkt[1]->block) + 1;
			if (req->tblock == req->block)
			{
				req->pkt[1]->block = htons(++req->tblock);
				req->bytesRead[1] = fread(&req->pkt[1]->buffer, 1, req->blksize, req->file);

				if (errno)
				{
					req->serverError.opcode = htons(5);
					req->serverError.errorcode = htons(4);
					sprintf(req->serverError.errormessage, strerror(errno));
					sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
					logMess(req, 1);
					cleanReq(req);
					return errno;
				}
				else if (req->bytesRead[1] < req->blksize)
				{
					fclose(req->file);
					req->file = 0;
				}
			}
		}
	}
	else if (ntohs(req->pkt[1]->block) == req->block)
	{
		errno = 0;
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)req->pkt[1], req->bytesRead[1] + 4, 0, (sockaddr*)&req->client, req->clientsize);
		memcpy(&req->acout, req->pkt[1], 4);
		errno = WSAGetLastError();

		if (errno)
		{
			sprintf(req->serverError.errormessage, "Communication Error");
			logMess(req, 1);
			cleanReq(req);
			return errno;
		}

		if (req->file)
		{
			req->tblock = ntohs(req->pkt[0]->block) + 1;
			if (req->tblock == req->block)
			{
				req->pkt[0]->block = htons(++req->tblock);
				req->bytesRead[0] = fread(&req->pkt[0]->buffer, 1, req->blksize, req->file);

				if (errno)
				{
					req->serverError.opcode = htons(5);
					req->serverError.errorcode = htons(4);
					sprintf(req->serverError.errormessage, strerror(errno));
					sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
					logMess(req, 1);
					cleanReq(req);
					return errno;
				}
				else if (req->bytesRead[0] < req->blksize)
				{
					fclose(req->file);
					req->file = 0;
				}
			}
		}
	}
	else //if (ntohs(req->pkt[0]->block) < req->block && ntohs(req->pkt[1]->block) < req->block)
	{
		sprintf(logBuff, "Client %s %s, %i Blocks Served", req->mapname, req->path, req->fblock - 1);
		logMess(logBuff, 2);
		cleanReq(req);
	}

	return 0;
}

int processRecv(request *req)
{
	req->expiry = time(NULL) + req->timeout;
	errno = 0;
	req->bytesReady = 4;
	sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->acout, req->bytesReady, 0, (sockaddr*)&req->client, req->clientsize);
	errno = WSAGetLastError();
	if (errno)
	{
		sprintf(req->serverError.errormessage, "Communication Error");
		logMess(req, 1);
		cleanReq(req);
		return errno;
	}

	if (req->bytesRecd > 4 && (fwrite(&datain->buffer, req->bytesRecd - 4, 1, req->file) != 1 || errno))
	{
		req->serverError.opcode = htons(5);
		req->serverError.errorcode = htons(3);
		strcpy(req->serverError.errormessage, "Disk full or allocation exceeded");
		sendto(cfig.tftpConn[req->sockInd].sock, (const char*)&req->serverError, strlen(req->serverError.errormessage) + 5, 0, (sockaddr*)&req->client, req->clientsize);
		logMess(req, 1);
		cleanReq(req);
		return 1;
	}

	//printf("%u\n", req->bytesRecd);

	if ((WORD)req->bytesRecd < req->blksize + 4)
	{
		sprintf(logBuff, "Client %s %s, %u Blocks Received", req->mapname, req->path, req->fblock);
		logMess(logBuff, 2);
		cleanReq(req);
	}

	return 0;
}

void cleanReq(request *req)
{
	if (req->file)
		fclose(req->file);

	if (req->pkt[0])
		free(req->pkt[0]);

	if (req->pkt[1])
		free(req->pkt[1]);

	req->file = 0;
	req->pkt[0] = 0;
	req->pkt[1] = 0;
	req->attempt = UCHAR_MAX;

}

char* myGetToken(char* buff, BYTE index)
{
	while (*buff)
	{
		if (index)
			index--;
		else
			break;

		buff += strlen(buff) + 1;
	}

	return buff;
}

WORD myTokenize(char *target, char *source, char *sep, bool whiteSep)
{
	bool found = true;
	char *dp = target;
	WORD kount = 0;

	while (*source)
	{
		if (sep && sep[0] && strchr(sep, (*source)))
		{
			found = true;
			source++;
			continue;
		}
		else if (whiteSep && *source <= 32)
		{
			found = true;
			source++;
			continue;
		}

		if (found)
		{
			if (target != dp)
			{
				*dp = 0;
				dp++;
			}
			kount++;
		}

		found = false;
		*dp = *source;
		dp++;
		source++;
	}

	*dp = 0;
	dp++;
	*dp = 0;

	//printf("%s\n", target);

	return kount;
}

char* myTrim(char *target, char *source)
{
	while ((*source) && (*source) <= 32)
		source++;

	int i = 0;

	for (; i < 511 && source[i]; i++)
		target[i] = source[i];

	target[i] = source[i];
	i--;

	for (; i >= 0 && target[i] <= 32; i--)
		target[i] = 0;

	return target;
}

void mySplit(char *name, char *value, char *source, char splitChar)
{
	int i = 0;
	int j = 0;
	int k = 0;

	for (; source[i] && j <= 510 && source[i] != splitChar; i++, j++)
	{
		name[j] = source[i];
	}

	if (source[i])
	{
		i++;
		for (; k <= 510 && source[i]; i++, k++)
		{
			value[k] = source[i];
		}
	}

	name[j] = 0;
	value[k] = 0;

	myTrim(name, name);
	myTrim(value, value);
	//printf("%s %s\n", name, value);
}

bool getSection(char *sectionName, char *buffer, BYTE serial, char *fileName)
{
	//printf("%s=%s\n",fileName,sectionName);
	char section[128];
	sprintf(section, "[%s]", sectionName);
	myUpper(section);
	FILE *f = fopen(fileName, "rt");
	char buff[512];
	BYTE found = 0;

	if (f)
	{
		while (fgets(buff, 511, f))
		{
			myUpper(buff);
			myTrim(buff, buff);

			if (strstr(buff, section) == buff)
			{
				found++;
				if (found == serial)
				{
					//printf("%s=%s\n",fileName,sectionName);
					while (fgets(buff, 511, f))
					{
						myTrim(buff, buff);

						if (strstr(buff, "[") == buff)
							break;

						if ((*buff) >= '0' && (*buff) <= '9' || (*buff) >= 'A' && (*buff) <= 'Z' || (*buff) >= 'a' && (*buff) <= 'z' || ((*buff) && strchr("/\\?*", (*buff))))
						{
							buffer += sprintf(buffer, "%s", buff);
							buffer++;
						}
					}
					break;
				}
			}
		}
		fclose(f);
	}

	*buffer = 0;
	*(buffer + 1) = 0;
	return (found == serial);
}

char *IP2String(char *target, DWORD ip)
{
	data15 inaddr;
	inaddr.ip = ip;
	sprintf(target, "%u.%u.%u.%u", inaddr.octate[0], inaddr.octate[1], inaddr.octate[2], inaddr.octate[3]);
	return target;
}

char *myUpper(char *string)
{
	char diff = 'a' - 'A';

	WORD len = strlen(string);

	for (int i = 0; i < len; i++)
		if (string[i] >= 'a' && string[i] <= 'z')
			string[i] -= diff;

	return string;
}

char *myLower(char *string)
{
	char diff = 'a' - 'A';

	WORD len = strlen(string);

	for (int i = 0; i < len; i++)
		if (string[i] >= 'A' && string[i] <= 'Z')
			string[i] += diff;

	return string;
}

void init()
{
	memset(&cfig, 0, sizeof(cfig));

	GetModuleFileName(NULL, iniFile, MAX_PATH);
	char *iniFileExt = strrchr(iniFile, '.');
	strcpy(iniFileExt, ".ini");
	GetModuleFileName(NULL, logFile, MAX_PATH);
	iniFileExt = strrchr(logFile, '.');
	strcpy(iniFileExt, ".log");

	char iniStr[4096];
	char name[256];
	char value[256];

	if (verbatim)
	{
		cfig.logLevel = 2;
		printf("%s\n\n", sVersion);
	}
	else if (getSection("LOGGING", iniStr, 1, iniFile))
	{
		char *iniStrPtr = myGetToken(iniStr, 0);

		if (!iniStrPtr[0] || !strcasecmp(iniStrPtr, "None"))
			cfig.logLevel = 0;
		else if (!strcasecmp(iniStrPtr, "Errors"))
			cfig.logLevel = 1;
		else if (!strcasecmp(iniStrPtr, "All"))
			cfig.logLevel = 2;
		else if (!strcasecmp(iniStrPtr, "Debug"))
			cfig.logLevel = 3;
		else
			cfig.logLevel = 255;
	}

	if (!verbatim && cfig.logLevel)
	{
		cfig.logfile = fopen(logFile, "wt");

		if (cfig.logfile)
		{
			fclose(cfig.logfile);
			cfig.logfile = fopen(logFile, "at");
			fprintf(cfig.logfile, "%s\n", sVersion);
		}
	}

	if (cfig.logLevel == UCHAR_MAX)
	{
		cfig.logLevel = 1;
		sprintf(logBuff, "Section [LOGGING], Invalid Logging Level: %s, ignored", myGetToken(iniStr, 0));
		logMess(logBuff, 1);
	}

	WORD wVersionRequested = MAKEWORD(1, 1);
	WSAStartup(wVersionRequested, &cfig.wsaData);

	if (cfig.wsaData.wVersion != wVersionRequested)
	{
		sprintf(logBuff, "WSAStartup Error");
		logMess(logBuff, 1);
	}

	if (getSection("LISTEN-ON", iniStr, 1, iniFile))
	{
		char *iniStrPtr = myGetToken(iniStr, 0);

		for (int i = 0; i < MAX_SERVERS && iniStrPtr[0]; iniStrPtr = myGetToken(iniStrPtr, 1))
		{
			char name[256];
			strncpy(name, iniStrPtr, UCHAR_MAX);
			WORD port = 69;
			char *dp = strchr(name, ':');
			if (dp)
			{
				*dp = 0;
				dp++;
				port = atoi(dp);
			}

			DWORD ip = my_inet_addr(name);

			if (ip)
			{
				for (BYTE j = 0; j < MAX_SERVERS; j++)
				{
					if (cfig.servers[j] == ip)
						break;
					else if (!cfig.servers[j])
					{
						cfig.servers[j] = ip;
						cfig.ports[j] = port;
						i++;
						break;
					}
				}
			}
			else
			{
				sprintf(logBuff, "Warning: Section [LISTEN-ON], Invalid IP Address %s, ignored", iniStrPtr);
				logMess(logBuff, 1);
			}
		}
	}

	if (getSection("HOME", iniStr, 1, iniFile))
	{
		char *iniStrPtr = myGetToken(iniStr, 0);
		for (; iniStrPtr[0]; iniStrPtr = myGetToken(iniStrPtr, 1))
		{
			char name[MAX_PATH + 1];
			strncpy(name, iniStrPtr, MAX_PATH);
			char *value = strchr(name, '=');
			if (value)
			{
				*value = 0;
				value++;
				if (!cfig.homes[0].alias[0] && cfig.homes[0].target[0])
				{
					sprintf(logBuff, "Section [HOME], alias and bare path mixup, entry %s ignored", iniStrPtr);
					logMess(logBuff, 1);
				}
				else if (strchr(name, fileSep) || strchr(name, '/') || strchr(name, '>') || strchr(name, '<') || strchr(name, '.'))
				{
					sprintf(logBuff, "Section [HOME], invalid chars in alias %s, entry ignored", name);
					logMess(logBuff, 1);
				}
				else if (name[0] && strlen(name) < 64 && value[0])
				{
					for (int i = 0; i < 8; i++)
					{
						if (cfig.homes[i].alias[0] && !strcasecmp(name, cfig.homes[i].alias))
						{
							sprintf(logBuff, "Section [HOME], Duplicate Entry: %s ignored", iniStrPtr);
							logMess(logBuff, 1);
							break;
						}
						else if (!cfig.homes[i].alias[0])
						{
							strcpy(cfig.homes[i].alias, name);
							strcpy(cfig.homes[i].target, value);

							if (cfig.homes[i].target[strlen(cfig.homes[i].target) - 1] != fileSep)
							{
								tempbuff[0] = fileSep;
								tempbuff[1] = 0;
								strcat(cfig.homes[i].target, tempbuff);
							}

							break;
						}
					}
				}
				else
				{
					sprintf(logBuff, "Section [HOME], alias name too large", name);
					logMess(logBuff, 1);
				}
			}
			else if (!cfig.homes[0].alias[0] && !cfig.homes[0].target[0])
			{
				strcpy(cfig.homes[0].target, name);

				if (cfig.homes[0].target[strlen(cfig.homes[0].target) - 1] != fileSep)
				{
					tempbuff[0] = fileSep;
					tempbuff[1] = 0;
					strcat(cfig.homes[0].target, tempbuff);
				}
			}
			else if (cfig.homes[0].alias[0])
			{
				sprintf(logBuff, "Section [HOME], alias and bare path mixup, entry %s ignored", iniStrPtr);
				logMess(logBuff, 1);
			}
			else if (cfig.homes[0].target[0])
			{
				sprintf(logBuff, "Section [HOME], Duplicate Path: %s ignored", iniStrPtr);
				logMess(logBuff, 1);
			}
			else
			{
				printf(logBuff, "Section [HOME], missing = sign, Invalid Entry: %s ignored", iniStrPtr);
				logMess(logBuff, 1);
			}
		}
	}

	if (!cfig.homes[0].target[0])
	{
		GetModuleFileName(NULL, cfig.homes[0].target, UCHAR_MAX);
		char *iniFileExt = strrchr(cfig.homes[0].target, fileSep);
		*(++iniFileExt) = 0;
	}

	cfig.fileRead = true;

	if (getSection("TFTP-OPTIONS", iniStr, 1, iniFile))
	{
		char *iniStrPtr = myGetToken(iniStr, 0);
		for (; strlen(iniStrPtr); iniStrPtr = myGetToken(iniStrPtr, 1))
		{
			mySplit(name, value, iniStrPtr, '=');

			if (strlen(value))
			{
				if (!strcasecmp(name, "blksize"))
				{
					DWORD tblksize = atol(value);

					if (tblksize < 512)
						blksize = 512;
					else if (tblksize > 65464)
						blksize = 65464;
					else
						blksize = tblksize;
				}
				else if (!strcasecmp(name, "timeout"))
				{
					timeout = atol(value);
					if (timeout < 1)
						timeout = 1;
					else if (timeout > UCHAR_MAX)
						timeout = UCHAR_MAX;
				}
				else if (!strcasecmp(name, "Read"))
				{
					if (strchr("Yy", *value))
						cfig.fileRead = true;
					else
						cfig.fileRead = false;
				}
				else if (!strcasecmp(name, "Write"))
				{
					if (strchr("Yy", *value))
						cfig.fileWrite = true;
					else
						cfig.fileWrite = false;
				}
				else if (!strcasecmp(name, "Overwrite"))
				{
					if (strchr("Yy", *value))
						cfig.fileOverwrite = true;
					else
						cfig.fileOverwrite = false;
				}
				else
				{
					sprintf(logBuff, "Warning: unknown option %s, ignored", name);
					logMess(logBuff, 1);
				}
			}
		}
	}

	if (getSection("ALLOWED-CLIENTS", iniStr, 1, iniFile))
	{
		char *iniStrPtr = myGetToken(iniStr, 0);
		for (int i = 0; i < 32 && iniStrPtr[0]; iniStrPtr = myGetToken(iniStrPtr, 1))
		{
			DWORD rs = 0;
			DWORD re = 0;
			mySplit(name, value, iniStrPtr, '-');
			rs = htonl(my_inet_addr(name));

			if (strlen(value))
				re = htonl(my_inet_addr(value));
			else
				re = rs;

			if (rs && rs != INADDR_NONE && re && re != INADDR_NONE && rs <= re)
			{
				cfig.hostRanges[i].rangeStart = rs;
				cfig.hostRanges[i].rangeEnd = re;
				i++;
			}
			else
			{
				sprintf(logBuff, "Section [ALLOWED-CLIENTS] Invalid entry %s in ini file, ignored", iniStrPtr);
				logMess(logBuff, 1);
			}
		}
	}

//	if (!cfig.servers[0])
//		getServ();

	int i = 0;

	for (int j = 0; j < MAX_SERVERS; j++)
	{
 		if (j && !cfig.servers[j])
 			break;

 		cfig.tftpConn[i].sock = socket(PF_INET,
		                               SOCK_DGRAM,
		                               IPPROTO_UDP);
#ifdef USE_BROADCAST_UDP
		//modify
		int err0 = 0;
		bool bReuse = true;
		err0 = ::setsockopt(cfig.tftpConn[i].sock, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuse, sizeof(bool));
		if (err0<0)
		{
			DWORD dwErr = GetLastError();
			sprintf(logBuff, "set sock reuse error");
			logMess(logBuff, 1);
			return;
		}

		int loop = 0;
		err0 = setsockopt(cfig.tftpConn[i].sock, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));
		if (err0<0)
		{
			DWORD dwErr = GetLastError();
			sprintf(logBuff, "set sock loop error");
			logMess(logBuff, 1);
			return;
		}
#endif

		if (cfig.tftpConn[i].sock == INVALID_SOCKET)
		{
			sprintf(logBuff, "Failed to Create Socket");
			logMess(logBuff, 1);
			continue;
		}

		cfig.tftpConn[i].addr.sin_family = AF_INET;

		if (!cfig.ports[j])
			cfig.ports[j] = UDP_PORT;// 30000;// 69; modify

		cfig.tftpConn[i].addr.sin_addr.s_addr = cfig.servers[j];
		cfig.tftpConn[i].addr.sin_port = htons(cfig.ports[j]);

		socklen_t nRet = bind(cfig.tftpConn[i].sock,
		                      (sockaddr*)&cfig.tftpConn[i].addr,
		                      sizeof(struct sockaddr_in)
		                     );

		if (nRet == SOCKET_ERROR)
		{
			closesocket(cfig.tftpConn[i].sock);
			sprintf(logBuff, "%s Port %u, bind failed", IP2String(tempbuff, cfig.servers[j]), cfig.ports[j]);
			logMess(logBuff, 1);
			continue;
		}
#ifdef USE_BROADCAST_UDP
		//modify
		struct ip_mreq mreq;/*加入广播组*/
		mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);//广播地址
		mreq.imr_interface.s_addr = INADDR_ANY; //网络接口为默认
		/*将本机加入广播组*/
		int err = setsockopt(cfig.tftpConn[i].sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
		if (err<0)
		{
			DWORD dwErr = GetLastError();
			sprintf(logBuff, "set sock error");
			logMess(logBuff, 1);
			return ;
		}
#endif

		if (cfig.maxFD < cfig.tftpConn[i].sock)
			cfig.maxFD = cfig.tftpConn[i].sock;

		cfig.tftpConn[i].server = cfig.tftpConn[i].addr.sin_addr.s_addr;
		cfig.tftpConn[i].port = htons(cfig.tftpConn[i].addr.sin_port);
		i++;
	}

	cfig.maxFD++;

	if (!cfig.tftpConn[0].port)
	{
		sprintf(logBuff, "no listening interfaces available, stopping..\nPress Enter to exit\n");
		logMess(logBuff, 1);
		getchar();
		exit(-1);
	}
	else if (verbatim)
	{
		printf("starting TFTP...\n");
	}
	else
	{
		sprintf(logBuff, "starting TFTP service");
		logMess(logBuff, 1);
	}

	for (int i = 0; i < 8; i++)
		if (cfig.homes[i].target[0])
		{
			sprintf(logBuff, "alias /%s is mapped to %s", cfig.homes[i].alias, cfig.homes[i].target);
			logMess(logBuff, 1);
		}

	if (cfig.hostRanges[0].rangeStart)
	{
		char temp[128];

		for (WORD i = 0; i <= sizeof(cfig.hostRanges) && cfig.hostRanges[i].rangeStart; i++)
		{
			sprintf(logBuff, "%s", "permitted clients: ");
			sprintf(temp, "%s-", IP2String(tempbuff, htonl(cfig.hostRanges[i].rangeStart)));
			strcat(logBuff, temp);
			sprintf(temp, "%s", IP2String(tempbuff, htonl(cfig.hostRanges[i].rangeEnd)));
			strcat(logBuff, temp);
			logMess(logBuff, 1);
		}
	}
	else
	{
		sprintf(logBuff, "%s", "permitted clients: all");
		logMess(logBuff, 1);
	}

	sprintf(logBuff, "max blksize: %u", blksize);
	logMess(logBuff, 1);
	sprintf(logBuff, "default blksize: %u", 512);
	logMess(logBuff, 1);
	sprintf(logBuff, "default timeout: %u", timeout);
	logMess(logBuff, 1);
	sprintf(logBuff, "file read allowed: %s", cfig.fileRead ? "Yes" : "No");
	logMess(logBuff, 1);
	sprintf(logBuff, "file create allowed: %s", cfig.fileWrite ? "Yes" : "No");
	logMess(logBuff, 1);
	sprintf(logBuff, "file overwrite allowed: %s", cfig.fileOverwrite ? "Yes" : "No");
	logMess(logBuff, 1);

	if (!verbatim)
	{
		sprintf(logBuff, "logging: %s", cfig.logLevel > 1 ? "all" : "errors");
		logMess(logBuff, 1);
	}

	for (int i = 0; i < MAX_SERVERS && cfig.tftpConn[i].port; i++)
	{
		sprintf(logBuff, "listening on: %s:%i", IP2String(tempbuff, cfig.tftpConn[i].server), cfig.tftpConn[i].port);
		logMess(logBuff, 1);
	}
}

void logMess(char *logBuff, BYTE logLevel)
{
	if (verbatim)
		printf("%s\n", logBuff);
	else if (cfig.logfile && logLevel <= cfig.logLevel)
	{
		char currentTime[32];
		time_t t = time(NULL);
		tm *ttm = localtime(&t);
		strftime(currentTime, sizeof(currentTime), "%d-%b-%y %X", ttm);
		fprintf(cfig.logfile, "[%s] %s\n", currentTime, logBuff);
		fflush(cfig.logfile);
	}
}

void logMess(request *req, BYTE logLevel)
{
	if (verbatim)
	{
		if (!req->serverError.errormessage[0])
			printf(req->serverError.errormessage, strerror(errno));

		if (req->path[0])
			printf("Client %s:%u %s, %s\n", IP2String(tempbuff, req->client.sin_addr.s_addr), ntohs(req->client.sin_port), req->path, req->serverError.errormessage);
		else
			printf("Client %s:%u, %s\n", IP2String(tempbuff, req->client.sin_addr.s_addr), ntohs(req->client.sin_port), req->serverError.errormessage);

	}
	else if (cfig.logfile && logLevel <= cfig.logLevel)
	{
		char currentTime[32];
		time_t t = time(NULL);
		tm *ttm = localtime(&t);
		strftime(currentTime, sizeof(currentTime), "%d-%b-%y %X", ttm);

		if (req->path[0])
			fprintf(cfig.logfile, "[%s] Client %s:%u %s, %s\n", currentTime, IP2String(tempbuff, req->client.sin_addr.s_addr), ntohs(req->client.sin_port), req->path, req->serverError.errormessage);
		else
			fprintf(cfig.logfile, "[%s] Client %s:%u, %s\n", currentTime, IP2String(tempbuff, req->client.sin_addr.s_addr), ntohs(req->client.sin_port), req->serverError.errormessage);

		fflush(cfig.logfile);
	}
}
