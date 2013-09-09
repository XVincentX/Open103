// Open103.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Open103.h"


// This is an example of an exported variable
OPEN103_API int nOpen103=0;

// This is an example of an exported function.
OPEN103_API int fnOpen103(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see Open103.h for the class definition
COpen103::COpen103()
{
	return;
}
