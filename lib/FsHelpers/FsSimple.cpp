#pragma once

#include <cstddef>
#include <Arduino.h>
#include <vector>

#include "FsSimple.h"

class FsSimple::Impl {
 public:
  const esp_partition_t* partition = nullptr;
  const Header* header = nullptr;
  const uint8_t* mmap_data = nullptr;
};

bool FsSimple::begin() {
  assert(impl == nullptr && "begin called multiple times");
  impl = new Impl();

  impl->partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
  if (!impl->partition) {
    Serial.printf("[%lu] [FSS] SPIFFS partition not found\n", millis());
    return false;
  }

  spi_flash_mmap_handle_t map_handle; // unused
  size_t len = impl->partition->size;
  auto err = esp_partition_mmap(impl->partition, 0, len, SPI_FLASH_MMAP_DATA,
                                (const void**)&impl->mmap_data, &map_handle);
  if (err != 0 || impl->mmap_data == nullptr) {
    Serial.printf("[%lu] [FSS] mmap failed, code: %d\n", millis(), err);
    return false;
  }

  impl->header = (const Header*)impl->mmap_data;
  if (impl->header->magic != MAGIC) {
    Serial.printf("[%lu] [FSS] Invalid magic: 0x%08X\n", millis(), impl->header->magic);
    return false;
  }

  Serial.printf("[%lu] [FSS] FsSimple initialized\n", millis());
  return true;
}

const FsSimple::Header* FsSimple::getRoot() {
  assert(impl != nullptr);
  return impl->header;
}

const uint8_t* FsSimple::mmap(const FsSimple::FileEntry* entry) {
  assert(impl != nullptr);
  assert(impl->header != nullptr);
  assert(impl->mmap_data != nullptr);

  size_t offset = sizeof(Header);
  for (size_t i = 0; i < MAX_FILES; i++) {
    const FileEntry& e = impl->header->entries[i];
    if (&e == entry) {
      break;
    }
    offset += e.size;
    offset += get_padding(e.size);
  }
  return impl->mmap_data + offset;
}

bool FsSimple::erase(size_t size) {
  assert(impl != nullptr);
  assert(impl->partition != nullptr);
  assert(size <= impl->partition->size);

  auto err = esp_partition_erase_range(impl->partition, 0, size);
  if (err != 0) {
    Serial.printf("[%lu] [FSS] erase failed, code: %d\n", millis(), err);
    return false;
  }
  return true;
}

bool FsSimple::write(uint32_t offset, const uint8_t* data, size_t len) {
  assert(impl != nullptr);
  assert(impl->partition != nullptr);
  assert(offset + len <= impl->partition->size);

  auto err = esp_partition_write(impl->partition, offset, data, len);
  if (err != 0) {
    Serial.printf("[%lu] [FSS] write failed at offset %u, code: %d\n", millis(), offset, err);
    return false;
  }
  return true;
}
