//
// SporeAutoSave - https://github.com/Rosalie241/SporeAutoSave
//  Copyright (C) 2021 Rosalie Wanders <rosalie@mailbox.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License version 3.
//  You should have received a copy of the GNU Affero General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#ifndef AUTOSAVESTRATEGY_HPP
#define AUTOSAVESTRATEGY_HPP

#include <Spore\BasicIncludes.h>

#include <chrono>
#include <vector>
#include <filesystem>

enum class MessageID : uint32_t
{
    OnSave = 0x1cd20f0,
    OnPauseToggled = 0x3867294,
};

class AutoSaveStrategy :
    public Simulator::ISimulatorStrategy,
    public App::IUnmanagedMessageListener
{
private:
    int m_RefCount = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_NextSaveTime;
    int m_SaveIntervalInMinutes = 0;
    int64_t m_TimeLeftUntilSaveWhenPausedInSeconds = 0;

    std::filesystem::path m_BackupSavePath;
    size_t m_MaximumAmountOfBackupSaves = 0;

    bool m_IsInValidMode = false;
    bool m_IsPaused = false;

    std::vector<std::filesystem::path> GetBackupSaveList();
    bool BackupSave();
    void SaveGame();

public:
    AutoSaveStrategy(int saveIntervalInMinutes, int maximumAmountOfBackupSaves, std::filesystem::path backupSavePath)
        : m_SaveIntervalInMinutes(saveIntervalInMinutes),
          m_MaximumAmountOfBackupSaves(maximumAmountOfBackupSaves),
          m_BackupSavePath(backupSavePath)
    {
    }

    int AddRef() override { return ++m_RefCount; }
    int Release() override { if (--m_RefCount == 0) { delete this; return 0; }; return m_RefCount; }

    void Initialize() override { }
    void Dispose() override { }

    const char* GetName() const override { return "AutoSaveStrategy"; }

    void OnModeExited(uint32_t previousModeID, uint32_t newModeID) override { }
    void OnModeEntered(uint32_t previousModeID, uint32_t newModeID) override;

    uint32_t GetLastGameMode() const override { return 0; }
    uint32_t GetCurrentGameMode() const override { return 0; }

    bool func24h(uint32_t) override { return false; }

    bool Write(Simulator::ISerializerStream*) override { return false; }
    bool Read(Simulator::ISerializerStream*) override { return false; }

    void OnLoad(const Simulator::cSavedGameHeader& savedGame) override { }
    bool WriteToXML(Simulator::XmlSerializer*) override { return false; }

    void Update(int deltaTime, int deltaGameTime) override;
    void PostUpdate(int deltaTime, int deltaGameTime) override { }

    void func40h(uint32_t) override { }
    void func44h(uint32_t) override { }
    void func48h() override { }
    void func4Ch() override { }

    std::vector<uint32_t> GetHandledMessageIDs(void);
    bool HandleMessage(uint32_t messageID, void* message) override;
};


#endif // AUTOSAVESTRATEGY_HPP
