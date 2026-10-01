#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <set>

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

enum class ObjectType : std::int8_t
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
    std::uint32_t action_count;
    std::vector<std::uint32_t> action_ids;
};

enum class EventActionScope : std::int8_t
{
    SwitchOrTrigger = 1,
    Global = 2,
    GameObject = 3,
    State = 4,
    All = 5,
    AllExcept = 6
};

enum class EventActionType : std::int8_t
{
    Stop = 1,
    Pause = 2,
    Resume = 3,
    Play = 4,
    Trigger = 5,
    Mute = 6,
    UnMute = 7,
    SetVoicePitch = 8,
    ResetVoicePitch = 9,
    SetVoiceVolume = 10,
    ResetVoiceVolume = 11,
    SetBusVolume = 12,
    ResetBusVolume = 13,
    SetVoiceLowPassFilter = 14,
    ResetVoiceLowPassFilter = 15,
    EnableState = 16,
    DisableState = 17,
    SetState = 18,
    SetGameParameter = 19,
    ResetGameParameter = 20,
    SetSwitch = 21,
    ToggleBypass = 22,
    ResetBypassEffect = 23,
    Break = 24,
    Seek = 25
};

enum class EventActionParameterType : std::int8_t
{
    Delay = 0x0E,
    Play = 0x0F,
    Probability = 0x10
};

struct EventActionObject
{
    EventActionScope scope;
    EventActionType action_type;
    std::uint32_t game_object_id;
    std::uint8_t parameter_count;
    std::vector<EventActionParameterType> parameters_types;
    std::vector<std::int8_t> parameters;
};

struct Options
{
    bool swap_byte_order;
    bool no_directory;
    bool dump_objects;
};

struct PackageEntry
{
    std::uint64_t id;
    std::uint32_t block_size;
    std::uint32_t size;
    std::uint32_t start_block;
    std::uint32_t language_id;
};

int Swap32(const uint32_t dword)
{
#ifdef __GNUC__
	return __builtin_bswap32(dword);
#elif _MSC_VER
    return _byteswap_ulong(dword);
#endif
}

template <typename T>
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

bool HasArgument(const std::vector<std::filesystem::path>& arguments, const std::filesystem::path& argument)
{
    return std::find(arguments.begin(), arguments.end(), argument) != arguments.end();
}

int ExtractBank(const std::filesystem::path& bnk_filename, std::fstream& bnk_file, const Options& options)
{
    const auto swap_byte_order = options.swap_byte_order;
    const auto no_directory = options.no_directory;
    const auto dump_objects = options.dump_objects;

    auto data_offset = std::size_t{ 0U };
    auto files = std::vector<Index>{};
    auto content_section = Section{};
    auto content_index = Index{};
    auto bank_header = BankHeader{};
    auto objects = std::vector<Object>{};
    auto event_objects = std::map<std::uint32_t, EventObject>{};
    auto event_action_objects = std::map<std::uint32_t, EventActionObject>{};

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
                        auto count = std::uint8_t{ 0 };
                        ReadContent(bnk_file, count);
                        event.action_count = static_cast<std::uint32_t>(count);
                    }
                    else
                    {
                        ReadContent(bnk_file, event.action_count);
                    }

                    for (auto j = 0U; j < event.action_count; ++j)
                    {
                        auto action_id = std::uint32_t{ 0 };
                        ReadContent(bnk_file, action_id);
                        event.action_ids.push_back(action_id);
                    }

                    event_objects[object.id] = event;
                }
                else if (object.type == ObjectType::EventAction)
                {
                    auto event_action = EventActionObject{};

                    ReadContent(bnk_file, event_action.scope);
                    ReadContent(bnk_file, event_action.action_type);
                    ReadContent(bnk_file, event_action.game_object_id);

                    bnk_file.seekg(1, std::ios_base::cur);

                    ReadContent(bnk_file, event_action.parameter_count);

                    for (auto j = 0U; j < static_cast<std::size_t>(event_action.parameter_count); ++j)
                    {
                        auto parameter_type = EventActionParameterType{};
                        ReadContent(bnk_file, parameter_type);
                        event_action.parameters_types.push_back(parameter_type);
                    }

                    for (auto j = 0U; j < static_cast<std::size_t>(event_action.parameter_count); ++j)
                    {
                        auto parameter = std::int8_t{ 0 };
                        ReadContent(bnk_file, parameter);
                        event_action.parameters.push_back(parameter);
                    }

                    bnk_file.seekg(1, std::ios_base::cur);
                    bnk_file.seekg(object.size - 13 - event_action.parameter_count * 2, std::ios_base::cur);

                    event_action_objects[object.id] = event_action;
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
            std::cout << "Unable to write objects file '" << object_filename.u8string() << "'\n";
            return EXIT_FAILURE;
        }

        for (auto& [type, size, id] : objects)
        {
            object_file << "Object ID: " << id << "\n";

            switch (type)
            {
            case ObjectType::Event:
                object_file << "\tType: Event\n";
                object_file << "\tNumber of Actions: " << event_objects[id].action_count << "\n";

                for (auto& action_id : event_objects[id].action_ids)
                {
                    object_file << "\tAction ID: " << action_id << "\n";
                }
                break;
            case ObjectType::EventAction:
                object_file << "\tType: EventAction\n";
                object_file << "\tAction Scope: " << static_cast<int>(event_action_objects[id].scope) << "\n";
                object_file << "\tAction Type: " << static_cast<int>(event_action_objects[id].action_type) << "\n";
                object_file << "\tGame Object ID: " << static_cast<int>(event_action_objects[id].game_object_id) << "\n";
                object_file << "\tNumber of Parameters: " << static_cast<int>(event_action_objects[id].parameter_count) << "\n";

                for (auto j = 0; j < event_action_objects[id].parameter_count; ++j)
                {
                    object_file << "\t\tParameter Type: " << static_cast<int>(event_action_objects[id].parameters_types[j]) << "\n";
                    object_file << "\t\tParameter: " << static_cast<int>(event_action_objects[id].parameters[j]) << "\n";
                }
                break;
            default:
                object_file << "\tType: " << static_cast<int>(type) << "\n";
            }
        }

        std::cout << "Objects file was written to: " << object_filename.u8string() << "\n";
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
            std::cout << "Unable to write file '" << wem_filename.u8string() << "'\n";
            continue;
        }

        auto data = std::vector<char>(size, 0U);

        bnk_file.seekg(data_offset + offset);
        bnk_file.read(data.data(), size);
        wem_file.write(data.data(), size);
    }

    std::cout << "Files were extracted to: " << output_directory.u8string() << "\n";
    return EXIT_SUCCESS;
}

std::uint32_t ReadUInt32(std::fstream& file, const bool swap_byte_order)
{
    auto value = std::uint32_t{ 0 };
    ReadContent(file, value);
    return swap_byte_order ? static_cast<std::uint32_t>(Swap32(value)) : value;
}

std::map<std::uint32_t, std::string> ReadPackageLanguages(std::fstream& file, const std::uint32_t map_size, const bool swap_byte_order)
{
    auto languages = std::map<std::uint32_t, std::string>{};
    auto map = std::vector<char>(map_size, 0);
    file.read(map.data(), map_size);

    const auto read_value = [&](const std::size_t position)
    {
        auto value = std::uint32_t{ 0 };
        std::memcpy(&value, map.data() + position, sizeof(value));
        return swap_byte_order ? static_cast<std::uint32_t>(Swap32(value)) : value;
    };

    if (map_size < sizeof(std::uint32_t))
    {
        return languages;
    }

    const auto count = read_value(0);

    for (auto i = std::size_t{ 0 }; i < count && (i + 1) * 8 + 4 <= map.size(); ++i)
    {
        const auto name_offset = read_value(4 + i * 8);
        const auto id = read_value(8 + i * 8);

        // The names are plain ASCII, either stored as 8-bit or as UTF-16 characters
        const auto is_wide = name_offset + 1U < map.size() && (map[name_offset] == '\0' || map[name_offset + 1U] == '\0');
        const auto step = std::size_t{ is_wide ? 2U : 1U };
        auto position = std::size_t{ name_offset } + (is_wide && map[name_offset] == '\0' ? 1U : 0U);
        auto name = std::string{};

        for (; position < map.size() && map[position] != '\0'; position += step)
        {
            name += map[position];
        }

        languages[id] = name;
    }

    return languages;
}

bool ReadPackageEntries(std::fstream& file, const std::uint32_t table_size, const bool has_wide_ids, const bool swap_byte_order, std::vector<PackageEntry>& entries)
{
    const std::size_t table_end = static_cast<std::size_t>(file.tellg()) + table_size;
    const auto entry_size = std::uint64_t{ has_wide_ids ? 24U : 20U };

    if (table_size >= sizeof(std::uint32_t))
    {
        const auto count = ReadUInt32(file, swap_byte_order);

        if (count * entry_size > table_size - sizeof(std::uint32_t))
        {
            return false;
        }

        for (auto i = 0U; i < count; ++i)
        {
            auto entry = PackageEntry{};

            if (has_wide_ids)
            {
                const auto first = std::uint64_t{ ReadUInt32(file, swap_byte_order) };
                const auto second = std::uint64_t{ ReadUInt32(file, swap_byte_order) };
                entry.id = swap_byte_order ? (first << 32U | second) : (second << 32U | first);
            }
            else
            {
                entry.id = ReadUInt32(file, swap_byte_order);
            }

            entry.block_size = ReadUInt32(file, swap_byte_order);
            entry.size = ReadUInt32(file, swap_byte_order);
            entry.start_block = ReadUInt32(file, swap_byte_order);
            entry.language_id = ReadUInt32(file, swap_byte_order);
            entries.push_back(entry);
        }
    }

    file.seekg(table_end);
    return static_cast<bool>(file);
}

bool CopyContent(std::fstream& source, const std::uint64_t offset, std::uint64_t size, const std::filesystem::path& target_filename)
{
    auto target = std::fstream{ target_filename, std::ios::out | std::ios::binary };

    if (!target.is_open())
    {
        return false;
    }

    auto buffer = std::vector<char>(1024U * 1024U, 0);
    source.seekg(offset);

    while (size > 0U && source)
    {
        const auto chunk = static_cast<std::streamsize>(std::min<std::uint64_t>(size, buffer.size()));
        source.read(buffer.data(), chunk);
        target.write(buffer.data(), source.gcount());
        size -= static_cast<std::uint64_t>(chunk);
    }

    return size == 0U && source && target;
}

int ExtractPackage(const std::filesystem::path& pck_filename, std::fstream& pck_file, const Options& options)
{
    const auto swap_byte_order = options.swap_byte_order;

    // Skip the AKPK signature
    pck_file.seekg(4);

    const auto header_size = ReadUInt32(pck_file, swap_byte_order);
    const auto version = ReadUInt32(pck_file, swap_byte_order);
    const auto languages_size = ReadUInt32(pck_file, swap_byte_order);
    const auto banks_size = ReadUInt32(pck_file, swap_byte_order);
    const auto streams_size = ReadUInt32(pck_file, swap_byte_order);
    auto externals_size = std::uint32_t{ 0 };

    // Older packages don't have a table for external files
    const auto tables_size = std::uint64_t{ languages_size } + banks_size + streams_size;

    if (header_size != tables_size + 16U)
    {
        externals_size = ReadUInt32(pck_file, swap_byte_order);

        if (header_size != tables_size + externals_size + 20U)
        {
            std::cout << "Unknown or encrypted package, the header doesn't add up\n";
            return EXIT_FAILURE;
        }
    }

    std::cout << "Wwise Package Version: " << version << "\n";

    auto languages = ReadPackageLanguages(pck_file, languages_size, swap_byte_order);
    auto banks = std::vector<PackageEntry>{};
    auto streams = std::vector<PackageEntry>{};
    auto externals = std::vector<PackageEntry>{};

    if (!ReadPackageEntries(pck_file, banks_size, false, swap_byte_order, banks)
        || !ReadPackageEntries(pck_file, streams_size, false, swap_byte_order, streams)
        || !ReadPackageEntries(pck_file, externals_size, true, swap_byte_order, externals))
    {
        std::cout << "Unknown or encrypted package, the file tables can't be read\n";
        return EXIT_FAILURE;
    }

    // Reset EOF
    pck_file.clear();

    std::cout << "Found " << banks.size() << " BNK files, " << streams.size() << " streamed WEM files and " << externals.size() << " external WEM files\n";

    if (banks.empty() && streams.empty() && externals.empty())
    {
        std::cout << "No files discovered to be extracted\n";
        return EXIT_SUCCESS;
    }

    auto output_directory = pck_filename.parent_path();

    if (!options.no_directory)
    {
        output_directory = CreateOutputDirectory(pck_filename);
    }

    // The same ID can exist once per language, as such they can't share a directory
    auto used_languages = std::set<std::uint32_t>{};
    for (const auto* entries : { &banks, &streams, &externals })
    {
        for (const auto& entry : *entries)
        {
            used_languages.insert(entry.language_id);
        }
    }

    const auto language_name = [&](const std::uint32_t id)
    {
        const auto language = languages.find(id);
        return language != languages.end() && !language->second.empty() ? language->second : std::to_string(id);
    };

    auto list_file = std::fstream{};

    if (options.dump_objects)
    {
        auto list_filename = output_directory;
        list_filename = list_filename.append("files.txt");
        list_file.open(list_filename, std::ios::out | std::ios::binary);

        if (!list_file.is_open())
        {
            std::cout << "Unable to write files list '" << list_filename.u8string() << "'\n";
            return EXIT_FAILURE;
        }

        std::cout << "Files list was written to: " << list_filename.u8string() << "\n";
    }

    std::cout << "Start extracting...\n";

    const auto package_size = file_size(pck_filename);
    const auto extract = [&](const std::vector<PackageEntry>& entries, const std::string& type, const std::string& extension)
    {
        for (const auto& entry : entries)
        {
            const auto offset = std::uint64_t{ entry.start_block } * std::max(entry.block_size, 1U);

            if (list_file.is_open())
            {
                list_file << type << " ID: " << entry.id << "\n";
                list_file << "\tLanguage: " << language_name(entry.language_id) << "\n";
                list_file << "\tOffset: " << offset << "\n";
                list_file << "\tSize: " << entry.size << "\n";
            }

            if (offset + entry.size > package_size)
            {
                std::cout << "Skipping " << entry.id << ", it points outside of the package\n";
                continue;
            }

            auto filename = output_directory;

            if (used_languages.size() > 1U)
            {
                filename = filename.append(language_name(entry.language_id));
                create_directories(filename);
            }

            filename = filename.append(std::to_string(entry.id) + extension);

            if (!CopyContent(pck_file, offset, entry.size, filename))
            {
                std::cout << "Unable to write file '" << filename.u8string() << "'\n";
                pck_file.clear();
                continue;
            }

            if (extension == ".bnk")
            {
                // Embedded sound banks get extracted as well, always into their own directory
                auto bnk_file = std::fstream{ filename, std::ios::binary | std::ios::in };
                ExtractBank(filename, bnk_file, Options{ swap_byte_order, false, options.dump_objects });
            }
        }
    };

    extract(banks, "Bank", ".bnk");
    extract(streams, "Stream", ".wem");
    extract(externals, "External", ".wem");

    std::cout << "Files were extracted to: " << output_directory.u8string() << "\n";
    return EXIT_SUCCESS;
}

int Run(const std::vector<std::filesystem::path>& arguments)
{
    std::cout << "Wwise *.BNK / *.PCK File Extractor\n";
    std::cout << "(c) RAWR 2015-2026 - https://rawr4firefall.com\n\n";

    // Has no argument(s)
    if (arguments.size() < 2)
    {
        std::cout << "Usage: bnkextr filename.bnk|filename.pck [/swap] [/nodir] [/obj]\n";
        std::cout << "\t/swap - swap byte order (use it for unpacking 'Army of Two')\n";
        std::cout << "\t/nodir - create no additional directory for the extracted files\n";
        std::cout << "\t/obj - generate an objects.txt (BNK) or files.txt (PCK) file with the extracted data\n";
        return EXIT_SUCCESS;
    }

    const auto& filename = arguments[1];
    const auto options = Options{
        HasArgument(arguments, "/swap"),
        HasArgument(arguments, "/nodir"),
        HasArgument(arguments, "/obj")
    };

    auto file = std::fstream{ filename, std::ios::binary | std::ios::in };

    // Could not open input file
    if (!file.is_open())
    {
        std::cout << "Can't open input file: " << filename.u8string() << "\n";
        return EXIT_FAILURE;
    }

    // The signature decides the format, not the file extension
    char sign[4] = {};
    file.read(sign, sizeof(sign));
    file.clear();
    file.seekg(0);

    if (Compare(sign, "AKPK"))
    {
        return ExtractPackage(filename, file, options);
    }

    return ExtractBank(filename, file, options);
}

// Windows only passes Unicode arguments through the wide entry point
#ifdef _WIN32
int wmain(int argument_count, wchar_t* arguments[])
#else
int main(int argument_count, char* arguments[])
#endif
{
    return Run(std::vector<std::filesystem::path>(arguments, arguments + argument_count));
}
