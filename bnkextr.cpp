#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <cstdint>

struct Index;
struct Section;

#pragma pack(push, 1)
struct Index
{
    std::uint32_t id;
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

int swap32(const uint32_t dw)
{
#ifdef __GNUC__
	return __builtin_bswap32(dw);
#elif _MSC_VER
    return _byteswap_ulong(dw);
#endif
}

int main(int argument_count, char* arguments[])
{
    std::cout << "Wwise *.BNK File Extractor\n";
    std::cout << "(c) RAWR 2015-2021 - https://rawr4firefall.com\n\n";

    // Has no argument(s)
    if (argument_count < 2)
    {
        std::cout << "Usage: bnkextr filename.bnk [/swap] [/nodir]\n";
        std::cout << "\t/swap - swap byte order (use it for unpacking 'Army of Two')\n";
        std::cout << "\t/nodir - create no additional directory for the *.wem files\n";
        return EXIT_SUCCESS;
    }

    auto bnk_filename = std::filesystem::path{ std::string{ arguments[1] } };
    auto swap_byte_order = (argument_count == 3 && std::strncmp(arguments[2], "/swap", 5U) == 0)
        || (argument_count == 4 && std::strncmp(arguments[3], "/swap", 5U) == 0);
    auto no_directory = (argument_count == 3 && std::strncmp(arguments[2], "/nodir", 5U) == 0)
        || (argument_count == 4 && std::strncmp(arguments[3], "/nodir", 5U) == 0);

    auto bnk_file = std::fstream{ bnk_filename, std::ios::binary | std::ios::in };

    // Could not open BNK file
    if (!bnk_file.is_open())
    {
        std::cout << "Can't open input file: " << bnk_filename << "\n";
        return EXIT_FAILURE;
    }

    auto data_offset = std::size_t{ 0U };
    auto files = std::vector<Index>{};
    auto content_section = Section{};
    auto content_index = Index{};

    while (bnk_file.read(reinterpret_cast<char*>(&content_section), sizeof(content_section)))
    {
        std::size_t section_pos = bnk_file.tellg();

        if (swap_byte_order)
        {
            content_section.size = swap32(content_section.size);
        }

        if (std::strncmp(content_section.sign, "DIDX", 4U) == 0)
        {
            // Read file indices
            for (auto i = 0U; i < content_section.size; i += sizeof(content_index))
            {
                bnk_file.read(reinterpret_cast<char*>(&content_index), sizeof(content_index));
                files.push_back(content_index);
            }
        }
        else if (std::strncmp(content_section.sign, "STID", 4U) == 0)
        {
            // To be implemented
        }
        else if (std::strncmp(content_section.sign, "DATA", 4U) == 0)
        {
            data_offset = bnk_file.tellg();
        }

        // Seek to the end of the section
        bnk_file.seekg(section_pos + content_section.size);
    }

    // Reset EOF
    bnk_file.clear();

    if (data_offset == 0U || files.empty())
    {
        std::cout << "No WEM files discovered to be extracted\n";
        return EXIT_SUCCESS;
    }

    std::cout << "Found " << files.size() << " WEM files\n";
    std::cout << "Start extracting...\n";

    auto wem_directory = bnk_filename.parent_path();

    if (!no_directory)
    {
        auto directory_name = bnk_filename.filename().replace_extension("");
        auto directory = bnk_filename.replace_filename(directory_name);
        create_directory(directory);
        wem_directory = directory;
    }

    for (auto& file : files)
    {
        auto wem_filename = wem_directory;
        wem_filename = wem_filename.append(std::to_string(file.id)).replace_extension(".wem");
        auto wem_file = std::fstream{ wem_filename, std::ios::out | std::ios::binary };

        if (swap_byte_order)
        {
            file.size = swap32(file.size);
            file.offset = swap32(file.offset);
        }

        if (wem_file.is_open())
        {
            auto data = std::vector<char>(file.size, 0U);

            bnk_file.seekg(data_offset + file.offset);
            bnk_file.read(static_cast<char*>(data.data()), file.size);
            wem_file.write(static_cast<char*>(data.data()), file.size);
        }
    }

    std::cout << "Files were extracted to: " << wem_directory.string() << "\n";
}