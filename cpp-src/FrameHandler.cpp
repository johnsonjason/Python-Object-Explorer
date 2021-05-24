#include "pch.h"
#include "FrameHandler.h"
PyObjDir* GetObjDir;
FrameFunc* NewFrameFunc;
PyThreadState* GlobalState;
TstateFunc* GetTstate;
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

std::vector<BYTE> OldFunction;
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

//
// A handler which operates based on the value of the non-zero _Executable
//
void HandleFunction() {

	PyObject* ActualObject = nullptr;

	//
	// While in the PyFrame_New hook, _Executable is only cared about if it is zero or non-zero in order to continue into a handler,
	// Here it cares about the actual value and converts them into actual statements
	//
	switch (_Executable) {
	case 1:
		//
		// Directly retrieve an object by its name and path, i.e [BASE NAME].class.int where BASE NAME may be an object or global variable. 
		// The first parameter specifies the path, the second parameter specifies the base name, and the third parameter specifies the root (modules, or globals)
		// Why use a base name? Some names, like modules have periods in them already, which will interfere with parsing i.e [module.module].class.int
		//
		QueryResponse = EnumerateObjectReal(std::get<0>(Query), std::get<1>(Query), std::get<2>(Query));
		break;
	case 3:
		//
		// Retrieve the tree in relation to the foreign runtime's Python modules
		//
		ModuleTree();
		break;
	case 4:
		//
		// Retrieve the tree in relation to the foreign runtime's global state
		//
		GlobalTree();
		break;
	case 5:
		EnumerateObjectTree(std::get<1>(QueryEx), std::get<0>(QueryEx), std::get<2>(QueryEx), std::get<3>(QueryEx));
		break;
	case 6:
		ActualObject = EnumerateObjectReal(std::get<1>(QueryVal), std::get<0>(QueryVal), std::get<2>(QueryVal));
		if (ActualObject != nullptr) {
			GetObjectValue(ActualObject, std::get<4>(QueryVal), std::get<1>(QueryVal));
		}
		break;
	case 7:
		ActualObject = EnumerateObjectReal(std::get<1>(QueryVal), std::get<0>(QueryVal), std::get<2>(QueryVal));
		if (ActualObject != nullptr) {
			SetObjectValue(ActualObject, std::get<4>(QueryVal), std::get<0>(QueryVal), &SetValue);
		}
		break;
	case 8:
		ActualObject = SwapRoot;
		if (ActualObject != nullptr) {
			SwapObjectValue(ActualObject, AttributeToSwap, ToSwap);
		}
		break;
	}


}


void DoCall(PyThreadState* tstate) {
	if (_CallMode == 2) {
		std::string DirectCallOutput;
		DirectCallOutput = PyUnicode_AsUTF8(PyObject_Str(PyObject_CallObject(QueryRequest, nullptr)));
		send(GlobalReceiver, DirectCallOutput.c_str(), DirectCallOutput.size(), 0);
		_CallMode = false;
		return;
	}

	//
	// Constantly locate the object in each frame's evaluation stack until it is found, and then try to call it
	//
	std::string AttribType = Py_TYPE(*(tstate->frame->f_valuestack))->tp_name;

	//
	// Convert the object name, tp_name to lower case
	//
	std::transform(AttribType.begin(), AttribType.end(), AttribType.begin(),
		[](unsigned char c) { return std::tolower(c); });

	std::transform(_CallObject.begin(), _CallObject.end(), _CallObject.begin(),
		[](unsigned char c) { return std::tolower(c); });

	//
	// Find the object by its name and then get an instance of it, and call the method from that instance
	//
	if (AttribType.find(_CallObject) != std::string::npos) {
		PyObject* Instance = *(tstate->frame->f_valuestack);
		PyObject_CallMethod(Instance, _CallMethod.c_str(), nullptr);
		_CallMode = false;
	}

	return;
}

void DoAudit(PyThreadState* tstate) {
	if (_AuditFilter == "NO_FILTER") {
		_AuditMode = false;
		return;
	}

	//
	// Check if the value stack pointer is null before further accessing it
	//
	if (*(tstate->frame->f_valuestack) == nullptr) {
		return;
	}
	//
	// Check if the object tp_name is not set, before continuing access
	//
	if (Py_TYPE(*(tstate->frame->f_valuestack))->tp_name == nullptr) {
		return;
	}

	std::string AttribType = Py_TYPE(*(tstate->frame->f_valuestack))->tp_name;
	std::transform(AttribType.begin(), AttribType.end(), AttribType.begin(),
		[](unsigned char c) { return std::tolower(c); });

	std::transform(_AuditFilter.begin(), _AuditFilter.end(), _AuditFilter.begin(),
		[](unsigned char c) { return std::tolower(c); });

	//
	// Check if the tp_name matches the audit filter
	//
	if (AttribType.find(_AuditFilter) != std::string::npos) {
		std::cout << "OBJECT AUDIT: " << Py_TYPE(tstate->frame->f_valuestack[0])->tp_name << std::endl;
	}

	return;
}

void DoDebug(PyThreadState* tstate) {
	if (*(tstate->frame->f_valuestack) != nullptr) {
		std::string AttribType = Py_TYPE(*(tstate->frame->f_valuestack))->tp_name;

		std::transform(AttribType.begin(), AttribType.end(), AttribType.begin(),
			[](unsigned char c) { return std::tolower(c); });

		std::transform(QueryStr.begin(), QueryStr.end(), QueryStr.begin(),
			[](unsigned char c) { return std::tolower(c); });

		//
		// Check if the object name matches the name we are looking for
		//
		if (AttribType.find(QueryStr) != std::string::npos) {

			//
			// Get a tree of the object, send the tree, and then spin, waiting for remote socket to cancel debugging
			//
			std::string Sender = EnumerateDirectObjectTree(*(tstate->frame->f_valuestack));
			send(GlobalReceiver, Sender.c_str(), Sender.size(), 0);
			while (_FrameDebug) {
				Sleep(50);
			}
			//
			// Do set _Executable to  false so that it does not execute again - with the same parameters and unnecessarily
			//
			_Executable = false;
			return;
		}
	}
}


__declspec(dllexport) PyThreadState* GetThreadState() {
	DWORD OldProtection = NULL;

	VirtualProtect(GetTstate, 1, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(GetTstate, OldFunction.data(), OldFunction.size());
	VirtualProtect(GetTstate, 1, OldProtection, &OldProtection);

	//
	// Generate an actual new frame
	//

	PyThreadState* tstate = PyThreadState_Get();

	VirtualProtect(GetTstate, 1, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(GetTstate, &JumpHook[0], JumpHook.size());
	VirtualProtect(GetTstate, 1, OldProtection, &OldProtection);

	//
	// This is used to call a static function or method (WIP) under the actual context of the hooked runtime instead of a foreign runtime
	// If _CallMode is not set, this simply passes and the branch is not executed, the next condition in line is evaluated
	//
	if (_CallMode) {
		DoCall(tstate);
		return tstate;
	}

	//
	// This is used to audit - or generate logs of objects in the bottom of the eval frames stack
	//
	if (_AuditMode) {
		DoAudit(tstate);
		return tstate;
	}

	//
	// This is executable mode, a "true hook" mode where actual commands are passed to a switch statement for each PyFrame's execution,
	// This is the basis of how the trees are generated, including the debug context tree - although some things with the debug context are handled a bit differently
	//
	if (_Executable) {
		//
		// Assign the modern state of the globals and thread state
		//
		GlobalState = tstate;
		Globals = tstate->frame->f_globals;

		//
		// Check if debugging is enabled - debugging is a "hack" and not debugging in its truest sense, in this case it's an overglorified form of the auditing library
		//
		if (_FrameDebug) {
			DoDebug(tstate);
			return tstate;
		}
		else {
			//
			// Redirect to a switch-case handler where global variables are used
			//
			HandleFunction();
			_Executable = false;
		}
	}
	return tstate;
}
//
// PyFrame_New hook
// tstate - current thread executing under the GIL (we can use tstate to directly access the interpreter state and get modules)
// code - the code object to process
// globals - a list of global variables
// locals - a list of locals being used
// Through this hook we can operate on and alternate between different states
//
__declspec(dllexport) PyFrameObject* GetFrame(PyThreadState* tstate, PyCodeObject* code, PyObject* globals, PyObject* locals)
{
	DWORD OldProtection = NULL;

	VirtualProtect(NewFrameFunc, 1, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(NewFrameFunc, OldFunction.data(), OldFunction.size());
	VirtualProtect(NewFrameFunc, 1, OldProtection, &OldProtection);

	//
	// Generate an actual new frame
	//

	PyFrameObject* CurrentFramePtr = NewFrameFunc(tstate, code, globals, locals);

	VirtualProtect(NewFrameFunc, 1, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(NewFrameFunc, &JumpHook[0], JumpHook.size());
	VirtualProtect(NewFrameFunc, 1, OldProtection, &OldProtection);


	//
	// This is used to call a static function or method (WIP) under the actual context of the hooked runtime instead of a foreign runtime
	// If _CallMode is not set, this simply passes and the branch is not executed, the next condition in line is evaluated
	//
	if (_CallMode) {
		DoCall(tstate);
		return CurrentFramePtr;
	}

	//
	// This is used to audit - or generate logs of objects in the bottom of the eval frames stack
	//
	if (_AuditMode) {
		DoAudit(tstate);
		return CurrentFramePtr;
	}

	//
	// This is executable mode, a "true hook" mode where actual commands are passed to a switch statement for each PyFrame's execution,
	// This is the basis of how the trees are generated, including the debug context tree - although some things with the debug context are handled a bit differently
	//
	if (_Executable) {
		//
		// Assign the modern state of the globals and thread state
		//
		GlobalState = tstate;
		Globals = globals;

		//
		// Check if debugging is enabled - debugging is a "hack" and not debugging in its truest sense, in this case it's an overglorified form of the auditing library
		//
		if (_FrameDebug) {
			DoDebug(tstate);
			return CurrentFramePtr;
		}
		else {
			//
			// Redirect to a switch-case handler where global variables are used
			//
			HandleFunction();
			_Executable = false;
		}
	}

	return CurrentFramePtr;
}

//
// Initialize the PyFrame_New hook and have it run once
// FrameAddr - the offset for PyFrame_New from the base module
// DirAddr - the offset for PyObject_Dir from the base module
//
void InitializeFramer(DWORD_PTR FrameAddr, DWORD_PTR DirAddr, std::string module, bool nuitka) {
	//
	// Initialize the Python 3.6.8 Runtime
	//

	Py_Initialize();

	if (nuitka) {
		GetObjDir = (PyObjDir*)GetProcAddress(GetModuleHandleA(module.c_str()), "PyObject_Dir");
		GetTstate = (TstateFunc*)GetProcAddress(GetModuleHandleA(module.c_str()), "PyThreadState_Get");
		if (GetObjDir == nullptr || GetTstate == nullptr) {
			return;
		}
		DWORD OldProtection = 0;
        
		//
		// Redirect the external runtime's PyFrame_New() to our hook. NewFrameFunc is a PyFrame_New routine initialized as a global variable
		//

		_Executable = true;

		JumpHook = Convert64ToJmp((DWORD_PTR)GetThreadState);
		OldFunction.resize(JumpHook.size());
		memcpy(OldFunction.data(), GetTstate, JumpHook.size());

		VirtualProtect(GetTstate, 1, PAGE_EXECUTE_READWRITE, &OldProtection);
		memcpy(GetTstate, &JumpHook[0], JumpHook.size());
		VirtualProtect(GetTstate, 1, OldProtection, &OldProtection);

		//
		// First initial code run, wait until a frame is received
		//
		while (_Executable) {
			Sleep(10);
		}
		return;
	}

	// PyFrame_New()
	NewFrameFunc = (FrameFunc*)((DWORD_PTR)GetModuleHandleA(module.c_str()) + FrameAddr);
	// PyObject_Dir()
	GetObjDir = (PyObjDir*)((DWORD_PTR)GetModuleHandleA(module.c_str()) + DirAddr);

	DWORD OldProtection = 0;

	//
	// Redirect the external runtime's PyFrame_New() to our hook. NewFrameFunc is a PyFrame_New routine initialized as a global variable
	//

	_Executable = true;

	JumpHook = Convert64ToJmp((DWORD_PTR)GetFrame);
	OldFunction.resize(JumpHook.size());
	memcpy(OldFunction.data(), NewFrameFunc, JumpHook.size());

	VirtualProtect(NewFrameFunc, 1, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(NewFrameFunc, &JumpHook[0], JumpHook.size());
	VirtualProtect(NewFrameFunc, 1, OldProtection, &OldProtection);

	//
	// First initial code run, wait until a frame is received
	//
	while (_Executable) {
		Sleep(10);
	}
}
