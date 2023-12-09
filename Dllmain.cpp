#include <windows.h>
#include <iostream>
#include"detours.h"

//Constant
HMODULE myhmod;
FILE* pFile = nullptr;

typedef struct _WSABUF {
    ULONG len;
    CHAR* buf;
} WSABUF, * LPWSABUF;

int WsaSendHook(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesSent,
    DWORD                              dwFlags
) 
{

    std::wcout << "===============================" <<  std::endl;
    std::wcout << L"lpNumberOfBytesSent : " << lpNumberOfBytesSent << std::endl;
    std::wcout << L"Buffer (encrypted) : " << lpBuffers->buf << std::endl;
    return 0;
}


int Main()
{
    AllocConsole();
    freopen_s(&pFile, "CONOUT$", "w", stdout);


    //hook there
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    PVOID func = DetourFindFunction("WS2_32.dll", "WSASend");
    DetourAttach(&(PVOID&)func, (PVOID)WsaSendHook);


    DetourTransactionCommit();


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
        myhmod = hModule;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
    }
    return TRUE;
}
