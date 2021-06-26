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

int Swap32(const uint32_t dword)
{
#ifdef __GNUC__
	return __builtin_bswap32(dw);
#elif _MSC_VER
    return _byteswap_ulong(dword);
#endif
}

template<typename T>
bool ReadStructure(std::fstream& file, T& structure)
{
    return static_cast<bool>(file.read(reinterpret_cast<char*>(&structure), sizeof(structure)));
}

std::filesystem::path CreateOutputDirectory(std::filesystem::path bnk_filename)
{
    const auto directory_name = bnk_filename.filename().replace_extension("");
    auto directory = bnk_filename.replace_filename(directory_name);
    create_directory(directory);
    return directory;
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

    while (ReadStructure(bnk_file, content_section))
    {
        std::size_t section_pos = bnk_file.tellg();

        if (swap_byte_order)
        {
            content_section.size = Swap32(content_section.size);
        }

        if (std::strncmp(content_section.sign, "DIDX", 4U) == 0)
        {
            // Read file indices
            for (auto i = 0U; i < content_section.size; i += sizeof(content_index))
            {
                ReadStructure(bnk_file, content_index);
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
        wem_directory = CreateOutputDirectory(bnk_filename);
    }

    for (auto& [id, offset, size] : files)
    {
        auto wem_filename = wem_directory;
        wem_filename = wem_filename.append(std::to_string(id)).replace_extension(".wem");
        auto wem_file = std::fstream{ wem_filename, std::ios::out | std::ios::binary };

        if (swap_byte_order)
        {
            size = Swap32(size);
            offset = Swap32(offset);
        }

        if (wem_file.is_open())
        {
            auto data = std::vector<char>(size, 0U);

            bnk_file.seekg(data_offset + offset);
            bnk_file.read(data.data(), size);
            wem_file.write(data.data(), size);
        }
    }

    std::cout << "Files were extracted to: " << wem_directory.string() << "\n";
}
