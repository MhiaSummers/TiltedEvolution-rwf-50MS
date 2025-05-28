#include <filesystem>
#include <iostream>
#include <fstream>
#include <regex>
#define SPDLOG_WCHAR_FILENAMES
#include <spdlog/formatter.h>

#include "utils/Error.h"
#include "DllBlocklist.h"
#include "../Launcher.h"
#include "../client/ScriptExtender.h"

namespace stubs
{
// clang-format off

    // A list of modules blocked for a variety of reasons, but mainly
    // for causing crashes and incompatibility with ST
    const wchar_t* const kDllBlocklist[] = {
 //     L"EngineFixes.dll",          // Skyrim Engine Fixes, breaks our hooks
        L"SkyrimSoulsRE.dll",        // Our mod implements this with special handling
        L"crashhandler64.dll",       // Stream crash handler, breaks heap
        L"fraps64.dll",              // Breaks tilted ui
        L"SpecialK64.dll",           // breaks rendering
        L"ReShade64_SpecialK64.dll", // same reason
        L"NvCamera64.dll",           // broken af nvidia stuff, blacklisted for now, needs fix later
       // L"atiuxp64.dll",
       // L"aticfx64.dll"
    };

// clang-format on

struct DllGreyList
{
    const wchar_t* m_dllName;
    const wchar_t* m_configLocation;
    const char*    m_regex;
    const wchar_t* m_prompt;
    const char*    m_configToAppend;
} kDllGreyList[] = 
{
    {
        L"EngineFixes.dll",
        L"Data\\SKSE\\Plugins\\EngineFixes.toml",
         "^# SKYRIM TOGETHER REBORN required settings\n",
        L"Specific settings must be changed for EngineFixes to be compatile with Skyrim Together Reborn. Make the Change, or Exit?",
            "# SKYRIM TOGETHER REBORN required settings\n"
            "# Skyrim Together Reborn requires these settings for EngineFixes compatibility. Change anything following at your own risk.\n"
            "[Patches]\n"
            "MemoryManager = false                   # STR requires false.        # Replaces Skyrim's global allocator\n"
            "ScaleformAllocator = false              # STR requires false.        # Replaces the scaleform allocator\n"
            "[Fixes]\n"
            "AnimationLoadSignedCrash = false        # STR requires false.        #Fix a misplaced use of a signed value.Should allow to load more animations before CTD\n"
    }
};

enum GreyListDisposition
{
    kNotGreyList,       // Not on the greylist at all
    kGreyListAccept,    // It is on the greylist with a good config, or the changes were accepted
    kGreyListAbort      // It is on the greylist and the configuration is no good.
};

static bool file_contains_regex(const std::filesystem::path& path, const char* pattern)
{
    std::ifstream file(path);
    if (!file)
    {
        auto msg = fmt::format(L"Error: Unable to open file {}", path.c_str());
        Die(msg.c_str(), true);
        return false;
    }

    std::regex regex_pattern;
    try
    {
         regex_pattern = pattern;
    }
    catch (const std::regex_error& e)
    {
        auto msg = fmt::format(L"Invalid regular expression");
        Die(msg.c_str(), true);
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (std::regex_search(line, regex_pattern))
        {
            return true;
        }
    }

    return false;
}


enum GreyListDisposition IsDllGreyListBlocked(std::wstring_view dllName)
{
    launcher::LaunchContext* LC = launcher::GetLaunchContext();
    enum GreyListDisposition retval = kNotGreyList;

    if (!IsScriptExtenderLoaded())
        return kNotGreyList;
    for (auto greyListEntry : kDllGreyList)
    {
        if (std::wcscmp(dllName.data(), greyListEntry.m_dllName) == 0)
        {
            std::string regex(greyListEntry.m_regex);
            std::filesystem::path filepath = LC->gamePath / greyListEntry.m_configLocation;
            if (file_contains_regex(filepath, greyListEntry.m_regex))
            {
                retval = kGreyListAccept;
                break;
            }
        }
    }
    return retval;
}



bool IsDllBlocked(std::wstring_view dllName)
{
    for (const wchar_t* dllEntry : kDllBlocklist)
    {
        if (std::wcscmp(dllName.data(), dllEntry) == 0)
        {
            return true;
        }
    }

    return false;
}
} // namespace stubs
