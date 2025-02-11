# Armored Core (PS2) Emblem Tool GUI

Tool for extracting and injecting PNGs to/from Armored Core PS2 emblem saves.

It mostly works for me but its kinda half-cooked spaghetti right now so YMMV

## Usage

1. **Download a binary from the releases page.** I'll try and put 64 bit windows
static builds there when I update things. Even though wxWidgets supports Mac and Linux,
I don't have a Mac machine, and I'm kinda dumb and am having trouble with the 
linux version right now - you can try building it yourself for the time being (see below).
Also try to extract the program to a folder that your OS will let it write files to
(ie not Program Files) so it can make backups of saves.

2. **Get the RAW emblem save on your computer somehow.** If you play with actual hardware
there are ways to transfer memory cards to PC, you'll have to use something like
[mymc](http://www.csclub.uwaterloo.ca:11068/mymc/) 
to convert/extract the raw save. If you emulate with PCSX2 the simplest way
just use the *"convert"* option in pcsx2's memcard menu to convert the memory card
to a folder on your hard drive where you can go in and find the emblem. 
The folder and save file will both be named something like *BASLUS-20435E00*
where *BASLUS-20435* is the game code (AC3) and *E00* means the first emblem save. 

    NOTE: Last Raven has a different save structure. LR emblem saves are stored in a single 
    folder named *BASLUS-21338EMB* (for the US version) and
    each emblem is a file in that directory named *dataX* where *X* is a number 0-7. 

3. **Use the tool with your save.**

    **For the GUI tool:**
    * You can drag and drop, press the Browse button, or *File > Open Save...* to load
    a save, the emblem should display (nearly) instantly in the window.
    * You can then
    extract that image with *File > Extract Image As...* .
    * You can drag and drop or 
    *File > Inject Image...* to inject an image into the save and the emblem in the
    window should change (nearly) instantly. 
    * When injecting the tool makes a backup of
    the save in the folder *acet-backup* in the application's directory and you should get
    warning messages if things go too wrong.
    * Multiple basic image formats are supported
    and will get quanitzed and resized as needed.

    **For the CLI/Coke Classic™ tool:**
    * Pass in an emblem save by itself to extract a PNG
    * Pass in both an emblem save and a PNG to inject the PNG into the save.
    * You can try dragging and dropping onto the exe in windows to pass in the files:
    one save to extract and both the save and PNG to inject.
    * **The image must be a 128 x 128 PNG file with a maximum of 255 colors.** 
    You can have transparency but anything not fully opaque will be made fully
    transparent. It *"should"* work with these limits since it counts colors
    but I've ran into problems using some online converters. The safest bet is 
    probably to just use something like ImageMagick to convert to PNG8 beforehand.

4. **Reverse what you did in step 2 to get the save back into the memory card** and hopefully
it loads in-game and everything is groovy. If not you have the backups the tool
made and at the very least its just the emblem save and not your game save if worst
case happens and the file is ruined.

## Building

First install wxWidgets and dependencies and then clone/download this repo and build
it with cmake. 

    mkdir build
    cmake -S . -B build/
    cmake --build build

Cmake *will* try to download and build wxWidgets if it doesn't find it on your system
but that will make building take a long time and it might not even work if you are
missing some dependencies. So it's probably better to install from your package manager
or something. I'm pretty bad with cmake and I know there is some shit I still need to sort out
in that area, sorry in advance.

## Shout Outs

[Armored Core 3 Emblem Extractor/Injector](https://www.vg-resource.com/thread-23051.html)
I never got this Armored Core 3 tool to work for me but looking at it's code helped
me fix a couple things I had wrong when I was first making the cli tool.

The cli/classic version uses [LodePNG](https://lodev.org/lodepng/) to handle PNG files.
Very useful and easy for idiots like me. It's licence is included in the code but
you can also peep it [here](https://github.com/lvandeve/lodepng/blob/master/LICENSE).

The gui version of this tool uses [wxWidgets](https://wxwidgets.org/). Allows acet-gui
to be cross platform (providing I didn't screw something up). wxWidgets has their
own [licence](https://github.com/wxWidgets/wxWidgets/blob/master/docs/licence.txt).

