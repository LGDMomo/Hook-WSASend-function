#include <windows.h>
#include <iostream>
#include"detours.h"

//Constant
HMODULE myhmod;
FILE* pFile = nullptr;


void HookSend(SOCKET s,const char* buf,int len,int flags)
{

    std::wcout << "===============================" << std::endl;
    std::cout << "Buffer : " << buf << std::endl;
    std::cout << "Buffer length : " << len << std::endl;
    std::wcout << "===============================" << std::endl;

    //return len;
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

    DetourAttach(&(PVOID&)func, (PVOID)HookSend);

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
        myhmod = hModule;
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main, 0, 0, 0);
    }
    return TRUE;
}
