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
#define IDC_BLOCK 1010
#define IDC_BLOCK_RECV 1011


#define WSAEVENT HANDLE

BOOL isSendChecked;
BOOL isSendToChecked;
BOOL isWsaSendChecked;

BOOL isRecvChecked;
BOOL isRecvFromChecked;
BOOL isWSARecvChecked;

BOOL isAOBTranslate;

BOOL isBlocked;

BOOL isBlockedRecv;

#define DEFAULT_PORT 27015

//Constant
HMODULE myhmod;
FILE* pFile = nullptr;
HWND hwndOutput = nullptr;
HWND hwndInput = nullptr;
HWND hwndInputLen = nullptr;


bool CustomPacket = false;


struct sockaddr_in clientService;
WSABUF WSADataInjection;
CHAR* BufferInjected = (char*)"";


//backup in case
SOCKET ConnectSocket = INVALID_SOCKET;

//Proto functions
typedef int (WINAPI* SendPtr)(SOCKET s, const char* buf, int len, int flags);
typedef int (WINAPI* WSASendPtr)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
typedef int (WINAPI* WSARecvPtr)(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
typedef int (WINAPI* ConnectCustom)(SOCKET s, const SOCKADDR* sAddr, int nameLen);
typedef int (WINAPI* RecvCustom)(SOCKET s, char* buf, int len, int flags);
typedef int (WINAPI* SendToCustom)(SOCKET s, const char* buf, int len, int flags, const sockaddr* to, int ToLen);
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

PCSTR WSA_TO_SERVER_IP = "";
int WSA_TO_PORT;


// Function prototype for the AppendTextFromDLL method in the C# application
typedef void(*AppendTextFromDLLFunc)(const char*);

// Global variable to hold the AppendTextFromDLL method pointer
AppendTextFromDLLFunc pAppendTextFromDLL = NULL;

// Function to send text to the C# application via named pipes
void SendTextToCSharp(const char* text)
{
    HANDLE pipe = CreateFile("\\\\.\\pipe\\SendDataPipe", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (pipe != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        WriteFile(pipe, text, strlen(text), &bytesWritten, NULL);
        CloseHandle(pipe);
    }
}


//For send() hook it to read the buffer and print it  
int WSAAPI MySend(SOCKET s, const char* buf, int len, int flags)
{
    int result = pSend(s, buf, len, flags);

    // Check if it's checked
    if (len > 150)
    {
        SendTextToCSharp("=======================================\n");
        SendTextToCSharp("Send() Sent Data : \n");

        // Print buffer content as an array of bytes
        if (isAOBTranslate) {
            SendTextToCSharp("Buffer content (hex): ");
            for (int i = 0; i < len; ++i)
            {
                // Convert each byte to its hexadecimal representation
                char hex[4];
                sprintf_s(hex, "%02X ", static_cast<unsigned char>(buf[i]));
                SendTextToCSharp(hex);
            }
        }
        else
        {
            SendTextToCSharp("Buffer : \n");
            SendTextToCSharp(buf);
        }
        SendTextToCSharp("\n");
    }
        

    return result;
}

//For WSASEnd() hook it to read the buffer and print it                    
int WSAAPI MyWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    
    if (lpBuffers[0].len > 50)
    {

        SendTextToCSharp("=======================================\n");
        SendTextToCSharp("WSASend() Sent Data : \n");

        for (DWORD i = 0; i < dwBufferCount; ++i)
        {

            if (CustomPacket)
            {
                SendTextToCSharp("Packet injection detected successfully!\n");
                SendTextToCSharp("\n");
                lpBuffers[i].buf = lpBuffers[i].buf; // injecting our buffer to the buffer being sent (we replacing it)
                lpBuffers[i].len = strlen(lpBuffers[i].buf);
            }


            SendTextToCSharp("Debug injected packet (hex): ");
            const char* bC = reinterpret_cast<const char*>(lpBuffers[i].buf);
            for (DWORD j = 0; j <strlen(lpBuffers[i].buf); ++j) {
                char hex[4];
                sprintf_s(hex, "%02X ", static_cast<unsigned char>(bC[j]));
                SendTextToCSharp(hex);
            }


            const char* bufferContent = reinterpret_cast<const char*>(lpBuffers[i].buf);
            DWORD bufferLength = lpBuffers[i].len;
            if (isAOBTranslate) {
                // Print buffer content as an array of bytes
                SendTextToCSharp("Buffer content (hex): ");
                for (DWORD j = 0; j < bufferLength; ++j)
                {
                    // Convert each byte to its hexadecimal representation
                    char hex[4];
                    sprintf_s(hex, "%02X ", static_cast<unsigned char>(bufferContent[j]));
                    SendTextToCSharp(hex);
                }
            }
            else
            {
                SendTextToCSharp("Buffer : \n");
                SendTextToCSharp(bufferContent);
            }
        }

        SendTextToCSharp("\n");
    }
    //sending packet
    int result = 1;
    if (true) { //isBlocked != BST_CHECKED
        int result = pWsaSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
        CustomPacket = false;
    }

    return result;
}

// For sendto() hook it to read the buffer and print it
int WSAAPI MySendTo(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen)
{
    // Call the original sendto() function
    int result = pSendTo(s, buf, len, flags, to, tolen);

    // Extracting the IP and port of the receiver
    const sockaddr_in* clientService = reinterpret_cast<const sockaddr_in*>(to);
    unsigned long ipAddress = clientService->sin_addr.s_addr;

    std::string MyIpAddress = std::to_string(ipAddress);
    const char* IpConstChar = MyIpAddress.c_str();

    TO_SERVER_IP = static_cast<int>(ipAddress);
    TO_PORT = static_cast<int>(ntohs(clientService->sin_port));

    if (result >= 0 && len > 200)
    {
        SendTextToCSharp("=======================================\n");
        SendTextToCSharp("SendTo() Sent Data (hex): \n");

        // Print buffer content as an array of bytes
        if (isAOBTranslate) {
            SendTextToCSharp("Buffer content (hex): \n");
            for (int i = 0; i < len; ++i)
            {
                // Convert each byte to its hexadecimal representation
                char hex[4];
                sprintf_s(hex, "%02X ", static_cast<unsigned char>(buf[i]));
                SendTextToCSharp(hex);
            }
        }
        else
        {
            SendTextToCSharp("Buffer : \n");
            SendTextToCSharp(buf);
        }
        SendTextToCSharp("\n");
    }
        
        

    return result;
}



// For WSARecv() hook it to read the buffer and print it
int WSAAPI MyWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{

    int result = 0;
    // Call the original WSARecv() function
    

    if (false) { //If blocked Recieved
        if (strlen(lpBuffers[0].buf) < 90) { // Send only packet smaller than 90 bytes to avoid crashing
            result = pWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, dwFlags, lpOverlapped, lpCompletionRoutine);
        }
    }
    else
    {
        result = pWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, dwFlags, lpOverlapped, lpCompletionRoutine);
    }

    if (false) { //if wsa recv
        SendTextToCSharp("=======================================\n");
        if (result == 0)
        {
            SendTextToCSharp("WSARecv : Received Data : \n");
            for (DWORD i = 0; i < dwBufferCount; ++i)
            {
                SendTextToCSharp(lpBuffers[i].buf);
            }
            SendTextToCSharp("\n");
        }

        SendTextToCSharp("\n");
    }
    return result;
}


//For recv() hook it to read the buffer and print it  
int WSAAPI MyRecv(SOCKET s, char* buf, int len, int flags)
{
    // Call the original recv() function
    int result = pRecv(s, buf, len, flags);
    if (false) {
        SendTextToCSharp("=======================================\n");
        if (result >= 0)
        {
            SendTextToCSharp("Recv : Received Data : \n");
            SendTextToCSharp(buf);
            SendTextToCSharp("\n");
        }

        SendTextToCSharp("\n");
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


    SendTextToCSharp("\n");
    SendTextToCSharp("IP address being used is: ");
    SendTextToCSharp(ipAddresss.c_str());
    SendTextToCSharp("\n");

    SendTextToCSharp("Port being used is: ");
    SendTextToCSharp(std::to_string(ntohs(clientService->sin_port)).c_str());
    SendTextToCSharp("\n");

    return pConnect(s, sAddr, nameLen);
}

int WSAAPI MyGetAddrinfo(PCSTR Host, PCSTR port, const ADDRINFOA* pHints, PADDRINFOA* ppResult)
{

    SendTextToCSharp("Host : ");
    SendTextToCSharp((const char*)Host);
    SendTextToCSharp("\n");


    SendTextToCSharp("Port : ");
    SendTextToCSharp((const char*)port);
    SendTextToCSharp("\n");

    WSA_TO_SERVER_IP = (const char*)Host;
    WSA_TO_PORT = static_cast<int>(PORT);

    return pGetAddrInfo(Host, port, pHints, ppResult);
}

void TranslateToPlainText(const char* buffer)
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
    strcpy((char*)buffer, finaltest);
}

void InjectPacket(const char* buffer)
{
    // Prepare the data to send
    const char* sendData = buffer;
    int sendDataLength = strlen(sendData);

    WSADataInjection.buf = const_cast<char*>(sendData);
    WSADataInjection.len = sendDataLength;

    BufferInjected = WSADataInjection.buf;


    SendTextToCSharp("Packet applied successfully!\n");
    SendTextToCSharp("\n");
    CustomPacket = true;
}

void SendToDll(const char* buffer)
{
    size_t arraySize = strlen(buffer);
    SOCKET ConnectSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Use SOCK_DGRAM for UDP
    sockaddr_in RecvAddr;
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(TO_PORT);
    RecvAddr.sin_addr.s_addr = TO_SERVER_IP;
    int iResult = pSendTo(ConnectSocket, (const char*)buffer, static_cast<int>(arraySize), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));

    if (iResult >= 0) {
        SendTextToCSharp("Packet sent successfully!\n");
        SendTextToCSharp("\n");
    }

    closesocket(ConnectSocket);
}

void SendDll(const char* buffer)
{
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = SERVER_IP;
    clientService.sin_port = htons(PORT);

    pConnect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));

    //send()
    size_t arraySize = strlen(buffer);
    int SentBytes = pSend(ConnectSocket, (const char*)buffer, static_cast<int>(arraySize), 0);

    if (SentBytes != SOCKET_ERROR) {
        SendTextToCSharp("Packet sent successfully !\n");
        SendTextToCSharp("\n");
    }

    shutdown(ConnectSocket, SD_SEND);
    closesocket(ConnectSocket);
}

void Main()
{
    MessageBoxA(0, "Successfully Injected", "RainTool", MB_OK);
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
