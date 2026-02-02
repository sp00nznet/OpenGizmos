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

    // Add known DAT files
    fileNames_ = {
        "GIZMO.DAT",
        "GIZMO256.DAT",
        "GIZMO16.DAT",
        "PUZZLE.DAT",
        "PUZ256.DAT",
        "PUZ16.DAT",
        "FONT.DAT",
        "AE.DAT",
        "AE256.DAT",
        "AUTO.DAT",
        "AUTO256.DAT",
        "PLANE.DAT",
        "PLANE256.DAT",
        "SSGWIN.DAT",
        "[RAW] GIZMO.DAT @ 0x80000"  // Raw file view mode
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

void AssetViewerWindow::onFileSelected() {
    if (!resourceList_ || !cache_) return;

    int sel = (int)SendMessage(fileCombo_, CB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)fileNames_.size()) return;

    selectedFile_ = sel;
    std::string filename = fileNames_[sel];

    // Check if this is raw file view mode
    if (filename.find("[RAW]") == 0) {
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

    // Load palette from AUTO256.BMP (game's actual 256-color palette)
    uint8_t palette[256][4] = {}; // BGRA format
    bool paletteLoaded = false;

    // Try game palette first (AUTO256.BMP)
    std::string palettePath = gamePath + "/INSTALL/AUTO256.BMP";
    std::ifstream palFile(palettePath, std::ios::binary);
    if (palFile) {
        palFile.seekg(54); // Palette starts at offset 54 in BMP
        palFile.read(reinterpret_cast<char*>(palette), 1024);
        paletteLoaded = true;
    } else {
        // Fallback to SSGWINCD location
        palettePath = gamePath + "/SSGWINCD/../INSTALL/AUTO256.BMP";
        palFile.open(palettePath, std::ios::binary);
        if (palFile) {
            palFile.seekg(54);
            palFile.read(reinterpret_cast<char*>(palette), 1024);
            paletteLoaded = true;
        }
    }

    // Update info
    char info[512];
    snprintf(info, sizeof(info),
             "%s VIEW: %s\r\nOffset: 0x%X\r\nSize: %dx%d\r\nPixels: %zu\r\nMode: %s\r\nPalette: %s\r\n",
             useRLE ? "RLE" : "RAW",
             baseFile.c_str(), offset, width, height, data.size(),
             useRLE ? "RLE decompressed (FF xx count)" : "Raw palette indices",
             paletteLoaded ? "Game palette (AUTO256.BMP)" : "Default (colors may be wrong)");
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
    std::string filename = fileNames_[selectedFile_];

    // Get raw data
    auto data = cache_->getRawResource(filename, res.typeId, res.id);
    if (data.empty()) return;

    // Update info text
    std::string info = "File: " + filename + "\r\n";
    info += "Resource: " + res.typeName + " #" + std::to_string(res.id) + "\r\n";
    info += "Size: " + std::to_string(res.size) + " bytes\r\n";
    info += "Offset: 0x" + std::to_string(res.offset) + "\r\n\r\n";

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

    // Parse sprite header
    if (data.size() >= 12) {
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
