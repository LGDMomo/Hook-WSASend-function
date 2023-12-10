#pragma once
#include <windows.h>
#include <iostream>
#include"detours.h"
#include<string>
#include<winsock.h>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")
#define WSAAPI                  FAR PASCAL
#define IDC_READ_BUTTON 1001 // Adjust the value as needed
#define IDC_RESET_BUTTON 1002 // Adjust the value as needed
#define DEFAULT_PORT 27015

//Constant
HMODULE myhmod;
FILE* pFile = nullptr;
HWND hwndOutput = nullptr;
HWND hwndInput = nullptr;
HWND hwndInputLen = nullptr;
HWND hwndInputSocket = nullptr;



//struct for correct data handling
typedef struct _OVERLAPPED* LPWSAOVERLAPPED;
typedef struct _WSABUF {
    ULONG len;     /* the length of the buffer */
    _Field_size_bytes_(len) CHAR FAR* buf; /* the pointer to the buffer */
} WSABUF, FAR* LPWSABUF;
typedef void(CALLBACK* LPWSAOVERLAPPED_COMPLETION_ROUTINE)(IN DWORD dwError, IN DWORD cbTransferred, IN LPWSAOVERLAPPED lpOverlapped, IN DWORD dwFlags);
struct sockaddr_in clientService;

//add text
void AppendText(const char* text) {
    int len = GetWindowTextLength(hwndOutput);
    SendMessage(hwndOutput, EM_SETSEL, len, len);
    SendMessage(hwndOutput, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
}

// Function to convert SOCKET to string
std::string SocketToString(SOCKET socket) {
    std::ostringstream oss;
    oss << socket;
    return oss.str();
}

// Function to convert string to SOCKET
SOCKET StringToSocket(const std::string& str) {
    std::istringstream iss(str);
    SOCKET socketValue;
    iss >> socketValue;
    return socketValue;
}

//backup in case
SOCKET ConnectSocket = INVALID_SOCKET;

//Proto functions
typedef int (WINAPI* SendPtr)(SOCKET s, const char* buf, int len, int flags);
typedef int (WINAPI* WSASendPtr)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
typedef int (WINAPI* ConnectCustom)(SOCKET s, const SOCKADDR* sAddr,int nameLen);



//Lib
HMODULE hLib = LoadLibrary("WS2_32.dll");

//get the internal function 
SendPtr pSend = (SendPtr)GetProcAddress(hLib, "send");
ConnectCustom pConnect = (ConnectCustom)GetProcAddress(hLib, "connect");
WSASendPtr pWsaSend = (WSASendPtr)GetProcAddress(hLib, "WSASend");



//For send()
int WSAAPI MySend(SOCKET s, const char* buf, int len, int flags)
{
    //std::wcout << "===============================" << std::endl;
    //std::cout << "Buffer : \n" << buf << std::endl;
    //std::cout << "Buffer length : " << len << std::endl;
    //std::cout << "Flag : " << flags << std::endl;

    AppendText("=======================================\n");
    AppendText("Buffer : \n");
    AppendText(buf);

    AppendText("\n");

    AppendText("Buffer Length: ");
    std::string myLen = std::to_string(len);
    const char* LenConstChar = myLen.c_str();
    AppendText(LenConstChar);

    AppendText("\n");

    AppendText("Flags : ");
    std::string myFlags = std::to_string(flags);
    const char* FlagsConstChar = myFlags.c_str();
    AppendText(FlagsConstChar);

    AppendText("\n");

    AppendText("Socket : ");
    std::string socket_string = SocketToString(s);
    AppendText(socket_string.c_str());

    AppendText("\n");
    return pSend(s, buf, len, flags);
}

//For WSASEnd()                      
int WSAAPI MyWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    //std::wcout << "===============================" << std::endl;
    //std::wcout << L"Number of bytes sent : " << *lpNumberOfBytesSent << std::endl;
    //std::wcout << L"Buffer : \n" << *lpBuffers->buf << std::endl;

    AppendText("=======================================\n");
    AppendText("Buffer : \n");
    AppendText(lpBuffers->buf);

    AppendText("\n");

    AppendText("Number of bytes sent : ");
    AppendText((const char*)lpNumberOfBytesSent);

    return pWsaSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

/*int WSAAPI MyConnect(SOCKET s, const SOCKADDR* sAddr, int nameLen)
{
    // Assuming sAddr is of type SOCKADDR_IN
    const sockaddr_in* clientService = reinterpret_cast<const sockaddr_in*>(sAddr);
    unsigned long ipAddress = clientService->sin_addr.s_addr;
    
    std::string MyIpAddress = std::to_string(ipAddress);
    const char* IpConstChar = MyIpAddress.c_str();

    AppendText("IP address being used is: ");
    AppendText(IpConstChar);


    // You can also print the port if needed
    AppendText("Port being used is: ");
    AppendText(std::to_string(ntohs(clientService->sin_port)).c_str());

    return 0;
}
*/


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {


    //creating the menu and text box and more
    case WM_CREATE:
        hwndInput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", nullptr, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL,
            10, 340, 760, 70, hwnd, nullptr, nullptr, nullptr);

        hwndInputLen = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", nullptr, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 415, 760, 30, hwnd, nullptr, nullptr, nullptr);

        hwndInputSocket = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", nullptr, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 450, 760, 30, hwnd, nullptr, nullptr, nullptr);

        hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, (LPCSTR)"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 10, 760, 320, hwnd, nullptr, nullptr, nullptr);

        CreateWindow("BUTTON", "Send Packet", WS_CHILD | WS_VISIBLE, 10, 500, 90, 45, hwnd, (HMENU)IDC_READ_BUTTON, nullptr, nullptr);
        CreateWindow("BUTTON", "Reset Output", WS_CHILD | WS_VISIBLE, 120, 500, 90, 45, hwnd, (HMENU)IDC_RESET_BUTTON, nullptr, nullptr);
        break;
    //for button commands and stuff
    case WM_COMMAND:
        // Handle button click
        if (LOWORD(wParam) == IDC_READ_BUTTON) {
            // Read text from the input box
            char buffer[2048]; // Adjust the buffer size as needed
            GetWindowText(hwndInput, buffer, sizeof(buffer));

            char BufferLen[20];
            GetWindowText(hwndInputLen, BufferLen, sizeof(BufferLen));

            //Sending the packet with the parameters read from the input boxes
            //int SentBytes = pSend(NewSocket, (const char*)buffer, (int)BufferLen, 0);
            //AppendText(buffer);


            struct sockaddr_in clientService;

            // Create a SOCKET for connecting to server
            ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


            clientService.sin_family = AF_INET;
            clientService.sin_addr.s_addr = 1797002856;
            clientService.sin_port = htons(80);

            int iResult = connect(ConnectSocket,(SOCKADDR*) &clientService, sizeof(clientService));
            if (iResult == SOCKET_ERROR) {
                AppendText("connect failed with error: \n");

                // An error occurred, print the error code
                int errorCode = WSAGetLastError();
                std::string errorMessage = "Connecting to the socket. Error code: " + std::to_string(errorCode);
                AppendText(errorMessage.c_str());
                AppendText("\n");
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }
            else
            {
                AppendText("Socket created successfully !");
                AppendText("\n");
            }


            int SentBytes = pSend(ConnectSocket, (const char*)buffer, std::stoi(BufferLen), 0);
            

            std::string MyBytesSent = std::to_string(SentBytes);
            const char* ConstCharBytesSent = MyBytesSent.c_str();

            if (SentBytes == SOCKET_ERROR) {
                // An error occurred, print the error code
                int errorCode = WSAGetLastError();
                std::string errorMessage = "Error sending the packet. Error code: " + std::to_string(errorCode);
                AppendText(errorMessage.c_str());
                AppendText("\n");
            }
            else {
                std::string MyBytesSent = std::to_string(SentBytes);
                const char* ConstCharBytesSent = MyBytesSent.c_str();
                AppendText("Sent the packet and sent : ");
                AppendText(ConstCharBytesSent);
                AppendText(" bytes");
                closesocket(ConnectSocket);
            }
        }
        if (LOWORD(wParam) == IDC_RESET_BUTTON) {
            //Reset the output box text
            SendMessage(hwndOutput, WM_SETTEXT, 0, (LPARAM)"");
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


int Main()
{
    //AllocConsole();
    //freopen_s(&pFile, "CONOUT$", "w", stdout);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = (LPCSTR)"MyWindowClass";

    RegisterClass(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, (LPCSTR)"RainBot's Packet Logger", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, wc.hInstance, nullptr);



    const char* Choice = "send";

    DetourRestoreAfterWith();

    //for send
    if (Choice == "send")
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&)pSend, (PVOID)MySend);
        //DetourAttach(&(PVOID&)pConnect, (PVOID)MyConnect);

        DetourTransactionCommit();
    }


    if (Choice == "WSASend")
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)pWsaSend, (PVOID)MyWSASend);
        DetourTransactionCommit();
    }


    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //exiting
    while (true)
    {
        if (GetAsyncKeyState(VK_END))
        {
            break;
        }

        Sleep(100);
    }

    MessageBoxA(0, "UnInjecting", "Bye", 0);
    FreeLibraryAndExitThread((HMODULE)myhmod, 0);
    //FreeConsole();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        myhmod = hModule;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
    }
    return TRUE;
}
