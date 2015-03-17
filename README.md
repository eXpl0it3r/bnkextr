Wwise *.BNK File Extractor
==========================

This is a C++ rewrite of **bnkextr** originally written by CTPAX-X in Delphi.
It extracts `WEM` files from the ever more popular Wwise `BNK` format.

Use [ww2ogg](https://github.com/hcs64/ww2ogg) to convert `WEM` files to the `OGG` format.

Usage
-----

```
bnkextr filename.bnk [/swap]
    /swap - swap byte order (use it for unpacking 'Army of Two')
```

Build
-----

```
g++ bnkextr.cpp -std=c++11 -static -O2 -s -o bnkextr.exe
```
