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
static int g_AutoSaveInterval;

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

// save function from game
static_detour(GameSaveFunction, void(void))
{
	void detoured(void)
	{
		return original_function();
	}
};

//
// Thread functions
//

static void AutosaveThreadFunc()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::minutes(g_AutoSaveInterval));
		GameSaveFunction::original_function();
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

