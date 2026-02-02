#include "ne_resource.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace opengg {

NEResourceExtractor::NEResourceExtractor() = default;
NEResourceExtractor::~NEResourceExtractor() = default;

bool NEResourceExtractor::open(const std::string& path) {
    filePath_ = path;
    resources_.clear();

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        lastError_ = "Failed to open file: " + path;
        return false;
    }

    // Read DOS header
    DOSHeader dosHeader;
    file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
    if (!file || dosHeader.magic != DOS_MAGIC) {
        lastError_ = "Invalid DOS header (not MZ)";
        return false;
    }

    // Seek to NE header
    file.seekg(dosHeader.neHeaderOffset);
    if (!file) {
        lastError_ = "Invalid NE header offset";
        return false;
    }

    // Read NE header
    NEHeader neHeader;
    file.read(reinterpret_cast<char*>(&neHeader), sizeof(neHeader));
    if (!file || neHeader.magic != NE_MAGIC) {
        lastError_ = "Invalid NE header (not NE executable)";
        return false;
    }

    neHeaderOffset_ = dosHeader.neHeaderOffset;
    alignmentShift_ = neHeader.alignmentShift;

    // Calculate resource table offset (relative to NE header)
    uint32_t resourceTableOffset = neHeaderOffset_ + neHeader.resourceTableOffset;
    file.seekg(resourceTableOffset);
    if (!file) {
        lastError_ = "Failed to seek to resource table";
        return false;
    }

    // Read alignment shift from resource table
    uint16_t resAlignShift;
    file.read(reinterpret_cast<char*>(&resAlignShift), sizeof(resAlignShift));
    if (resAlignShift > 0) {
        alignmentShift_ = resAlignShift;
    }

    // Parse resource types
    while (file) {
        NEResourceTypeInfo typeInfo;
        file.read(reinterpret_cast<char*>(&typeInfo), sizeof(typeInfo));

        if (!file || typeInfo.typeId == 0) {
            break;  // End of resource table
        }

        // Read all resources of this type
        for (uint16_t i = 0; i < typeInfo.count; ++i) {
            NEResourceNameInfo nameInfo;
            file.read(reinterpret_cast<char*>(&nameInfo), sizeof(nameInfo));

            if (!file) {
                break;
            }

            Resource res;
            res.typeId = typeInfo.typeId;
            res.id = nameInfo.id;
            res.offset = static_cast<uint32_t>(nameInfo.offset) << alignmentShift_;
            res.size = static_cast<uint32_t>(nameInfo.length) << alignmentShift_;
            res.flags = nameInfo.flags;

            // Determine type name
            res.typeName = getResourceTypeName(typeInfo.typeId);

            resources_.push_back(res);
        }
    }

    return true;
}

std::vector<Resource> NEResourceExtractor::listResources() const {
    return resources_;
}

std::vector<Resource> NEResourceExtractor::listResourcesByType(uint16_t type) const {
    std::vector<Resource> result;
    for (const auto& res : resources_) {
        if (res.typeId == type) {
            result.push_back(res);
        }
    }
    return result;
}

std::vector<uint8_t> NEResourceExtractor::extractResource(uint16_t type, uint16_t id) {
    for (const auto& res : resources_) {
        if (res.typeId == type && res.id == id) {
            return extractResourceByOffset(res.offset, res.size);
        }
    }
    lastError_ = "Resource not found";
    return {};
}

std::vector<uint8_t> NEResourceExtractor::extractResource(const Resource& res) {
    return extractResourceByOffset(res.offset, res.size);
}

std::vector<uint8_t> NEResourceExtractor::extractResourceByOffset(uint32_t offset, uint32_t size) {
    std::ifstream file(filePath_, std::ios::binary);
    if (!file) {
        lastError_ = "Failed to reopen file";
        return {};
    }

    file.seekg(offset);
    if (!file) {
        lastError_ = "Failed to seek to resource offset";
        return {};
    }

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file) {
        lastError_ = "Failed to read resource data";
        return {};
    }

    return data;
}

bool NEResourceExtractor::extractBitmap(uint16_t id, const std::string& outPath) {
    auto data = extractResource(NE_RT_BITMAP, id);
    if (data.empty()) {
        return false;
    }

    // Windows bitmap resources don't include the BITMAPFILEHEADER
    // We need to add it when saving as a .bmp file

    // Parse BITMAPINFOHEADER to get size info
    if (data.size() < 40) {
        lastError_ = "Bitmap data too small";
        return false;
    }

    uint32_t headerSize = *reinterpret_cast<uint32_t*>(data.data());
    int32_t width = *reinterpret_cast<int32_t*>(data.data() + 4);
    int32_t height = *reinterpret_cast<int32_t*>(data.data() + 8);
    uint16_t bitCount = *reinterpret_cast<uint16_t*>(data.data() + 14);

    // Calculate palette size
    uint32_t paletteSize = 0;
    if (bitCount <= 8) {
        uint32_t colorCount = *reinterpret_cast<uint32_t*>(data.data() + 32);
        if (colorCount == 0) {
            colorCount = 1u << bitCount;
        }
        paletteSize = colorCount * 4;
    }

    // Calculate offsets
    uint32_t dataOffset = 14 + headerSize + paletteSize;
    uint32_t fileSize = 14 + static_cast<uint32_t>(data.size());

    // Create BITMAPFILEHEADER
    uint8_t fileHeader[14] = {0};
    fileHeader[0] = 'B';
    fileHeader[1] = 'M';
    *reinterpret_cast<uint32_t*>(fileHeader + 2) = fileSize;
    *reinterpret_cast<uint32_t*>(fileHeader + 10) = dataOffset;

    // Write complete bitmap file
    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile) {
        lastError_ = "Failed to create output file";
        return false;
    }

    outFile.write(reinterpret_cast<char*>(fileHeader), 14);
    outFile.write(reinterpret_cast<char*>(data.data()), data.size());

    return true;
}

std::string NEResourceExtractor::getResourceTypeName(uint16_t typeId) const {
    switch (typeId) {
        case NE_RT_CURSOR:       return "CURSOR";
        case NE_RT_BITMAP:       return "BITMAP";
        case NE_RT_ICON:         return "ICON";
        case NE_RT_MENU:         return "MENU";
        case NE_RT_DIALOG:       return "DIALOG";
        case NE_RT_STRING:       return "STRING";
        case NE_RT_FONTDIR:      return "FONTDIR";
        case NE_RT_FONT:         return "FONT";
        case NE_RT_ACCELERATOR:  return "ACCELERATOR";
        case NE_RT_RCDATA:       return "RCDATA";
        case NE_RT_GROUP_CURSOR: return "GROUP_CURSOR";
        case NE_RT_GROUP_ICON:   return "GROUP_ICON";
        default:
            // Custom or unknown type
            if (typeId & 0x8000) {
                return "CUSTOM_" + std::to_string(typeId & 0x7FFF);
            }
            return "UNKNOWN";
    }
}

std::string NEResourceExtractor::getLastError() const {
    return lastError_;
}

} // namespace opengg
