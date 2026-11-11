//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2021 Rosalie Wanders <rosalie@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License version 3.
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//
// dllmain.cpp : Defines the entry point for the DLL application.

//
// Includes
// 

#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "Config.hpp"
#include "AutoSaveStrategy.hpp"

#include <thread>
#include <string>
#include <filesystem>
#include <chrono>

//
// Helper functions
//

static void DisplayError(const char* fmt, ...)
{
    char buf[200];

    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    MessageBoxA(NULL, buf, "SporeAutoSave", MB_OK | MB_ICONERROR);
}

//
// Boilerplate
//

void Initialize()
{
    int autoSaveIntervalInMinutes = 0;
    int maximumAmountOfBackupSaves = 0;
    std::filesystem::path backupSavePath;
    wchar_t envBuffer[MAX_PATH];

    if (!Config::Initialize())
    {
        DisplayError("Config::Initialize() Failed!");
        return;
    }

    std::wstring intervalString = Config::GetValue(L"IntervalInMinutes", L"10");
    try
    {
        autoSaveIntervalInMinutes = std::stoi(intervalString);

        // safety check
        if (autoSaveIntervalInMinutes <= 0)
        {
            throw std::exception();
        }
    }
    catch (...)
    {
        DisplayError("IntervalInMinutes is invalid, disabling SporeAutoSave!");
        return;
    }

    backupSavePath = Resource::Paths::GetDirFromID(Resource::PathID::AppData);
    backupSavePath += "\\Games";

    std::wstring maximumBackupsString = Config::GetValue(L"MaximumAmountOfBackupSaves", L"5");
    try
    {
        maximumAmountOfBackupSaves = std::stoi(maximumBackupsString);

        // safety check
        if (maximumAmountOfBackupSaves <= 0)
        {
            throw std::exception();
        }
    }
    catch (...)
    {
        DisplayError("MaximumAmountOfBackupSaves is invalid, disabling SporeAutoSave!");
        return;
    }

    static AutoSaveStrategy autoSaveStrategy(autoSaveIntervalInMinutes, maximumAmountOfBackupSaves, backupSavePath);

    // add all message IDs
    for (const uint32_t messageID : autoSaveStrategy.GetHandledMessageIDs())
    {
        MessageManager.AddUnmanagedListener(&autoSaveStrategy, messageID);
    }

    SimulatorSystem.AddStrategy(&autoSaveStrategy, id("SporeAutoSave"));
}

void Dispose()
{
    // This method is called when the game is closing
}

void AttachDetours()
{   
    // Call the attach() method on any detours you want to add
    // For example: cViewer_SetRenderType_detour::attach(GetAddress(cViewer, SetRenderType));
}


// Generally, you don't need to touch any code here
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ModAPI::AddPostInitFunction(Initialize);
        ModAPI::AddDisposeFunction(Dispose);

        PrepareDetours(hModule);
        AttachDetours();
        CommitDetours();
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

