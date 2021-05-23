#include "pch.h"
#include "ObjEnum.h"

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
