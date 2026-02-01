#pragma once

#include "formats/ne_format.h"
#include <string>
#include <vector>
#include <cstdint>

namespace opengg {

// Resource descriptor
struct Resource {
    uint16_t typeId;
    uint16_t id;
    uint32_t offset;
    uint32_t size;
    uint16_t flags;
    std::string typeName;
    std::string name;  // If named resource
};

// NE (New Executable) Resource Extractor
// Used for extracting resources from .DAT and .RSC files which are NE DLLs
class NEResourceExtractor {
public:
    NEResourceExtractor();
    ~NEResourceExtractor();

    // Open an NE executable file
    bool open(const std::string& path);

    // List all resources in the file
    std::vector<Resource> listResources() const;

    // List resources of a specific type
    std::vector<Resource> listResourcesByType(uint16_t type) const;

    // Extract a resource by type and ID
    std::vector<uint8_t> extractResource(uint16_t type, uint16_t id);

    // Extract a resource by descriptor
    std::vector<uint8_t> extractResource(const Resource& res);

    // Extract a bitmap resource and save as BMP file
    bool extractBitmap(uint16_t id, const std::string& outPath);

    // Get last error message
    std::string getLastError() const;

private:
    std::vector<uint8_t> extractResourceByOffset(uint32_t offset, uint32_t size);
    std::string getResourceTypeName(uint16_t typeId) const;

    std::string filePath_;
    std::vector<Resource> resources_;
    uint32_t neHeaderOffset_ = 0;
    uint16_t alignmentShift_ = 0;
    std::string lastError_;
};

} // namespace opengg
