#include "pch.h"
#include "FrameUtil.h"
// The server socket
SOCKET GlobalSocket;

// The socket used to communicate with the client
SOCKET GlobalReceiver;

PyObject* SwapRoot;
std::string AttributeToSwap;
PyObject* ToSwap;

//
// Retrieve a specific item from a list by name
//
PyObject* GetListItem(PyObject* List, Py_ssize_t ListSize, std::string Item) {
    for (Py_ssize_t i = 0; i < ListSize; i++) {
        PyObject* ListItem = PyList_GetItem(List, i);
        if (PyUnicode_AsUTF8(ListItem) == Item) {
            return ListItem;
        }
    }
    return nullptr;
}

//
// Find a module in the foreign interpreter state's modules,
// ModuleName - the module to search for in the dictionary
//
PySet FindModule(std::string ModuleName) {
    std::string Name;
    PyObject* Key;
    PyObject* Value;
    Py_ssize_t Pos = 0;

    while (PyDict_Next(GlobalState->interp->modules, &Pos, &Key, &Value)) {
        Name = PyUnicode_AsUTF8(Key);
        if (ModuleName == Name) {
            return { Value, GetObjDir(Value) };
        }
    }
    return { nullptr, nullptr };
}

//
// Find a global in the frame's globals,
// ModuleName - the global to search for in the dictionary
//
PySet FindGlobal(std::string GlobalName) {
    std::string Name;
    PyObject* Key;
    PyObject* Value;
    Py_ssize_t Pos = 0;

    while (PyDict_Next(Globals, &Pos, &Key, &Value)) {
        Name = PyUnicode_AsUTF8(Key);
        if (GlobalName == Name) {
            return { Value, GetObjDir(Value) };
        }
    }
    return { nullptr, nullptr };
}

//
// Send over IPC, a tree view of the modules
//
void ModuleTree() {
    std::string Name;
    std::string SocketQuery;
    PyObject* Key;
    PyObject* Value;
    Py_ssize_t Pos = 0;

    while (PyDict_Next(GlobalState->interp->modules, &Pos, &Key, &Value)) {
        Name = PyUnicode_AsUTF8(Key);
        SocketQuery += Name + " (" + Py_TYPE(Value)->tp_name + ") [" + int_to_hex((DWORD_PTR)Value) + "]|";
    }
    send(GlobalReceiver, SocketQuery.c_str(), SocketQuery.size(), 0);
}

//
// Send over IPC, a tree view of the globals
//
void GlobalTree() {
    std::string Name;
    std::string SocketQuery;
    PyObject* Key;
    PyObject* Value;
    Py_ssize_t Pos = 0;

    while (PyDict_Next(Globals, &Pos, &Key, &Value)) {
        Name = PyUnicode_AsUTF8(Key);
        SocketQuery += Name + " (" + Py_TYPE(Value)->tp_name + ") [" + int_to_hex((DWORD_PTR)Value) + "]|";
    }
    send(GlobalReceiver, SocketQuery.c_str(), SocketQuery.size(), 0);
}
