#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>

const unsigned kSaveSize = 0x4440;
const unsigned kLRSaveSize = 0x4420;
const unsigned kImageOffset = 0x24; // The actual image data starts at offset 0x24
const unsigned kLRImageOffset = 0x4;
const unsigned kImageWidth = 128;
const unsigned kImageHeight = 128;
const unsigned kNumPixels = 0x4000; // 128*128 pixels
const unsigned kPaletteSize = 0x400; // 256 colors * 4 channels

const uint8_t kEmblemHeader[12] = {
    0x20, 0x44, 0x00, 0x00, 0x20, 0x44, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00
};

size_t OffsetIndex(size_t i, size_t emblem_offset);

uint32_t PackColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

uint8_t UnscramblePalette(uint8_t index);

size_t FindOffset(const std::vector<uint8_t>& save);

bool FindPaletteOffset(std::string filename, size_t& offset, size_t stop = 0xFFFF);

bool FindPaletteOffset(std::string filename, size_t stop = 0xFFFF);

void InjectImage(std::vector<uint8_t>& save, const std::vector<uint8_t>& image);

std::vector<uint8_t> ExtractImage(const std::vector<uint8_t>& save);

std::vector<uint8_t> ReadSaveFile(std::string filename);
