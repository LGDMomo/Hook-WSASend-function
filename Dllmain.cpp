#include <windows.h>
#include <iostream>
#include <list>
#include <psapi.h>
#include<vector>
#include"Hook.h"

//Constant
HMODULE myhmod;
FILE* pFile = nullptr;
int sendHookLen{ 5 };

typedef struct _WSABUF {
    ULONG len;
    CHAR* buf;
} WSABUF, * LPWSABUF;

DWORD gBufferCount;
WSABUF gsentBuffer;
DWORD jmpBackAddrSend;



//additional "resgisters" to use for tmp values:
void* teax;
void* tebx;
void* tecx;
void* tedx;
void* tesi;
void* tedi;
void* tebp;
void* tesp;
void* thisPTR;

void __declspec(naked) sendHookFunc() {
    __asm {
        mov thisPTR, ecx
        mov teax, eax; backup registers
        mov tebx, ebx
        mov tecx, ecx
        mov tedx, edx
        mov tesi, esi
        mov tedi, edi
        mov tebp, ebp
        mov tesp, esp; end of backup
        mov eax, [esp + 0x8]
        //mov gsentBuffer, eax
        //mov eax, [esp + 0xC]
        //mov sentLen, eax
    }

    /*In the next block we restore our registers and execute the overwritten code where our hook got placed.
    Then we jump back to normal execution.
    The jmpBackAddrSend variable contains the correct address! DONT WRITE TO IT!*/
    __asm {

        jmp[jmpBackAddrSend]
    }
}

int Main()
{

    AllocConsole();
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    DWORD NumberOfBytesSent = 0;

    typedef void(__thiscall* InternalSend)(void* thispointer, LPWSABUF Buffer,DWORD BufferCount); //Send function signature
    InternalSend Send = reinterpret_cast<InternalSend>(0x0F020A0); //call 013720A0
    DWORD ToHookSend{ 0 };

    ToHookSend += 0x0F01CE7;
    jmpBackAddrSend = ToHookSend + 5;



    std::cout << "Gonna place a jmp at : " << std::hex << ToHookSend << std::endl;
    std::cout << "Jump back address : " << jmpBackAddrSend << std::endl;
    std::cout << "Call like this (thisptr) : " << thisPTR << std::endl;
    Hook sendHook{ reinterpret_cast<void*>(0x0F01CE7), reinterpret_cast<void*>(sendHookFunc), 5 };

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
