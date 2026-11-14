//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2026 Rosalie Wanders <rosalie@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License version 3.
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#include "stdafx.h"
#include "AutoSaveStrategy.hpp"
#include "Config.hpp"

#include <Spore\UTFWin\cSPUIMessageBox.h>

#include <atomic>

//
// Local Variables
//

static std::atomic<bool> l_SkipSaveDialog = false;

//
// Detours
//

// this is sadly kinda hacky, but we cannot disable the 'the game on planet X has been saved' dialog creation
// because it's called in the middle of a function (see march2017 0x00b29e29), so as a workaround
// detour UTFWin::cSPUIMessageBox::ShowDialog() and check for the correct resource key IDs alongside
// a local variable boolean to check if we should skip it.
static_detour(ShowDialogDetour, bool(UTFWin::MessageBoxCallback*, ResourceKey*))
{
    bool detoured(UTFWin::MessageBoxCallback* callback, ResourceKey* name)
    {
        // skip showing dialog when the resource key matches
        // and the local variable to skip the dialog is set
        if (callback != nullptr && name != nullptr)
        {
            if (l_SkipSaveDialog.load() &&
                name->typeID == 0x00B1B104 &&
                name->groupID == 0xF5F6FDE8 &&
                name->instanceID == 0x478A48AB)
            {
                callback->OnButtonPress(nullptr, UTFWin::cSPUIMessageBox::kButtonOK);
                l_SkipSaveDialog.store(false);
                return true;
            }
        }

        return original_function(callback, name);
    }
};

//
// Exported Functions
//

std::vector<std::filesystem::path> AutoSaveStrategy::GetBackupSaveList(std::filesystem::path saveName)
{
    std::vector<std::filesystem::path> backupSaveList;
    std::wstring backupPathPrefix = saveName.wstring() + L".Backup.";

    for (const auto& entry : std::filesystem::directory_iterator(m_BackupSavePath))
    {
        std::wstring path = entry.path().wstring();
        if (path.find(backupPathPrefix) == std::wstring::npos ||
            !std::filesystem::is_directory(entry))
        { // skip directories which don't contain .Backup.
          // or aren't a directory
            continue;
        }

        backupSaveList.push_back(entry.path());
    }

    // sort directories, make sure the oldest backup save is first
    std::sort(backupSaveList.begin(), backupSaveList.end(), [](const std::filesystem::path& a, const std::filesystem::path& b)
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

    std::filesystem::path currentSavePath = Resource::Paths::GetSaveArea(Resource::SaveAreaID::GamesGame0)->GetLocation();
    std::filesystem::path backupSavePath = m_BackupSavePath;
    std::filesystem::path saveName = currentSavePath.has_filename() ? currentSavePath.filename() : currentSavePath.parent_path().filename();
    std::vector<std::filesystem::path> backupSaveList = GetBackupSaveList(saveName);
    std::error_code errorCode;

    // make sure we only keep a certain amount of backup saves
    if (backupSaveList.size() >= m_MaximumAmountOfBackupSaves)
    {
        const std::filesystem::path& oldestBackupSave = backupSaveList.at(0);
        std::filesystem::remove_all(oldestBackupSave, errorCode);
        if (errorCode)
        {
            App::ConsolePrintF("SporeAutoSave: std::filesystem::remove_all(\"%s\") failed: %s",
                oldestBackupSave.string().c_str(), errorCode.message().c_str());
            return false;
        }
    }

    char buffer[256] = { 0 };
    time_t currentTime = time(nullptr);
    tm currentTimeTm = { 0 };

    errno_t err = localtime_s(&currentTimeTm, &currentTime);
    if (err != 0)
    {
        strerror_s(buffer, sizeof(buffer), err);
        App::ConsolePrintF("SporeAutoSave: localtime_s() failed: %s", buffer);
        return false;
    }

    strftime(buffer, sizeof(buffer), ".Backup.%F.%H_%M_%S", &currentTimeTm);

    backupSavePath += L"\\" + saveName.wstring();
    backupSavePath += buffer;

    std::filesystem::copy(currentSavePath, backupSavePath, std::filesystem::copy_options::recursive, errorCode);
    if (errorCode)
    {
        App::ConsolePrintF("SporeAutoSave: std::filesystem::copy(\"%s\", \"%s\") failed: %s", 
            currentSavePath.c_str(), backupSavePath.c_str(), errorCode.message().c_str());
        return false;
    }

    return true;
}

void AutoSaveStrategy::SaveGame()
{
    l_SkipSaveDialog.store(true);
    MessageManager.MessageSend(static_cast<uint32_t>(MessageID::OnSave), nullptr);
}

void AutoSaveStrategy::OnModeEntered(uint32_t previousModeID, uint32_t newModeID)
{
    std::wstring value = L"invalid";

    switch (newModeID)
    {
    default:
        m_IsInValidMode = false;
        break;

    case GameModeIDs::kGameCell:
        value = Config::GetValue(L"SaveInCellStage", L"1");
        break;
    case GameModeIDs::kGameCreature:
        value = Config::GetValue(L"SaveInCreatureStage", L"1");
        break;
    case GameModeIDs::kGameTribe:
        value = Config::GetValue(L"SaveInTribalStage", L"1");
        break;
    case GameModeIDs::kGameCiv:
        value = Config::GetValue(L"SaveInCivilizationStage", L"1");
        break;
    case GameModeIDs::kGameSpace:
        value = Config::GetValue(L"SaveInSpaceStage", L"1");
        break;
    }

    if (value.empty() || value == L"1")
    {
        m_NextSaveTime = std::chrono::high_resolution_clock::now() + std::chrono::minutes(m_SaveIntervalInMinutes);
        m_IsInValidMode = true;
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

    handledMessageIDs.push_back(static_cast<uint32_t>(MessageID::OnSave));
    handledMessageIDs.push_back(static_cast<uint32_t>(MessageID::OnPauseToggled));

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

    case static_cast<uint32_t>(MessageID::OnSave):
        m_NextSaveTime = std::chrono::high_resolution_clock::now() +
                         std::chrono::minutes(m_SaveIntervalInMinutes);
        break;

    case static_cast<uint32_t>(MessageID::OnPauseToggled):
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

void AutoSaveStrategy::AttachDetours(void)
{
    ShowDialogDetour::attach(GetAddress(UTFWin::cSPUIMessageBox, ShowDialog));
}

