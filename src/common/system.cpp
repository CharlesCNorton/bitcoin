// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>

#include <logging.h>
#include <util/string.h>
#include <util/time.h>

#ifndef WIN32
#include <sys/stat.h>
#else
#include <codecvt>
#endif

#ifdef HAVE_MALLOPT_ARENA_MAX
#include <malloc.h>
#endif

#ifdef HAVE_LINUX_SYSINFO
#include <sys/sysinfo.h>
#endif

#include <cstdlib>
#include <locale>
#include <stdexcept>
#include <string>
#include <thread>

// Application startup time (used for uptime calculation)
const int64_t nStartupTime = GetTime();

std::string ShellEscape(const std::string& arg)
{
    std::string escaped = arg;
    ReplaceAll(escaped, "'", "'\\''");
    return "'" + escaped + "'";
}

#if HAVE_SYSTEM
void runCommand(const std::string& strCommand)
{
    if (strCommand.empty()) return;
#ifndef WIN32
    int nErr = ::system(strCommand.c_str());
#else
    int nErr = ::_wsystem(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t>().from_bytes(strCommand).c_str());
#endif
    if (nErr)
        LogPrintf("runCommand error: system(%s) returned %d\n", strCommand, nErr);
}
#endif

void SetupEnvironment()
{
#ifdef HAVE_MALLOPT_ARENA_MAX
    // glibc-specific: On 32-bit systems set the number of arenas to 1.
    // By default, since glibc 2.10, the C library will create up to two heap
    // arenas per core. This is known to cause excessive virtual address space
    // usage in our usage. Work around it by setting the maximum number of
    // arenas to 1.
    if (sizeof(void*) == 4) {
        mallopt(M_ARENA_MAX, 1);
    }
#endif
    // On most POSIX systems (e.g. Linux, but not BSD) the environment's locale
    // may be invalid, in which case the "C.UTF-8" locale is used as fallback.
#if !defined(WIN32) && !defined(MAC_OSX) && !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
    try {
        std::locale(""); // Raises a runtime error if current locale is invalid
    } catch (const std::runtime_error&) {
        setenv("LC_ALL", "C.UTF-8", 1);
    }
#elif defined(WIN32)
    // Set the default input/output charset is utf-8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

#ifndef WIN32
    constexpr mode_t private_umask = 0077;
    umask(private_umask);
#endif
}

bool SetupNetworking()
{
#ifdef WIN32
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR || LOBYTE(wsadata.wVersion ) != 2 || HIBYTE(wsadata.wVersion) != 2)
        return false;
#endif
    return true;
}

int GetNumCores()
{
    return std::thread::hardware_concurrency();
}

// Obtain the application startup time (used for uptime calculation)
int64_t GetStartupTime()
{
    return nStartupTime;
}

size_t g_low_memory_threshold = 10 * 1024 * 1024 /* 10 MB */;

bool SystemNeedsMemoryReleased()
{
    if (g_low_memory_threshold <= 0) {
        // Intentionally bypass other metrics when disabled entirely
        return false;
    }
#ifdef WIN32
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        if (mem_status.dwMemoryLoad >= 99 ||
            mem_status.ullAvailPhys < g_low_memory_threshold ||
            mem_status.ullAvailVirtual < g_low_memory_threshold) {
            LogPrintf("%s: YES: %s%% memory load; %s available physical memory; %s available virtual memory\n", __func__, int(mem_status.dwMemoryLoad), size_t(mem_status.ullAvailPhys), size_t(mem_status.ullAvailVirtual));
            return true;
        }
    }
#endif
#ifdef HAVE_LINUX_SYSINFO
    struct sysinfo sys_info;
    if (!sysinfo(&sys_info)) {
        // Explicitly 64-bit in case of 32-bit userspace on 64-bit kernel
        const uint64_t free_ram = uint64_t(sys_info.freeram) * sys_info.mem_unit;
        const uint64_t buffer_ram = uint64_t(sys_info.bufferram) * sys_info.mem_unit;
        if (free_ram + buffer_ram < g_low_memory_threshold) {
            LogPrintf("%s: YES: %s free RAM + %s buffer RAM\n", __func__, free_ram, buffer_ram);
            return true;
        }
    }
#endif
    // NOTE: sysconf(_SC_AVPHYS_PAGES) doesn't account for caches on at least Linux, so not safe to use here
    return false;
}
