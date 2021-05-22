#pragma once
#include "pch.h"

__declspec(dllexport) PyFrameObject* GetFrame(PyThreadState* tstate, PyCodeObject* code, PyObject* globals, PyObject* locals);

void HandleFunction();
