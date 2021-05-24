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

//
// Enumerate a tree for an object directly by its address
//
std::string EnumerateDirectObjectTree(PyObject* DirectObject) {
    std::string TreeNode;

    //
    // Get the object's directory via the foreign runtimes PyObject_Dir(), which returns a list
    // Iterate over the list and then get the actual address of each object via their internal tp_getattro
    // Although realistically in most cases you can just use PyObject_GetAttr, this is old code
    //
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

//
// Return the pointer to the object from EnumerateObject
//
PyObject* EnumerateObjectReal(std::string ObjectPath, std::string BaseName, bool UseGlobals) {
    return std::get<0>(EnumerateObject(ObjectPath, BaseName, UseGlobals));
}

//
// Return the list of an object
//
PyObject* EnumerateObjectOutput(std::string ObjectPath, std::string BaseName, bool UseGlobals) {
    return std::get<1>(EnumerateObject(ObjectPath, BaseName, UseGlobals));
}

//
// Build a tree for an object based on the following parameters:
// ObjectPath - The path to an object after its BaseName (acting as a root)
// BaseName - The root of an object's path, it is separate from the path because root names can contain periods which interferes with parsing
// UseGlobals - whether we search in interp->modules or _frame->f_globals
// LastObject - the last object of the path, can prevent specific crashes of attributeless objects
//
void EnumerateObjectTree(std::string ObjectPath, std::string BaseName, bool UseGlobals, std::string LastObject) {
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
        //
        // Get the base object set within the globals
        //
        Module = FindGlobal(BaseName);
    }
    else {
        //
        // Get the base object set within the modules
        //
        Module = FindModule(BaseName);
    }

    PyObject* ReturnObject = std::get<0>(Module);
    PyObject* ReturnObjectList = std::get<1>(Module);

    if (ReturnObject == nullptr) {
        return;
    }

    //
    // Return a tree of base object names
    //
    if (LastObject == "_NONE_" || ObjectPath == "_NONE_") {
        std::string SocketQuery;
        for (Py_ssize_t v = 0; v < Py_SIZE(ReturnObjectList); v++) {
            PyObject* ListItem = PyList_GetItem(ReturnObjectList, v);
            SocketQuery += std::string(PyUnicode_AsUTF8(ListItem)) + " (" + Py_TYPE(Py_TYPE(std::get<0>(Module))->tp_getattro(ReturnObject, ListItem))->tp_name + ") [" + int_to_hex((DWORD_PTR)PyObject_GetAttr(ReturnObject, ListItem)) + "]|";
        }
        send(GlobalReceiver, SocketQuery.c_str(), SocketQuery.size(), 0);
        return;
    }

    for (size_t i = 0; i < PathTokens.size(); i++) {
        PyObject* ObjectName = GetListItem(ReturnObjectList, Py_SIZE(ReturnObjectList), PathTokens[i]);

        if (ObjectName == nullptr) {
            return;
        }

        ReturnObject = Py_TYPE(std::get<0>(Module))->tp_getattro(ReturnObject, ObjectName);

        if (ReturnObject == nullptr) {
            return;
        }

        ReturnObjectList = GetObjDir(ReturnObject);

        if (PyUnicode_AsUTF8(ObjectName) == LastObject && i == PathTokens.size() - 1) {
            std::string SocketQuery;
            bool Abstract = false;
            for (Py_ssize_t v = 0; v < Py_SIZE(ReturnObjectList); v++) {
                PyObject* ListItem = PyList_GetItem(ReturnObjectList, v);
                if (std::string(PyUnicode_AsUTF8(ListItem)) == "__abstractmethods__") {
                    Abstract = true;
                }

                if (Abstract == false) {

                    if (PyObject_HasAttr(ReturnObject, ListItem) != 0) {
                        PyObject* ListObject = PyObject_GetAttr(ReturnObject, ListItem);
                        SocketQuery += std::string(PyUnicode_AsUTF8(ListItem)) + " (" + std::string(Py_TYPE(ListObject)->tp_name) + ") [" + int_to_hex((DWORD_PTR)ListObject) + "]|";
                    }
                }
                else {
                    SocketQuery += std::string(PyUnicode_AsUTF8(ListItem)) + " (" + "Unknown" + ")|";
                }
            }
            Abstract = false;
            send(GlobalReceiver, SocketQuery.c_str(), SocketQuery.size(), 0);
            return;
        }

    }
}
