#include "acet.h"

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

size_t ReadSave(std::string filename, std::vector<uint8_t> &buffer) {

	size_t offset = 0;

	std::ifstream infile(filename, std::ios::binary|std::ios::ate);
	if (!infile) {
		std::cerr << "Error - can't open file " << filename << "\n";
		exit(EXIT_FAILURE);
	}

	size_t save_size = infile.tellg();
	buffer.resize(save_size);
	
	infile.seekg(0, infile.beg);
	infile.read((char*)buffer.data(), save_size);

	if (save_size > kSaveSize) {
		for (size_t i = 0, j = 0; i < save_size; i++) {
			if (j == sizeof(kEmblemHeader)) {
				offset = i - j;
				break;
			} else if (buffer[i] == kEmblemHeader[j]) {
				j++;
			} else {
				j = 0;
			}
		}
	} else if (save_size < kSaveSize) {
		std::cout << "File " << filename << " too small to be an emblem save\n";
		infile.close();
		exit(EXIT_FAILURE);
	}

	infile.close();
	return offset;
}

size_t bufferIndex(size_t i, size_t emblem_offset) {
	return emblem_offset + kImageOffset + i + (kImageOffset+i)/0x3FF;
}

void InjectImage(std::string savename, std::string imagename) {
    std::vector<uint8_t> buffer;
    size_t emblem_offset = ReadSave(savename, buffer);

	// Load png
	std::vector<unsigned char> image;
	unsigned w = kImageWidth, h = kImageHeight;
	unsigned error = lodepng::decode(image, w, h, imagename);

	if (error) {
		std::cout << "Decoder error " << error << ": " << lodepng_error_text(error) 
			<< std::endl;
		return;
	}

	std::unordered_map<uint32_t, int> palette_map = { {PackColor(0,0,0,0), 0} };

	for (int i = 0; i < image.size() / 4; i++) {

		// Anything not fully opaque will be made completely transparent
		uint32_t c = 0;
		if (image[i*4+3] == 0xFF) {
			image[i*4+3] = 0x80;
			c = PackColor(image[i*4], image[i*4+1], image[i*4+2], image[i*4+3]);
		}

		// Adding color to hash map to count non-duplicate colors and build palette
		if (palette_map.find(c) == palette_map.end()) {
			if (palette_map.size() > 256) {
				std::cout << "Your PNG has more than 255 colors + alpha color" << std::endl;
				return;
			}
			palette_map[c] = palette_map.size();

			for (int channel = 0; channel < 4; channel++) {
				size_t color_index = bufferIndex(
					kNumPixels + (palette_map[c] * 4) + channel,
					emblem_offset
				);
				buffer[color_index] = image[i*4+channel];
			}
		}

		buffer[bufferIndex(i, emblem_offset)] = UnscramblePalette(palette_map[c]);
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
			buffer[prev_check] = sum;
			sum = 0;
		} else {
			sum += buffer[i + emblem_offset];
		}
	}

	// Subtract the extra bytes from the orignal save that got caught in our sum
	sum -= (buffer[prev_check + 0x36] + 0x01);

	// Final checksum has it's own magic number
	buffer[prev_check + 0x36] = ~(sum + 0xFF) + 1 + 0xCA; 

	std::ofstream outfile(savename + "-modified", std::ios::binary|std::ios::trunc);
	outfile.write((char*)buffer.data(), buffer.size());
	outfile.close();
}

// Extracts an image from the buffer and writes it to file
void WriteImage(std::string savename, std::string filename) {
    std::vector<uint8_t> buffer;
    size_t emblem_offset = ReadSave(savename, buffer);

	uint8_t pixels[kNumPixels]; 
	uint8_t colors[kPaletteSize]; 

	for (int i = 0; i < kNumPixels; i++) {
		pixels[i] = buffer[bufferIndex(i, emblem_offset)];
	}

	for (int i = 0; i < kPaletteSize; i++) {
		uint8_t c = buffer[bufferIndex(kNumPixels + i, emblem_offset)];
		if (i % 4 == 3 && c != 0)
			c = 0xFF;

		colors[i] = c;
	}

	// Using LodePNG to encode and write the png
	std::vector<unsigned char> image(kNumPixels * 4, 0);
	for (int i = 0; i < kNumPixels; i++)
		for (int c = 0; c < 4; c++)
			image[i*4 + c] = colors[UnscramblePalette(pixels[i])*4 + c];

	unsigned error = lodepng::encode(filename + ".png", image, kImageWidth, kImageHeight);
	if (error) std::cout << "Encoder error " << error << ": " 
		<< lodepng_error_text(error) << std::endl;
}