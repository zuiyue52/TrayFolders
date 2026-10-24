// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "adaptation_profile.h"

AdaptationProfile g_adaptation;
std::atomic<bool> g_runtimeValidated, g_runtimeRejected;

inline bool LoadedRange(const BYTE* base, const IMAGE_NT_HEADERS64* nt, uint32_t rva, size_t length, bool executable) {
    if (rva >= nt->OptionalHeader.SizeOfImage || length > nt->OptionalHeader.SizeOfImage - rva) return false;
    auto sections = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        const auto& s = sections[i];
        uint64_t end = (uint64_t)s.VirtualAddress + std::max(s.Misc.VirtualSize, s.SizeOfRawData);
        if (rva >= s.VirtualAddress && (uint64_t)rva + length <= end) {
            MEMORY_BASIC_INFORMATION memory{};
            if (!VirtualQuery(base + rva, &memory, sizeof(memory)) || memory.State != MEM_COMMIT ||
                (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) || (const BYTE*)memory.BaseAddress + memory.RegionSize < base + rva + length) return false;
            return (s.Characteristics & IMAGE_SCN_MEM_READ) && !(s.Characteristics & IMAGE_SCN_MEM_WRITE) &&
                ((s.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0) == executable;
        }
    }
    return false;
}

inline bool ValidateLoadedProfile(HMODULE module, const AdaptationProfile& p) {
    if (!module || !ValidProfile(p)) return false;
    auto base = (const BYTE*)module;
    auto dos = (const IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 64 || dos->e_lfanew > 0x100000) return false;
    auto nt = (const IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->FileHeader.NumberOfSections > 96 || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->FileHeader.TimeDateStamp != p.timestamp || nt->OptionalHeader.SizeOfImage != p.imageSize) return false;
    auto dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if (!dir.VirtualAddress || dir.Size > 65536 || (uint64_t)dir.VirtualAddress + dir.Size > p.imageSize) return false;
    bool identity = false;
    for (size_t i = 0; i + sizeof(IMAGE_DEBUG_DIRECTORY) <= dir.Size; i += sizeof(IMAGE_DEBUG_DIRECTORY)) {
        auto entry = (const IMAGE_DEBUG_DIRECTORY*)(base + dir.VirtualAddress + i);
        if (entry->Type != IMAGE_DEBUG_TYPE_CODEVIEW || entry->SizeOfData < 24 ||
            (uint64_t)entry->AddressOfRawData + entry->SizeOfData > p.imageSize) continue;
        auto cv = base + entry->AddressOfRawData;
        DWORD age{}; memcpy(&age, cv + 20, 4);
        if (!memcmp(cv, "RSDS", 4) && !memcmp(cv + 4, &p.guid, 16) && age == p.age) identity = true;
    }
    if (!identity) return false;
    wchar_t path[32768];
    if (!GetModuleFileNameW(module, path, ARRAYSIZE(path))) return false;
    const wchar_t* leaf = wcsrchr(path, L'\\'); leaf = leaf ? leaf + 1 : path;
    if (_wcsicmp(leaf, p.adapter == 1 ? L"Taskbar.View.dll" : L"SystemTray.dll")) return false;
    for (size_t i = 0; i < kSymbolCount; i++) {
        auto kind = kRequiredSymbols[i].kind;
        if (!LoadedRange(base, nt, p.rvas[i], kind == SymbolKind::Vtable ? 24 : 16, kind == SymbolKind::Function)) return false;
        if (kind != SymbolKind::Vtable) {
            if (memcmp(base + p.rvas[i], p.bytes[i], 16)) return false;
        } else {
            auto table = (const BYTE* const*)(base + p.rvas[i]);
            for (int j = 0; j < 3; j++) {
                auto address = (uintptr_t)table[j];
                if (address < (uintptr_t)base || address - (uintptr_t)base >= p.imageSize ||
                    !LoadedRange(base, nt, (DWORD)(address - (uintptr_t)base), 1, true)) return false;
            }
        }
    }
    return true;
}

inline bool IsSupportedModule(HMODULE module) {
    AdaptationProfile candidate;
    if (!ReadProfile(ModuleDirectory() + L"compatibility.bin", candidate) || !ValidateLoadedProfile(module, candidate)) return false;
    g_adaptation = candidate;
    return true;
}
