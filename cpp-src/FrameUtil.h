#pragma once
#include "pch.h"

typedef struct ATTRIB_VALUE {
    int Type = 0;
    void* Value = nullptr;
} _ATTRIB_VALUE;

typedef std::tuple<PyObject*, PyObject*> PySet;
typedef PyFrameObject* FrameFunc(PyThreadState* tstate, PyCodeObject* code, PyObject* globals, PyObject* locals);
typedef PyObject* PyObjDir(PyObject* obj);

// The thread state actively used by the foreign interpreter
extern PyThreadState* GlobalState;

// The globals present in the foreign interpreter state
extern PyObject* Globals;
extern PyObject* Locals;

// The server socket
extern SOCKET GlobalSocket;

// The socket used to communicate with the client
extern SOCKET GlobalReceiver;


// PyFrame_New()
extern FrameFunc* NewFrameFunc;
// PyObject_Dir()
extern PyObjDir* GetObjDir;


template< typename T >
std::string int_to_hex(T i)
{
    std::stringstream stream;
    stream << "0x"
        << std::setfill('0') << std::setw(sizeof(T) * 2)
        << std::hex << i;
    return stream.str();
}

void InitializeFramer();
PyObject* GetListItem(PyObject* List, Py_ssize_t ListSize, std::string Item);
PySet FindModule(std::string ModuleName);
PySet FindGlobal(std::string GlobalName);

void ModuleTree();
void GlobalTree();

void GetObjectValue(PyObject* ArgObject, std::string ObjectType, std::string ObjectPath);
void SetObjectValue(PyObject* ArgObject, std::string ObjectType, std::string ObjectBase, ATTRIB_VALUE* Value);
