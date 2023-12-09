#pragma once
#include <windows.h>
#include <iostream>
#include"detours.h"

#define WSAAPI                  FAR PASCAL

//Constant
HMODULE myhmod;
FILE* pFile = nullptr;

//struct for correct data handling

typedef struct _OVERLAPPED* LPWSAOVERLAPPED;

typedef struct _WSABUF {
    ULONG len;     /* the length of the buffer */
    _Field_size_bytes_(len) CHAR FAR* buf; /* the pointer to the buffer */
} WSABUF, FAR* LPWSABUF;

typedef void(CALLBACK* LPWSAOVERLAPPED_COMPLETION_ROUTINE)(IN DWORD dwError,IN DWORD cbTransferred,IN LPWSAOVERLAPPED lpOverlapped,IN DWORD dwFlags);


typedef int (WINAPI* SendPtr)(SOCKET s, const char* buf, int len, int flags);
HMODULE hLib = LoadLibrary("WS2_32.dll");
SendPtr pSend = (SendPtr)GetProcAddress(hLib, "send");

int WSAAPI MySend(SOCKET s, const char* buf, int len, int flags)
{
    std::wcout << "===============================" << std::endl;
    std::cout << "Buffer : " << buf << std::endl;
    std::cout << "Buffer length : " << len << std::endl;
    std::wcout << "===============================" << std::endl;
    return pSend(s, buf, len, flags);
}

int WSAAPI HookSend(SOCKET s,const char* buf,int len,int flags)
{
    std::wcout << "===============================" << std::endl;
    std::cout << "Buffer : " << buf << std::endl;
    std::cout << "Buffer length : " << len << std::endl;
    std::wcout << "===============================" << std::endl;

    return len;
}                                                                                                  //,DWORD dwFlags,LPWSAOVERLAPPED lpOverlapped,LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
int WSAAPI HookWSASend(SOCKET s,LPWSABUF lpBuffers,DWORD dwBufferCount,LPDWORD lpNumberOfBytesSent)
{
    std::wcout << "===============================" << std::endl;
    std::wcout << L"Number of bytes sent : " << *lpNumberOfBytesSent << std::endl;
    std::wcout << L"Buffer (encrypted) : " << *lpBuffers->buf << std::endl;
    std::wcout << "===============================" << std::endl;
    return 0;
}


int Main()
{
    AllocConsole();
    freopen_s(&pFile, "CONOUT$", "w", stdout);

    MessageBoxA(0, "Hooked ", "Bye", 0);


    //addr_send = (DWORD)GetProcAddress(GetModuleHandle(TEXT("WS2_32.dll")), "send");
    //hook there /////////////////////////
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    PVOID func = DetourFindFunction("WS2_32.dll", "send");
    
    DetourAttach(&(PVOID&)func, (PVOID)MySend);

    DetourTransactionCommit();
    ////////////////////////


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
    FreeConsole();
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
