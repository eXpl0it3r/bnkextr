# Wwise *.BNK File Extractor

This is a C++ rewrite and extension of **bnkextr** originally written by CTPAX-X in Delphi.
It extracts `WEM` files from the ever more popular Wwise `BNK` format.

Use [ww2ogg](https://github.com/hcs64/ww2ogg) to convert `WEM` files to the `OGG` format.

## Usage

```
Usage: bnkextr filename.bnk [/swap] [/nodir] [/obj]
        /swap - swap byte order (use it for unpacking 'Army of Two')
        /nodir - create no additional directory for the *.wem files
        /obj - generate an objects.txt file with the extracted object data
```

## BNK Format

- See the [original Delphi code](bnkextr.dpr) for the initial file specification
- See the [XeNTaX wiki](https://wiki.xentax.com/index.php/Wwise_SoundBank_(*.bnk)) for a more complete file specification
- See the [bnk.bt](bnk.bt) file for [010 Editor](https://www.sweetscape.com/010editor/) specification
- See the [bnk.ksy](bnk.ksy) file for [Kaitai Struct](https://kaitai.io/) specification (slightly incomplete)

### Supported HIRC Events

- Generic event type logging
- Event
- EventAction

## Build

### CMake

```
cmake .
cmake --build .
```

### GCC

```
g++ bnkextr.cpp -std=c++17 -static -O2 -s -o bnkextr.exe
```

## License

- `bnkextr.dpr` falls under the original copryright holders rights and is solely kept for archival purpose
- `bnkextr.cpp` is available under 2 licenses: Public Domain or MIT -- choose whichever you prefer