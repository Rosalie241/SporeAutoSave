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

		// TODO, check if in menu, sporepedia, settings..
		bool shouldSave = Simulator::IsCellGame() ||
							Simulator::IsTribeGame() ||
							Simulator::IsCivGame() ||
							Simulator::IsSpaceGame();

		// reset timer
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
BOOL APIENTRY DllMain( HMODULE hModule,
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

