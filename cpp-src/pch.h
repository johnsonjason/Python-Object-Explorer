// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <tuple>
#include <algorithm>
#ifdef _DEBUG
#undef _DEBUG
#include "C:\\Program Files\\Python36\\include\\Python.h"
#include "C:\\Program Files\\Python36\\include\\frameobject.h"
#include "C:\\Program Files\\Python36\\include\\pystate.h"
#define _DEBUG
#else
#include "C:\\Program Files\\Python36\\include\\Python.h"
#include "C:\\Program Files\\Python36\\include\\frameobject.h"
#include "C:\\Program Files\\Python36\\include\\pystate.h"
#endif

#pragma comment(lib, "ws2_32.lib")

#endif //PCH_H
