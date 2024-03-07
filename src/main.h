#pragma once

#include "common/console.h"

#define NOMINMAX

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#else
#include <time.h>
#include <stdio.h>

#define MAX_PATH 260

#endif

#include <iostream>  //std::string, std::cout
#include <vector>    //std::vector
#include <map>		 //std::map
#include <string>    //std::operator+
#include <cstdint>   //int definitions
#include <time.h>    //time, localtime functions
#include <signal.h>  //signals...
#include <iterator>
#include <algorithm>
#include <numeric>
#include <random>
#include <iomanip>
#include <sstream>
#include <functional>

#include "obfuscate.h"

#include "serverinstance.h"
#include "event.h"

extern CEvent g_Event;
extern CCriticalSection g_ServerCriticalSection;