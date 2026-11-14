//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2026 Rosalie Wanders <rosalie@mailbox.org>
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

#include <string>
#include <filesystem>

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

    MessageBoxA(nullptr, buf, "SporeAutoSave", MB_OK | MB_ICONERROR);
}

//
// Boilerplate
//

void Initialize()
{
    std::filesystem::path backupSavePath;

    if (!Config::Initialize())
    {
        DisplayError("Config::Initialize() Failed!");
        return;
    }

    int intervalInMinutes = Config::GetIntValue(L"IntervalInMinutes", -1);
    if (intervalInMinutes <= 0)
    {
        DisplayError("IntervalInMinutes is invalid, disabling SporeAutoSave!");
        return;
    }

    int maximumAmountOfBackupSaves = Config::GetIntValue(L"MaximumAmountOfBackupSaves", -1);
    if (maximumAmountOfBackupSaves <= 0)
    {
        DisplayError("MaximumAmountOfBackupSaves is invalid, disabling SporeAutoSave!");
        return;
    }

    backupSavePath = Resource::Paths::GetDirFromID(Resource::PathID::AppData);
    backupSavePath += "\\Games";

    static AutoSaveStrategy autoSaveStrategy(intervalInMinutes, maximumAmountOfBackupSaves, backupSavePath);

    // add all message IDs
    for (const uint32_t messageID : autoSaveStrategy.GetHandledMessageIDs())
    {
        MessageManager.AddUnmanagedListener(&autoSaveStrategy, messageID);
    }

    SimulatorSystem.AddStrategy(&autoSaveStrategy, id("SporeAutoSave"));
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ModAPI::AddPostInitFunction(Initialize);

        PrepareDetours(hModule);
        AutoSaveStrategy::AttachDetours();
        CommitDetours();
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

