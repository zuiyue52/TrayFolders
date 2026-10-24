// SPDX-License-Identifier: GPL-3.0-only
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <algorithm>
#include <string>
std::wstring ModuleDirectory() { return L""; }
#include "../../src/native/loaded_profile.h"
#include "../../src/native/pe_image.h"
#include "known_symbols_26200.h"
#include <cstdio>

void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void Seal(AdaptationProfile& profile) { Check(Sha256(&profile, offsetof(AdaptationProfile, digest), profile.digest), "hash failed"); }
int wmain(int argc, wchar_t** argv) {
    HMODULE mapped{};
    try {
        if (argc != 3) return 2;
        PeImage image(argv[1]);
        AdaptationProfile profile;
        Check(ReadProfile(argv[2], profile), "valid profile could not be read");
        Check(image.Matches(profile), "profile does not match source image");
        if (image.nt.FileHeader.TimeDateStamp == kModuleTimestamp && image.nt.OptionalHeader.SizeOfImage == kModuleImageSize) {
            Check(kSymbolCount == ARRAYSIZE(kSymbols), "catalog size changed");
            for (size_t i = 0; i < kSymbolCount; i++) Check(profile.rvas[i] == kSymbols[i].rva, "dynamic RVA differs from known working baseline");
        }
        // Map the component without resolving imports or running DLL entry points.
        mapped = LoadLibraryExW(argv[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (!mapped) throw std::runtime_error("test mapping failed: " + std::to_string(GetLastError()));
        Check(ValidateLoadedProfile(mapped, profile), "loaded component verification rejected the valid profile");
        auto invalid = profile; invalid.age++; Seal(invalid);
        Check(!image.Matches(invalid) && !ValidateLoadedProfile(mapped, invalid), "different PDB age accepted");
        invalid = profile; invalid.guid.Data1++; Seal(invalid);
        Check(!image.Matches(invalid) && !ValidateLoadedProfile(mapped, invalid), "different PDB GUID accepted");
        invalid = profile; invalid.rvas[1] = profile.imageSize + 1; Seal(invalid);
        Check(!image.Matches(invalid) && !ValidateLoadedProfile(mapped, invalid), "out-of-image symbol accepted");
        invalid = profile; invalid.rvas[1] = profile.rvas[0]; Seal(invalid);
        Check(!image.Matches(invalid) && !ValidateLoadedProfile(mapped, invalid), "data address accepted as function");
        invalid = profile; invalid.bytes[1][0] ^= 1; Seal(invalid);
        Check(!image.Matches(invalid) && !ValidateLoadedProfile(mapped, invalid), "modified function bytes accepted");
        invalid = profile; invalid.timestamp++; Seal(invalid);
        Check(!ValidateLoadedProfile(mapped, invalid), "stale image timestamp accepted");
        invalid = profile; invalid.digest[0] ^= 1;
        Check(!ValidProfile(invalid), "corrupt profile accepted");
        invalid = profile; invalid.schema++; Seal(invalid);
        Check(!ValidProfile(invalid), "unknown profile schema accepted");
        invalid = profile; invalid.adapter = profile.adapter == 1 ? 2 : 1; Seal(invalid);
        Check(!ValidateLoadedProfile(mapped, invalid), "wrong component adapter accepted");
        FreeLibrary(mapped);
        printf("PASS: dynamic symbol baseline, loaded identity, signature bytes, PE ranges, wrong version/adapter/schema and corrupted cache rejection.\n");
        return 0;
    } catch (const std::exception& e) { if (mapped) FreeLibrary(mapped); fprintf(stderr, "%s\n", e.what()); return 1; }
}
