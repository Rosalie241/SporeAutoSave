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

#include <thread>
#include <string>

//
// Global variables
//

static std::thread g_AutoSaveThread;
static bool g_Initialized = false;
static int g_AutoSaveInterval = -1;
static bool g_ManuallySavedByUser = false;
static const uint32_t g_RequiredWindowList[] =
{
    // options button
    // 0x9DB9E495.spui
    0x010ED852
};
static const uint32_t g_WindowExclusionList[] =
{
    // options window
    // 0x9DB9E495.spui
    0x0615B50D,

    // space stage communication panel
    // 0x79C4F18B.spui
    0x01C3BB0C,
    // 0x9831B38E.spui
    0x0755F180,
    // 0xCC65C2D4.spui
    0x076B3543,

    // messagebox
    // 0x02A6B9AE.spui
    0x01510D07,
};
// g_RequiredWindowList & g_WindowExclusionList
static const uint32_t g_WindowList[] =
{
    0x010ED852,
    0x0615B50D,
    0x01C3BB0C,
    0x01510D07,
};

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

static bool ArrayContains(const uint32_t list[], int listSize, uint32_t item)
{
    for (int i = 0; i < listSize; i++)
    {
        uint32_t listItem = list[i];
        if (listItem == item)
        {
            return true;
        }
    }

    return false;
}

//
// Detour functions
//

// TODO: find save function when user presses ctrl + s
// and set g_ManuallySavedByUser there

// save function from game
static_detour(GameSaveFunction, void(void))
{
    void detoured(void)
    {
        g_ManuallySavedByUser = true;
        return original_function();
    }
};

//
// Thread functions
//
static void AutosaveThreadFunc()
{
    // when we just got ingame, we don't want to save directly,
    // so add an initial sleep bool
    bool justInGameHasSlept = false;

    // we need a manual timer sadly,
    // because when we're ingame and 
    // i.e the user has just entered a game, 
    // the timer should reset,
    // if I use sleep(specified duration),
    // it's possible that the duration doubles,
    // or that the duration is less than specified
    // due to jumping ingame mid-sleep.
    int secondsPassed = 0;
    int minutesPassed = 0;

    while (true)
    {
        bool specifiedTimePassed = minutesPassed >= g_AutoSaveInterval;

        bool shouldSave = !Simulator::IsLoadingGameMode() &&
            (
                Simulator::IsCellGame() ||
                Simulator::IsCreatureGame() ||
                Simulator::IsTribeGame() ||
                Simulator::IsCivGame() ||
                ( // when we're in the space stage, make sure we're not on a planet
                    Simulator::IsSpaceGame() &&
                    (
                        Simulator::GetCurrentContext() != Simulator::SpaceContext::kSpaceContextPlanet
                    )
                )
           );

        if (shouldSave)
        { // verify window states
            UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();

            for (uint32_t windowId : g_WindowList)
            {
                UTFWin::IWindow* window = mainWindow == nullptr ? nullptr : mainWindow->FindWindowByID(windowId);

                UTFWin::WindowFlags flags = window == nullptr ?
                    (UTFWin::WindowFlags)0 :
                    window->GetFlags();

                bool windowVisible = window == nullptr ? false : window->IsVisible();

                // iterate over each parent and make sure it's visible
                if (windowVisible)
                {
                    UTFWin::IWindow* parent = window->GetParent();
                    while (parent != nullptr)
                    {
                        if (!parent->IsVisible())
                        {
                            windowVisible = false;
                            break;
                        }

                        parent = parent->GetParent();
                    }
                }

                if (windowVisible)
                { // if window is visible and in the exclusion list, don't save
                    if (ArrayContains(g_WindowExclusionList, ARRAYSIZE(g_WindowExclusionList), windowId))
                    {
                        shouldSave = false;
                        break;
                    }
                }
                else
                { // if window is not visible and in the required list, don't save
                   if (ArrayContains(g_RequiredWindowList, ARRAYSIZE(g_RequiredWindowList), windowId))
                    {
                        shouldSave = false;
                        break;
                    }
                }
            }
        }

        // reset timer when needed
        if (specifiedTimePassed)
        {
            secondsPassed = minutesPassed = 0;
        }

        if (g_ManuallySavedByUser)
        { // handle manually saving by user
            g_ManuallySavedByUser = false;
            justInGameHasSlept = true;
        }
        else if (!justInGameHasSlept && shouldSave)
        { // we can save but just got ingame
            justInGameHasSlept = true;
        }
        else if (shouldSave)
        { // call the save function when we're in a game
            if (specifiedTimePassed)
            {
                GameSaveFunction::original_function();
            }
        }
        else
        { // if we're not in a game, keep retrying until we are
            justInGameHasSlept = false;
            // let's not spin up the cpu too much...
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // let's not spin up the cpu too much
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // increase seconds & minutes
        secondsPassed++;
        if (secondsPassed >= 60)
        {
            secondsPassed = 0;
            minutesPassed++;
        }
    }
}

//
// Boilerplate
//

void Initialize()
{
    if (!g_Initialized)
    {
        return;
    }

    if (!Config::Initialize())
    {
        DisplayError("Config::Initialize() Failed!");
        return;
    }

    std::string intervalString = Config::GetValue("IntervalInMinutes", "10");
    try
    {
        g_AutoSaveInterval = std::stoi(intervalString);

        // safety check
        if (g_AutoSaveInterval <= 0)
        {
            throw std::exception();
        }
    }
    catch (std::exception)
    {
        DisplayError("IntervalInMinutes Is Invalid, Disabling SporeAutoSave!");
        return;
    }

    g_AutoSaveThread = std::thread(AutosaveThreadFunc);

    // This method is executed when the game starts, before the user interface is shown
    // Here you can do things such as:
    //  - Add new cheats
    //  - Add new simulator classes
    //  - Add new game modes
    //  - Add new space tools
    //  - Change materials
}

void Dispose()
{
    // This method is called when the game is closing
}

void AttachDetours()
{
    DWORD_PTR base_addr = (DWORD_PTR)GetModuleHandle(NULL);
    LONG ret = 0;

    // RVA latest = 0xA53F00
    ret = GameSaveFunction::attach(base_addr + 0xA53F00);
    if (ret != NO_ERROR)
    {
        DisplayError("GameSaveFunction::attach() Failed: %li", ret);
        return;
    }

    g_Initialized = true;

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

