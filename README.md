# Python-Object-Explorer

## Overview

A 64-bit object explorer to inspect Python objects at run-time. This is intended to work with both interpreted Python as well as with static compilers.
This was developed with the **Python 3.6.8** Runtime in mind, so support for lower or higher versions may be buggier. 
In addition, this supports the Nuitka static compiler as well as others.

The **default mode** is set to be used with **Nuitka with Python 3.6.8 compatibility**

With the explorer, it will use IPC via sockets to send views of tree nodes for objects. The following "roots" are defined: 

* Globals (A tree of all global attributes)
* Modules (A tree of all directly imported modules)
* Debug Context (In-hook filter for getting an object as it is on the current frame in `PyFrame_New`/Interpreted or `PyThreadState_Get`/Nuitka)

The tree nodes will not have [+] expansion symbols immediately until clicked, this is so a fresh state of an object's attributes can always be retrieved.

Example usage:
![Example](https://i.imgur.com/KhS0sR7.png)

In this example, the debug was filtering for "bool" objects and the Debug Context tree represents the attributes of the filtered bool object.

The last line of output is from an object within the modules that had a list object named `__all__`

## How to build

### C++

* Visual Studio 2019
* Create a project directory called ObjExp
* Move the ObjExp.sln file from cpp-src in the new directory
* Create a subdirectory called ObjExp
* Place the source/header files and the vcxproj files from cpp-src in it
* Install [Python 3.6.8 Runtime](https://www.python.org/downloads/release/python-368/) to C:\Program Files\
* If you wish to make changes such as evaluating interpreter objects instead of objects in a statically compiled runtime like Nuitka, refer to Line 20 in dllmain.cpp and plug-in the offsets for PyFrame_New (parameter 1), PyObject_Dir (parameter 2), the name of the module where those offsets are (parameter 3), and set nuitka (parameter 4) to false.
* If you wish to get console I/O, then uncomment lines 15-18 in dllmain.cpp, and 301. Uncomment 300 depending on the circumstances of a pre-existing console.
* Compile

### C#

* Visual Studio 2019
* In the PyperLoader directory, click PyperLoader.sln
* Compile

## How to use GUI

The GUI will not load until a socket is connected, and currently (TBA) the GUI does not inject the ObjExp DLL for you, you must use another program to inject the DLL which will bind
the socket for the GUI to connect to, and then it will load. If it never loads then terminate the process.

* Value Text field - This is input to be used with the buttons (except the Get Value button)
* Set Filter - Sets the `QueryStr` filter in the ObjExp DLL, this will be used with the Debug On/Off button
* Debug On/Off - The first click will filter for an object of `QueryStr` and when a result is found, it stops executions of the interpreter 
and presents the object's attributes in Debug Context. The second click will resume execution and the Debug Context will be lost when it is selected again.
* Audit - Writes to the target process's CLI (if one exists, if not then modify the src to allow AllocConsole with stdio redirection to CONOUT$/CONIN$ as it is commented out) each time
an object with the specified name in the Value field is found in a frame from a hook
* Set Value - supports setting two of the most primitive values (int and float - which is really a double to CPython) for the currently selected attribute
* Swap Value - a bit more difficult to use, but you must select the parent of the object you want to swap, and then type: `OBJECTNAME,0x12345678` where `0x12345678` is the address of the object you want `OBJECTNAME` to point to moving forward
* Get Value - Gets the value at the selected object (supporst: int, float, string, list, and list with tuple)

## Personal Disclaimer

There **are bugs**. Initially I wrote this very quickly for a statically compiled Python program I was reverse engineering without regard for memory safety or efficient/clean code. For example,
the objects used in ObjExp's handlers contain either borrowed references or new references which do not get incremented or decremented for their reference count. Meaning that memory will never be freed. Currently the functionality is not called enough in a way that it would lead to massive resource consumption.

Not all objects have attributes, accessing one without attributes will cause crashes. There is one situational fix when working with classes containing `__abstract__` where their types are set to `Unknown` to prevent ObjExp from accessing those and crashing.
Etc.
