#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

struct Mix_Chunk;
struct _Mix_Music;
typedef _Mix_Music Mix_Music;

namespace opengg {

// Forward declaration
class AssetCache;

// Audio channel for positional audio
struct AudioChannel {
    int channel;
    float volume;
    float pan;  // -1.0 = left, 0.0 = center, 1.0 = right
    bool loop;
};

// Audio System
// Handles sound effects and music using SDL_mixer
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // Initialize audio subsystem
    bool initialize(int frequency = 44100, int channels = 2, int chunkSize = 2048);

    // Shutdown
    void shutdown();

    // Set asset cache for loading sounds
    void setAssetCache(AssetCache* cache);

    // Volume control (0.0 - 1.0)
    void setSFXVolume(float volume);
    void setMusicVolume(float volume);
    float getSFXVolume() const { return sfxVolume_; }
    float getMusicVolume() const { return musicVolume_; }

    // Master volume
    void setMasterVolume(float volume);
    float getMasterVolume() const { return masterVolume_; }

    // Mute control
    void setMuted(bool muted);
    bool isMuted() const { return muted_; }

    // Sound effects
    int playSound(const std::string& id, float volume = 1.0f);
    int playSound(Mix_Chunk* chunk, float volume = 1.0f);
    int playSoundLooped(const std::string& id, int loops = -1, float volume = 1.0f);

    // Positional audio (for 2D spatial sound)
    int playSoundAt(const std::string& id, int x, int y, float volume = 1.0f);
    void setListenerPosition(int x, int y);

    // Channel control
    void stopChannel(int channel);
    void stopAllChannels();
    bool isChannelPlaying(int channel) const;
    void setChannelVolume(int channel, float volume);
    void setChannelPan(int channel, float pan);

    // Music
    bool playMusic(const std::string& id, int loops = -1);
    bool playMusic(Mix_Music* music, int loops = -1);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    bool isMusicPlaying() const;
    bool isMusicPaused() const;

    // Music transitions
    void fadeInMusic(const std::string& id, int fadeMs, int loops = -1);
    void fadeOutMusic(int fadeMs);
    void crossfadeMusic(const std::string& id, int fadeMs);

    // Queue system for sequential playback
    void queueSound(const std::string& id);
    void clearQueue();

    // Preload sounds for faster playback
    void preloadSound(const std::string& id);
    void preloadMusic(const std::string& id);
    void unloadSound(const std::string& id);
    void unloadMusic(const std::string& id);

    // Get last error
    std::string getLastError() const { return lastError_; }

private:
    Mix_Chunk* getChunk(const std::string& id);
    Mix_Music* getMusicHandle(const std::string& id);
    int calculatePan(int soundX, int soundY) const;
    uint8_t calculateDistance(int soundX, int soundY) const;

    AssetCache* assetCache_ = nullptr;

    // Volume settings
    float masterVolume_ = 1.0f;
    float sfxVolume_ = 1.0f;
    float musicVolume_ = 1.0f;
    bool muted_ = false;

    // Listener position for positional audio
    int listenerX_ = 320;  // Center of 640x480 screen
    int listenerY_ = 240;

    // Sound/music caches
    std::unordered_map<std::string, Mix_Chunk*> sounds_;
    std::unordered_map<std::string, Mix_Music*> musicCache_;

    // Current music
    std::string currentMusicId_;

    // Sound queue
    std::vector<std::string> soundQueue_;

    bool initialized_ = false;
    std::string lastError_;
};

} // namespace opengg
