#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <lodepng.h>

#include "acet.h"

using namespace std;

const uint8_t kPngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

void ExitMessage(std::string message) {
	std::cout << message
		<< "\nExiting\n" 
		<< "Press enter key to continue...\n";
	std::cin.get();
	exit(EXIT_FAILURE);
}

vector<uint8_t> ReadFile(string filename) {
	ifstream infile(filename, ios::binary|ios::ate);
	if (!infile)
		ExitMessage("Cannot open file " + filename);

	size_t save_size = infile.tellg();
	vector<uint8_t> buffer(save_size, 0);

	infile.seekg(0, infile.beg);
	infile.read((char*)buffer.data(), save_size);
	infile.close();

	return buffer;
}

bool isPNG(string filename) {
    ifstream infile(filename, ios::binary);
	if (!infile)
		ExitMessage("Cannot open file " + filename);

    unsigned char b = 0;
    bool ret = true;

    for (int i = 0; i < 8; i++) {
        infile.read((char*)&b, 1);
        if (b != kPngSig[i]) {
            ret = false;
            break;
        }
    }

    infile.close();
    return ret;
}

void WriteImage(string savename) {
	vector<uint8_t> save = ReadFile(savename);
	vector<uint8_t> image = ExtractImage(save);
	vector<uint8_t> png;

	unsigned error = lodepng::encode(png, image, kImageWidth, kImageHeight);

	if(error) {
		cout << "Encoder error " << error << ": " << lodepng_error_text(error) << endl;
		ExitMessage("");
	}

	lodepng::save_file(png, savename + ".png");
}

void ModifySave(string savename, string imagename) {
	vector<uint8_t> save = ReadFile(savename);
	vector<uint8_t> image;

	ofstream outfile(savename + ".backup", ios::binary);
	outfile.write((char*)save.data(), save.size());
	outfile.close();

	unsigned w = kImageWidth, h = kImageHeight;
	unsigned error = lodepng::decode(image, w, h, imagename);
	if (error) {
		cout << "Decoder error " << error << ": " << lodepng_error_text(error) << endl;
		ExitMessage("");
	}

	InjectImage(save, image);

	outfile.open(savename, ios::binary);
	outfile.write((char*)save.data(), save.size());
	outfile.close();
}

int main(int argc, char** argv) {
	try {
		if (argc == 2) {
			if (isPNG(argv[1]))
				ExitMessage("PNG detected without save file - you need to specify a save file");
			WriteImage(argv[1]);

		} else if (argc == 3) {
			bool b[2];
			b[0] = isPNG(argv[1]);
			b[1] = isPNG(argv[2]);

			if (b[0] && b[1])
				ExitMessage("Both arguments are PNGs - Need a savefile to inject to");
			else if (isPNG(argv[1]))
				ModifySave(argv[2], argv[1]);
			else if (isPNG(argv[2]))
				ModifySave(argv[1], argv[2]);
			else
				ExitMessage("Neither argument is a PNG - Need a PNG image to inject");

		} else {
			cout << "AC Emblem Tool\n";
			cout << "--------------\n\n";
			cout << "PNG extraction:\n\n\t" << argv[0] << " SAVEFILE\n\n";
			cout << "PNG injection:\n\n\t" << argv[0] << " SAVEFILE yourimage.png\n\n";
			cout << "Press enter key to continue...";
			cin.get();
		}
	} catch (exception &e) {
		ExitMessage(e.what());
	} catch (...) {
		ExitMessage("Something went wrong. I have no clue what.");
	}

	return EXIT_SUCCESS;
}
