#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <lodepng.h>
#include <unordered_map>

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

void ModifySavePS1(string savename, string imagename, bool isMOA = false, int block = 0, int slot = 0) {
	vector<uint8_t> image;
	unsigned w = 64, h = 64;
	unsigned error = lodepng::decode(image, w, h, imagename);
	if (error) {
		cout << "Decoder error " << error << ": " << lodepng_error_text(error) << endl;
		ExitMessage("");
	}
  InjectImageAC1(savename, image, true, 1, 0);
}

void parseArgs(int argc, char** argv) {
  unordered_map<string, vector<string>> argmap;
  string save = "";
  string image = "";
  string extract = "";
  vector<int> ps1pos;
  bool backup = true;
  bool help = false;
  bool ps1 = false;
  bool moa = false;
  bool lr = false;

  string last = "";
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      last = argv[i];
      argmap[last] = vector<string>();
    } else {
      argmap[last].push_back(argv[i]);
    }
  }

  if (argc == 1) help = true;

  for (auto flag : argmap) {
    cout << flag.first << ": ";
    for (auto arg : flag.second) {
      cout << arg << " ";
    }
    cout << "\n";
  }
  cout << "\n";

  for (string s : argmap[""]) {
    if (isPNG(s)) {
      image = s;
    } else {
      if (isPS1Save(s)) ps1 = true;
      save = s;
    }
  }

  if (argmap.find("-e") != argmap.end()) {
    if (!argmap["-e"].empty()) {
      extract = argmap["-e"][0];
    }
  }

  if (argmap.find("-i") != argmap.end() && !argmap["-i"].empty())
    image = argmap["-i"][0];

  if (argmap.find("-nb") != argmap.end())
    backup = false;

  if (argmap.find("-h") != argmap.end())
    help = true;

  if (argmap.find("-ps1") != argmap.end()) {
    ps1 = true;
    for (auto s : argmap["-ps1"]) {
      try {
        ps1pos.push_back(stoi(s));
      } catch (...) {
        ExitMessage("parseArgs: Couldn't parse int in '-ps1' args");
      }
    }
  }

  if (argmap.find("-moa") != argmap.end()) {
    moa = true;
  }

  if (ps1 && ps1pos.size() != 2) {
    ExitMessage((string)"parseArgs: A ps1 save requires 2 ints passed into the '-ps1' flag:\n"
      + "1. The memory card block number 0-14 (pass 0 if it is a single save instead of a card)\n"
      + "NOTE: 0-14 since even though PS1 memory cards have 15 blocks, gen 1 emblem saves use 2 blocks\n"
      + "2. The emblem slot in the save 0-6. Emblem saves have 7 slots for emblems in the PS1 games\n");
  }

  if (argmap.find("-lr") != argmap.end()) {
    if (moa) {
      ExitMessage("parseArgs: Conflicting flags '-lr' and -moa");
    } else if (ps1) {
      ExitMessage("parseArgs: -lr flag used on a PS1 save");
    }
    lr = true;
  }

  if (help) {
    cout << "Temp help message\n";
    /*cout << "AC Emblem Tool\n";*/
    /*cout << "--------------\n\n";*/
    /*cout << "PNG extraction:\n\n\t" << argv[0] << " SAVEFILE\n\n";*/
    /*cout << "PNG injection:\n\n\t" << argv[0] << " SAVEFILE yourimage.png\n\n";*/
    /*cout << "Press enter key to continue...";*/
    /*cin.get();*/
  }

  if (save.empty()) {
    if (help) ExitMessage("");
    ExitMessage("No save file");
  }

  if (image.empty() && extract.empty() && argmap.find("-nde") == argmap.end()) {
    extract = save + ".png";
  }

  if (!extract.empty()) {
    cout << "EXTRACTING [" << extract << "] from [" << save << "]\n";
    if (ps1) {
      vector<uint8_t> image = ExtractImageAC1(argv[1], moa, ps1pos[0], ps1pos[1]);
      vector<uint8_t> png;
      unsigned error = lodepng::encode(png, image, 64, 64);
      if(error) {
        cout << "Encoder error " << error << ": " << lodepng_error_text(error) << endl;
        ExitMessage("");
      }
      lodepng::save_file(png, extract);
    } else {
      WriteImage(save);
    }
  }

  if (!image.empty()) {
    cout << "INJECTING [" << image << "] into [" << save << "]";
    if (ps1) {
      cout << " at PS1 MEM BLOCK " << ps1pos[0] << " and EMBLEM SLOT " << ps1pos[1];
      ModifySavePS1(save, image, moa, ps1pos[0], ps1pos[1]);
    } else {
      ModifySave(save, image);
    }
    cout << "\n";
  }
}

int main(int argc, char** argv) {
  /*cout << argc << "\n";*/
	try {
    parseArgs(argc, argv);
	} catch (exception &e) {
		ExitMessage(e.what());
	} catch (...) {
		ExitMessage("Something went wrong. I have no clue what.");
	}

	return EXIT_SUCCESS;
}
