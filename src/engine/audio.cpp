#include "audio.h"
#include "asset_cache.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>
#include <algorithm>

namespace opengg {

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::initialize(int frequency, int channels, int chunkSize) {
    // Initialize SDL audio subsystem
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        lastError_ = "SDL_Init(AUDIO) failed: " + std::string(SDL_GetError());
        return false;
    }

    // Open audio device
    if (Mix_OpenAudio(frequency, MIX_DEFAULT_FORMAT, channels, chunkSize) < 0) {
        lastError_ = "Mix_OpenAudio failed: " + std::string(Mix_GetError());
        return false;
    }

    // Allocate mixing channels
    Mix_AllocateChannels(32);

    // Initialize MIDI support (for FluidSynth)
    int flags = MIX_INIT_MID;
    int initted = Mix_Init(flags);
    if ((initted & flags) != flags) {
        // MIDI not available, but continue anyway
        // Game can still play WAV sounds
    }

    initialized_ = true;
    return true;
}

void AudioSystem::shutdown() {
    if (!initialized_) return;

    // Stop all audio
    Mix_HaltChannel(-1);
    Mix_HaltMusic();

    // Free loaded sounds
    for (auto& pair : sounds_) {
        if (pair.second) {
            Mix_FreeChunk(pair.second);
        }
    }
    sounds_.clear();

    // Free loaded music
    for (auto& pair : musicCache_) {
        if (pair.second) {
            Mix_FreeMusic(pair.second);
        }
    }
    musicCache_.clear();

    Mix_CloseAudio();
    Mix_Quit();
    initialized_ = false;
}

void AudioSystem::setAssetCache(AssetCache* cache) {
    assetCache_ = cache;
}

void AudioSystem::setSFXVolume(float volume) {
    sfxVolume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update all playing channels
    int effectiveVolume = static_cast<int>(sfxVolume_ * masterVolume_ * MIX_MAX_VOLUME);
    if (muted_) effectiveVolume = 0;

    for (int i = 0; i < Mix_AllocateChannels(-1); ++i) {
        if (Mix_Playing(i)) {
            Mix_Volume(i, effectiveVolume);
        }
    }
}

void AudioSystem::setMusicVolume(float volume) {
    musicVolume_ = std::clamp(volume, 0.0f, 1.0f);

    int effectiveVolume = static_cast<int>(musicVolume_ * masterVolume_ * MIX_MAX_VOLUME);
    if (muted_) effectiveVolume = 0;

    Mix_VolumeMusic(effectiveVolume);
}

void AudioSystem::setMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);

    // Update both SFX and music
    setSFXVolume(sfxVolume_);
    setMusicVolume(musicVolume_);
}

void AudioSystem::setMuted(bool muted) {
    muted_ = muted;
    setMasterVolume(masterVolume_);  // Trigger volume update
}

Mix_Chunk* AudioSystem::getChunk(const std::string& id) {
    // Check cache
    auto it = sounds_.find(id);
    if (it != sounds_.end()) {
        return it->second;
    }

    // Try to load from asset cache
    if (assetCache_) {
        Mix_Chunk* chunk = assetCache_->getSound(id);
        if (chunk) {
            sounds_[id] = chunk;
            return chunk;
        }
    }

    return nullptr;
}

Mix_Music* AudioSystem::getMusicHandle(const std::string& id) {
    // Check cache
    auto it = musicCache_.find(id);
    if (it != musicCache_.end()) {
        return it->second;
    }

    // Try to load from asset cache
    if (assetCache_) {
        Mix_Music* music = assetCache_->getMusic(id);
        if (music) {
            musicCache_[id] = music;
            return music;
        }
    }

    return nullptr;
}

int AudioSystem::playSound(const std::string& id, float volume) {
    Mix_Chunk* chunk = getChunk(id);
    if (!chunk) {
        lastError_ = "Sound not found: " + id;
        return -1;
    }
    return playSound(chunk, volume);
}

int AudioSystem::playSound(Mix_Chunk* chunk, float volume) {
    if (!chunk || !initialized_) return -1;

    // Calculate effective volume
    float effectiveVolume = volume * sfxVolume_ * masterVolume_;
    if (muted_) effectiveVolume = 0;

    int mixVolume = static_cast<int>(effectiveVolume * MIX_MAX_VOLUME);

    // Play on first available channel
    int channel = Mix_PlayChannel(-1, chunk, 0);
    if (channel >= 0) {
        Mix_Volume(channel, mixVolume);
    }

    return channel;
}

int AudioSystem::playSoundLooped(const std::string& id, int loops, float volume) {
    Mix_Chunk* chunk = getChunk(id);
    if (!chunk || !initialized_) return -1;

    float effectiveVolume = volume * sfxVolume_ * masterVolume_;
    if (muted_) effectiveVolume = 0;

    int mixVolume = static_cast<int>(effectiveVolume * MIX_MAX_VOLUME);

    int channel = Mix_PlayChannel(-1, chunk, loops);
    if (channel >= 0) {
        Mix_Volume(channel, mixVolume);
    }

    return channel;
}

int AudioSystem::playSoundAt(const std::string& id, int x, int y, float volume) {
    Mix_Chunk* chunk = getChunk(id);
    if (!chunk || !initialized_) return -1;

    int channel = Mix_PlayChannel(-1, chunk, 0);
    if (channel < 0) return -1;

    // Calculate distance attenuation
    uint8_t distance = calculateDistance(x, y);

    // Calculate stereo panning
    int pan = calculatePan(x, y);

    // Apply effects
    Mix_SetDistance(channel, distance);
    Mix_SetPanning(channel, static_cast<uint8_t>(255 - pan), static_cast<uint8_t>(pan));

    float effectiveVolume = volume * sfxVolume_ * masterVolume_;
    if (muted_) effectiveVolume = 0;

    Mix_Volume(channel, static_cast<int>(effectiveVolume * MIX_MAX_VOLUME));

    return channel;
}

void AudioSystem::setListenerPosition(int x, int y) {
    listenerX_ = x;
    listenerY_ = y;
}

int AudioSystem::calculatePan(int soundX, int soundY) const {
    // Calculate horizontal offset from listener
    int dx = soundX - listenerX_;

    // Clamp to screen width and convert to 0-255 range
    // 0 = full left, 255 = full right, 127 = center
    float panRatio = static_cast<float>(dx) / 320.0f;  // Half screen width
    panRatio = std::clamp(panRatio, -1.0f, 1.0f);

    return static_cast<int>((panRatio + 1.0f) * 127.5f);
}

uint8_t AudioSystem::calculateDistance(int soundX, int soundY) const {
    // Calculate distance from listener
    int dx = soundX - listenerX_;
    int dy = soundY - listenerY_;
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    // Normalize to 0-255 range (0 = closest, 255 = furthest)
    // Max hearing distance = 800 pixels (larger than screen diagonal)
    float normalized = distance / 800.0f;
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    return static_cast<uint8_t>(normalized * 255);
}

void AudioSystem::stopChannel(int channel) {
    if (channel >= 0) {
        Mix_HaltChannel(channel);
    }
}

void AudioSystem::stopAllChannels() {
    Mix_HaltChannel(-1);
}

bool AudioSystem::isChannelPlaying(int channel) const {
    return Mix_Playing(channel) != 0;
}

void AudioSystem::setChannelVolume(int channel, float volume) {
    if (channel >= 0) {
        float effectiveVolume = volume * sfxVolume_ * masterVolume_;
        if (muted_) effectiveVolume = 0;

        Mix_Volume(channel, static_cast<int>(effectiveVolume * MIX_MAX_VOLUME));
    }
}

void AudioSystem::setChannelPan(int channel, float pan) {
    if (channel >= 0) {
        // pan: -1.0 = left, 0.0 = center, 1.0 = right
        pan = std::clamp(pan, -1.0f, 1.0f);
        uint8_t left = static_cast<uint8_t>((1.0f - pan) * 127.5f);
        uint8_t right = static_cast<uint8_t>((1.0f + pan) * 127.5f);
        Mix_SetPanning(channel, left, right);
    }
}

bool AudioSystem::playMusic(const std::string& id, int loops) {
    Mix_Music* music = getMusicHandle(id);
    if (!music) {
        lastError_ = "Music not found: " + id;
        return false;
    }
    return playMusic(music, loops);
}

bool AudioSystem::playMusic(Mix_Music* music, int loops) {
    if (!music || !initialized_) return false;

    // Stop current music
    Mix_HaltMusic();

    // Set volume
    int effectiveVolume = static_cast<int>(musicVolume_ * masterVolume_ * MIX_MAX_VOLUME);
    if (muted_) effectiveVolume = 0;
    Mix_VolumeMusic(effectiveVolume);

    // Play
    if (Mix_PlayMusic(music, loops) < 0) {
        lastError_ = "Mix_PlayMusic failed: " + std::string(Mix_GetError());
        return false;
    }

    return true;
}

void AudioSystem::stopMusic() {
    Mix_HaltMusic();
    currentMusicId_.clear();
}

void AudioSystem::pauseMusic() {
    Mix_PauseMusic();
}

void AudioSystem::resumeMusic() {
    Mix_ResumeMusic();
}

bool AudioSystem::isMusicPlaying() const {
    return Mix_PlayingMusic() != 0;
}

bool AudioSystem::isMusicPaused() const {
    return Mix_PausedMusic() != 0;
}

void AudioSystem::fadeInMusic(const std::string& id, int fadeMs, int loops) {
    Mix_Music* music = getMusicHandle(id);
    if (!music) return;

    Mix_FadeInMusic(music, loops, fadeMs);
    currentMusicId_ = id;
}

void AudioSystem::fadeOutMusic(int fadeMs) {
    Mix_FadeOutMusic(fadeMs);
}

void AudioSystem::crossfadeMusic(const std::string& id, int fadeMs) {
    // Start fading out current music
    Mix_FadeOutMusic(fadeMs / 2);

    // Note: SDL_mixer doesn't support true crossfade
    // We'd need to schedule the new music to start after fade
    // For now, just fade out then start new
    currentMusicId_ = id;
}

void AudioSystem::queueSound(const std::string& id) {
    soundQueue_.push_back(id);
}

void AudioSystem::clearQueue() {
    soundQueue_.clear();
}

void AudioSystem::preloadSound(const std::string& id) {
    getChunk(id);  // This will load and cache
}

void AudioSystem::preloadMusic(const std::string& id) {
    getMusicHandle(id);  // This will load and cache
}

void AudioSystem::unloadSound(const std::string& id) {
    auto it = sounds_.find(id);
    if (it != sounds_.end()) {
        if (it->second) {
            Mix_FreeChunk(it->second);
        }
        sounds_.erase(it);
    }
}

void AudioSystem::unloadMusic(const std::string& id) {
    auto it = musicCache_.find(id);
    if (it != musicCache_.end()) {
        if (it->second) {
            Mix_FreeMusic(it->second);
        }
        musicCache_.erase(it);
    }
}

} // namespace opengg
