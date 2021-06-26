#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <cstdint>
#include <map>

struct Index;
struct Section;

#pragma pack(push, 1)
struct Index
{
    std::uint32_t id;
    std::uint32_t offset;
    std::uint32_t size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Section
{
    char sign[4];
    std::uint32_t size;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BankHeader
{
    std::uint32_t version;
    std::uint32_t id;
};
#pragma pack(pop)

enum class ObjectType : char
{
    SoundEffectOrVoice = 2,
    EventAction = 3,
    Event = 4,
    RandomOrSequenceContainer = 5,
    SwitchContainer = 6,
    ActorMixer = 7,
    AudioBus = 8,
    BlendContainer = 9,
    MusicSegment = 10,
    MusicTrack = 11,
    MusicSwitchContainer = 12,
    MusicPlaylistContainer = 13,
    Attenuation = 14,
    DialogueEvent = 15,
    MotionBus = 16,
    MotionFx = 17,
    Effect = 18,
    Unknown = 19,
    AuxiliaryBus = 20
};

#pragma pack(push, 1)
struct Object
{
    ObjectType type;
    std::uint32_t size;
    std::uint32_t id;
};
#pragma pack(pop)

struct EventObject
{
    std::uint32_t length;
    std::vector<std::uint32_t> action_ids;
};

int Swap32(const uint32_t dword)
{
#ifdef __GNUC__
	return __builtin_bswap32(dw);
#elif _MSC_VER
    return _byteswap_ulong(dword);
#endif
}

template<typename T>
bool ReadContent(std::fstream& file, T& structure)
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

bool Compare(char* char_string, const std::string& string)
{
    return std::strncmp(char_string, string.c_str(), string.length()) == 0;
}

bool HasArgument(char* arguments[], const int argument_count, const std::string& argument)
{
    for(auto i = 0U; i < static_cast<std::size_t>(argument_count); ++i)
    {
        if (Compare(arguments[i], argument))
        {
            return true;
        }
    }

    return false;
}

int main(int argument_count, char* arguments[])
{
    std::cout << "Wwise *.BNK File Extractor\n";
    std::cout << "(c) RAWR 2015-2021 - https://rawr4firefall.com\n\n";

    // Has no argument(s)
    if (argument_count < 2)
    {
        std::cout << "Usage: bnkextr filename.bnk [/swap] [/nodir] [/obj]\n";
        std::cout << "\t/swap - swap byte order (use it for unpacking 'Army of Two')\n";
        std::cout << "\t/nodir - create no additional directory for the *.wem files\n";
        std::cout << "\t/obj - generate an objects.txt file with the extracted object data\n";
        return EXIT_SUCCESS;
    }

    auto bnk_filename = std::filesystem::path{ std::string{ arguments[1] } };
    auto swap_byte_order = HasArgument(arguments, argument_count, "/swap");
    auto no_directory = HasArgument(arguments, argument_count, "/nodir");
    auto dump_objects = HasArgument(arguments, argument_count, "/obj");

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
    auto bank_header = BankHeader{};
    auto objects = std::vector<Object>{};
    auto event_objects = std::map<std::uint32_t, EventObject>{};

    while (ReadContent(bnk_file, content_section))
    {
        const std::size_t section_pos = bnk_file.tellg();

        if (swap_byte_order)
        {
            content_section.size = Swap32(content_section.size);
        }

        if (Compare(content_section.sign, "BKHD"))
        {
            ReadContent(bnk_file, bank_header);
            bnk_file.seekg(content_section.size - sizeof(BankHeader), std::ios_base::cur);

            std::cout << "Wwise Bank Version: " << bank_header.version << "\n";
            std::cout << "Bank ID: " << bank_header.id << "\n";
        }
        else if (Compare(content_section.sign, "DIDX"))
        {
            // Read file indices
            for (auto i = 0U; i < content_section.size; i += sizeof(content_index))
            {
                ReadContent(bnk_file, content_index);
                files.push_back(content_index);
            }
        }
        else if (Compare(content_section.sign, "STID"))
        {
            // To be implemented
        }
        else if (Compare(content_section.sign, "DATA"))
        {
            data_offset = bnk_file.tellg();
        }
        else if (Compare(content_section.sign, "HIRC"))
        {
            auto object_count = std::uint32_t{ 0 };
            ReadContent(bnk_file, object_count);

            for (auto i = 0U; i < object_count; ++i)
            {
                auto object = Object{};
                ReadContent(bnk_file, object);

                if (object.type == ObjectType::Event)
                {
                    auto event = EventObject{};

                    if (bank_header.version >= 134)
                    {
                        auto count = unsigned char{ 0 };
                        ReadContent(bnk_file, count);
                        event.length = static_cast<std::size_t>(count);
                    }
                    else
                    {
                        auto count = std::uint32_t{ 0 };
                        ReadContent(bnk_file, count);
                        event.length = count;
                    }

                    for (auto j = 0U; j < event.length; ++j)
                    {
                        auto action_id = std::uint32_t{ 0 };
                        ReadContent(bnk_file, action_id);
                        event.action_ids.push_back(action_id);
                    }

                    event_objects[object.id] = event;
                }

                bnk_file.seekg(object.size - sizeof(std::uint32_t), std::ios_base::cur);
                objects.push_back(object);
            }
        }

        // Seek to the end of the section
        bnk_file.seekg(section_pos + content_section.size);
    }

    // Reset EOF
    bnk_file.clear();

    auto output_directory = bnk_filename.parent_path();

    if (!no_directory)
    {
        output_directory = CreateOutputDirectory(bnk_filename);
    }

    // Dump objects information
    if (dump_objects)
    {
        auto object_filename = output_directory;
        object_filename = object_filename.append("objects.txt");
        auto object_file = std::fstream{ object_filename, std::ios::out | std::ios::binary };

        if (!object_file.is_open())
        {
            std::cout << "Unable to write objects file '" << object_filename.string() << "'\n";
            return -1;
        }

        for (auto& object : objects)
        {
            object_file << "Object ID: " << object.id << "\n";


            switch (object.type)
            {
            case ObjectType::Event:
                object_file << "\tType: Event\n";
                object_file << "\tNumber of Actions: " << event_objects[object.id].length << "\n";

                for (auto& action_id : event_objects[object.id].action_ids)
                {
                    object_file << "\tAction ID: " << action_id << "\n";
                }
                break;
            default:
                object_file << "\tType: " << static_cast<int>(object.type) << "\n";
            }
        }

        std::cout << "Objects file was written to: " << object_filename.string() << "\n";
    }

    // Extract WEM files
    if (data_offset == 0U || files.empty())
    {
        std::cout << "No WEM files discovered to be extracted\n";
        return EXIT_SUCCESS;
    }

    std::cout << "Found " << files.size() << " WEM files\n";
    std::cout << "Start extracting...\n";

    for (auto& [id, offset, size] : files)
    {
        auto wem_filename = output_directory;
        wem_filename = wem_filename.append(std::to_string(id)).replace_extension(".wem");
        auto wem_file = std::fstream{ wem_filename, std::ios::out | std::ios::binary };

        if (swap_byte_order)
        {
            size = Swap32(size);
            offset = Swap32(offset);
        }

        if (!wem_file.is_open())
        {
            std::cout << "Unable to write file '" << wem_filename.string() << "'\n";
            continue;
        }

        auto data = std::vector<char>(size, 0U);

        bnk_file.seekg(data_offset + offset);
        bnk_file.read(data.data(), size);
        wem_file.write(data.data(), size);
    }

    std::cout << "Files were extracted to: " << output_directory.string() << "\n";
}
