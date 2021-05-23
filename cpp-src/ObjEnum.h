#include "FrameUtil.h"
#pragma once


std::string EnumerateDirectObjectTree(PyObject* DirectObject);
PySet EnumerateObject(std::string ObjectPath, std::string BaseName, bool UseGlobals);
PyObject* EnumerateObjectReal(std::string ObjectPath, std::string BaseName, bool UseGlobals);
PyObject* EnumerateObjectOutput(std::string ObjectPath, std::string BaseName, bool UseGlobals);
void EnumerateObjectTree(std::string ObjectPath, std::string BaseName, bool UseGlobals, std::string LastObject);
