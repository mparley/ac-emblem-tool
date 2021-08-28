#include "acet.h"

using namespace std;

const uint8_t kPngSig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

bool isPNG(string filename) {
    ifstream infile(filename, ios::binary);
	if (!infile) {
		cerr << "Error - can't open file " << filename << "\n";
		return false;
    }

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

int main(int argc, char** argv) {
	if (argc == 2) {
		WriteImage(argv[1]);
	} else if (argc == 3) {
		if (isPNG(argv[1]))
			InjectImage(argv[2], argv[1]);
		else
			InjectImage(argv[1], argv[2]);
	} else {
		cout << "AC Emblem Tool\n\n";
		cout << "PNG extraction:\n\tacee savefile\n";
		cout << "PNG injection:\n\tacee savefile yourimage.png\n\n";
	}

	return EXIT_SUCCESS;
}
