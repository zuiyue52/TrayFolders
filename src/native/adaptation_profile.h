// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "symbol_catalog.h"
#include <bcrypt.h>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

// Shared, bounded wire format. Bump schema whenever the ABI/catalog changes.
#pragma pack(push, 1)
struct AdaptationProfile {
    uint32_t magic = 0x50414654, schema = 1, adapter = 0;
    uint32_t timestamp = 0, imageSize = 0;
    GUID guid{};
    uint32_t age = 0, count = kSymbolCount;
    BYTE imageHash[32]{};
    uint32_t rvas[kSymbolCount]{};
    BYTE bytes[kSymbolCount][16]{};
    BYTE digest[32]{};
};
#pragma pack(pop)

inline bool Sha256(const void* data, size_t size, BYTE* output) {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (ok) ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
    if (ok) ok = size <= MAXDWORD && BCryptHashData(hash, (PUCHAR)data, (ULONG)size, 0) >= 0;
    if (ok) ok = BCryptFinishHash(hash, output, 32, 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
}
inline bool ValidProfile(const AdaptationProfile& p) {
    BYTE digest[32];
    return p.magic == 0x50414654 && p.schema == 1 && (p.adapter == 1 || p.adapter == 2) &&
        p.count == kSymbolCount && p.age && p.imageSize &&
        Sha256(&p, offsetof(AdaptationProfile, digest), digest) && !memcmp(digest, p.digest, 32);
}
inline std::vector<BYTE> ReadBoundedFile(const std::wstring& path, size_t maxSize) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot read file");
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || (uint64_t)size.QuadPart > maxSize) {
        CloseHandle(file); throw std::runtime_error("File size out of bounds");
    }
    std::vector<BYTE> data((size_t)size.QuadPart);
    DWORD read{};
    bool ok = ReadFile(file, data.data(), (DWORD)data.size(), &read, nullptr) && read == data.size();
    CloseHandle(file);
    if (!ok) throw std::runtime_error("Incomplete file read");
    return data;
}
inline bool ReadProfile(const std::wstring& path, AdaptationProfile& p) {
    try {
        auto bytes = ReadBoundedFile(path, sizeof(p));
        if (bytes.size() != sizeof(p)) return false;
        memcpy(&p, bytes.data(), sizeof(p));
        return ValidProfile(p);
    } catch (...) { return false; }
}
inline void WriteAtomic(const std::wstring& path, const void* data, size_t size) {
    auto temp = path + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create adaptation file");
    DWORD written{};
    bool ok = WriteFile(file, data, (DWORD)size, &written, nullptr) && written == size && FlushFileBuffers(file);
    CloseHandle(file);
    if (ok) ok = MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok) { DeleteFileW(temp.c_str()); throw std::runtime_error("Cannot save adaptation file"); }
}
