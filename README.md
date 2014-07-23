Open103
=======
Open103 is an open source, feature complete reference implementation of [IEC 60870-5-103](http://en.wikipedia.org/wiki/IEC_60870-5) protocol which define systems used for telecontrol.

Build status: [![Build status](https://ci.appveyor.com/api/projects/status/81xpjfyt727fdnok/branch/master)](https://ci.appveyor.com/project/XVincentX/open103/branch/master)


It includes a **C++** core part and an optional .NET binding (_that will never be finished_ due to lack of time).
There is no download associated, so jump to documentation to get how drop in lib in your code.

IEC 60870 part 5 is one of the IEC 60870 set of standards which define systems used for telecontrol (supervisory control and data acquisition) in electrical engineering and power system automation applications. Part 5 provides a communication profile for sending basic telecontrol messages between two systems, which uses permanent directly connected data circuits between the systems. The IEC Technical Committee 57 (Working Group 03) have developed a protocol standard for telecontrol, teleprotection, and associated telecommunications for electric power systems. The result of this work is IEC 60870-5.

Please, **DO NOT** try to use this library without reading the documentation. It will avoid you a lot of headaches.
Have fun!


# Quick usage guide

**WARNING:** i'm not going to cover IEC103 reference. Buy the documents and study!

This is not a real library. You have to drop in the source code in your project.
At first, the library makes all communication throught an abstract class, that must be derived in order to get it working.
Usually communicaton happens on a serial port. However, is very usual to use serial gateway converter that have to work on ethernet Tcp port.
Based on your need, create a CommunicationPort implementation that fits your use.

As reference, you may want to use this _SerialCommPort_

```c++
#pragma once
#include "communicationport.h"
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

```

Once you have got your code working, you should usual start with a StationInit
function (see IEC103 reference) and then enter into the loop and start grabbing
messages from protection using the GetNextAsdu function

Then you are able to manage the message, get your data and respond, if needed,
 to the protection.
