/*
http://www.geocities.jp/aoyoume/aotuv/index.html
http://rarewares.org/ogg-oggenc.php#oggenc-aotuv
http://www.eveonline.com/ingameboard.asp?a=topic&threadID=1018956
http://forum.xentax.com/viewtopic.php?f=17&t=3477
http://wiki.xentax.com/index.php?title=Wwise_SoundBank_(*.bnk)

.BNK Format specifications

char {4} - header (BKHD) // BanK HeaDer
uint32 {4} - size of BKHD
uint32 {4} - unknown (version?)
uint32 {4} - unknown
uint32 {4} - unknown
uint32 {4} - unknown
byte {x} - zero padding (if any)

char {4} - header (DIDX) // Data InDeX
uint32 {4} - size of DIDX
following by records 12 bytes each:
	uint32 {4} - unknown
	uint32 {4} - relative file offset from start of DATA, 16 bytes aligned
	uint32 {4} - file size

char {4} - header (DATA)
uint32 {4} - size of DATA

char {4} - header (HIRC) // ???
uint32 {4} - size of HIRC

char {4} - header (STID) // Sound Type ID
uint32 {4} - size of STID
uint32 {4} - Always 1?
uint32 {4} - Always 1?
uint32 {4} - unknown
byte {1} - TID Length (TL)
char {TL} - TID string (usually same as filename, but without extension)

Init.bnk
STMG
HIRC
FXPR
ENVS
*/

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

struct Index;
struct Section;

#pragma pack(push, 1)
struct Index
{
	std::uint32_t unknown;
	std::uint32_t offset;
	std::uint32_t size;
};

#pragma pack(push, 1)
struct Section
{
	char sign[4];
	std::uint32_t size;
};
#pragma pack(pop)
#pragma pack(pop)

int swap32(const int dw)
{
#ifdef __GNUC__
	return __builtin_bswap32(dw);
#elif _MSC_VER
	return _byteswap_ulong(dw);
#endif
}

std::string zero_padding(unsigned int number)
{
	if(number < 10)
		return "000" + std::to_string(number);
	else if(number < 100)
		return "00" + std::to_string(number);
	else if(number < 1000)
		return "0" + std::to_string(number);
	else
		return std::to_string(number);
}

int main(int argc, char* argv[])
{
	std::cout << "Wwise *.BNK File Extractor" << std::endl;
	std::cout << "(c) CTPAX-X Team 2009-2010 - http://www.CTPAX-X.org" << std::endl;
	std::cout << "(c) RAWR 2015 - http://www.rawr4firefall.com" << std::endl;
	std::cout << std::endl;

	// Has no argument(s)
	if((argc < 2) || (argc > 3))
	{
		std::cout << "Usage: bnkextr filename.bnk [/swap]" << std::endl;
		std::cout << "/swap - swap byte order (use it for unpacking 'Army of Two')" << std::endl;
		return 0;
	}

	std::fstream bnkfile;
	bnkfile.open(argv[1], std::ios::binary | std::ios::in);

	// Could not open BNK file
	if(!bnkfile.is_open())
	{
		std::cout << "Can't open input file: " << argv[1] << std::endl;
		return 0;
	}

	unsigned int data_pos = 0;
	std::vector<Index> files;
	Section content_section;
	Index content_index;

	while(bnkfile.read(reinterpret_cast<char*>(&content_section), sizeof(content_section)))
	{
		unsigned int section_pos = bnkfile.tellg();

		// Was the /swap command used?
		if(argc > 3)
			content_section.size = swap32(content_section.size);

		if(std::strncmp(content_section.sign, "DIDX", 4) == 0)
		{
			// Read files
			for(unsigned int i = 0; i < content_section.size; i += sizeof(content_index))
			{
				bnkfile.read(reinterpret_cast<char*>(&content_index), sizeof(content_index));
				files.push_back(content_index);
			}
		}
		else if(std::strncmp(content_section.sign, "STID", 4) == 0)
		{
			// To be implemented
		}
		else if(std::strncmp(content_section.sign, "DATA", 4) == 0)
		{
			// Get DATA offset
			data_pos = bnkfile.tellg();
		}

		// Seek to the end of the section
		bnkfile.seekg(section_pos + content_section.size);
	}

	// Reset EOF
	bnkfile.clear();

	// Extract files
	if((data_pos > 0) && (files.size() > 0))
	{
		for(std::size_t i = 0; i < files.size(); ++i)
		{
			std::string filename = zero_padding(i + 1) + ".wem";

			std::fstream wemfile;
			wemfile.open(filename, std::ios::out | std::ios::binary);

			// Was the /swap command used?
			if(argc > 3)
			{
				files[i].size = swap32(files[i].size);
				files[i].offset = swap32(files[i].offset);
			}

			if(wemfile.is_open())
			{
				std::vector<char> data(files[i].size, 0);

				bnkfile.seekg(data_pos + files[i].offset);
				bnkfile.read(static_cast<char*>(data.data()), files[i].size);
				wemfile.write(static_cast<char*>(data.data()), files[i].size);
			}
		}
	}
}
