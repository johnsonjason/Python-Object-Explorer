// dllmain.cpp : Defines the entry point for the DLL application.
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "pch.h"
#include "FrameHandler.h"


DWORD WINAPI InterpretObjects(LPVOID lpParam) 
{

    //
    // Redirect stdout, stdin, and stderr to Windows virtual console I/O handles, may crash on consoles already existing
    // Commenting this out means we will be missing audit mode, but debug mode will still work
    //

    //FILE* fDummy;
    //freopen_s(&fDummy, "CONIN$", "r", stdin);
    //freopen_s(&fDummy, "CONOUT$", "w", stderr);
    //freopen_s(&fDummy, "CONOUT$", "w", stdout);

    InitializeFramer(0, 0, "python36.dll", true);

    //
    // Create a socket for IPC with C# GUI
    //

    WSAData WsaInit;
    WSAStartup(MAKEWORD(2, 2), &WsaInit);
    GlobalSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN GlobalAddress = { 0 };
    GlobalAddress.sin_family = AF_INET;
    GlobalAddress.sin_port = htons(8787);
    GlobalAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(GlobalSocket, (SOCKADDR*)&GlobalAddress, sizeof(GlobalAddress));

    listen(GlobalSocket, 1);

    //
    // Accept the client and set a default query string
    //

    GlobalReceiver = accept(GlobalSocket, NULL, NULL);
    QueryStr = "NO_FILTER";
    while (true) {
        //
        // Receive data on the socket
        //
        char ReceiveData[1024];
        int RecvResult = recv(GlobalReceiver, ReceiveData, sizeof(ReceiveData), 0);

        //
        // Disconnect and return from the function, terminating the thread if the socket disconnected
        //
        if (RecvResult == SOCKET_ERROR) {
            if (_Executable) {
                _Executable = false;
            }
            if (_AuditMode) {
                _AuditMode = false;
            }
            if (_CallMode) {
                _CallMode = false;
            }
            closesocket(GlobalReceiver);
            closesocket(GlobalSocket);
            return 0;
        }

        //
        // Convert the data to a string and process commands from it
        //
        std::string ReceiveStr(ReceiveData);


        //
        // Return a base tree for the modules, or return a base tree for the globals
        //
        if (ReceiveStr == "_moduletree_") {
            _Executable = 3;
        }
        else if (ReceiveStr == "_globaltree_") {
            _Executable = 4;
        }
        else {

            //
            // Do command sub-processing
            //
            std::istringstream iss(ReceiveStr);
            std::vector<std::string> tokens;
            std::string token;
            while (std::getline(iss, token, '|')) {
                if (!token.empty()) {
                    tokens.push_back(token);
                }
            }

            if (tokens[0] == "TREE_MOD") {
                //
                // Further enumerate a tree within the modules root directory
                //
                QueryEx = { tokens[1], tokens[2], false, tokens[3] };
                EnumerateObjectTree(tokens[1], tokens[2], false, tokens[3]);
                _Executable = 5;
            }
            else if (tokens[0] == "TREE_GLOBAL") {
                //
                // Further enumerate a tree in the globals root directory
                //
                QueryEx = { tokens[1], tokens[2], true, tokens[3] };
                _Executable = 5;
            }
            else if (tokens[0] == "GLOBALVAL") {
                //
                // Retrieve a value from an object in the globals directory
                //
                QueryVal = { tokens[1], tokens[2], true, tokens[3], tokens[4] };
                _Executable = 6;
            }
            else if (tokens[0] == "MODVAL") {
                //
                // Retrieve a value from an object in the modules directory
                //
                QueryVal = { tokens[1], tokens[2], false, tokens[3], tokens[4] };
                _Executable = 6;
            }
            else if (tokens[0] == "GLOBALVALSET") {
                //
                // Retrieve a value from an object in the globals directory
                //
                QueryVal = { tokens[1], tokens[2], true, tokens[3], tokens[4] };
                if (tokens[4] == "int") {
                    SetValue.Type = ValueType::ValueTypeInt;
                    SetValue.Value = malloc(sizeof(int));
                    *(int*)SetValue.Value = atoi(tokens[5].c_str());
                } 
                else if (tokens[4] == "double") {
                    SetValue.Type = ValueType::ValueTypeDouble;
                    SetValue.Value = malloc(sizeof(double));
                    *(double*)SetValue.Value = std::stod(tokens[5]);
                }
                _Executable = 7;
            }
            else if (tokens[0] == "MODVALSET") {
                //
                // Retrieve a value from an object in the modules directory
                //
                QueryVal = { tokens[1], tokens[2], false, tokens[3], tokens[4] };
                if (tokens[4] == "int") {
                    SetValue.Type = ValueType::ValueTypeInt;
                    SetValue.Value = malloc(sizeof(long long));
                    *(long long*)SetValue.Value = atoi(tokens[5].c_str());
                }
                else if (tokens[4] == "double") {
                    SetValue.Type = ValueType::ValueTypeDouble;
                    SetValue.Value = malloc(sizeof(double));
                    *(double*)SetValue.Value = std::stod(tokens[3]);
                }
                _Executable = 7;
            }
            else if (tokens[0] == "MODVALSWAP") {
                //
                // Set one object's attribute to point to another object
                // SwapRoot = the object which has an attribute that must be changed
                // AttributeToSwap = the name of the attribute to change
                //
                DWORD_PTR QueryInt = 0;
                std::stringstream ss;
                ss << std::hex << tokens[1];
                ss >> QueryInt;


                SwapRoot = (PyObject*)QueryInt;
                AttributeToSwap = tokens[2];

                QueryInt = 0;
                std::stringstream sd;
                sd << std::hex << tokens[3];
                sd >> QueryInt;
                //
                // ToSwap = the object to change AttributeToSwap with
                //
                ToSwap = (PyObject*)QueryInt;
                _Executable = 8;
            }
            else if (tokens[0] == "querystr") {
                //
                // Set the value for the query filter
                //
                QueryStr = tokens[1];
            }
            else if (tokens[0] == "enum") {
                if (!_FrameDebug) {
                    continue;
                }

                //
                // Enumerate an object tree directly when in debug mode, by retrieving it's address
                // Only allowed for direct address enumeration because the foreign interpreter is in a suspended spinning state
                //
                DWORD_PTR QueryVal = 0;
                std::stringstream ss;
                ss << std::hex << tokens[1];
                ss >> QueryVal;
                QueryRequest = (PyObject*)QueryVal;
                std::string ResultStr = EnumerateDirectObjectTree(QueryRequest);
                send(GlobalReceiver, ResultStr.c_str(), ResultStr.size(), 0);
            }
            else if (tokens[0] == "debug") {
                //
                // Set the frame hook or thread state hook to suspend upon a named filter object being "hit"
                //
                if (!_FrameDebug) {
                    _FrameDebug = true;
                    _Executable = true;
                }
                else {
                    _FrameDebug = false;
                    _Executable = true;
                    QueryStr = "NO_FILTER";
                }
            }
            else if (tokens[0] == "__getval__") {
                //
                // Retrieve the value of an immediate object without passing through frame/thread state hooks
                //
                DWORD_PTR QueryVal = 0;
                std::stringstream ss;
                ss << std::hex << tokens[1];
                ss >> QueryVal;
                QueryRequest = (PyObject*)QueryVal;
                GetObjectValue(QueryRequest, tokens[2], "Debug Context");
            }
            else if (tokens[0] == "__setval__") {
                //
                // Set the value of an immediate object without passing through frame/thread state hooks
                //
                DWORD_PTR QueryVal = 0;
                std::stringstream ss;
                ss << std::hex << tokens[1];
                ss >> QueryVal;
                QueryRequest = (PyObject*)QueryVal;
                if (tokens[2] == "int") {
                    SetValue.Type = ValueType::ValueTypeInt;
                    SetValue.Value = malloc(sizeof(int));
                    *(int*)SetValue.Value = atoi(tokens[3].c_str());
                }
                else if (tokens[2] == "double") {
                    SetValue.Type = ValueType::ValueTypeDouble;
                    SetValue.Value = malloc(sizeof(double));
                    *(double*)SetValue.Value = std::stod(tokens[3]);
                }
                SetObjectValue(QueryRequest, tokens[2], "Debug Context", &SetValue);
            }
            else if (tokens[0] == "_audit_") {
                std::cout << "AUDIT_MODE: " << tokens[1] << std::endl;
                if (_AuditMode) {
                    _AuditMode = false;
                }
                else {
                    _AuditMode = true;
                }
                if (tokens.size() > 1) {
                    std::cout << "SETTING AUDIT FILTER.\n";
                    _AuditFilter = tokens[1];
                }
                else
                {
                    _AuditFilter = "NO_FILTER";
                }
            }
            else if (tokens[0] == "voidcall") {
                //
                // Call a static function within a frame/thread state hook via PyObject_CallObject
                //
                DWORD_PTR QueryVal = 0;
                std::stringstream ss;
                ss << std::hex << tokens[1];
                ss >> QueryVal;
                QueryRequest = (PyObject*)QueryVal;
                _CallMode = 2;
            }

        }
    }
    closesocket(GlobalReceiver);
    closesocket(GlobalSocket);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //FreeConsole();
        //AllocConsole();
        CloseHandle(CreateThread(NULL, NULL, InterpretObjects, NULL, NULL, NULL));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

