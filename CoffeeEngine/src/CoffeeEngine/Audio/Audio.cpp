#include "Audio.h"

#include "CoffeeEngine/Scene/Components.h"

#include <cassert>
#include <fstream>
#include <sstream>

namespace Coffee
{

    // Global pointer for the low-level IO
    CAkFilePackageLowLevelIODeferred* g_lowLevelIO = nullptr;

    std::vector<Ref<Audio::AudioBank>> Audio::audioBanks;
    std::vector<AudioSourceComponent*> Audio::audioSources;
    std::vector<AudioListenerComponent*> Audio::audioListeners;

    void Audio::Init()
    {
        if (!InitializeMemoryManager())
            return;

        if (!InitializeStreamManager())
            return;

        if (!InitializeLowLevelIO())
            return;

        if (!InitializeSoundEngine())
            return;

        if (!InitializeMusicEngine())
            return;

        if (!InitializeSpatialAudio())
            return;

        if (!InitializeCommunicationModule())
            return;

        g_lowLevelIO->SetBasePath(AKTEXT("assets/audio/Wwise Project/GeneratedSoundBanks/Windows"));

        LoadAudioBanks();

        AudioZone::SearchAvailableBusChannels();
    }

    void Audio::RegisterGameObject(uint64_t gameObjectID)
    {
        AK::SoundEngine::RegisterGameObj(gameObjectID);
    }

    void Audio::UnregisterGameObject(uint64_t gameObjectID)
    {
        AK::SoundEngine::UnregisterGameObj(gameObjectID);
    }

    void Audio::UnregisterAllGameObjects()
    {
        for (auto& audioSource : audioSources)
        {
            UnregisterAudioSourceComponent(*audioSource);
        }

        for (auto& audioListener : audioListeners)
        {
            UnregisterAudioListenerComponent(*audioListener);
        }
    }

    void Audio::Set3DPosition(uint64_t gameObjectID, glm::vec3 pos, glm::vec3 forward, glm::vec3 up)
    {
        AkSoundPosition newPos;
        newPos.SetPosition(pos.x, pos.y, -pos.z);

        forward = glm::normalize(glm::vec3(forward.x, forward.y, -forward.z));
        up = glm::normalize(up - glm::dot(up, forward) * forward);

        newPos.SetOrientation(forward.x, forward.y, forward.z, up.x, up.y, up.z);
        AK::SoundEngine::SetPosition(gameObjectID, newPos);
    }

    void Audio::PlayEvent(AudioSourceComponent& audioSourceComponent)
    {
        AK::SoundEngine::PostEvent(audioSourceComponent.eventName.c_str(), audioSourceComponent.gameObjectID);
        audioSourceComponent.isPlaying = true;
        audioSourceComponent.isPaused = false;
    }

    void Audio::StopEvent(AudioSourceComponent& audioSourceComponent)
    {
        AK::SoundEngine::ExecuteActionOnEvent(audioSourceComponent.eventName.c_str(), AK::SoundEngine::AkActionOnEventType_Stop, audioSourceComponent.gameObjectID);
        audioSourceComponent.isPlaying = false;
        audioSourceComponent.isPaused = false;
    }

    void Audio::PauseEvent(AudioSourceComponent& audioSourceComponent)
    {
        AK::SoundEngine::ExecuteActionOnEvent(audioSourceComponent.eventName.c_str(), AK::SoundEngine::AkActionOnEventType_Pause, audioSourceComponent.gameObjectID);
        audioSourceComponent.isPaused = true;
    }

    void Audio::ResumeEvent(AudioSourceComponent& audioSourceComponent)
    {
        AK::SoundEngine::ExecuteActionOnEvent(audioSourceComponent.eventName.c_str(), AK::SoundEngine::AkActionOnEventType_Resume, audioSourceComponent.gameObjectID);
        audioSourceComponent.isPaused = false;
    }

    void Audio::SetSwitch(const char* switchGroup, const char* switchState, uint64_t gameObjectID)
    {
        AK::SoundEngine::SetSwitch(switchGroup, switchState, gameObjectID);
    }

    void Audio::SetVolume(uint64_t gameObjectID, float newVolume)
    {
        AK::SoundEngine::SetGameObjectOutputBusVolume(gameObjectID, AK_INVALID_GAME_OBJECT, newVolume);
    }

    void Audio::RegisterAudioSourceComponent(AudioSourceComponent& audioSourceComponent)
    {
        for (const auto& source : audioSources)
        {
            if (source->gameObjectID == audioSourceComponent.gameObjectID)
                return;
        }

        if (audioSourceComponent.gameObjectID == -1)
            audioSourceComponent.gameObjectID = UUID();

        audioSources.push_back(&audioSourceComponent);

        RegisterGameObject(audioSourceComponent.gameObjectID);
    }

    void Audio::UnregisterAudioSourceComponent(AudioSourceComponent& audioSourceComponent)
    {
        if (!audioSourceComponent.eventName.empty() && audioSourceComponent.isPlaying)
            StopEvent(audioSourceComponent);

        audioSourceComponent.toDelete = true;

        AudioZone::UnregisterObject(audioSourceComponent.gameObjectID);

        UnregisterGameObject(audioSourceComponent.gameObjectID);

        auto it = std::ranges::find(audioSources, &audioSourceComponent);
        audioSources.erase(it);
    }

    void Audio::RegisterAudioListenerComponent(AudioListenerComponent& audioListenerComponent)
    {
        for (const auto* listener : audioListeners)
        {
            if (listener->gameObjectID == audioListenerComponent.gameObjectID)
                return;
        }

        if (audioListenerComponent.gameObjectID == -1)
            audioListenerComponent.gameObjectID = UUID();

        audioListeners.push_back(&audioListenerComponent);

        RegisterGameObject(audioListenerComponent.gameObjectID);
        AK::SoundEngine::SetDefaultListeners(&audioListenerComponent.gameObjectID, audioListeners.size());
    }

    void Audio::UnregisterAudioListenerComponent(AudioListenerComponent& audioListenerComponent)
    {
        UnregisterGameObject(audioListenerComponent.gameObjectID);

        audioListenerComponent.toDelete = true;

        auto it = std::ranges::find(audioListeners, &audioListenerComponent);
        audioListeners.erase(it);
    }

    void Audio::PlayInitialAudios()
    {
        for (auto& audioSource : audioSources)
        {
            if (audioSource->playOnAwake)
                PlayEvent(*audioSource);
        }
    }

    void Audio::StopAllEvents()
    {
        for (auto& audioSource : audioSources)
        {
            if (audioSource->isPlaying)
                StopEvent(*audioSource);
        }
    }

    void Audio::ProcessAudio()
    {
        AudioZone::Update();

        AK::SoundEngine::RenderAudio();
    }

    bool Audio::InitializeMemoryManager()
    {
        AkMemSettings memSettings;
        AK::MemoryMgr::GetDefaultSettings(memSettings);

        if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
        {
            assert(!"Could not create the Memory Manager.");
            return false;
        }
        return true;
    }

    bool Audio::InitializeStreamManager()
    {
        AkStreamMgrSettings stmSettings;
        AK::StreamMgr::GetDefaultSettings(stmSettings);

        if (!AK::StreamMgr::Create(stmSettings))
        {
            assert(!"Could not create the Stream Manager.");
            return false;
        }
        return true;
    }

    bool Audio::InitializeLowLevelIO()
    {
        AkDeviceSettings deviceSettings;
        AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

        g_lowLevelIO = new CAkFilePackageLowLevelIODeferred();
        if (g_lowLevelIO->Init(deviceSettings) != AK_Success)
        {
            assert(!"Could not initialize the Low-Level IO.");
            return false;
        }
        return true;
    }

    bool Audio::InitializeSoundEngine()
    {
        AkInitSettings initSettings;
        AkPlatformInitSettings platformInitSettings;
        AK::SoundEngine::GetDefaultInitSettings(initSettings);
        AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);

        if (AK::SoundEngine::Init(&initSettings, &platformInitSettings) != AK_Success)
        {
            assert(!"Could not initialize the Sound Engine.");
            return false;
        }
        return true;
    }

    bool Audio::InitializeMusicEngine()
    {
        AkMusicSettings musicInitSettings;
        AK::MusicEngine::GetDefaultInitSettings(musicInitSettings);

        if (AK::MusicEngine::Init(&musicInitSettings) != AK_Success)
        {
            assert(!"Could not initialize the Music Engine.");
            return false;
        }
        return true;
    }

    bool Audio::InitializeSpatialAudio()
    {
        AkSpatialAudioInitSettings spatialAudioSettings;
        if (AK::SpatialAudio::Init(spatialAudioSettings) != AK_Success)
        {
            assert(!"Could not initialize the Spatial Audio module.");
            return false;
        }
        return true;
    }

    bool Audio::InitializeCommunicationModule()
    {
#ifndef AK_OPTIMIZED
        AkCommSettings commSettings;
        AK::Comm::GetDefaultInitSettings(commSettings);

        AKPLATFORM::SafeStrCpy(commSettings.szAppNetworkName, "Coffee Engine", AK_COMM_SETTINGS_MAX_STRING_SIZE);

        if (AK::Comm::Init(commSettings) != AK_Success)
        {
            assert(!"Could not initialize the Communication module.");
            return false;
        }
#endif // AK_OPTIMIZED
        return true;
    }

    bool Audio::LoadAudioBanks()
    {
        std::ifstream file("assets/audio/Wwise Project/GeneratedSoundBanks/Windows/SoundbanksInfo.json");
        if (!file.is_open())
            return false;

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        rapidjson::Document banksInfo;
        if (banksInfo.Parse(buffer.str().c_str()).HasParseError()
            || !banksInfo.HasMember("SoundBanksInfo")
            || !banksInfo["SoundBanksInfo"].HasMember("SoundBanks"))
        {
            return false;
        }

        const auto& soundBanks = banksInfo["SoundBanksInfo"]["SoundBanks"];
        if (!soundBanks.IsArray())
            return false;

        for (const auto& bankData : soundBanks.GetArray())
        {
            if (!bankData.HasMember("ShortName") || !bankData["ShortName"].IsString())
                continue;

            const std::string shortName = bankData["ShortName"].GetString();

            auto audioBank = CreateRef<AudioBank>();
            audioBank->name = shortName;

            if (bankData.HasMember("Events") && bankData["Events"].IsArray())
            {
                for (const auto& eventData : bankData["Events"].GetArray())
                {
                    if (eventData.HasMember("Name") && eventData["Name"].IsString())
                    {
                        std::string eventName = eventData["Name"].GetString();
                        audioBank->events.push_back(eventName);
                    }
                }
            }

            AkBankID bankID;
            AK::SoundEngine::LoadBank(audioBank->name.c_str(), bankID);

            audioBanks.push_back(audioBank);
        }

        return true;
    }

    void Audio::Shutdown()
    {
        AudioZone::Shutdown();

        UnregisterAllGameObjects();

        // Unload the soundbanks
        AK::SoundEngine::ClearBanks();

#ifndef AK_OPTIMIZED
        // Terminate the Communication module
        AK::Comm::Term();
#endif // AK_OPTIMIZED

        // Terminate the Music Engine
        AK::MusicEngine::Term();

        // Terminate the Sound Engine
        AK::SoundEngine::Term();

        // Destroy the Stream Manager and terminate the low-level IO
        if (AK::IAkStreamMgr::Get())
        {
            g_lowLevelIO->Term();
            delete g_lowLevelIO;
            AK::IAkStreamMgr::Get()->Destroy();
        }

        // Terminate the Memory Manager
        AK::MemoryMgr::Term();
    }
} // namespace Coffee
