#include "acet.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <fstream>

// Offsets the index and accounts for checksum bytes shifting the index
size_t OffsetIndex(size_t i, size_t emblem_offset) {
	return emblem_offset + i + (i)/0x3FF;
}

uint32_t PackColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	return (r << 24) | (g << 16) | (b << 8) | a;
}

// This maps the pixel's color ids to the actual position in the 
// palette because for whatever reason it's all out of order
uint8_t UnscramblePalette(uint8_t index) {
	switch ((index % 32) / 8) {
		case 1:
			return index + 8;
			break;
		case 2:
			return index - 8;
			break;
		default:
			return index;
			break;
	}
}

// Looks at a save and finds where the actual emblem data starts
size_t FindOffset(const std::vector<uint8_t>& save) {
	size_t offset = 0;

	if (save.size() > kSaveSize) {
		bool found = false;

		for (size_t i = 0, j = 0; i < save.size(); i++) {
			if (j == sizeof(kEmblemHeader)) {
				offset = i - j;
				found = true;
				break;
			} else if (save[i] == kEmblemHeader[j])
				j++;
			else
				j = 0;
		}

		if (!found)
			throw std::runtime_error("FindOffset: Could't find emblem data in the save file.");

	} else if (save.size() < kSaveSize && save.size() != kLRSaveSize) {
		throw std::runtime_error("FindOffset: File is too small to be an Emblem Save");
	}

	return offset;
}

bool FindPaletteOffset(std::string filename, size_t& offset, size_t stop) {
  bool found = false;
  auto infile = std::ifstream(filename, std::ios::binary);

  if (!infile.is_open())
    throw std::runtime_error("FindPaletteOffset: Couldn't open file");

  infile.seekg(0, infile.end);
  size_t filesize = infile.tellg();
  if (filesize < stop) stop = filesize;
  infile.seekg(0, infile.beg);

  while(infile.good() && (size_t) infile.tellg() < stop) {
		uint8_t buffer[13];
    size_t curr = infile.tellg();

		infile.read((char*)buffer, 13);
    infile.seekg(curr+1);

		// std::cout << std::hex << curr << " " << (size_t)buffer[0] << ": [";
		// for (int i = 0; i < 13; i++) {
		// 	std::cout << std::hex << " " << (size_t) buffer[i];
		// }
		// std::cout << " ]\n";

    found = true;
    for (int i = 0, off = 0; i < 12; i++) {
      if (curr+i % 0x400 == 0x3FF) off = 1;

      if (i < 4 && buffer[i+off] != 0x00) {
        found = false;
        break;
      } else if ((i == 7 || i == 11) && buffer[i+off] != 0x80) {
        found = false;
        break;
      }
    }

    if (found) {
			offset = curr;
			break;
		}
  }

  infile.close();
  return found;
}

bool FindPaletteOffset(std::string filename, size_t stop) {
  size_t val = 0;
  return FindPaletteOffset(filename, val, stop);
}

// Injects image into the emblem save. Modified save is passed back thru param
void InjectImage(std::vector<uint8_t>& save, const std::vector<uint8_t>& image) {
	bool isLR = save.size() == kLRSaveSize;
	size_t emblem_offset = FindOffset(save);
	size_t image_offset = (isLR) ? kLRImageOffset : kImageOffset;
	std::unordered_map<uint32_t, int> palette_map = { {PackColor(0,0,0,0), 0} };

	for (int i = 0; i < image.size() / 4; i++) {
		// Anything not fully opaque will be made completely transparent
		uint32_t c = 0;
		if (image[i*4+3] == 0xFF)
			c = PackColor(image[i*4], image[i*4+1], image[i*4+2], 0x80);

		// Adding color to hash map to count non-duplicate colors and build palette
		if (palette_map.find(c) == palette_map.end()) {
			if (palette_map.size() > 256)
				throw std::runtime_error("InjectImage: Counted more than 255 colors + alpha in the image");

			palette_map[c] = palette_map.size();

			for (int channel = 0; channel < 4; channel++) {
				size_t color_index = OffsetIndex(
					image_offset + kNumPixels + (palette_map[c] * 4) + channel,
					emblem_offset
				);

				int shift = 24 - (channel * 8);
				save[color_index] = (c >> shift) & 0xFF;
			}
		}

		// LR doesn't have scrambled colors
		int p = (isLR) ? palette_map[c] : UnscramblePalette(palette_map[c]);
		save[OffsetIndex(i + image_offset, emblem_offset)] = p;
	}

	// Apply the checksums to the emblem data
	// Basically ~(sum + len) + 1 but the first one has a magic number added to sum
	uint8_t sum = (isLR) ? 0xB8 : 0x98;

	// The last checksum isn't on the 0x400 interval so we need to find the 2nd 
	// to last checksum location
	size_t prev_check = 0;

	size_t save_size = (isLR) ? kLRSaveSize : kSaveSize;

	for (size_t i = 0; i < save_size; i++) {
		if ((i + 1) % 0x400 == 0) {
			sum = ~(sum + 0x3FF) + 1;
			prev_check = i + emblem_offset;
			save[prev_check] = sum;
			sum = 0;
		} else {
			sum += save[i + emblem_offset];
		}
	}

	// The size of last interval
	size_t dangling = (isLR) ? 0x15 : 0x35;

	// Subtract the two extra bytes from the orignal save that got caught in our sum
	sum -= (save[prev_check + dangling] + 0x01);

	// Final checksum
	save[prev_check + dangling] = ~(sum + dangling) + 1; 

	return;
}

// Extracts an image from the save
std::vector<uint8_t> ExtractImage(const std::vector<uint8_t>& save) {
	bool isLR = save.size() == kLRSaveSize;
	size_t emblem_offset = FindOffset(save);
	size_t image_offset = (isLR) ? kLRImageOffset : kImageOffset;

	uint8_t pixels[kNumPixels]; 
	uint8_t colors[kPaletteSize]; 

	for (int i = 0; i < kNumPixels; i++) {
		pixels[i] = save[OffsetIndex(image_offset + i, emblem_offset)];
	}

	for (int i = 0; i < kPaletteSize; i++) {
		uint8_t c = save[OffsetIndex(image_offset + kNumPixels + i, emblem_offset)];

		// The emblem only has on/off alpha but encodes full opacity as 0x80
		if (i % 4 == 3 && c != 0)
			c = 0xFF;

		colors[i] = c;
	}

	std::vector<uint8_t> image(kNumPixels * 4, 0);
	for (int i = 0; i < kNumPixels; i++)
		for (int c = 0; c < 4; c++)
			image[i*4 + c] = (isLR) ? colors[pixels[i]*4 + c] : colors[UnscramblePalette(pixels[i])*4 + c];

	return image;
}

std::vector<uint8_t> ReadSaveFile(std::string filename) {
	std::ifstream infile(filename, std::ios::binary|std::ios::ate);
	if (!infile)
		throw std::runtime_error("ReadSaveFile: Could't open file");

	size_t save_size = infile.tellg();
	std::vector<uint8_t> buffer(save_size, 0);

	infile.seekg(0, infile.beg);
	infile.read((char*)buffer.data(), save_size);
	infile.close();

	return buffer;
}

std::vector<uint8_t> ExtractImageAC1(const std::string& filename, bool isMOA, uint8_t block, uint8_t emblemSlot) {
  if (block > 14)
    throw std::runtime_error("ExtractImageAC1: Block out of bounds (above 14)");
  if (emblemSlot > 6)
    throw std::runtime_error("ExtractImageAC1: Emblem slot out of bounds");

  std::ifstream infile(filename, std::ios::binary);
  if (!infile)
    throw std::runtime_error("ExtractImageAC1: Couldn't open file");

  const uint8_t factor = 256 / 32;
  uint8_t header[2];
  uint8_t checksum[4];
  uint8_t palette[32];
  uint8_t colors[64*64/2];
  std::vector<uint8_t> output;
  output.reserve(64*64*4);

  infile.seekg(0, infile.beg);
  infile.read((char*)header, 2);

  if (header[0] == 'S')
    block = 0;
  else if (block == 0) {
    infile.close();
    throw std::runtime_error("ExtractImageAC1: Attempted accessing block 0 in Memory Card");
  }

  const size_t off = (0x2000 * block) + (0x820 * emblemSlot);
  const size_t checksumOff = (isMOA) ? 0x180 : 0x200;

  infile.seekg(off, infile.beg);
  infile.seekg(checksumOff, infile.cur);
  infile.read((char*)checksum, 4);
  infile.read((char*)palette, 32);
  infile.read((char*)colors, 64*64/2);
  std::cout << "Post position " << infile.tellg() << "\n";
  infile.close();

  for (uint8_t cByte : colors) {
    uint8_t left = cByte >> 4;
    uint8_t right = cByte & 0b1111;
    uint8_t cIndex[2] = { right, left };

    for (auto i : cIndex) {
      uint8_t a = (i == 0) ? 0 : 0xFF;
      output.push_back((palette[i*2] & 0b11111) * factor);
      output.push_back((((palette[i*2+1] & 0b11) << 3) | (palette[i*2] >> 5)) * factor);
      output.push_back((palette[i*2+1] >> 2) * factor);
      output.push_back(a);
    }
  }

  std::cout << "Checksum ";
  for (auto c : checksum)
    std::cout << std::hex << (int)c << " ";
  std::cout << "\n";
  std::cout << "Size " << output.size() << "\n";
  return output;
}

void InjectImageAC1(std::string filename, const std::vector<uint8_t>& image, bool isMOA, uint8_t block, uint8_t emblemSlot) {
  if (block > 14)
    throw std::runtime_error("InjectImageAC1: Block out of bounds (above 14)");
  if (emblemSlot > 6)
    throw std::runtime_error("InjectImageAC1: Emblem slot out of bounds");
  if (image.size() != 64 * 64 * 4)
    throw std::runtime_error("InjectImageAC1: Incorrect image size");

  std::fstream fs(filename, std::ios::in | std::ios::out |std::ios::binary);
  if (!fs)
    throw std::runtime_error("InjectImageAC1: Couldn't open file");

  char header[2];
  fs.read(header, 2);
  if (header[0] == 'S') {
    block = 0;
  } else if (block == 0) {
    fs.close();
    throw std::runtime_error("InjectImageAC1: Attempted accessing block 0 in Memory Card");
  }

	std::unordered_map<uint16_t, int> palette_map = { {0x0000, 0} };
  uint8_t emblemColors[64*64] = {};
  const uint8_t factor = 256 / 32;

	for (int i = 0; i < image.size() / 4; i++) {
		// Anything not fully opaque will be made completely transparent
		uint16_t c = 0;
		if (image[i*4+3] == 0xFF) {
      uint8_t r = image[i*4] / factor;
      uint8_t g = image[i*4+1] / factor;
      uint8_t b = image[i*4+2] / factor;
      uint8_t left = r | (g << 5);
      uint8_t right = (g >> 3) | (b << 2);
      c = (left << 8) | (right);
    }

		// Adding color to hash map to count non-duplicate colors and build palette
		if (palette_map.find(c) == palette_map.end()) {
			if (palette_map.size() > 16)
				throw std::runtime_error("InjectImageAC1: Counted more than 255 colors + alpha in the image");

			palette_map[c] = palette_map.size();
      /*std::cout << std::hex << c << "\n";*/
		}

    emblemColors[i] = palette_map[c];
	}

  uint8_t paletteBytes[32] = {};
  for (auto p : palette_map) {
    int i = p.second * 2;
    paletteBytes[i] = p.first >> 8;
    paletteBytes[i+1] = p.first & 0xFF;
  }

  uint8_t emblemBytes[64*64/2] = {};
  for (int i = 0; i < 64*64; i+=2) {
    emblemBytes[i/2] = (emblemColors[i+1] << 4) | (emblemColors[i] & 0b1111);
  }

  const size_t off = (0x2000 * block) + (0x820 * emblemSlot);
  const size_t checksumOff = (isMOA) ? 0x184 : 0x204;

  fs.seekp(off);
  fs.seekp(checksumOff, fs.cur);
  fs.write((char*)paletteBytes, 32);
  fs.write((char*)emblemBytes, 64*64/2);
  fs.close();
  ApplyChecksumAC1(filename, isMOA, block);
	return;
}

void ApplyChecksumAC1(std::string filename, bool isMOA, uint8_t block) {
  if (block > 14)
    throw std::runtime_error("ApplyChecksumAC1: Block out of bounds (above 14)");

  std::fstream fs(filename, std::ios::in | std::ios::out |std::ios::binary);
  if (!fs)
    throw std::runtime_error("ApplyChecksumAC1: Couldn't open file");

  const size_t buffSize = 0x38e2;
  const size_t checksumOff = (isMOA) ? 0x180 : 0x200;

  uint8_t check[4] = {};
  uint8_t buffer[buffSize];
  fs.seekg(0x2000 * block);
  fs.seekg(checksumOff, fs.cur);
  fs.read((char*)check, 4);
  fs.read((char*)buffer, buffSize);

  /*uint32_t old = check[0] | (check[1] << 8) | (check[2] << 16) | (check[3] << 24);*/
  /*std::cout << "OLD: "<< std::hex << old << "\n";*/

  uint32_t sum = 0;
  for (auto b : buffer) {
    sum += b;
  }

  sum += buffSize;
  sum = -sum;

  /*uint32_t diff = sum - old;*/
  /*std::cout << std::hex << "DIFF: " << diff << "\n";*/
  /*std::cout << "NEW: " << std::hex << sum << "\n";*/

  check[0] = uint8_t(sum);
  check[1] = uint8_t(sum >> 8);
  check[2] = uint8_t(sum >> 16);
  check[3] = uint8_t(sum >> 24);

  fs.seekp(0x2000 * block);
  fs.seekp(checksumOff, fs.cur);
  fs.write((char*)check, 4);
  fs.close();
}

std::vector<std::string> ListPS1MCBlocks(std::string filename) {
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs)
    throw std::runtime_error("ListPS1MCBlocks: Couldn't open file");

  std::vector<std::string> out;
  char mcID[2] = {};

  ifs.read(mcID, 2);
  if (mcID[0] != 'M' || mcID[1] != 'C') {
    ifs.close();
    throw std::runtime_error("ListPS1MCBlocks: calling on non MC file");
  }

  for (int i = 0; i < 16; i++) {
    ifs.seekg((0x80 * i) + 0xA);
    char buffer[21] = {};
    ifs.read(buffer, 21);
    std::string savename = buffer;

    savename.erase(std::remove_if(savename.begin(), savename.end(), 
      [](unsigned char c){ return std::isspace(c);}), savename.end());

    out.push_back(savename);
  }

  ifs.close();
  return out;
}

bool isPS1Save(std::string filename) {
  std::ifstream infile(filename, std::ios::binary);
  if (!infile)
    throw std::runtime_error("isPS1Save: Cannot open file " + filename);

  char b[2];
  infile.read(b, 2);
  infile.close();

  return (b[0] == 'S' || b[0] == 'M') && (b[1] == 'C');
}
