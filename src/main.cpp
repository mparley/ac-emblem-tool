#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "lodepng.h"

using namespace std;

const int kSaveSize = 0x4440;
const int kImageOffset = 0x24; // The actual image data starts at offset 0x24
const int kImageWidth = 128;
const int kImageHeight = 128;
const int kNumPixels = 0x4000; // 128*128 pixels
const int kPaletteSize = 0x400; // 256 colors * 4 channels

void ReadSave(string filename, uint8_t (&buffer)[kSaveSize]) {

	ifstream infile(filename, ios::binary);
	if (!infile) {
		cerr << "Error - can't open file " << filename << "\n";
		return;
	}

	// Reads in the data while skipping the checksum bytes
	for (int i = 0; i < kSaveSize && infile.read((char*)&buffer[i], 1); i++)
		if ( ((infile.tellg() + 1) % kPaletteSize) == 0 )
			infile.seekg(1, ios_base::cur);

	infile.close();
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

void InjectImage(string imagename, string savename, uint8_t (&buffer)[kSaveSize]) {

	// Load png
	vector<unsigned char> image;
	unsigned w = kImageWidth, h = kImageHeight;
	unsigned error = lodepng::decode(image, w, h, imagename);

	if (error) {
		cout << "Decoder error " << error << ": " << lodepng_error_text(error) 
			<< endl;
		return;
	}

	unordered_map<uint32_t, int> palette_map = { {PackColor(0,0,0,0), 0} };

	for (int i = 0; i < image.size() / 4; i++) {

		// Anything not fully opaque will be made completely transparent
		uint32_t c = 0;
		if (image[i*4+3] == 0xFF) {
			image[i*4+3] = 0x80;
			c = PackColor(image[i*4], image[i*4+1], image[i*4+2], image[i*4+3]);
		}

		// Adding color to hash map
		if (palette_map.find(c) == palette_map.end()) {
			if (palette_map.size() > 256) {
				cout << "Your PNG has more than 255 colors + alpha color" << endl;
				return;
			}
			palette_map[c] = palette_map.size();

			int color_index = kImageOffset + kNumPixels + (palette_map[c] * 4);
			for (int channel = 0; channel < 4; channel++) 
				buffer[color_index + channel] = image[i*4+channel];
		}

		buffer[kImageOffset + i] = UnscramblePalette(palette_map[c]);
	}

	uint8_t sum = 0;
	ofstream outfile(savename + "-modified", ios::binary|ios::trunc);
	const int kStrippedSize = kImageOffset + kNumPixels + kPaletteSize;

	// Write the savefile and apply the checksums, i tracks the data index
	// j tracks the output index
	for (int i = 0, j = 0; i < kStrippedSize; i++, j++) {

		if (j % 0x400 == 0x3FF) {
			sum = ~(sum + 0xFF) + 1;

			// The first checksum needs 104 added to it for whatever reason
			if (j == 0x3FF) sum += 0x68; 

			outfile.write((char*)&sum, 1);
			sum = 0;
			j++;
		}

		outfile.write((char*)&buffer[i], 1);
		sum += buffer[i];
	}

	// The final checksum needs 202 added to it
	sum = ~(sum + 0xFF) + 1 + 0xCA;
	outfile.write((char*)&sum, 1);
	outfile.write((char*)&buffer[kStrippedSize+1], 10);
	outfile.close();
}

// Extracts an image from the buffer and writes it to file
void WriteImage(uint8_t (&buffer)[kSaveSize], string filename = "output") {
	uint8_t pixels[kNumPixels]; 
	uint8_t colors[kPaletteSize]; 

	for (int i = 0; i < kNumPixels; i++)
		pixels[i] = buffer[kImageOffset + i];

	for (int i = 0; i < kPaletteSize; i++) {
		uint8_t c = buffer[kImageOffset + kNumPixels + i];
		if (i % 4 == 3 && c != 0)
			c = 0xFF;

		colors[i] = c;
	}

	// Using LodePNG to encode and write the png
	vector<unsigned char> image(kNumPixels * 4, 0);
	for (int i = 0; i < kNumPixels; i++)
		for (int c = 0; c < 4; c++)
			image[i*4 + c] = colors[UnscramblePalette(pixels[i])*4 + c];

	unsigned error = lodepng::encode(filename + ".png", image, kImageWidth, kImageHeight);
	if (error) cout << "Encoder error " << error << ": " 
		<< lodepng_error_text(error) << endl;
}

int main(int argc, char** argv) {

	uint8_t buffer[kSaveSize] = {0};

	if (argc == 2) {
		ReadSave(argv[1], buffer);
		WriteImage(buffer);
	} else if (argc == 3) {
		ReadSave(argv[1], buffer);
		InjectImage(argv[2], argv[1], buffer);
	} else {
		cout << "AC Emblem Tool\n\n";
		cout << "PNG extraction:\n\tacee savefile\n";
		cout << "PNG injection:\n\tacee savefile yourimage.png\n\n";
	}

	return EXIT_SUCCESS;
}
