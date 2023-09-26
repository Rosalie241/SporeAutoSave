//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2021 Rosalie Wanders <rosalie@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License version 3.
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#pragma once

#include <iostream>

namespace Config
{
    bool Initialize();

    /// <summary>
    ///		Retrieves the value for keyName, returns defaultValue when not found
    /// </summary>
    /// <param name="keyName"></param>
    /// <param name="defaultValue"></param>
    /// <returns></returns>
    std::wstring GetValue(std::wstring keyName, std::wstring defaultValue);

    /// <summary>
    ///		Sets the value for keyName with value
    /// </summary>
    /// <param name="keyName"></param>
    /// <param name="value"></param>
    /// <returns>Whether setting the value was successful</returns>
    bool SetValue(std::wstring keyName, std::wstring value);
}
