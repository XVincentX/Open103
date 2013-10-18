#pragma once
#include <communicationport.h>
#include <Windows.h>

class SerialCommPort :
	public CommunicationPort
{
public:
	SerialCommPort(string name)
	{
		port = CreateFile(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		if (port == NULL)
		{

		}

	}
	virtual int Read(unsigned char* thePacket, int maxlen)
	{
		DWORD dw;
		ReadFile(port,thePacket,maxlen,&dw,0);
		return dw;
	}

	virtual int Write(unsigned char* thePacket, int len)
	{
		DWORD dw;
		WriteFile(port,thePacket,len,&dw,0);
		return dw;
	}

	virtual ~SerialCommPort(void)
	{
		if (port != NULL)
			CloseHandle(port);

		port = NULL;
	}

private:
	HANDLE port;
};

