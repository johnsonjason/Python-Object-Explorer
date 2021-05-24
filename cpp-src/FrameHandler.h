#pragma once
#include "FrameUtil.h"
#include "ObjEnum.h"


typedef PyFrameObject* FrameFunc(PyThreadState* tstate, PyCodeObject* code, PyObject* globals, PyObject* locals);
typedef PyThreadState* TstateFunc(void);
typedef PyObject* PyObjDir(PyObject* obj);

typedef std::tuple<std::string, std::string, bool> ExecutionQuery;
typedef std::tuple<std::string, std::string, bool, std::string> ExecutionQueryEx;

typedef std::tuple<std::string, std::string, bool, std::string, std::string> ValueQuery;

extern _ATTRIB_VALUE SetValue;
extern ExecutionQuery Query;
extern ExecutionQueryEx QueryEx;
extern ValueQuery QueryVal;
extern PyObject* QueryResponse;
extern PyObject* QueryRequest;


// Should we execute? When used in the initial PyFrame_New hook, it only cares if it is non-zero or not. In the handler function, it cares about the actual specific values above 0 to perform different cases
extern int _Executable;

// If stepping through specific frames until a type name is found
extern int _FrameDebug;

// If stepping through frames to audit to the target process's console their typename - to see if these frames ever get executed
extern int _AuditMode;

// If we are going to call PyObject_CallObject on a callable in the frame hook
extern int _CallMode;

extern std::string _CallObject;
extern std::string _CallMethod;
extern std::string _AuditFilter;

// QueryStr is the filter string that will be set first, and then will be located when Debug mode is enabled, without debug mode it is inactive
extern std::string QueryStr;

void InitializeFramer(DWORD_PTR FrameAddr, DWORD_PTR DirAddr, std::string module, bool nuitka);
