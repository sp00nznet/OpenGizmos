#include "ne_resource.h"
#include "grp_archive.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstring>

namespace fs = std::filesystem;
using namespace opengg;

void printUsage(const char* progName) {
    std::cout << "OpenGizmos Asset Tool\n\n";
    std::cout << "Usage: " << progName << " <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  list-ne <file>           List resources in NE file (.DAT, .RSC)\n";
    std::cout << "  extract-ne <file> <out>  Extract all resources from NE file\n";
    std::cout << "  list-grp <file>          List files in GRP archive\n";
    std::cout << "  extract-grp <file> <out> Extract all files from GRP archive\n";
    std::cout << "  info <gamepath>          Show game file information\n";
    std::cout << "  validate <gamepath>      Validate game installation\n";
    std::cout << "\n";
}

void listNE(const std::string& path) {
    NEResourceExtractor ne;

    if (!ne.open(path)) {
        std::cerr << "Error: " << ne.getLastError() << "\n";
        return;
    }

    auto resources = ne.listResources();
    std::cout << "Found " << resources.size() << " resources in " << path << "\n\n";

    std::cout << "Type\t\tID\tSize\tOffset\n";
    std::cout << "----\t\t--\t----\t------\n";

    for (const auto& res : resources) {
        std::cout << res.typeName << "\t";
        if (res.typeName.length() < 8) std::cout << "\t";
        std::cout << res.id << "\t";
        std::cout << res.size << "\t";
        std::cout << "0x" << std::hex << res.offset << std::dec << "\n";
    }
}

void extractNE(const std::string& path, const std::string& outDir) {
    NEResourceExtractor ne;

    if (!ne.open(path)) {
        std::cerr << "Error: " << ne.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto resources = ne.listResources();
    int extracted = 0;

    for (const auto& res : resources) {
        std::string filename = res.typeName + "_" + std::to_string(res.id);

        // Add extension based on type
        if (res.typeId == RT_BITMAP) {
            filename += ".bmp";
        } else if (res.typeId == RT_RCDATA) {
            filename += ".dat";
        } else {
            filename += ".bin";
        }

        std::string outPath = outDir + "/" + filename;

        if (res.typeId == RT_BITMAP) {
            if (ne.extractBitmap(res.id, outPath)) {
                extracted++;
                std::cout << "Extracted: " << filename << "\n";
            }
        } else {
            auto data = ne.extractResource(res);
            if (!data.empty()) {
                std::ofstream outFile(outPath, std::ios::binary);
                outFile.write(reinterpret_cast<char*>(data.data()), data.size());
                extracted++;
                std::cout << "Extracted: " << filename << "\n";
            }
        }
    }

    std::cout << "\nExtracted " << extracted << " resources.\n";
}

void listGRP(const std::string& path) {
    GrpArchive grp;

    if (!grp.open(path)) {
        std::cerr << "Error: " << grp.getLastError() << "\n";
        return;
    }

    auto files = grp.listFiles();
    std::cout << "Found " << files.size() << " files in " << path << "\n\n";

    std::cout << "Name\t\t\tSize\n";
    std::cout << "----\t\t\t----\n";

    for (const auto& name : files) {
        auto* entry = grp.getEntry(name);
        std::cout << name;
        if (name.length() < 8) std::cout << "\t";
        if (name.length() < 16) std::cout << "\t";
        std::cout << "\t" << (entry ? entry->size : 0) << "\n";
    }
}

void extractGRP(const std::string& path, const std::string& outDir) {
    GrpArchive grp;

    if (!grp.open(path)) {
        std::cerr << "Error: " << grp.getLastError() << "\n";
        return;
    }

    fs::create_directories(outDir);

    auto files = grp.listFiles();
    int extracted = 0;

    for (const auto& name : files) {
        auto data = grp.extract(name);
        if (!data.empty()) {
            std::string outPath = outDir + "/" + name;
            std::ofstream outFile(outPath, std::ios::binary);
            outFile.write(reinterpret_cast<char*>(data.data()), data.size());
            extracted++;
            std::cout << "Extracted: " << name << "\n";
        }
    }

    std::cout << "\nExtracted " << extracted << " files.\n";
}

void showInfo(const std::string& gamePath) {
    std::cout << "Game Path: " << gamePath << "\n\n";

    // Check for key files
    std::vector<std::pair<std::string, std::string>> keyFiles = {
        {"SSGWIN32.EXE", "Main executable"},
        {"SSGWINCD/GIZMO.DAT", "16-color graphics"},
        {"SSGWINCD/GIZMO256.DAT", "256-color graphics"},
        {"SSGWINCD/PUZZLE.DAT", "Puzzle graphics"},
        {"SSGWINCD/FONT.DAT", "Fonts"},
        {"MOVIES/INTRO.SMK", "Intro video"},
    };

    std::cout << "File Status:\n";
    std::cout << "------------\n";

    for (const auto& [file, desc] : keyFiles) {
        std::string fullPath = gamePath + "/" + file;
        bool exists = fs::exists(fullPath);

        std::cout << (exists ? "[OK]  " : "[--]  ");
        std::cout << desc << " (" << file << ")";

        if (exists) {
            auto size = fs::file_size(fullPath);
            std::cout << " - " << size << " bytes";
        }

        std::cout << "\n";
    }

    // List DAT files
    std::cout << "\nDAT Files Found:\n";
    std::string datDir = gamePath + "/SSGWINCD";
    if (fs::exists(datDir)) {
        for (const auto& entry : fs::directory_iterator(datDir)) {
            if (entry.path().extension() == ".DAT") {
                NEResourceExtractor ne;
                if (ne.open(entry.path().string())) {
                    auto resources = ne.listResources();
                    std::cout << "  " << entry.path().filename().string();
                    std::cout << " - " << resources.size() << " resources\n";
                }
            }
        }
    }

    // List GRP files
    std::cout << "\nGRP Files Found:\n";
    std::string assetDir = gamePath + "/ASSETS";
    if (fs::exists(assetDir)) {
        for (const auto& entry : fs::directory_iterator(assetDir)) {
            if (entry.path().extension() == ".GRP") {
                GrpArchive grp;
                if (grp.open(entry.path().string())) {
                    auto files = grp.listFiles();
                    std::cout << "  " << entry.path().filename().string();
                    std::cout << " - " << files.size() << " files\n";
                }
            }
        }
    }

    // List SMK files
    std::cout << "\nVideo Files Found:\n";
    std::string movieDir = gamePath + "/MOVIES";
    if (fs::exists(movieDir)) {
        for (const auto& entry : fs::directory_iterator(movieDir)) {
            if (entry.path().extension() == ".SMK") {
                auto size = fs::file_size(entry.path());
                std::cout << "  " << entry.path().filename().string();
                std::cout << " - " << (size / 1024) << " KB\n";
            }
        }
    }
}

bool validateGame(const std::string& gamePath) {
    std::cout << "Validating game installation at: " << gamePath << "\n\n";

    std::vector<std::string> requiredFiles = {
        "SSGWIN32.EXE",
        "SSGWINCD/GIZMO.DAT",
    };

    std::vector<std::string> optionalFiles = {
        "SSGWINCD/GIZMO256.DAT",
        "SSGWINCD/PUZZLE.DAT",
        "SSGWINCD/FONT.DAT",
        "MOVIES/INTRO.SMK",
    };

    bool valid = true;

    std::cout << "Required Files:\n";
    for (const auto& file : requiredFiles) {
        std::string fullPath = gamePath + "/" + file;
        bool exists = fs::exists(fullPath);
        std::cout << (exists ? "  [OK] " : "  [MISSING] ") << file << "\n";
        if (!exists) valid = false;
    }

    std::cout << "\nOptional Files:\n";
    for (const auto& file : optionalFiles) {
        std::string fullPath = gamePath + "/" + file;
        bool exists = fs::exists(fullPath);
        std::cout << (exists ? "  [OK] " : "  [--] ") << file << "\n";
    }

    // Try to open a DAT file
    std::cout << "\nFile Format Validation:\n";

    std::string gizmoDat = gamePath + "/SSGWINCD/GIZMO.DAT";
    if (fs::exists(gizmoDat)) {
        NEResourceExtractor ne;
        if (ne.open(gizmoDat)) {
            auto resources = ne.listResources();
            std::cout << "  [OK] GIZMO.DAT is valid NE format (" << resources.size() << " resources)\n";
        } else {
            std::cout << "  [FAIL] GIZMO.DAT: " << ne.getLastError() << "\n";
            valid = false;
        }
    }

    std::cout << "\n" << (valid ? "Validation PASSED" : "Validation FAILED") << "\n";
    return valid;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    if (command == "list-ne" && argc >= 3) {
        listNE(argv[2]);
    } else if (command == "extract-ne" && argc >= 4) {
        extractNE(argv[2], argv[3]);
    } else if (command == "list-grp" && argc >= 3) {
        listGRP(argv[2]);
    } else if (command == "extract-grp" && argc >= 4) {
        extractGRP(argv[2], argv[3]);
    } else if (command == "info" && argc >= 3) {
        showInfo(argv[2]);
    } else if (command == "validate" && argc >= 3) {
        return validateGame(argv[2]) ? 0 : 1;
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
