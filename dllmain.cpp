// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include <iostream>

#define KeyDown -32768
#define KeyUp 0

struct values
{
    uintptr_t moduleBase;
    uintptr_t localPlayer;
    int myTeam;
    int tbDelay;
    int myWeaponId;
}val;

struct gameOffsets
{
    uintptr_t dwLocalPlayer = 0xDBF4CC;
    uintptr_t dwForceAttack = 0x320BE10;
    uintptr_t dwEntityList = 0x4DDB92C;
    uintptr_t m_iCrosshairId = 0x11838;
    uintptr_t m_iTeamNum = 0xF4;
    uintptr_t m_iHealth = 0x100;
    uintptr_t m_vecOrigin = 0x138;
    uintptr_t m_hActiveWeapon = 0x2F08;
    uintptr_t m_iItemDefinitionIndex = 0x2FBA;
    uintptr_t m_bIsScoped = 0x9974;
}offsets;

struct vector
{
    float x, y, z;
};

bool checkIfScoped()
{
    return *(int*)(val.localPlayer + offsets.m_bIsScoped);
}

void setTBDelay(float distance)
{
    float delay;
    switch (val.myWeaponId)
    {
    case 60: delay = 3; break; // M4A1-S
    case 7: delay = 3.3; break; // AK
    case 40: delay = 0.15; break; // SCOUT
    case 9: delay = 0.15; break; // AWP
    case 38: delay = 0.5; break; // SCAR-20
    default: delay = 0;
    }
    val.tbDelay = delay * distance;
    std::cout << val.tbDelay << std::endl;
}

void getWeapon()
{
    int weapon = *(int*)(val.localPlayer + offsets.m_hActiveWeapon);
    int weaponEntity = *(int*)(val.moduleBase + offsets.dwEntityList + ((weapon & 0xFFF) - 1) * 0x10);
    if (weaponEntity != NULL)
        val.myWeaponId = *(int*)(weaponEntity + offsets.m_iItemDefinitionIndex);

    //std::cout << val.myWeaponId << std::endl;
}

float getDistance(uintptr_t entity)
{
    vector myLocation = *(vector*)(val.localPlayer + offsets.m_vecOrigin);
    vector enemyLocation = *(vector*)(entity + offsets.m_vecOrigin);
    
    return sqrt(pow(myLocation.x - enemyLocation.x, 2) + pow(myLocation.y - enemyLocation.y, 2) + pow(myLocation.z - enemyLocation.z, 2)) * 0.0254;
}

void shoot()
{
    Sleep(val.tbDelay);
    *(int*)(val.moduleBase + offsets.dwForceAttack) = 5;
    Sleep(20);
    *(int*)(val.moduleBase + offsets.dwForceAttack) = 4;
}

bool checkTBot()
{
    int crosshair = *(int*)(val.localPlayer + offsets.m_iCrosshairId);
    
    if (crosshair > 0 && crosshair < 64)
    {
        uintptr_t entity = *(uintptr_t*)(val.moduleBase + offsets.dwEntityList + (crosshair - 1) * 0x10);
        int team = *(int*)(entity + offsets.m_iTeamNum);
        int health = *(int*)(entity + offsets.m_iHealth);
        if (val.myTeam != team && health > 0)
        {
            float distance = getDistance(entity);
            getWeapon();
            setTBDelay(distance);
            if (val.myWeaponId == 40 || val.myWeaponId == 9 || val.myWeaponId == 38)
                return checkIfScoped();
            else
                return true;
        }
        else
            return false;
    }
    return false;
}

void handleTBot()
{
    if (checkTBot())
        shoot();
}

DWORD WINAPI HackThread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    bool bTriggerbot = false, keyHeld = false;

    val.moduleBase = (uintptr_t)GetModuleHandle(L"client.dll");

    val.localPlayer = *(uintptr_t*)(val.moduleBase + offsets.dwLocalPlayer);

    

    while (!GetAsyncKeyState(VK_END))
    {
        
        if (GetAsyncKeyState(VK_NUMPAD1) & 1)
        {
            val.myTeam = *(int*)(val.localPlayer + offsets.m_iTeamNum);
            bTriggerbot = !bTriggerbot;
        }

        if (GetAsyncKeyState(VK_F2) == KeyDown && !keyHeld)
        {
            keyHeld = true;
            bTriggerbot = true;
        }

        if (GetAsyncKeyState(VK_F2) == KeyUp && keyHeld)
        {
            keyHeld = false;
            bTriggerbot = false;
        }


        if (bTriggerbot)
            handleTBot();

        Sleep(1);
    }

    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
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
    {
        HANDLE hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, 0);
        if (hThread) CloseHandle(hThread);
    }
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

