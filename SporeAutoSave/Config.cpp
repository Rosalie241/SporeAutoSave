//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2021 Rosalie Wanders <rosalie@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License version 3.
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//
// SporeServerConfig.cpp : Defines the functions for the static library.
//

#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include "Config.hpp"

#include <filesystem>

#define APP_NAME L"SporeAutoSave"

static std::filesystem::path ConfigPath;

namespace Config
{
    bool Initialize()
    {     
        ConfigPath = Resource::Paths::GetSaveArea(Resource::SaveAreaID::Preferences)->GetLocation();
        ConfigPath += "\\SporeAutoSave.ini";

        // create config file when it doesn't exist
        if (!std::filesystem::is_regular_file(ConfigPath))
        {
            return SetValue(L"IntervalInMinutes", L"10") &&
                    SetValue(L"MaximumAmountOfBackupSaves", L"5") &&
                    SetValue(L"SaveInCellStage", L"1") &&
                    SetValue(L"SaveInCreatureStage", L"1") &&
                    SetValue(L"SaveInTribalStage", L"1" )&&
                    SetValue(L"SaveInCivilizationStage", L"1") &&
                    SetValue(L"SaveInSpaceStage", L"1");
        }

        return true;
    }

    std::wstring GetValue(std::wstring keyName, std::wstring defaultValue)
    {
        wchar_t buf[MAX_PATH];

        GetPrivateProfileStringW(APP_NAME, keyName.c_str(), defaultValue.c_str(), buf, MAX_PATH, ConfigPath.wstring().c_str());

        return std::wstring(buf);
    }

    bool SetValue(std::wstring keyName, std::wstring value)
    {
        return WritePrivateProfileStringW(APP_NAME, keyName.c_str(), value.c_str(), ConfigPath.wstring().c_str());
    }
}