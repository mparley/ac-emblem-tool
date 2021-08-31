# Armored Core (PS2) Emblem Tool

Tool for extracting and injecting PNGs to/from Armored Core PS2 emblem saves.

While replaying through the games, I thought it would be cool to use an image from
my PC as an emblem. After some searching, I found a few tools that I unfortunately
couldn't get to work.

[Armoured Core 2 Emblem Tool](https://www.ps2savetools.com/download/armoured-core-2-emblem-tool/)
that requires an older version X-Port/Sharkport save format that none of the 
ps2 save converters I could find exported to.

[Armored Core 3 Emblem Extractor/Injector](https://www.vg-resource.com/thread-23051.html)
which for garbled the extracted image and would throw errors when I tried injecting.

So I ended up slapping this tool together referencing the AC3 tool's source and
spending a day or so staring at hex data to figure out the format. I also leveraged
[LodePNG](https://lodev.org/lodepng/) to handle encoding/decoding PNGs.

So far I've tested and confirmed it's working with the following games on PS2:

    Armored Core 2 (US)
    Armored Core 2: Another Age (US)
    Armored Core 3 (US)
    Silent Line: Armored Core (US)
    Armored Core 3: Silent Line (JP)

Haven't gotten around to the others yet but they will probably work too.

### Usage

This tool should work with any save format that doesn't mess with the raw emblem
data. So far I've tested a few like .psu and .sps/.xpo and they seem to work with
common save tools and an emulator but I haven't tried on the actual hardware.

My personal favorite though is to just use the "convert" option in pcsx2's memcard
menu to convert a memory card to a folder structure and access the emblem save file
directly. The folder and save file will both be named something like *BASLUS-20435E00*
where *BASLUS-20435* is the game code (AC3) and *E00* means the first emblem save.

**The image must be a 128 x 128 png file with a maximum of 255 colors.** 
You can have transparency but anything not fully opaque will be made fully
transparent.

Otherwise, using the tool is pretty simple. 

1. Just clone/download this repo and compile it. Or for windows grab the binary I
included on the releases page. 

        mkdir build
        cmake -S . -B build/

2. Copy the emblem save and png to the same directory as the acet tool.

3. To extract an image to ``output.png`` run the command and pass in the save file as 
    the only argument.

        ./acet SAVEFILE

    To inject an image pass the save and the image as two arguments (order doesn't
    matter). This will write out the new file into ``SAVEFILE-modified``.

        ./acet SAVEFILE image.png

    **Alternatively you can just drag and drop files onto the tool**: a single file 
    to extract or both files to inject.

4. Take the ``SAVEFILE-modified`` and transfer it back into your memory card and
    load up the emblem save in-game.