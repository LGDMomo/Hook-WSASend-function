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

//Params of the function WSASend 
DWORD gBufferCount;
WSABUF gsentBuffer;


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

DWORD jmpBackAddrSend;

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

    __asm {
        mov eax, teax; start to restore registers
        mov ebx, tebx
        mov ecx, tecx
        mov edx, tedx
        mov esi, tesi
        mov edi, tedi
        mov ebp, tebp
        mov esp, tesp; end of restore
        mov ebp, esp
        push ebx
        mov ebx, ecx
        jmp[jmpBackAddrSend]
    }
}

BOOL Hook(void* Address, void* OurFunction, int len)
{
    if (len >= 5) //make sure we can fit our hook at the Address where we want to hook (our hook = 1bytes jmp , + 4bytes address where to jump)
    {
        DWORD protection;//dword means its an address
        VirtualProtect(Address, len, PAGE_EXECUTE_READWRITE, &protection);//make it so we can read and write where the address is (Store the previous permission at the address of (protection)

        DWORD realtiveAddress = ((DWORD)OurFunction - (DWORD)Address) - 5;//find out how much we need to jmp by (End - Start) - (jmp size)

        *(BYTE*)Address = 0xE9; //Changing the value at the address we are hooking at , to make it a jmp instruction 0xDEADBEEF (dec [eax]) -> (jmp)
        *(DWORD*)((DWORD)Address + 1) = realtiveAddress; //Make it so : 0xDEADBEEF (dec [eax]) -> (jmp relativeAddress) so it jump to the address where our code will be
        

        //Restore protection
        DWORD temp;
        VirtualProtect(Address, len, protection, &temp);//make it so the new protection is the old protection we stored in the (protection) variable

        return true;
    }
    else
    {
        return false;
    }
}


int Main()
{
    AllocConsole();
    freopen_s(&pFile, "CONOUT$", "w", stdout);

    int HookLength = 5;
    DWORD ToHookSend = 0x0F01CE7;
    jmpBackAddrSend = ToHookSend + HookLength;

    

    if (Hook((void*)ToHookSend, (void*)sendHookFunc, 5))
    {
        std::cout << "Gonna place a jmp at : " << std::hex << ToHookSend << std::endl;
        std::cout << "Jump back address : " << jmpBackAddrSend << std::endl;
        std::cout << "Esp  : " << tesp << std::endl;
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
