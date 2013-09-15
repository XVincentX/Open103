// DotNetOpen103.h

#pragma once


using namespace System;

namespace DotNetOpen103 {

	public ref class IEC8705103Protocol
	{
		public IEC8705103Protocol(String^ portName,Int32 busAddress)
		{

		}

		Boolean GetNextAsdu(array<Byte> data, Int32 Class)
		{
		}

		~IEC8705103Protocol()
		{
			delete pManager;
		}
	private:
		IEC8705103Manager *pManager;
	};
}
