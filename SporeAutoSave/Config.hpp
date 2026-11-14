//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2026 Rosalie Wanders <rosalie@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License version 3.
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#pragma once

#include <string>

namespace Config
{
    bool Initialize();

    /// <summary>
    ///		Retrieves the value for keyName, returns defaultValue when not found
    /// </summary>
    std::wstring GetStringValue(std::wstring keyName, std::wstring defaultValue);

    /// <summary>
    ///		Retrieves the boolean value for keyName, returns false when not found
    /// </summary>
    bool GetBoolValue(std::wstring keyName);


    /// <summary>
    ///		Retrieves the integer value for keyName, returns defaultValue when not found
    /// </summary>
    int GetIntValue(std::wstring keyName, int defaultValue);

    /// <summary>
    ///		Sets the value for keyName with value
    /// </summary>
    bool SetValue(std::wstring keyName, std::wstring value);

    /// <summary>
    ///		Sets the value for keyName with value
    /// </summary>
    bool SetValue(std::wstring keyName, int value);

    /// <summary>
    ///		Sets the value for keyName with value
    /// </summary>
    bool SetValue(std::wstring keyName, bool value);
}
