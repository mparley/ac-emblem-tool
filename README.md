# Armored Core (PS2) Emblem Tool

Tool for extracting and injecting PNGs to/from Armored Core PS2 emblem saves.

I was replaying through the games and thought it would be cool to make an emblem
on my PC and and transfer it to my game. After some searching I found two 
options:

[Armoured Core 2 Emblem Tool](https://www.ps2savetools.com/download/armoured-core-2-emblem-tool/)
which apparently requires .xpo or .spo save files from an old ps2 usb transfer
device called the SharkPort or X-Port depending on where you lived. I couldn't
get this working since none of the ps2 save converters I could find converted to
these formats. No knock on the converters, they could port to SharkPort/XPort v2
formats just fine but this tool didn't work with those newer formats.

[Armored Core 3 Emblem Extractor/Injector](https://www.vg-resource.com/thread-23051.html)
made by the user Chase on the vg-resource forums. For whatever reason when I tried extracting the output was garbled and I couldn't get injecting to work.

So I ended up slapping this tool together by using the source kindly included
with the AC3 extractor/injector as reference and spending a day or so staring at
hex data. I also leveraged the wonderful 
[LodePNG](https://github.com/lvandeve/lodepng)
to handle encoding/decoding PNGs.

So far I've tested and confirmed it's working with the following games on PS2:

* Armored Core 2 (US)
* Armored Core 2: Another Age (US)
* Armored Core 3 (US)

### Usage

**Currently this tool only works with the actual binary emblem data.** It
doesn't work with .psu, .max, or .mcd files - only with the actual emblem save.
For example an Armored Core 3 (US) emblem saved in the first slot will be called
*BASLUS-20435E00* and it would be sitting next to the *AC3E.ICO* and *icon.sys*
files on the memory card.

There are tools out there like 
[PS2 Save Builder](https://www.ps2savetools.com/download/ps2-save-builder/)
that can extract these files from various save formats but if you are playing 
on pcsx2, it is trivial to access these files by converting the pcsx2 memory 
card file to a folder structure with the "Convert" option in the memcard menu.

Also the image **must be a 128 x 128 png file with a maximum of 255 colors.** 
You can have transparency but anything not fully opaque will be made fully
transparent.

Otherwise, using the tool is pretty simple. 

1. Just clone/download this repo and compile it. For windows grab the binary I
included on the releases page. 

2. Copy the emblem file and png to the directory.

3. To extract run the command and pass in the emblem save as the argument:

        ./acet [EMBLEM]

    For windows (powershell):

        .\acet.exe [EMBLEM]

    The tool will extract the emblem to ``output.png``.

4. To inject an image pass the save and the image as the arguments:

        acet [EMBLEM] [image.png]

    For windows:

        .\acet.exe [EMBLEM] [image.png]

    The tool will write out a new file called ``[EMBLEM]-modified``. You can 
    then take that modified file and replace the original.
