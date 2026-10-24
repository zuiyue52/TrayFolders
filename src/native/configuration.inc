// Settings

struct FolderConfig {
    std::wstring id;
    std::wstring name;
    std::wstring glyph;
    std::vector<std::wstring> patterns;  // Lowercase.
};

struct Settings {
    std::vector<FolderConfig> folders;
    bool foldersAtEnd = false;
    bool showEmptyFolders = false;
};

Settings g_settings;

std::wstring ToLower(std::wstring s) {
    if (!s.empty()) {
        CharLowerBuff(s.data(), (DWORD)s.size());
    }
    return s;
}

std::wstring Trim(std::wstring_view s) {
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && iswspace(s[begin])) {
        begin++;
    }
    while (end > begin && iswspace(s[end - 1])) {
        end--;
    }
    return std::wstring(s.substr(begin, end - begin));
}

// Tooltips can contain line breaks and repeated spaces; collapse them so that
// patterns can be written on a single line.
std::wstring NormalizeName(std::wstring_view s) {
    std::wstring result;
    result.reserve(s.size());
    bool lastWasSpace = false;
    for (WCHAR c : s) {
        if (iswspace(c)) {
            if (!lastWasSpace) {
                result.push_back(L' ');
            }
            lastWasSpace = true;
        } else {
            result.push_back(c);
            lastWasSpace = false;
        }
    }
    return Trim(result);
}

std::wstring ParseGlyph(std::wstring_view value) {
    std::wstring glyph = Trim(value);
    std::wstring_view hex = glyph;
    if (!hex.starts_with(L"U+") && !hex.starts_with(L"u+") &&
        !hex.starts_with(L"0x") && !hex.starts_with(L"0X")) {
        return glyph;
    }
    hex.remove_prefix(2);
    if (hex.size() >= 4 && hex.size() <= 5 &&
        std::all_of(hex.begin(), hex.end(),
                    [](WCHAR c) { return !!iswxdigit(c); })) {
        UINT32 codePoint = wcstoul(std::wstring(hex).c_str(), nullptr, 16);
        if (codePoint <= 0xFFFF) {
            return std::wstring(1, (WCHAR)codePoint);
        }
        codePoint -= 0x10000;
        return {(WCHAR)(0xD800 + (codePoint >> 10)),
                (WCHAR)(0xDC00 + (codePoint & 0x3FF))};
    }
    return glyph;
}

Settings LoadSettings() {
    Settings settings;
    int count = std::clamp(ConfigInt(L"General", L"FolderCount"), 0, 100);
    for (int i = 0; i < count; ++i) {
        std::wstring section = L"Folder" + std::to_wstring(i);
        FolderConfig folder;
        folder.id = ConfigString(section.c_str(), L"Id");
        if (folder.id.empty()) folder.id = std::to_wstring(i);
        folder.name = Trim(ConfigString(section.c_str(), L"Name"));
        folder.glyph = ParseGlyph(ConfigString(section.c_str(), L"Glyph"));
        int patterns = std::clamp(ConfigInt(section.c_str(), L"Count"), 0, 500);
        for (int j = 0; j < patterns; ++j) {
            auto key = L"Icon" + std::to_wstring(j);
            auto pattern = NormalizeName(ConfigString(section.c_str(), key.c_str()));
            if (!pattern.empty()) folder.patterns.push_back(ToLower(pattern));
        }
        if (folder.name.empty()) folder.name = L"Folder";
        settings.folders.push_back(std::move(folder));
    }
    settings.foldersAtEnd = ConfigInt(L"General", L"FoldersAtEnd", 1) != 0;
    settings.showEmptyFolders = ConfigInt(L"General", L"ShowEmptyFolders", 0) != 0;
    return settings;
}

bool WildcardMatch(std::wstring_view pattern, std::wstring_view text) {
    size_t p = 0;
    size_t t = 0;
    size_t starP = std::wstring_view::npos;
    size_t starT = 0;
    while (t < text.size()) {
        if (p < pattern.size() &&
            (pattern[p] == L'?' || pattern[p] == text[t])) {
            p++;
            t++;
        } else if (p < pattern.size() && pattern[p] == L'*') {
            starP = p++;
            starT = t;
        } else if (starP != std::wstring_view::npos) {
            p = starP + 1;
            t = ++starT;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == L'*') {
        p++;
    }
    return p == pattern.size();
}

// An icon matches a pattern if its app name, tooltip or accessible name does.
int FindFolderForIcon(std::initializer_list<std::wstring_view> lowerNames) {
    for (size_t i = 0; i < g_settings.folders.size(); i++) {
        for (const auto& pattern : g_settings.folders[i].patterns) {
            for (auto name : lowerNames) {
                if (!name.empty() && WildcardMatch(pattern, name)) {
                    return (int)i;
                }
            }
        }
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
