# Wwise *.BNK File Extractor

This is a C++ rewrite and extension of **bnkextr** originally written by CTPAX-X in Delphi.
It extracts `WEM` files from the ever more popular Wwise `BNK` format and from Wwise `PCK` file packages (AKPK).

Use [ww2ogg](https://github.com/hcs64/ww2ogg) to convert `WEM` files to the `OGG` format.

## Usage

```
Usage: bnkextr filename.bnk|filename.pck [/swap] [/nodir] [/obj]
        /swap - swap byte order (use it for unpacking 'Army of Two')
        /nodir - create no additional directory for the extracted files
        /obj - generate an objects.txt (BNK) or files.txt (PCK) file with the extracted data
```

The format is detected by the file signature, not by the file extension.

## BNK Format

- See the [original Delphi code](bnkextr.dpr) for the initial file specification
- See the [XeNTaX wiki](https://web.archive.org/web/20230817173759/http://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)) for a more complete file specification
- See the [bnk.bt](bnk.bt) file for [010 Editor](https://www.sweetscape.com/010editor/) specification
- See the [bnk.ksy](https://github.com/WolvenKit/wwise-audio-tools/blob/master/ksy/bnk.ksy) file by ADawesomeguy for a [Kaitai Struct](https://kaitai.io/) specification

## PCK Format

- See the [pck.bt](pck.bt) file for [010 Editor](https://www.sweetscape.com/010editor/) specification
- Streamed and external files are extracted as `<id>.wem`
- Embedded sound banks are extracted as `<id>.bnk` and then unpacked into a directory of the same name
- If a package holds more than one language, every language gets its own directory
- Encrypted packages are not supported

### Supported HIRC Events

- Generic event type logging
- Event
- EventAction

## Build

### CMake

```
cmake -S . -B build/
cmake --build build/ --target install
```

### GCC

```
g++ bnkextr.cpp -std=c++17 -static -O2 -s -o bnkextr.exe
```

## License

- `bnkextr.dpr` falls under the original copryright holders rights and is solely kept for archival purpose
- `bnkextr.cpp` is available under 2 licenses: Public Domain or MIT -- choose whichever you prefer