#ifdef _WIN32

#include "asset_viewer.h"
#include "asset_cache.h"
#include "ne_resource.h"
#include <SDL.h>
#include <CommCtrl.h>
#include <windowsx.h>
#include <fstream>

#pragma comment(lib, "comctl32.lib")

namespace opengg {

AssetViewerWindow* AssetViewerWindow::instance_ = nullptr;

// Control IDs
enum {
    IDC_FILE_COMBO = 1001,
    IDC_RESOURCE_LIST = 1002,
    IDC_PREVIEW = 1003,
    IDC_INFO_EDIT = 1004,
};

AssetViewerWindow::AssetViewerWindow() {
    instance_ = this;

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
}

AssetViewerWindow::~AssetViewerWindow() {
    close();
    instance_ = nullptr;
}

bool AssetViewerWindow::show(HWND parent, AssetCache* cache, SDL_Renderer* renderer) {
    if (hwnd_) {
        SetForegroundWindow(hwnd_);
        return true;
    }

    cache_ = cache;
    sdlRenderer_ = renderer;

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = L"OpenGG_AssetViewer";
    RegisterClassExW(&wc);

    // Create window
    hwnd_ = CreateWindowExW(
        0,
        L"OpenGG_AssetViewer",
        L"Asset Viewer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        parent,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!hwnd_) {
        return false;
    }

    createControls();
    populateFileList();

    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);

    return true;
}

void AssetViewerWindow::close() {
    if (previewBitmap_) {
        DeleteObject(previewBitmap_);
        previewBitmap_ = nullptr;
    }
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void AssetViewerWindow::update() {
    if (!hwnd_) return;

    MSG msg;
    while (PeekMessage(&msg, hwnd_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void AssetViewerWindow::createControls() {
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // File selection label
    HWND label1 = CreateWindowW(L"STATIC", L"Source File:",
        WS_CHILD | WS_VISIBLE,
        10, 10, 80, 20, hwnd_, nullptr, nullptr, nullptr);
    SendMessage(label1, WM_SETFONT, (WPARAM)hFont, TRUE);

    // File dropdown
    fileCombo_ = CreateWindowW(L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        100, 8, 250, 200, hwnd_, (HMENU)IDC_FILE_COMBO, nullptr, nullptr);
    SendMessage(fileCombo_, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Resource list label
    HWND label2 = CreateWindowW(L"STATIC", L"Resources:",
        WS_CHILD | WS_VISIBLE,
        10, 40, 80, 20, hwnd_, nullptr, nullptr, nullptr);
    SendMessage(label2, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Resource listbox
    resourceList_ = CreateWindowW(L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        10, 60, 340, 400, hwnd_, (HMENU)IDC_RESOURCE_LIST, nullptr, nullptr);
    SendMessage(resourceList_, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Preview area label
    HWND label3 = CreateWindowW(L"STATIC", L"Preview:",
        WS_CHILD | WS_VISIBLE,
        360, 40, 80, 20, hwnd_, nullptr, nullptr, nullptr);
    SendMessage(label3, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Preview static control (for drawing)
    previewStatic_ = CreateWindowW(L"STATIC", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | SS_OWNERDRAW,
        360, 60, 512, 400, hwnd_, (HMENU)IDC_PREVIEW, nullptr, nullptr);

    // Info edit label
    HWND label4 = CreateWindowW(L"STATIC", L"Info:",
        WS_CHILD | WS_VISIBLE,
        10, 470, 40, 20, hwnd_, nullptr, nullptr, nullptr);
    SendMessage(label4, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Info multiline edit
    infoEdit_ = CreateWindowW(L"EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        10, 490, 860, 150, hwnd_, (HMENU)IDC_INFO_EDIT, nullptr, nullptr);
    SendMessage(infoEdit_, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void AssetViewerWindow::populateFileList() {
    if (!fileCombo_ || !cache_) return;

    SendMessage(fileCombo_, CB_RESETCONTENT, 0, 0);
    fileNames_.clear();

    // Add known DAT and RSC files with clear category labels
    fileNames_ = {
        // Gizmos & Gadgets
        "[GIZMOS] GIZMO.DAT",
        "[GIZMOS] GIZMO256.DAT",
        "[GIZMOS] GIZMO16.DAT",
        "[GIZMOS] PUZZLE.DAT",
        "[GIZMOS] PUZ256.DAT",
        "[GIZMOS] PUZ16.DAT",
        "[GIZMOS] FONT.DAT",
        "[GIZMOS] AE.DAT",
        "[GIZMOS] AE256.DAT",
        "[GIZMOS] AUTO.DAT",
        "[GIZMOS] AUTO256.DAT",
        "[GIZMOS] PLANE.DAT",
        "[GIZMOS] PLANE256.DAT",
        "[GIZMOS] SSGWIN.DAT",
        // Operation Neptune
        "[NEPTUNE] SORTER.RSC",
        "[NEPTUNE] COMMON.RSC",
        "[NEPTUNE] LABRNTH1.RSC",
        "[NEPTUNE] LABRNTH2.RSC",
        "[NEPTUNE] READER1.RSC",
        "[NEPTUNE] READER2.RSC",
        "[NEPTUNE] OT3.RSC",
        "[NEPTUNE] AUTORUN.RSC",
        // OutNumbered
        "[OUTNUMBERED] SSO1.DAT",
        "[OUTNUMBERED] SSOWINCD.DAT",
        "[OUTNUMBERED] SND.DAT",
        // Spellbound
        "[SPELLBOUND] SSR1.DAT",
        "[SPELLBOUND] SFX.DAT",
        "[SPELLBOUND] TASK.RSC",
        // Treasure MathStorm
        "[MATHSTORM] TMSDATA.DAT",
        // Raw file view mode
        "[RAW] GIZMO.DAT @ 0x80000"
    };

    for (const auto& name : fileNames_) {
        SendMessageA(fileCombo_, CB_ADDSTRING, 0, (LPARAM)name.c_str());
    }

    // Select first item
    if (!fileNames_.empty()) {
        SendMessage(fileCombo_, CB_SETCURSEL, 0, 0);
        selectedFile_ = 0;
        onFileSelected();
    }
}

// Helper to strip category prefix from filename
static std::string stripPrefix(const std::string& filename) {
    // Strip [GIZMOS], [NEPTUNE], etc. prefixes
    size_t pos = filename.find("] ");
    if (pos != std::string::npos && pos < 20) {
        return filename.substr(pos + 2);
    }
    return filename;
}

void AssetViewerWindow::onFileSelected() {
    if (!resourceList_ || !cache_) return;

    int sel = (int)SendMessage(fileCombo_, CB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)fileNames_.size()) return;

    selectedFile_ = sel;
    std::string displayName = fileNames_[sel];
    std::string filename = stripPrefix(displayName);

    // Update window title to show current file
    std::string title = "Asset Viewer - " + displayName;
    SetWindowTextA(hwnd_, title.c_str());

    // Check if this is raw file view mode
    if (displayName.find("[RAW]") == 0) {
        // Raw view mode - show offset/size controls instead of resources
        SendMessage(resourceList_, LB_RESETCONTENT, 0, 0);
        resources_.clear();

        // Add preset view options - RLE compressed and raw modes
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RLE] 64x64 @ 0x70000 (Sprite data)");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RLE] 128x128 @ 0x70000");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RLE] 80x80 @ 0x74000");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RLE] 64x64 @ 0x80000");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RLE] 128x128 @ 0x80000");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RAW] 320x200 @ 0x60000 (Header area)");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RAW] 320x200 @ 0x70000");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RAW] 320x200 @ 0x80000");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RAW] 320x200 @ 0xA0000");
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)"[RAW] 640x480 @ 0x60000");

        SetWindowTextA(infoEdit_, "RAW FILE VIEW MODE\r\n\r\n[RLE] = RLE decompressed (FF xx count format)\r\n[RAW] = Raw palette indices\r\n\r\nSelect a preset to view sprite/pixel data.\r\nPalette: AUTO256.BMP\r\n");

        selectedResource_ = -1;
        return;
    }

    // Clear and repopulate resource list
    SendMessage(resourceList_, LB_RESETCONTENT, 0, 0);
    resources_ = cache_->getNEResourceList(filename);

    for (const auto& res : resources_) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s #%u (%u bytes)",
                 res.typeName.c_str(), res.id, res.size);
        SendMessageA(resourceList_, LB_ADDSTRING, 0, (LPARAM)buf);
    }

    // Update info
    char info[512];
    snprintf(info, sizeof(info), "File: %s\r\nResources: %zu\r\n",
             filename.c_str(), resources_.size());
    SetWindowTextA(infoEdit_, info);

    selectedResource_ = -1;
}

void AssetViewerWindow::onResourceSelected() {
    int sel = (int)SendMessage(resourceList_, LB_GETCURSEL, 0, 0);
    if (sel < 0) return;

    selectedResource_ = sel;

    // Check if raw view mode
    std::string filename = fileNames_[selectedFile_];
    if (filename.find("[RAW]") == 0) {
        updateRawPreview(sel);
        return;
    }

    if (sel >= (int)resources_.size()) return;
    updatePreview();
}

void AssetViewerWindow::updateRawPreview(int preset) {
    // Parse the RAW filename to get the actual file
    std::string filename = fileNames_[selectedFile_];
    size_t atPos = filename.find("@ ");
    if (atPos == std::string::npos) return;

    // Extract base filename (between "] " and " @")
    size_t startPos = filename.find("] ") + 2;
    std::string baseFile = filename.substr(startPos, atPos - startPos - 1);

    // Build full path
    std::string gamePath = cache_->getGamePath();
    std::string fullPath = gamePath + "/SSGWINCD/" + baseFile;

    // Preset dimensions, offsets, and mode (RLE vs RAW)
    int width = 320, height = 200;
    uint32_t offset = 0x60000;
    bool useRLE = false;

    switch (preset) {
        case 0: width = 64; height = 64; offset = 0x70000; useRLE = true; break;
        case 1: width = 128; height = 128; offset = 0x70000; useRLE = true; break;
        case 2: width = 80; height = 80; offset = 0x74000; useRLE = true; break;
        case 3: width = 64; height = 64; offset = 0x80000; useRLE = true; break;
        case 4: width = 128; height = 128; offset = 0x80000; useRLE = true; break;
        case 5: width = 320; height = 200; offset = 0x60000; useRLE = false; break;
        case 6: width = 320; height = 200; offset = 0x70000; useRLE = false; break;
        case 7: width = 320; height = 200; offset = 0x80000; useRLE = false; break;
        case 8: width = 320; height = 200; offset = 0xA0000; useRLE = false; break;
        case 9: width = 640; height = 480; offset = 0x60000; useRLE = false; break;
    }

    // Read data from file
    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        SetWindowTextA(infoEdit_, ("Failed to open: " + fullPath).c_str());
        return;
    }

    std::vector<uint8_t> data;
    size_t expectedPixels = width * height;

    if (useRLE) {
        // Read extra data for RLE decompression
        file.seekg(offset);
        std::vector<uint8_t> rawData(expectedPixels * 2);  // Read extra for RLE
        file.read(reinterpret_cast<char*>(rawData.data()), rawData.size());

        // RLE decompress: FF <byte> <count> = repeat byte count times
        data.reserve(expectedPixels);
        size_t i = 0;
        while (data.size() < expectedPixels && i < rawData.size()) {
            if (rawData[i] == 0xFF && i + 2 < rawData.size()) {
                uint8_t byte = rawData[i + 1];
                uint8_t count = rawData[i + 2];
                if (count == 0) count = 1;
                for (int j = 0; j < count && data.size() < expectedPixels; ++j) {
                    data.push_back(byte);
                }
                i += 3;
            } else {
                data.push_back(rawData[i]);
                i++;
            }
        }
        // Pad if needed
        while (data.size() < expectedPixels) {
            data.push_back(0);
        }
    } else {
        // Raw mode - read pixels directly
        file.seekg(offset);
        data.resize(expectedPixels);
        file.read(reinterpret_cast<char*>(data.data()), data.size());
    }

    // Load palette - try multiple sources
    uint8_t palette[256][4] = {}; // BGRA format
    bool paletteLoaded = false;
    std::string paletteSource = "Default (colors may be wrong)";

    // List of palette files to try (in order of preference)
    std::vector<std::string> palettePaths = {
        gamePath + "/on_palettes/sorter_bmp.pal",      // Neptune palette
        gamePath + "/on_palettes/common_bmp.pal",     // Common palette
        gamePath + "/INSTALL/AUTO256.BMP",            // Game BMP palette
        gamePath + "/SSGWINCD/../INSTALL/AUTO256.BMP" // Alternate path
    };

    for (const auto& palettePath : palettePaths) {
        std::ifstream palFile(palettePath, std::ios::binary | std::ios::ate);
        if (!palFile) continue;

        auto fileSize = palFile.tellg();
        palFile.seekg(0);

        // Check if BMP (starts with "BM") or raw
        char magic[2];
        palFile.read(magic, 2);

        if (magic[0] == 'B' && magic[1] == 'M') {
            // BMP file - seek past header
            palFile.seekg(54);
            palFile.read(reinterpret_cast<char*>(palette), 1024);
            paletteLoaded = true;
            paletteSource = palettePath;
            break;
        } else if (fileSize == 1024) {
            // Raw 1024-byte palette
            palFile.seekg(0);
            palFile.read(reinterpret_cast<char*>(palette), 1024);
            paletteLoaded = true;
            paletteSource = palettePath;
            break;
        }
    }

    if (!paletteLoaded) {
        // Grayscale fallback
        for (int i = 0; i < 256; ++i) {
            palette[i][0] = palette[i][1] = palette[i][2] = static_cast<uint8_t>(i);
        }
    }

    // Update info
    char info[512];
    snprintf(info, sizeof(info),
             "%s VIEW: %s\r\nOffset: 0x%X\r\nSize: %dx%d\r\nPixels: %zu\r\nMode: %s\r\nPalette: %s\r\n",
             useRLE ? "RLE" : "RAW",
             baseFile.c_str(), offset, width, height, data.size(),
             useRLE ? "RLE decompressed (FF xx count)" : "Raw palette indices",
             paletteSource.c_str());
    SetWindowTextA(infoEdit_, info);

    // Create preview
    if (previewBitmap_) {
        DeleteObject(previewBitmap_);
        previewBitmap_ = nullptr;
    }

    previewWidth_ = width;
    previewHeight_ = height;
    previewPixels_.resize(width * height);

    // Apply palette to pixel data
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t c = data[y * width + x];

            uint8_t b = palette[c][0];
            uint8_t g = palette[c][1];
            uint8_t r = palette[c][2];

            previewPixels_[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    // Create GDI bitmap
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(previewStatic_);
    previewBitmap_ = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT,
                                    previewPixels_.data(), &bmi, DIB_RGB_COLORS);
    ReleaseDC(previewStatic_, hdc);

    InvalidateRect(previewStatic_, nullptr, TRUE);
}

void AssetViewerWindow::updatePreview() {
    if (selectedFile_ < 0 || selectedResource_ < 0) return;
    if (selectedResource_ >= (int)resources_.size()) return;

    const auto& res = resources_[selectedResource_];
    std::string displayName = fileNames_[selectedFile_];
    std::string filename = stripPrefix(displayName);

    // Get raw data
    auto data = cache_->getRawResource(filename, res.typeId, res.id);
    if (data.empty()) return;

    // Check if this is a Neptune RSC file
    bool isNeptuneRSC = (filename.find(".RSC") != std::string::npos) ||
                        (displayName.find("[NEPTUNE]") != std::string::npos);

    // Update info text
    std::string info = "File: " + filename + "\r\n";
    info += "Resource: " + res.typeName + " #" + std::to_string(res.id) + "\r\n";
    info += "Size: " + std::to_string(res.size) + " bytes\r\n";
    info += "Offset: 0x" + std::to_string(res.offset) + "\r\n";

    // Check header type for Neptune resources
    if (isNeptuneRSC && data.size() >= 4) {
        if (data[0] == 0x01 && data[1] == 0x00) {
            info += "Format: Neptune LE sprite header\r\n";
        } else if (data[0] == 0x00 && data[1] == 0x01) {
            info += "Format: Neptune BE sprite header\r\n";
        } else if (data.size() == 1536) {
            info += "Format: Neptune doubled-byte palette (256 colors)\r\n";
        }
    }
    info += "\r\n";

    // Add hex dump
    info += "Hex dump (first 128 bytes):\r\n";
    for (size_t i = 0; i < std::min(data.size(), size_t(128)); i += 16) {
        char line[80];
        char* p = line;
        p += sprintf(p, "%04zX: ", i);
        for (size_t j = 0; j < 16 && i + j < data.size(); ++j) {
            p += sprintf(p, "%02X ", data[i + j]);
        }
        *p++ = '\r';
        *p++ = '\n';
        *p = '\0';
        info += line;
    }

    SetWindowTextA(infoEdit_, info.c_str());

    // Try to decode as sprite and create preview bitmap
    if (previewBitmap_) {
        DeleteObject(previewBitmap_);
        previewBitmap_ = nullptr;
    }

    // Check for Neptune sprite format
    if (isNeptuneRSC && data.size() >= 16) {
        // Neptune sprite header detection
        bool isLEHeader = (data[0] == 0x01 && data[1] == 0x00);
        bool isBEHeader = (data[0] == 0x00 && data[1] == 0x01);

        if (isLEHeader || isBEHeader) {
            // Parse sprite count
            uint16_t spriteCount = isLEHeader
                ? (data[2] | (data[3] << 8))
                : (data[3]);  // BE format

            if (spriteCount > 0 && spriteCount < 500) {
                // Calculate header size and offset table position
                size_t headerSize = isLEHeader ? 14 : 16;
                size_t offsetTablePos = headerSize;

                // Read first sprite offset
                if (offsetTablePos + 4 <= data.size()) {
                    uint32_t firstOffset = data[offsetTablePos] |
                                          (data[offsetTablePos + 1] << 8) |
                                          (data[offsetTablePos + 2] << 16) |
                                          (data[offsetTablePos + 3] << 24);

                    // Decode first sprite with RLE
                    // Try common dimensions
                    int testWidths[] = {64, 94, 55, 43, 80, 128, 32, 48};
                    int bestWidth = 64, bestHeight = 64;

                    // Auto-detect by counting row terminators (0x00)
                    if (firstOffset < data.size()) {
                        int rowCount = 0;
                        int maxRowWidth = 0;
                        int currentRowWidth = 0;

                        for (size_t i = firstOffset; i < data.size() && rowCount < 200; ++i) {
                            if (data[i] == 0x00) {
                                if (currentRowWidth > maxRowWidth) {
                                    maxRowWidth = currentRowWidth;
                                }
                                currentRowWidth = 0;
                                rowCount++;
                            } else if (data[i] == 0xFF && i + 2 < data.size()) {
                                currentRowWidth += data[i + 2] + 1;
                                i += 2;
                            } else {
                                currentRowWidth++;
                            }
                        }

                        if (rowCount > 0 && maxRowWidth > 0) {
                            bestWidth = maxRowWidth;
                            bestHeight = rowCount;
                        }
                    }

                    // Clamp to reasonable sizes
                    bestWidth = std::min(std::max(bestWidth, 16), 256);
                    bestHeight = std::min(std::max(bestHeight, 16), 256);

                    previewWidth_ = bestWidth;
                    previewHeight_ = bestHeight;
                    previewPixels_.resize(bestWidth * bestHeight);

                    // Fill with transparent (magenta)
                    for (auto& p : previewPixels_) {
                        p = 0xFFFF00FF;
                    }

                    // Decode RLE sprite
                    int x = 0, y = 0;
                    size_t i = firstOffset;

                    while (i < data.size() && y < bestHeight) {
                        uint8_t byte = data[i++];

                        if (byte == 0x00) {
                            // Row terminator
                            x = 0;
                            y++;
                        } else if (byte == 0xFF && i + 1 < data.size()) {
                            // RLE: FF <pixel> <count>
                            uint8_t pixel = data[i++];
                            uint8_t count = data[i++];

                            for (int j = 0; j <= count && x < bestWidth; ++j) {
                                if (pixel != 0) {
                                    // Use rainbow palette for visualization
                                    uint8_t r = ((pixel >> 5) & 7) * 36;
                                    uint8_t g = ((pixel >> 2) & 7) * 36;
                                    uint8_t b = (pixel & 3) * 85;
                                    previewPixels_[y * bestWidth + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                                } else {
                                    previewPixels_[y * bestWidth + x] = 0xFF000000;
                                }
                                x++;
                            }
                        } else {
                            // Literal pixel
                            if (x < bestWidth) {
                                if (byte != 0) {
                                    uint8_t r = ((byte >> 5) & 7) * 36;
                                    uint8_t g = ((byte >> 2) & 7) * 36;
                                    uint8_t b = (byte & 3) * 85;
                                    previewPixels_[y * bestWidth + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                                } else {
                                    previewPixels_[y * bestWidth + x] = 0xFF000000;
                                }
                                x++;
                            }
                        }
                    }

                    // Update info with detected dimensions
                    char dimInfo[128];
                    snprintf(dimInfo, sizeof(dimInfo),
                            "\r\nDetected: %d sprites, first at offset 0x%X\r\n"
                            "Auto-detected dimensions: %dx%d\r\n",
                            spriteCount, firstOffset, bestWidth, bestHeight);
                    info += dimInfo;
                    SetWindowTextA(infoEdit_, info.c_str());

                    // Create GDI bitmap
                    BITMAPINFO bmi = {};
                    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bmi.bmiHeader.biWidth = bestWidth;
                    bmi.bmiHeader.biHeight = -bestHeight;
                    bmi.bmiHeader.biPlanes = 1;
                    bmi.bmiHeader.biBitCount = 32;
                    bmi.bmiHeader.biCompression = BI_RGB;

                    HDC hdc = GetDC(previewStatic_);
                    previewBitmap_ = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT,
                                                    previewPixels_.data(), &bmi, DIB_RGB_COLORS);
                    ReleaseDC(previewStatic_, hdc);
                }
            }
        }
    }

    // Fallback: Parse as Gizmos sprite header
    if (!previewBitmap_ && data.size() >= 12) {
        uint16_t width = data[0] | (data[1] << 8);
        uint16_t height = data[2] | (data[3] << 8);

        // Sanity check
        if (width > 0 && width <= 512 && height > 0 && height <= 512) {
            previewWidth_ = width;
            previewHeight_ = height;

            // Create pixel buffer
            previewPixels_.resize(width * height);

            // Fill with magenta (transparent indicator)
            for (auto& p : previewPixels_) {
                p = 0xFFFF00FF;
            }

            // Try to decode sprite data
            // Simple visualization: treat bytes after header as palette indices
            size_t headerSize = 12;
            size_t idx = headerSize;

            for (int y = 0; y < height && idx < data.size(); ++y) {
                for (int x = 0; x < width && idx < data.size(); ++x) {
                    uint8_t c = data[idx++];
                    if (c == 0) {
                        previewPixels_[y * width + x] = 0xFF000000; // Transparent/black
                    } else {
                        // Map to a color
                        uint8_t r = ((c >> 5) & 7) * 36;
                        uint8_t g = ((c >> 2) & 7) * 36;
                        uint8_t b = (c & 3) * 85;
                        previewPixels_[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                    }
                }
            }

            // Create GDI bitmap
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height; // Top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            HDC hdc = GetDC(previewStatic_);
            previewBitmap_ = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT,
                                            previewPixels_.data(), &bmi, DIB_RGB_COLORS);
            ReleaseDC(previewStatic_, hdc);
        }
    }

    // If no sprite decode, visualize raw data
    if (!previewBitmap_ && !data.empty()) {
        int gridW = 32;
        int gridH = std::min((int)(data.size() + gridW - 1) / gridW, 128);

        previewWidth_ = gridW * 4;
        previewHeight_ = gridH * 4;
        previewPixels_.resize(previewWidth_ * previewHeight_);

        // Fill with dark gray
        for (auto& p : previewPixels_) {
            p = 0xFF202020;
        }

        // Draw bytes as colored blocks
        for (size_t i = 0; i < data.size() && (int)(i / gridW) < gridH; ++i) {
            uint8_t value = data[i];
            int bx = (i % gridW) * 4;
            int by = (i / gridW) * 4;

            uint32_t color;
            if (value == 0x00) {
                color = 0xFF000000;
            } else if (value == 0xFF) {
                color = 0xFFFFFFFF;
            } else {
                uint8_t r = (value < 128) ? (255 - value * 2) : 0;
                uint8_t g = (value < 128) ? (value * 2) : (255 - (value - 128) * 2);
                uint8_t b = (value >= 128) ? ((value - 128) * 2) : 0;
                color = 0xFF000000 | (r << 16) | (g << 8) | b;
            }

            for (int dy = 0; dy < 4; ++dy) {
                for (int dx = 0; dx < 4; ++dx) {
                    int px = bx + dx;
                    int py = by + dy;
                    if (px < previewWidth_ && py < previewHeight_) {
                        previewPixels_[py * previewWidth_ + px] = color;
                    }
                }
            }
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = previewWidth_;
        bmi.bmiHeader.biHeight = -previewHeight_;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        HDC hdc = GetDC(previewStatic_);
        previewBitmap_ = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT,
                                        previewPixels_.data(), &bmi, DIB_RGB_COLORS);
        ReleaseDC(previewStatic_, hdc);
    }

    // Redraw preview
    InvalidateRect(previewStatic_, nullptr, TRUE);
}

void AssetViewerWindow::drawPreview(HDC hdc) {
    RECT rc;
    GetClientRect(previewStatic_, &rc);

    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(RGB(40, 40, 40));
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    if (previewBitmap_) {
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, previewBitmap_);

        // Calculate scaling to fit
        int srcW = previewWidth_;
        int srcH = previewHeight_;
        int dstW = rc.right - rc.left - 20;
        int dstH = rc.bottom - rc.top - 20;

        float scaleX = (float)dstW / srcW;
        float scaleY = (float)dstH / srcH;
        float scale = std::min(scaleX, scaleY);
        scale = std::min(scale, 4.0f); // Max 4x zoom

        int drawW = (int)(srcW * scale);
        int drawH = (int)(srcH * scale);
        int drawX = (rc.right - drawW) / 2;
        int drawY = (rc.bottom - drawH) / 2;

        SetStretchBltMode(hdc, COLORONCOLOR);
        StretchBlt(hdc, drawX, drawY, drawW, drawH,
                   memDC, 0, 0, srcW, srcH, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteDC(memDC);

        // Draw size info
        char sizeText[64];
        snprintf(sizeText, sizeof(sizeText), "%dx%d", srcW, srcH);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(150, 150, 150));
        TextOutA(hdc, 5, rc.bottom - 20, sizeText, (int)strlen(sizeText));
    } else {
        // Draw placeholder text
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(100, 100, 100));
        const char* text = "Select a resource to preview";
        DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

LRESULT CALLBACK AssetViewerWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AssetViewerWindow* self = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        self = (AssetViewerWindow*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = (AssetViewerWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (self) {
        switch (msg) {
            case WM_COMMAND:
                if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_FILE_COMBO) {
                    self->onFileSelected();
                }
                if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_RESOURCE_LIST) {
                    self->onResourceSelected();
                }
                break;

            case WM_DRAWITEM:
                if (wParam == IDC_PREVIEW) {
                    DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
                    self->drawPreview(dis->hDC);
                    return TRUE;
                }
                break;

            case WM_SIZE: {
                int w = LOWORD(lParam);
                int h = HIWORD(lParam);
                // Resize controls
                if (self->resourceList_) {
                    MoveWindow(self->resourceList_, 10, 60, 340, h - 280, TRUE);
                }
                if (self->previewStatic_) {
                    MoveWindow(self->previewStatic_, 360, 60, w - 380, h - 280, TRUE);
                }
                if (self->infoEdit_) {
                    MoveWindow(self->infoEdit_, 10, h - 210, w - 30, 180, TRUE);
                }
                break;
            }

            case WM_CLOSE:
                self->close();
                return 0;

            case WM_DESTROY:
                self->hwnd_ = nullptr;
                return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace opengg

#endif // _WIN32
