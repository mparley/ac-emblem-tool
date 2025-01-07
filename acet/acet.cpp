#include "acet.h"

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
				throw std::runtime_error("InjectImage: Counted more than 255 colors + alpha in the PNG");

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