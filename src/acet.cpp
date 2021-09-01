#include "acet.h"

void ExitMessage(std::string message) {
	std::cout << message
		<< "\nExiting\n" 
		<< "Press enter key to continue...\n";
	std::cin.get();
	exit(EXIT_FAILURE);
}

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

		if (!found) ExitMessage("Couldn't find emblem data in save file.");

	} else if (save.size() < kSaveSize) {
		ExitMessage("Save file is too small to be an emblem save");
	}

	return offset;
}

// Injects image into the emblem save. Modified save is passed back thru param
void InjectImage(std::vector<uint8_t>& save, const std::vector<uint8_t>& image) {
    size_t emblem_offset = FindOffset(save);
	std::unordered_map<uint32_t, int> palette_map = { {PackColor(0,0,0,0), 0} };

	for (int i = 0; i < image.size() / 4; i++) {
		// Anything not fully opaque will be made completely transparent
		uint32_t c = 0;
		if (image[i*4+3] == 0xFF)
			c = PackColor(image[i*4], image[i*4+1], image[i*4+2], 0x80);

		// Adding color to hash map to count non-duplicate colors and build palette
		if (palette_map.find(c) == palette_map.end()) {
			if (palette_map.size() > 256)
				ExitMessage("Your PNG has more than 255 colors + alpha color");

			palette_map[c] = palette_map.size();

			for (int channel = 0; channel < 4; channel++) {
				size_t color_index = OffsetIndex(
					kImageOffset + kNumPixels + (palette_map[c] * 4) + channel,
					emblem_offset
				);

				int shift = 24 - (channel * 8);
				save[color_index] = (c >> shift) & 0xFF;
			}
		}

		save[OffsetIndex(i + kImageOffset, emblem_offset)] = UnscramblePalette(palette_map[c]);
	}

	// Apply the checksums to the emblem data
	
	// The first checksum needs 0x98 included in the sum (or 0x68 added after)
	uint8_t sum = 0x98;

	// The last checksum isn't on the 0x400 interval so we need to find the 2nd 
	// to last checksum location
	size_t prev_check = 0;

	for (size_t i = 0; i < kSaveSize; i++) {
		if ((i + 1) % 0x400 == 0) {
			sum = ~(sum + 0xFF) + 1;
			prev_check = i + emblem_offset;
			save[prev_check] = sum;
			sum = 0;
		} else {
			sum += save[i + emblem_offset];
		}
	}

	// Subtract the two extra bytes from the orignal save that got caught in our sum
	sum -= (save[prev_check + 0x36] + 0x01);

	// Final checksum has it's own magic number
	save[prev_check + 0x36] = ~(sum + 0xFF) + 1 + 0xCA; 

	return;
}

// Extracts an image from the save
std::vector<uint8_t> ExtractImage(const std::vector<uint8_t>& save) {
    size_t emblem_offset = FindOffset(save);

	uint8_t pixels[kNumPixels]; 
	uint8_t colors[kPaletteSize]; 

	for (int i = 0; i < kNumPixels; i++) {
		pixels[i] = save[OffsetIndex(kImageOffset + i, emblem_offset)];
	}

	for (int i = 0; i < kPaletteSize; i++) {
		uint8_t c = save[OffsetIndex(kImageOffset + kNumPixels + i, emblem_offset)];

		// The emblem only has on/off alpha but encodes full opacity as 0x80
		if (i % 4 == 3 && c != 0)
			c = 0xFF;

		colors[i] = c;
	}

	std::vector<uint8_t> image(kNumPixels * 4, 0);
	for (int i = 0; i < kNumPixels; i++)
		for (int c = 0; c < 4; c++)
			image[i*4 + c] = colors[UnscramblePalette(pixels[i])*4 + c];

	return image;
}