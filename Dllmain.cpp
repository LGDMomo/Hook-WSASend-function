
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include"detours.h"
#include<string>
#include <sstream>
#include<vector>
#include<iomanip>
#include <ws2tcpip.h>


#pragma comment(lib, "Ws2_32.lib")
#define WSAAPI                  FAR PASCAL
#define IDC_READ_BUTTON 1001 // Adjust the value as needed
#define IDC_RESET_BUTTON 1002 // Adjust the value as needed
#define IDC_HOOK_CONNECT 1010 // Adjust the value as needed


#define IDC_CHECKBOX_SEND 1003
#define IDC_CHECKBOX_SENDTO 1004
#define IDC_CHECKBOX_WSASEND 1005

#define IDC_CHECKBOX_RECV 1006
#define IDC_CHECKBOX_RECVFROM 1007
#define IDC_CHECKBOX_WSARECV 1008
#define IDC_TRANSLATE_TO_AOB 1009


#define WSAEVENT HANDLE

BOOL isSendChecked;
BOOL isSendToChecked;
BOOL isWsaSendChecked;

BOOL isRecvChecked;
BOOL isRecvFromChecked;
BOOL isWSARecvChecked;

BOOL isAOBTranslate;

#define DEFAULT_PORT 27015

//Constant
HMODULE myhmod;
FILE* pFile = nullptr;
HWND hwndOutput = nullptr;
HWND hwndInput = nullptr;
HWND hwndInputLen = nullptr;

struct sockaddr_in clientService;

//add text
void AppendText(const char* text) {
    int len = GetWindowTextLength(hwndOutput);
    SendMessage(hwndOutput, EM_SETSEL, len, len);
    SendMessage(hwndOutput, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
}

//backup in case
SOCKET ConnectSocket = INVALID_SOCKET;

//Proto functions
typedef int (WINAPI* SendPtr)(SOCKET s, const char* buf, int len, int flags);
typedef int (WINAPI* WSASendPtr)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
typedef int (WINAPI* WSARecvPtr)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
typedef int (WINAPI* ConnectCustom)(SOCKET s, const SOCKADDR* sAddr, int nameLen);
typedef int (WINAPI* RecvCustom)(SOCKET s, char* buf, int len, int flags);
typedef int (WINAPI* SendToCustom)(SOCKET s, const char* buf, int len, int flags,const sockaddr* to,int ToLen);
typedef int (WINAPI* CustomGetaddrinfo)(PCSTR Host, PCSTR port, const ADDRINFOA* pHints, PADDRINFOA* ppResult);

//Lib
HMODULE hLib = LoadLibrary("WS2_32.dll");

//get the internal function 
SendPtr pSend = (SendPtr)GetProcAddress(hLib, "send");
ConnectCustom pConnect = (ConnectCustom)GetProcAddress(hLib, "connect");
WSASendPtr pWsaSend = (WSASendPtr)GetProcAddress(hLib, "WSASend");
RecvCustom pRecv = (RecvCustom)GetProcAddress(hLib, "recv");
WSARecvPtr pWSARecv = (WSARecvPtr)GetProcAddress(hLib, "WSARecv");
SendToCustom pSendTo = (SendToCustom)GetProcAddress(hLib, "sendto");
CustomGetaddrinfo pGetAddrInfo = (CustomGetaddrinfo)GetProcAddress(hLib, "getaddrinfo");

int SERVER_IP;
int PORT;

int TO_SERVER_IP;
int TO_PORT;

PCSTR WSA_TO_SERVER_IP;
PCSTR WSA_TO_PORT;

//For send() hook it to read the buffer and print it  
int WSAAPI MySend(SOCKET s, const char* buf, int len, int flags)
{
    int result = pSend(s, buf, len, flags);
    // Check if it's checked
    if (isSendChecked == BST_CHECKED) {
        AppendText("=======================================\n");
        AppendText("Send() Sent Data : \n");

        // Print buffer content as an array of bytes
        if (isAOBTranslate) {
            AppendText("Buffer content (hex): ");
            for (int i = 0; i < len; ++i)
            {
                // Convert each byte to its hexadecimal representation
                char hex[4];
                sprintf_s(hex, "%02X ", static_cast<unsigned char>(buf[i]));
                AppendText(hex);
            }
        }
        else
        {
            AppendText("Buffer : \n");
            AppendText(buf);
        }
        AppendText("\n");
    }
    return result;
}

//For WSASEnd() hook it to read the buffer and print it                    
int WSAAPI MyWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    int result = pWsaSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
    if (isWsaSendChecked == BST_CHECKED) {

        AppendText("=======================================\n");
        AppendText("WSASend() Sent Data : \n");

        for (DWORD i = 0; i < dwBufferCount; ++i)
        {
            const char* bufferContent = reinterpret_cast<const char*>(lpBuffers[i].buf);
            DWORD bufferLength = lpBuffers[i].len;

            if (isAOBTranslate) {
                // Print buffer content as an array of bytes
                AppendText("Buffer content (hex): ");
                for (DWORD j = 0; j < bufferLength; ++j)
                {
                    // Convert each byte to its hexadecimal representation
                    char hex[4];
                    sprintf_s(hex, "%02X ", static_cast<unsigned char>(bufferContent[j]));
                    AppendText(hex);
                }
            }
            else
            {
                AppendText("Buffer : \n");
                AppendText(bufferContent);
            }
        }

        AppendText("\n");
    }

    return result;
}

// For sendto() hook it to read the buffer and print it
int WSAAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
    // Call the original sendto() function
    int result = pSendTo(s, buf, len, flags, to, tolen);

    // Check if it's checked
    if (isSendToChecked == BST_CHECKED) {
        AppendText("=======================================\n");
        // Extracting the IP and port of the receiver
        const sockaddr_in* clientService = reinterpret_cast<const sockaddr_in*>(to);
        unsigned long ipAddress = clientService->sin_addr.s_addr;

        std::string MyIpAddress = std::to_string(ipAddress);
        const char* IpConstChar = MyIpAddress.c_str();

        TO_SERVER_IP = static_cast<int>(ipAddress);
        TO_PORT = static_cast<int>(ntohs(clientService->sin_port));

        if (result >= 0)
        {
            AppendText("SendTo() Sent Data (hex): \n");

            // Print buffer content as an array of bytes
            if (isAOBTranslate) {
                AppendText("Buffer content (hex): \n");
                for (int i = 0; i < len; ++i)
                {
                    // Convert each byte to its hexadecimal representation
                    char hex[4];
                    sprintf_s(hex, "%02X ", static_cast<unsigned char>(buf[i]));
                    AppendText(hex);
                }
            }
            else
            {
                AppendText("Buffer : \n");
                AppendText(buf);
            }
            AppendText("\n");
        }

        AppendText("\n");
    }

    return result;
}



// For WSARecv() hook it to read the buffer and print it
int WSAAPI MyWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{

    
    // Call the original WSARecv() function
    int result = pWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, dwFlags, lpOverlapped, lpCompletionRoutine);
    if (isWSARecvChecked == BST_CHECKED) {
        AppendText("=======================================\n");
        if (result == 0)
        {
            AppendText("WSARecv : Received Data : \n");
            for (DWORD i = 0; i < dwBufferCount; ++i)
            {
                AppendText(lpBuffers[i].buf);
            }
            AppendText("\n");
        }

        AppendText("\n");
    }
    return result;
}


//For recv() hook it to read the buffer and print it  
int WSAAPI MyRecv(SOCKET s, char* buf, int len, int flags)
{
    // Call the original recv() function
    int result = pRecv(s, buf, len, flags);
    if (isRecvChecked == BST_CHECKED) {
        AppendText("=======================================\n");
        if (result >= 0)
        {
            AppendText("Recv : Received Data : \n");
            AppendText(buf);
            AppendText("\n");
        }

        AppendText("\n");
    }
    return result;
}

//Hook the connect to get the IP and the PORT of the server for (send , recv)
int WSAAPI MyConnect(SOCKET s, const SOCKADDR* sAddr, int nameLen)
{
    // Assuming sAddr is of type SOCKADDR_IN
    const sockaddr_in* clientService = reinterpret_cast<const sockaddr_in*>(sAddr);
    unsigned long ipAddress = clientService->sin_addr.s_addr;

    std::string MyIpAddress = std::to_string(ipAddress);
    const char* IpConstChar = MyIpAddress.c_str();

    SERVER_IP = static_cast<int>(ipAddress);
    PORT = static_cast<int>(ntohs(clientService->sin_port));

    struct in_addr ipAddr;
    ipAddr.s_addr = htonl(ipAddress);

    // Convert to string and print
    std::string ipAddresss = inet_ntoa(ipAddr);

    
    AppendText("\n");
    AppendText("IP address being used is: ");
    AppendText(ipAddresss.c_str());
    AppendText("\n");

    AppendText("Port being used is: ");
    AppendText(std::to_string(ntohs(clientService->sin_port)).c_str());
    AppendText("\n");
    
    return pConnect(s, sAddr, nameLen);
}

int WSAAPI MyGetAddrinfo(PCSTR Host, PCSTR port, const ADDRINFOA* pHints, PADDRINFOA* ppResult)
{
    PCSTR MyHost = Host;
    PCSTR MyPort = port;
    WSA_TO_SERVER_IP = MyHost;
    WSA_TO_PORT = MyPort;

    AppendText("Host : ");
    AppendText((const char*)MyHost);
    AppendText("\n");


    AppendText("Port : ");
    AppendText((const char*)MyPort);

    AppendText("\n");
    return pGetAddrInfo(Host, port, pHints, ppResult);
}


//Events and stuff for the windows
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {


        //creating the menu and text box and more
    case WM_CREATE:
        // Create Buttons
        CreateWindow("STATIC", "Input Packet", WS_CHILD | WS_VISIBLE, 10, 355, 100, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindow("STATIC", "Output Packet", WS_CHILD | WS_VISIBLE, 10, 10, 100, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindow("STATIC", "Input Buffer Length", WS_CHILD | WS_VISIBLE, 10, 495, 150, 20, hwnd, nullptr, nullptr, nullptr);

        hwndInput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", nullptr, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL,
            10, 375, 760, 155, hwnd, nullptr, nullptr, nullptr);

        hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, (LPCSTR)"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 30, 760, 320, hwnd, nullptr, nullptr, nullptr);

        // Create Buttons
        CreateWindow("BUTTON", "Send Packet", WS_CHILD | WS_VISIBLE, 10, 550, 90, 45, hwnd, (HMENU)IDC_READ_BUTTON, nullptr, nullptr);
        CreateWindow("BUTTON", "Reset Output", WS_CHILD | WS_VISIBLE, 120, 550, 90, 45, hwnd, (HMENU)IDC_RESET_BUTTON, nullptr, nullptr);

        // Create checkboxes
        CreateWindow("BUTTON", "Send()", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 230, 550, 90, 25, hwnd, (HMENU)IDC_CHECKBOX_SEND, nullptr, nullptr);
        CreateWindow("BUTTON", "SendTo()", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 330, 550, 90, 25, hwnd, (HMENU)IDC_CHECKBOX_SENDTO, nullptr, nullptr);
        CreateWindow("BUTTON", "WSASend()", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 430, 550, 90, 25, hwnd, (HMENU)IDC_CHECKBOX_WSASEND, nullptr, nullptr);

        // Create checkbox for "Recv" aligned with "send()" checkbox
        CreateWindow("BUTTON", "Recv()", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 230, 580, 90, 25, hwnd, (HMENU)IDC_CHECKBOX_RECV, nullptr, nullptr);
        CreateWindow("BUTTON", "RecvFrom()", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 330, 580, 90, 25, hwnd, (HMENU)IDC_CHECKBOX_RECVFROM, nullptr, nullptr);
        CreateWindow("BUTTON", "WSARecv()", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 430, 580, 90, 25, hwnd, (HMENU)IDC_CHECKBOX_WSARECV, nullptr, nullptr);

        CreateWindow("BUTTON", "AOB", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 550, 570, 90, 25, hwnd, (HMENU)IDC_TRANSLATE_TO_AOB, nullptr, nullptr);

        break;
        //for button commands and stuff
    case WM_COMMAND:
        // Handle button click
        if (LOWORD(wParam) == IDC_READ_BUTTON) {
            // Read text from the input box
            char buffer[5000]; // Adjust the buffer size as needed
            GetWindowText(hwndInput, buffer, sizeof(buffer));


            //To plain text
            if (isAOBTranslate)
            {
                std::string inputText(buffer);
                inputText.erase(std::remove_if(inputText.begin(), inputText.end(), ::isspace), inputText.end());

                std::string asciiText;
                for (size_t i = 0; i < inputText.length(); i += 2) {
                    std::string hexValue = inputText.substr(i, 2);
                    char asciiChar = static_cast<char>(std::stoi(hexValue, nullptr, 16));
                    asciiText += asciiChar;
                }

                const char* finaltest = asciiText.c_str();
                strcpy(buffer, finaltest);
            }
            
            //Send()
            if (isSendChecked == BST_CHECKED) {
                ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                clientService.sin_family = AF_INET;
                clientService.sin_addr.s_addr = SERVER_IP;
                clientService.sin_port = htons(PORT);

                pConnect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));

                //send()
                size_t arraySize = strlen(buffer);
                int SentBytes = pSend(ConnectSocket, (const char*)buffer, static_cast<int>(arraySize), 0);

                if (SentBytes != SOCKET_ERROR) {
                    AppendText("Packet sent successfully !\n");
                    AppendText("\n");
                }

                shutdown(ConnectSocket, SD_SEND);
                closesocket(ConnectSocket);
            }

            
            // Sendto()
            if (isSendToChecked == BST_CHECKED) {
                size_t arraySize = strlen(buffer);
                SOCKET ConnectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Use SOCK_DGRAM for UDP
                sockaddr_in RecvAddr;
                RecvAddr.sin_family = AF_INET;
                RecvAddr.sin_port = htons(TO_PORT);
                RecvAddr.sin_addr.s_addr = TO_SERVER_IP;
                int iResult = sendto(ConnectSocket, (const char*)buffer, static_cast<int>(arraySize), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));

                if (iResult >= 0) {
                    AppendText("Packet sent successfully!\n");
                    AppendText("\n");
                }

                closesocket(ConnectSocket);
            }


            // WSASend()
            if (isWsaSendChecked == BST_CHECKED) {

                size_t arraySize = strlen(buffer);
                struct addrinfo* result = NULL;
                struct addrinfo hints;
                WSAOVERLAPPED SendOverlapped;

                SOCKET ListenSocket = INVALID_SOCKET;
                SOCKET AcceptSocket = INVALID_SOCKET;

                WSABUF DataBuf;
                DWORD SendBytes;
                DWORD Flags;

                int err = 0;
                int rc, i;

                SecureZeroMemory((PVOID)&hints, sizeof(struct addrinfo));

                // wildcard bind address for IPv4
                hints.ai_family = AF_INET;
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_TCP;
                hints.ai_flags = AI_PASSIVE;

                getaddrinfo(WSA_TO_SERVER_IP,WSA_TO_PORT, &hints, &result);
                ListenSocket = socket(result->ai_family,result->ai_socktype, result->ai_protocol);
                
                bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
                listen(ListenSocket, 1);
                
                AcceptSocket = accept(ListenSocket, NULL, NULL);
                
                SecureZeroMemory((PVOID)&SendOverlapped, sizeof(WSAOVERLAPPED));

                SendOverlapped.hEvent = WSACreateEvent();
                
                DataBuf.len = static_cast<int>(arraySize);
                DataBuf.buf = buffer;

                pWsaSend(AcceptSocket,&DataBuf, 1,&SendBytes, 0, &SendOverlapped, NULL);
                
                WSAResetEvent(SendOverlapped.hEvent);
                WSACloseEvent(SendOverlapped.hEvent);
                closesocket(AcceptSocket);
                closesocket(ListenSocket);
                freeaddrinfo(result);
                WSACleanup();
            }

            
        }
        if (LOWORD(wParam) == IDC_RESET_BUTTON) {
            //Reset the output box text
            SendMessage(hwndOutput, WM_SETTEXT, 0, (LPARAM)"");
        }

        isSendChecked = SendMessage(GetDlgItem(hwnd, IDC_CHECKBOX_SEND), BM_GETCHECK, 0, 0);
        isSendToChecked = SendMessage(GetDlgItem(hwnd, IDC_CHECKBOX_SENDTO), BM_GETCHECK, 0, 0);
        isWsaSendChecked = SendMessage(GetDlgItem(hwnd, IDC_CHECKBOX_WSASEND), BM_GETCHECK, 0, 0);


        isRecvChecked = SendMessage(GetDlgItem(hwnd, IDC_CHECKBOX_RECV), BM_GETCHECK, 0, 0);
        isRecvFromChecked = SendMessage(GetDlgItem(hwnd, IDC_CHECKBOX_RECVFROM), BM_GETCHECK, 0, 0);
        isWSARecvChecked = SendMessage(GetDlgItem(hwnd, IDC_CHECKBOX_WSARECV), BM_GETCHECK, 0, 0);

        isAOBTranslate = SendMessage(GetDlgItem(hwnd, IDC_TRANSLATE_TO_AOB), BM_GETCHECK, 0, 0);
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
    HWND hwnd = CreateWindow(wc.lpszClassName, (LPCSTR)"RainBot's Packet Logger", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, 800, 635, nullptr, nullptr, wc.hInstance, nullptr);

    AppendText("Init\n");


    //Init hook
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)pSend, (PVOID)MySend);
    DetourAttach(&(PVOID&)pConnect, (PVOID)MyConnect);
    DetourAttach(&(PVOID&)pRecv, (PVOID)MyRecv);
    DetourAttach(&(PVOID&)pWsaSend, (PVOID)MyWSASend);
    DetourAttach(&(PVOID&)pWSARecv, (PVOID)MyWSARecv);
    DetourAttach(&(PVOID&)pSendTo, (PVOID)MySendTo);
    DetourAttach(&(PVOID&)pGetAddrInfo, (PVOID)MyGetAddrinfo);

    DetourTransactionCommit();

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
