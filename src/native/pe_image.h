// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "adaptation_profile.h"
#include <algorithm>

struct PeImage {
    std::vector<BYTE> data;
    IMAGE_NT_HEADERS64 nt{};
    std::vector<IMAGE_SECTION_HEADER> sections;
    GUID guid{};
    uint32_t age{};
    std::wstring pdbName;
    BYTE hash[32]{};

    const BYTE* At(size_t offset, size_t length) const {
        if (offset > data.size() || length > data.size() - offset) throw std::runtime_error("Malformed PE range");
        return data.data() + offset;
    }
    const IMAGE_SECTION_HEADER* Section(uint32_t rva, size_t length) const {
        for (const auto& s : sections) {
            uint64_t end = (uint64_t)s.VirtualAddress + std::max(s.Misc.VirtualSize, s.SizeOfRawData);
            if (rva >= s.VirtualAddress && (uint64_t)rva + length <= end) return &s;
        }
        return nullptr;
    }
    const BYTE* Rva(uint32_t rva, size_t length) const {
        auto s = Section(rva, length);
        if (!s || (uint64_t)(rva - s->VirtualAddress) + length > s->SizeOfRawData) throw std::runtime_error("PE RVA has no file bytes");
        return At((size_t)s->PointerToRawData + rva - s->VirtualAddress, length);
    }
    explicit PeImage(const std::wstring& path) : data(ReadBoundedFile(path, 256 * 1024 * 1024)) {
        IMAGE_DOS_HEADER dos; memcpy(&dos, At(0, sizeof(dos)), sizeof(dos));
        if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 64) throw std::runtime_error("Invalid PE header");
        memcpy(&nt, At(dos.e_lfanew, sizeof(nt)), sizeof(nt));
        if (nt.Signature != IMAGE_NT_SIGNATURE || nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
            nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt.FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER64) ||
            !nt.FileHeader.NumberOfSections || nt.FileHeader.NumberOfSections > 96 || nt.OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_DEBUG)
            throw std::runtime_error("Only x64 PE32+ tray components are supported");
        size_t sectionOffset = dos.e_lfanew + 24 + nt.FileHeader.SizeOfOptionalHeader;
        sections.resize(nt.FileHeader.NumberOfSections);
        memcpy(sections.data(), At(sectionOffset, sections.size() * sizeof(sections[0])), sections.size() * sizeof(sections[0]));
        auto dir = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        if (!dir.VirtualAddress || dir.Size > 65536) throw std::runtime_error("Missing CodeView identity");
        for (size_t i = 0; i + sizeof(IMAGE_DEBUG_DIRECTORY) <= dir.Size; i += sizeof(IMAGE_DEBUG_DIRECTORY)) {
            IMAGE_DEBUG_DIRECTORY entry; memcpy(&entry, Rva(dir.VirtualAddress + (DWORD)i, sizeof(entry)), sizeof(entry));
            if (entry.Type != IMAGE_DEBUG_TYPE_CODEVIEW || entry.SizeOfData < 25 || entry.SizeOfData > 4096) continue;
            auto cv = Rva(entry.AddressOfRawData, entry.SizeOfData);
            if (memcmp(cv, "RSDS", 4)) continue;
            memcpy(&guid, cv + 4, 16); memcpy(&age, cv + 20, 4);
            auto name = (const char*)cv + 24;
            auto end = (const char*)memchr(name, 0, entry.SizeOfData - 24);
            if (!end) throw std::runtime_error("Invalid PDB name");
            std::string pathName(name, end);
            auto slash = pathName.find_last_of("/\\");
            auto leaf = pathName.substr(slash == std::string::npos ? 0 : slash + 1);
            if (leaf.empty() || leaf.size() > 128 || leaf.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos || !leaf.ends_with(".pdb"))
                throw std::runtime_error("Invalid PDB file name");
            pdbName.assign(leaf.begin(), leaf.end());
            break;
        }
        if (pdbName.empty() || !age || !Sha256(data.data(), data.size(), hash)) throw std::runtime_error("Cannot identify module");
    }
    bool ValidRva(size_t index, uint32_t rva) const {
        auto s = Section(rva, 16);
        if (!s || rva >= nt.OptionalHeader.SizeOfImage || (s->Characteristics & IMAGE_SCN_MEM_WRITE)) return false;
        bool executable = (s->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        return executable == (kRequiredSymbols[index].kind == SymbolKind::Function) && (s->Characteristics & IMAGE_SCN_MEM_READ);
    }
    bool Matches(const AdaptationProfile& p) const {
        if (!ValidProfile(p) || p.timestamp != nt.FileHeader.TimeDateStamp || p.imageSize != nt.OptionalHeader.SizeOfImage ||
            p.age != age || memcmp(&p.guid, &guid, 16) || memcmp(p.imageHash, hash, 32)) return false;
        for (size_t i = 0; i < kSymbolCount; i++) {
            if (!ValidRva(i, p.rvas[i]) || memcmp(Rva(p.rvas[i], 16), p.bytes[i], 16)) return false;
        }
        return true;
    }
};
