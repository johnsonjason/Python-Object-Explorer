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

//
// Same nature as DebugGetObject, except it is ran within the PyFrame_New hook to prevent special conditions surrounding foreign runtimes and GIL
// Some list objects - namely using extend() cause errors resulting in a complete crash
//
void GetObjectValue(PyObject* ArgObject, std::string ObjectType, std::string ObjectPath) {
    if (ObjectType == "list") {
        std::string SendObj = "List Value ( " + ObjectPath + " ) [ " + int_to_hex((DWORD_PTR)ArgObject) + " ]: [ ";
        std::string SendObjTuple;
        Py_ssize_t ListSize = Py_SIZE(ArgObject);
        for (Py_ssize_t i = 0; i < ListSize; i++) {
            PyObject* ListItem = PyList_GetItem(ArgObject, i);
            if (ListItem != nullptr) {
                if (std::string(Py_TYPE(ListItem)->tp_name) == "tuple") {
                    for (Py_ssize_t v = 0; v < Py_SIZE(ListItem); v++) {
                        SendObjTuple += "[Tuple " + std::to_string(i) + "] Value " + std::to_string(v) + " @ " + int_to_hex((DWORD_PTR)PyTuple_GetItem(ListItem, v)) + "\r\n";
                    }
                }
                PyObject* ListStr = PyObject_Str(ListItem);
                if (ListStr != nullptr) {
                    SendObj += "'" + std::string(PyUnicode_AsUTF8(ListStr)) + "', ";
                }
            }
        }
        SendObj += " ]\r\n" + SendObjTuple;
        send(GlobalReceiver, SendObj.c_str(), SendObj.size(), 0);
        return;
    }
    else if (ObjectType == "int") {
        std::string SendObj = "Int Value ( " + ObjectPath + " ) [ " + int_to_hex((DWORD_PTR)ArgObject) + " ]: ";
        SendObj += std::to_string(PyLong_AsLong(ArgObject));
        send(GlobalReceiver, SendObj.c_str(), SendObj.size(), 0);
        return;
    }
    else if (ObjectType == "float") {
        std::string SendObj = "Float Value ( " + ObjectPath + " ) [ " + int_to_hex((DWORD_PTR)ArgObject) + " ]: ";
        SendObj += std::to_string(((PyFloatObject*)ArgObject)->ob_fval);
        send(GlobalReceiver, SendObj.c_str(), SendObj.size(), 0);
        return;
    }
    else if (ObjectType == "str") {
        std::string SendObj = "String Value ( " + ObjectPath + " ) [ " + int_to_hex((DWORD_PTR)ArgObject) + " ]: ";
        SendObj += std::string(PyUnicode_AsUTF8(ArgObject));
        send(GlobalReceiver, SendObj.c_str(), SendObj.size(), 0);
        return;
    }
    else if (ObjectType == "builtin_function_or_method" || ObjectType == "function" || ObjectType == "method") {
        std::string SendObj = "Method Value ( " + ObjectPath + " ) [ " + int_to_hex((DWORD_PTR)ArgObject) + " ]: ";
        SendObj += "tp_call -> " + int_to_hex(PyMethod_GET_FUNCTION(ArgObject));
        send(GlobalReceiver, SendObj.c_str(), SendObj.size(), 0);
        return;
    }
    else {
        std::string SendObj = "Non-Supported Type ( " + ObjectPath + " ) [ " + int_to_hex((DWORD_PTR)ArgObject) + " ]";
        send(GlobalReceiver, SendObj.c_str(), SendObj.size(), 0);
        return;
    }
    send(GlobalReceiver, "Not programmed for value.", sizeof("Not programmed for value."), 0);
}

void SetObjectValue(PyObject* ArgObject, std::string ObjectType, std::string ObjectBase, ATTRIB_VALUE* Value) {
    if (ObjectType == "int") {
        long long Reval = *(long long*)Value->Value;
        DWORD_PTR Offset = (DWORD_PTR)ArgObject + 0x18;
        *(DWORD_PTR*)(Offset) = Reval;
        free(Value->Value);
        return;
    }
    else if (ObjectType == "float") {
        ((PyFloatObject*)ArgObject)->ob_fval = *(double*)Value->Value;
        free(Value->Value);
        return;
    }
}

void SwapObjectValue(PyObject* Swappable, std::string Name, PyObject* Value) {
    PyObject* SwappableList = GetObjDir(Swappable);

    for (Py_ssize_t i = 0; i < Py_SIZE(SwappableList); i++) {
        PyObject* ListItem = PyList_GetItem(SwappableList, i);
        std::string ItemStr = PyUnicode_AsUTF8(ListItem);
        if (ItemStr == Name) {
            if (ListItem != nullptr) {
                PyObject_SetAttr(Swappable, ListItem, Value);
            }
            break;
        }
    }
}
