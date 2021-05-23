#include "pch.h"
#include "ObjEnum.h"

//
// Retrieve an object by its named arguments, and return an address and attribute list for it
//
PySet EnumerateObject(std::string ObjectPath, std::string BaseName, bool UseGlobals) {
    std::istringstream ObjectDelimiter(ObjectPath);
    std::vector<std::string> PathTokens;
    std::string Path;
    while (std::getline(ObjectDelimiter, Path, '.')) {
        if (!Path.empty()) {
            PathTokens.push_back(Path);
        }
    }

    PySet Module;
    if (UseGlobals) {
        Module = FindGlobal(BaseName);
    }
    else {
        Module = FindModule(BaseName);
    }

    PyObject* ReturnObject = std::get<0>(Module);
    PyObject* ReturnObjectList = std::get<1>(Module);

    if (ObjectPath == "_NONE_") {
        return { ReturnObject, ReturnObjectList };
    }

    if (ReturnObject == nullptr) {
        return { nullptr, nullptr };
    }

    for (size_t i = 0; i < PathTokens.size(); i++) {
        PyObject* ObjectName = GetListItem(ReturnObjectList, Py_SIZE(ReturnObjectList), PathTokens[i]);

        if (ObjectName == nullptr) {
            return { nullptr, nullptr };
        }

        if (PyObject_HasAttr(ReturnObject, ObjectName)) {
            ReturnObject = Py_TYPE(std::get<0>(Module))->tp_getattro(ReturnObject, ObjectName);
        }
        else {
            return { nullptr, nullptr };
        }

        if (ReturnObject == nullptr) {
            return { nullptr, nullptr };
        }

        ReturnObjectList = GetObjDir(ReturnObject);
    }
    return { ReturnObject, ReturnObjectList };
}

std::string EnumerateDirectObjectTree(PyObject* DirectObject) {
    std::string TreeNode;
    PyObject* DirectObjectDir = GetObjDir(DirectObject);

    for (Py_ssize_t i = 0; i < Py_SIZE(DirectObjectDir); i++) {
        PyObject* ListItem = PyList_GetItem(DirectObjectDir, i);
        if (ListItem != nullptr) {
            if (PyObject_HasAttr(DirectObject, ListItem)) {
                PyObject* RealObject = Py_TYPE(DirectObject)->tp_getattro(DirectObject, ListItem);
                TreeNode += "|" + std::string(PyUnicode_AsUTF8(ListItem)) + " (" + Py_TYPE(RealObject)->tp_name + ") [" + int_to_hex((DWORD_PTR)RealObject) + "]";
            }
        }
    }

    return TreeNode;
}
