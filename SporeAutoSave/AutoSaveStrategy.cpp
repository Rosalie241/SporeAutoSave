//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2021 Rosalie Wanders <rosalie@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License version 3.
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#include "stdafx.h"
#include "AutoSaveStrategy.hpp"

std::vector<std::filesystem::path> AutoSaveStrategy::GetBackupSaveList()
{
    std::vector<std::filesystem::path> backupSaveList;

    for (const auto& entry : std::filesystem::directory_iterator(m_BackupSavePath))
    {
        std::string path = entry.path().string();
        if (path.find(".Backup.") == std::string::npos ||
            !std::filesystem::is_directory(entry))
        { // skip directories which don't contain .Backup.
          // or aren't a directory
            continue;
        }

        backupSaveList.push_back(entry.path());
    }

    // sort directories, make sure the oldest backup save is first
    std::sort(backupSaveList.begin(), backupSaveList.end(), [](std::filesystem::path& a, std::filesystem::path& b)
    {
        auto aTime = std::filesystem::last_write_time(a);
        auto bTime = std::filesystem::last_write_time(b);

        return aTime < bTime;
    });

    return backupSaveList;
}

bool AutoSaveStrategy::BackupSave()
{
    if (!std::filesystem::exists(m_BackupSavePath))
    { // do nothing if the save directory doesn't exist (yet)
        return true;
    }

    auto backupSaveList = GetBackupSaveList();

    // make sure we only keep a certain amount of backup saves
    if (backupSaveList.size() >= m_MaximumAmountOfBackupSaves)
    {
        std::filesystem::path oldestBackupSave = backupSaveList.at(0);
        try
        {
            std::filesystem::remove_all(oldestBackupSave);
        }
        catch (...)
        {
            App::ConsolePrintF("SporeAutoSave: std::filesystem::remove_all(\"%s\") failed!", oldestBackupSave.string().c_str());
            return false;
        }
    }

    char backupDirectoryBuffer[256] = { 0 };
    time_t currentTime = time(nullptr);
    tm currentTimeTm;

    if (localtime_s(&currentTimeTm, &currentTime) != 0)
    {
        App::ConsolePrintF("SporeAutoSave: localtime_s() failed!");
        return false;
    }

    strftime(backupDirectoryBuffer, sizeof(backupDirectoryBuffer), "\\Game0.Backup.%F.%H_%M_%S", &currentTimeTm);

    std::filesystem::path currentSavePath = Resource::Paths::GetSaveArea(Resource::SaveAreaID::GamesGame0)->GetLocation();;
    std::filesystem::path backupSavePath  = m_BackupSavePath;
    backupSavePath += backupDirectoryBuffer;

    try
    {
        std::filesystem::copy(currentSavePath, backupSavePath, std::filesystem::copy_options::recursive);
    }
    catch (...)
    {
        App::ConsolePrintF("SporeAutoSave: std::filesystem::copy(\"%s\", \"%s\") failed!", currentSavePath.c_str(), backupSavePath.c_str());
        return false;
    }

    return true;
}

void AutoSaveStrategy::SaveGame()
{
    CALL(Address(ModAPI::ChooseAddress(0x00e54560, 0x00e53f00)), void, Args(void), Args());
}

void AutoSaveStrategy::OnModeEntered(uint32_t previousModeID, uint32_t newModeID)
{
    switch (newModeID)
    {
    default:
        m_IsInValidMode = false;
        break;

    case GameModeIDs::kGameCell:
    case GameModeIDs::kGameCreature:
    case GameModeIDs::kGameTribe:
    case GameModeIDs::kGameCiv:
    case GameModeIDs::kGameSpace:
        m_NextSaveTime = std::chrono::high_resolution_clock::now() + std::chrono::minutes(m_SaveIntervalInMinutes);
        m_IsInValidMode = true;
        break;
    }
}

void AutoSaveStrategy::Update(int deltaTime, int deltaGameTime)
{
    // don't do anything when we aren't in a valid mode,
    // or when we're paused
    if (!m_IsInValidMode || m_IsPaused)
    {
        return;
    }
    
    // also do nothing when we're on a planet in space,
    // Question: find out how the game knows we can save?
    // Answer: see 0x00e030d9 (patched), it does the same
    if (Simulator::IsSpaceGame() &&
        Simulator::GetCurrentContext() == Simulator::SpaceContext::Planet)
    {
        return;
    }

    // also do nothing when the time hasnt expired yet
    if (std::chrono::high_resolution_clock::now() < m_NextSaveTime)
    {
        return;
    }

    if (!BackupSave())
    {
        App::ConsolePrintF("SporeAutoSave: BackupSave() failed, skipping save!");
        // try again in 1 minute when failed
        // to prevent console spam
        m_NextSaveTime =
            std::chrono::high_resolution_clock::now() +
            std::chrono::minutes(1);
        return;
    }

    SaveGame();
}

std::vector<uint32_t> AutoSaveStrategy::GetHandledMessageIDs(void)
{
    std::vector<uint32_t> handledMessageIDs;

    handledMessageIDs.push_back((uint32_t)MessageID::OnSave);
    handledMessageIDs.push_back((uint32_t)MessageID::OnPauseToggled);

    return handledMessageIDs;
}

bool AutoSaveStrategy::HandleMessage(uint32_t messageID, void* message)
{
    if (!m_IsInValidMode)
    {
        return false;
    }

    switch (messageID)
    {
    default:
        break;

    case (uint32_t)MessageID::OnSave:
        m_NextSaveTime = std::chrono::high_resolution_clock::now() +
                         std::chrono::minutes(m_SaveIntervalInMinutes);
        break;

    case (uint32_t)MessageID::OnPauseToggled:
        if (m_IsPaused && message == nullptr)
        { // unpaused
            m_NextSaveTime =
                std::chrono::high_resolution_clock::now() + 
                std::chrono::seconds(m_TimeLeftUntilSaveWhenPausedInSeconds);
        }
        else if (message != nullptr)
        { // paused
            m_TimeLeftUntilSaveWhenPausedInSeconds =
                std::chrono::duration_cast<std::chrono::seconds>(m_NextSaveTime - std::chrono::high_resolution_clock::now()).count();
        }
        m_IsPaused = (message != nullptr);
        break;
    }

    return false;
}

