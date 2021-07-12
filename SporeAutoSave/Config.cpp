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

#define APP_NAME "SporeAutoSave"

static std::string ConfigPath;

namespace Config
{
    bool Initialize()
    {
        char* appdata = getenv("APPDATA");

        if (appdata == nullptr)
        {
            // shouldn't happen
            return false;
        }

        ConfigPath = appdata;
        ConfigPath += "\\Spore\\Preferences\\SporeAutoSave.ini";

        // create config file when it doesn't exist
        if (GetFileAttributesA(ConfigPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            return SetValue("IntervalInMinutes", "10");
        }

        return true;
    }

    std::string GetValue(std::string keyName, std::string defaultValue)
    {
        char buf[MAX_PATH];

        GetPrivateProfileStringA(APP_NAME, keyName.c_str(), defaultValue.c_str(), buf, MAX_PATH, ConfigPath.c_str());

        return std::string(buf);
    }

    bool SetValue(std::string keyName, std::string value)
    {
        return WritePrivateProfileStringA(APP_NAME, keyName.c_str(), value.c_str(), ConfigPath.c_str());
    }
}