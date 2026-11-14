//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2026 Rosalie Wanders <rosalie@mailbox.org>
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
#include <algorithm>

#define APP_NAME L"SporeAutoSave"

static std::filesystem::path ConfigPath;

//
// Local Functions
//

static std::wstring string_lowercase(const std::wstring& string)
{
    std::wstring str = string;
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t c)
        {
            return (c >= L'A' && c <= L'Z') ? (c + L'a' - L'A') : c;
        });
    return str;
}


//
// Exported Functions
//

namespace Config
{
    bool Initialize()
    {     
        ConfigPath = Resource::Paths::GetSaveArea(Resource::SaveAreaID::Preferences)->GetLocation();
        ConfigPath += "\\SporeAutoSave.ini";

        // create config file when it doesn't exist
        if (!std::filesystem::is_regular_file(ConfigPath))
        {
            return SetValue(L"IntervalInMinutes", 10) &&
                    SetValue(L"MaximumAmountOfBackupSaves", 5) &&
                    SetValue(L"SaveInCellStage", true) &&
                    SetValue(L"SaveInCreatureStage", true) &&
                    SetValue(L"SaveInTribalStage", true)&&
                    SetValue(L"SaveInCivilizationStage", true) &&
                    SetValue(L"SaveInSpaceStage", true);
        }

        return true;
    }

    std::wstring GetStringValue(std::wstring keyName, std::wstring defaultValue)
    {
        wchar_t buf[MAX_PATH] = { 0 };

        GetPrivateProfileStringW(APP_NAME, keyName.c_str(), defaultValue.c_str(), buf, MAX_PATH, ConfigPath.wstring().c_str());

        return std::wstring(buf);
    }

    bool GetBoolValue(std::wstring keyName)
    {
        std::wstring value = string_lowercase(GetStringValue(keyName, L"true"));
        return value == L"1" || value == L"true";
    }

    int GetIntValue(std::wstring keyName, int defaultValue)
    {
        std::wstring value = GetStringValue(keyName, L"");
        int ret = defaultValue;
        wchar_t* endptr;

        if (value.empty())
        {
            return defaultValue;
        }

        ret = wcstol(value.c_str(), &endptr, 10);
        if (errno != ERANGE && endptr == (value.c_str() + value.size()))
        {
            return ret;
        }
        
        return defaultValue;
    }

    bool SetValue(std::wstring keyName, std::wstring value)
    {
        return WritePrivateProfileStringW(APP_NAME, keyName.c_str(), value.c_str(), ConfigPath.wstring().c_str());
    }

    bool SetValue(std::wstring keyName, int value)
    {
        return SetValue(keyName, std::to_wstring(value));
    }

    bool SetValue(std::wstring keyName, bool value)
    {
        return SetValue(keyName, std::wstring(value ? L"true" : L"false"));
    }
}