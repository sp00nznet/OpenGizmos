#pragma once

#ifdef _WIN32

#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct SDL_Texture;
struct SDL_Renderer;

namespace opengg {

class AssetCache;
struct Resource;

class AssetViewerWindow {
public:
    AssetViewerWindow();
    ~AssetViewerWindow();

    // Show the window (non-blocking)
    bool show(HWND parent, AssetCache* cache, SDL_Renderer* renderer);

    // Close the window
    void close();

    // Check if window is open
    bool isOpen() const { return hwnd_ != nullptr; }

    // Process messages (call from main loop)
    void update();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void createControls();
    void populateFileList();
    void onFileSelected();
    void onResourceSelected();
    void updatePreview();
    void updateRawPreview(int preset);
    void drawPreview(HDC hdc);

    HWND hwnd_ = nullptr;
    HWND fileCombo_ = nullptr;
    HWND resourceList_ = nullptr;
    HWND previewStatic_ = nullptr;
    HWND infoEdit_ = nullptr;

    AssetCache* cache_ = nullptr;
    SDL_Renderer* sdlRenderer_ = nullptr;

    std::vector<std::string> fileNames_;
    std::vector<Resource> resources_;
    int selectedFile_ = -1;
    int selectedResource_ = -1;

    // Preview bitmap
    HBITMAP previewBitmap_ = nullptr;
    int previewWidth_ = 0;
    int previewHeight_ = 0;
    std::vector<uint32_t> previewPixels_;

    static AssetViewerWindow* instance_;
};

} // namespace opengg

#endif // _WIN32
