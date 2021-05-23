#include "pch.h"
#include "FrameHandler.h"
PyObjDir* GetObjDir;
FrameFunc* NewFrameFunc;
PyThreadState* GlobalState;

PyObject* Globals;
PyObject* Locals;

int _Executable;
int _FrameDebug;
int _AuditMode;
int _CallMode;

_ATTRIB_VALUE SetValue = { 0 };

ExecutionQuery Query;
ExecutionQueryEx QueryEx;
ValueQuery QueryVal;
PyObject* QueryResponse;
PyObject* QueryRequest;

std::string _CallObject;
std::string _CallMethod;
std::string _AuditFilter;
std::string QueryStr;

BYTE OldFunction[] = {
    0x48, 0x89, 0x5C, 0x24, 0x08,
    0x48, 0x89, 0x6C, 0x24, 0x10,
    0x48, 0x89, 0x74, 0x24, 0x18
};

std::vector<unsigned char> JumpHook;

//
// Create a 64-bit hook, a direct jump to Value
// Value - The pointer that must be jumped to
//
std::vector<unsigned char> Convert64ToJmp(DWORD_PTR Value)
{
    std::vector<unsigned char> BPartOne;
    BPartOne.resize(sizeof(DWORD_PTR));
    memcpy(&BPartOne[0], &Value, sizeof(DWORD_PTR));

    BPartOne.insert(BPartOne.begin(), 0);
    BPartOne.insert(BPartOne.begin(), 0);
    BPartOne.insert(BPartOne.begin(), 0);
    BPartOne.insert(BPartOne.begin(), 0);

    BPartOne.insert(BPartOne.begin(), 0x25);
    BPartOne.insert(BPartOne.begin(), 0xFF);
    return BPartOne;
}
