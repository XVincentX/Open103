#ifndef COMMUNICATIONPORT_H
#define COMMUNICATIONPORT_H
#pragma once

#include <string>

using namespace std;


class CommunicationPort
{
public:
	CommunicationPort(string name) : _name(name)
	{
	}

	virtual ~CommunicationPort() {}
	virtual int Read(unsigned char* thePacket, int maxlen)=0;
	virtual int Write(unsigned char* thePacket, int len)=0;


protected:

private:
	string _name;
	int _errorCounter;
};


#endif // COMMUNICATIONPORT_H
