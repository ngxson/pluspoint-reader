#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

// Simple implementation of read-only packed filesystem
class FsSimple {
 public:
  FsSimple() = default;
  ~FsSimple() = default;

  static constexpr size_t MAX_FILES = 32;
  static constexpr size_t MAX_FILE_NAME_LENGTH = 32;
  static constexpr size_t MAX_ALLOC_SIZE = 3 * 1024 * 1024; // 3 MB
  static constexpr size_t ALIGNMENT = 4; // bytes
  static constexpr uint32_t MAGIC = 0x46535631; // "FSV1"

  enum FileType {
    FILETYPE_INVALID = 0,
    FILETYPE_FONT_REGULAR = 1,
  };

  struct __attribute__((packed)) FileEntry {
    uint32_t type;
    uint32_t size;
    char name[MAX_FILE_NAME_LENGTH];
  };
  static_assert(sizeof(FileEntry) == (4 + 4 + MAX_FILE_NAME_LENGTH));

  struct __attribute__((packed)) Header {
    uint32_t magic;
    FileEntry entries[MAX_FILES];
  };
  static_assert(sizeof(Header) == 4 + MAX_FILES * sizeof(FileEntry));
  static_assert(sizeof(Header) % ALIGNMENT == 0);

  size_t get_padding(size_t size) {
    size_t remainder = size % ALIGNMENT;
    return (remainder == 0) ? 0 : (ALIGNMENT - remainder);
  }

  bool begin();
  const Header* getRoot();
  const uint8_t* mmap(const FileEntry* entry);

  // flash writing
  bool erase(size_t size);
  bool write(uint32_t offset, const uint8_t* data, size_t len);

#ifdef FS_SIMPLE_ENABLE_WRITE
  // to be used by host CLI tool that creates the packed filesystem
  uint8_t writeData[MAX_ALLOC_SIZE];
  size_t writeDataSize = 0;

  void beginCreate() {
    for (size_t i = 0; i < MAX_FILES; i++) {
      writeHeader.entries[i].type = FILETYPE_INVALID;
      writeHeader.entries[i].size = 0;
      writeHeader.entries[i].name[0] = '\0';
    }
  }

  // return error message or nullptr if successful
  const char* addFile(Header& header, const FileEntry& entry, const uint8_t* data) {
    if (entry.size % ALIGNMENT != 0) {
      return "File size must be multiple of alignment";
    }
    if (writeDataSize + entry.size > MAX_ALLOC_SIZE) {
      return "Not enough space in FsSimple image";
    }
    if (entry.size == 0 || entry.name[0] == '\0') {
      return "Invalid file entry";
    }
    // find empty slot
    for (size_t i = 0; i < MAX_FILES; i++) {
      if (header.entries[i].type == FILETYPE_INVALID) {
        header.entries[i] = entry;
        // copy data
        size_t offset = writeDataSize;
        for (size_t j = 0; j < entry.size; j++) {
          writeData[offset + j] = data[j];
        }
        writeDataSize += entry.size; // (no need padding here as size is already aligned)
        return nullptr; // success
      } else if (strncmp(header.entries[i].name, entry.name, MAX_FILE_NAME_LENGTH) == 0) {
        return "File with the same name already exists";
      }
    }
    return "No empty slot available";
  }
#endif

 private:
  // using pimpl to allow re-using this header on host system without Arduino dependencies
  class Impl;
  Impl* impl = nullptr;
};
