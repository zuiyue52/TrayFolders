// SPDX-License-Identifier: GPL-3.0-only
// Runs outside Explorer. Uses only Windows DbgHelp and Microsoft's HTTPS symbol store.
#define NOMINMAX
#include "pe_image.h"
#include <dbghelp.h>
#include <winhttp.h>
#include <filesystem>
#include <cstdio>
#include <cwctype>

namespace fs = std::filesystem;
void Phase(const char* text) { printf("phase=%s\n", text); fflush(stdout); }
std::wstring Key(const PeImage& image) {
    const auto& g = image.guid;
    wchar_t text[64];
    swprintf(text, ARRAYSIZE(text), L"%08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X%lX",
        g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
        g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7], image.age);
    return text;
}
std::wstring Hex(const BYTE* bytes, size_t size) {
    std::wstring result;
    for (size_t i = 0; i < size; i++) { result += L"0123456789ABCDEF"[bytes[i] >> 4]; result += L"0123456789ABCDEF"[bytes[i] & 15]; }
    return result;
}
struct InternetHandle {
    HINTERNET value{};
    explicit InternetHandle(HINTERNET h) : value(h) { if (!h) throw std::runtime_error("Cannot initialize HTTPS download"); }
    ~InternetHandle() { WinHttpCloseHandle(value); }
};
bool CompletePdb(const fs::path& path) {
    try {
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        BYTE header[56]{}; DWORD read{}; LARGE_INTEGER size{};
        bool ok = ReadFile(file, header, sizeof(header), &read, nullptr) && read == sizeof(header) && GetFileSizeEx(file, &size);
        CloseHandle(file);
        if (!ok || memcmp(header, "Microsoft C/C++ MSF 7.00", 24)) return false;
        DWORD blockSize{}, blockCount{};
        memcpy(&blockSize, header + 32, 4); memcpy(&blockCount, header + 40, 4);
        return blockSize >= 512 && blockSize <= 65536 && !(blockSize & (blockSize - 1)) &&
            (uint64_t)blockSize * blockCount == (uint64_t)size.QuadPart && size.QuadPart <= 256 * 1024 * 1024;
    } catch (...) { return false; }
}
void Download(const PeImage& image, const fs::path& destination) {
    Phase("download");
    InternetHandle session(WinHttpOpen(L"TrayFolders/0.4", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, nullptr, nullptr, 0));
    WinHttpSetTimeouts(session.value, 10000, 15000, 20000, 30000);
    InternetHandle connection(WinHttpConnect(session.value, L"msdl.microsoft.com", INTERNET_DEFAULT_HTTPS_PORT, 0));
    auto resource = L"/download/symbols/" + image.pdbName + L"/" + Key(image) + L"/" + image.pdbName;
    InternetHandle request(WinHttpOpenRequest(connection.value, L"GET", resource.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH));
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));
    if (!WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0) || !WinHttpReceiveResponse(request.value, nullptr))
        throw std::runtime_error("Symbol download failed (network or proxy). Retry when online.");
    DWORD status{}, size = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &size, nullptr) || status != 200)
        throw std::runtime_error("Microsoft symbol server did not return a complete PDB (HTTP " + std::to_string(status) + ")");
    auto temp = destination.wstring() + L"." + std::to_wstring(GetCurrentProcessId()) + L".part";
    HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot write symbol cache");
    try {
        BYTE buffer[65536]; DWORD total = 0;
        for (;;) {
            DWORD count{}, written{};
            if (!WinHttpReadData(request.value, buffer, sizeof(buffer), &count)) throw std::runtime_error("Symbol download interrupted");
            if (!count) break;
            total += count;
            if (total > 256 * 1024 * 1024) throw std::runtime_error("PDB exceeds download limit");
            if (!WriteFile(file, buffer, count, &written, nullptr) || written != count) throw std::runtime_error("Cannot write symbol cache");
        }
        CloseHandle(file); file = INVALID_HANDLE_VALUE;
        if (!CompletePdb(temp)) throw std::runtime_error("Incomplete or invalid PDB download");
        if (!MoveFileExW(temp.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING)) throw std::runtime_error("Cannot finalize PDB cache");
    } catch (...) {
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        DeleteFileW(temp.c_str()); throw;
    }
}
std::wstring Canonical(std::wstring name) {
    for (size_t p; (p = name.find(L"__ptr64")) != std::wstring::npos;) name.erase(p, 7);
    std::erase_if(name, [](wchar_t c) { return iswspace(c); });
    return name;
}
struct Search {
    DWORD64 base{};
    AdaptationProfile* profile{};
    std::wstring names[kSymbolCount];
    bool ambiguous = false;
};
BOOL CALLBACK Collect(PSYMBOL_INFOW symbol, ULONG, void* data) {
    auto& search = *(Search*)data;
    wchar_t fullName[8192];
    if (!UnDecorateSymbolNameW(symbol->Name, fullName, ARRAYSIZE(fullName), 0)) return TRUE;
    auto name = Canonical(fullName);
    for (size_t i = 0; i < kSymbolCount; i++) {
        if (name != search.names[i]) continue;
        if (symbol->Address < search.base || symbol->Address - search.base >= search.profile->imageSize) { search.ambiguous = true; continue; }
        auto rva = (uint32_t)(symbol->Address - search.base);
        if (search.profile->rvas[i] && search.profile->rvas[i] != rva) search.ambiguous = true;
        search.profile->rvas[i] = rva;
    }
    return TRUE;
}
void Resolve(const std::wstring& module, const fs::path& pdbFolder, const PeImage& image, AdaptationProfile& profile) {
    Phase("resolve");
    HANDLE process = GetCurrentProcess();
    // No network or environment-controlled symbol paths inside DbgHelp.
    SymSetOptions(SYMOPT_EXACT_SYMBOLS | SYMOPT_IGNORE_NT_SYMPATH | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);
    if (!SymInitializeW(process, pdbFolder.c_str(), FALSE)) throw std::runtime_error("DbgHelp initialization failed");
    try {
        auto base = SymLoadModuleExW(process, nullptr, module.c_str(), nullptr, 0x10000000, 0, nullptr, 0);
        IMAGEHLP_MODULEW64 info{}; info.SizeOfStruct = sizeof(info);
        if (!base || !SymGetModuleInfoW64(process, base, &info) || info.SymType != SymPdb || info.PdbUnmatched ||
            memcmp(&info.PdbSig70, &image.guid, 16) || info.PdbAge != image.age ||
            fs::weakly_canonical(info.LoadedPdbName) != fs::weakly_canonical(pdbFolder / image.pdbName))
            throw std::runtime_error("PDB does not match the tray component");
        Search search{}; search.base = base; search.profile = &profile;
        for (size_t i = 0; i < kSymbolCount; i++) search.names[i] = Canonical(kRequiredSymbols[i].name);
        if (!SymEnumSymbolsW(process, base, L"*SystemTray*", Collect, &search) || search.ambiguous)
            throw std::runtime_error("Ambiguous or unreadable tray symbols");
        for (size_t i = 0; i < kSymbolCount; i++) {
            if (!profile.rvas[i]) throw std::runtime_error("Required tray ABI symbol missing: " + std::to_string(i));
            if (!image.ValidRva(i, profile.rvas[i])) throw std::runtime_error("Symbol points to an unexpected PE section");
            memcpy(profile.bytes[i], image.Rva(profile.rvas[i], 16), 16);
        }
        SymCleanup(process);
    } catch (...) { SymCleanup(process); throw; }
}
int wmain(int argc, wchar_t** argv) {
    try {
        if (argc < 4 || argc > 6) throw std::runtime_error("Usage: module cache-directory output-profile [--offline] [--parent=PID]");
        bool offline = false;
        for (int i = 4; i < argc; i++) {
            if (!wcscmp(argv[i], L"--offline")) offline = true;
            else if (!wcsncmp(argv[i], L"--parent=", 9)) {
                wchar_t* end{}; auto pid = wcstoul(argv[i] + 9, &end, 10);
                if (!pid || *end) throw std::runtime_error("Invalid parent process");
                HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, pid);
                if (!parent) throw std::runtime_error("Controller is no longer running");
                HANDLE watcher = CreateThread(nullptr, 0, [](void* handle) -> DWORD {
                    WaitForSingleObject(handle, INFINITE); CloseHandle(handle); ExitProcess(2);
                }, parent, 0, nullptr);
                if (!watcher) { CloseHandle(parent); throw std::runtime_error("Cannot monitor controller lifetime"); }
                CloseHandle(watcher);
            } else throw std::runtime_error("Unknown resolver option");
        }
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        Phase("detect");
        std::wstring module = fs::absolute(argv[1]).wstring();
        auto leaf = fs::path(module).filename().wstring();
        uint32_t adapter = !_wcsicmp(leaf.c_str(), L"Taskbar.View.dll") ? 1 : !_wcsicmp(leaf.c_str(), L"SystemTray.dll") ? 2 : 0;
        if (!adapter) throw std::runtime_error("No adapter for this module");
        PeImage image(module);
        auto cache = fs::absolute(argv[2]) / image.pdbName / Key(image);
        fs::create_directories(cache);
        auto profilePath = cache / (Hex(image.hash, 32) + L".v1.profile");
        AdaptationProfile profile;
        if (ReadProfile(profilePath, profile) && profile.adapter == adapter && image.Matches(profile)) {
            Phase("cache");
        } else {
            auto pdb = cache / image.pdbName;
            bool alreadyCached = CompletePdb(pdb);
            if (!alreadyCached) {
                if (offline) throw std::runtime_error("Matching PDB is not cached; offline preparation unavailable");
                Download(image, pdb);
            }
            profile = AdaptationProfile{};
            profile.adapter = adapter; profile.timestamp = image.nt.FileHeader.TimeDateStamp;
            profile.imageSize = image.nt.OptionalHeader.SizeOfImage; profile.guid = image.guid; profile.age = image.age;
            memcpy(profile.imageHash, image.hash, 32);
            try { Resolve(module, cache, image, profile); }
            catch (const std::exception& e) {
                if (!alreadyCached || offline || strcmp(e.what(), "PDB does not match the tray component")) throw;
                Download(image, pdb);
                std::fill(std::begin(profile.rvas), std::end(profile.rvas), 0);
                Resolve(module, cache, image, profile);
            }
            Phase("verify");
            if (!Sha256(&profile, offsetof(AdaptationProfile, digest), profile.digest) || !image.Matches(profile)) throw std::runtime_error("Adaptation verification failed");
            WriteAtomic(profilePath, &profile, sizeof(profile));
        }
        WriteAtomic(argv[3], &profile, sizeof(profile));
        printf("ready=adapter-%lu; symbols=%zu; identity=", (unsigned long)adapter, kSymbolCount);
        auto key = Key(image); printf("%ls\n", key.c_str());
        return 0;
    } catch (const std::exception& e) { fprintf(stderr, "%s\n", e.what()); return 1; }
}
