#include "utils/path_utils.hpp"

namespace utils {

std::wstring GetExePath() {
    wchar_t path[MAX_PATH]{};
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0) {
        return L"";
    }
    return std::wstring(path);
}

std::wstring GetExeDir() {
    std::wstring path = GetExePath();
    if (path.empty()) {
        return L"";
    }
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return path.substr(0, lastSlash + 1);
    }
    return L"";
}

std::wstring JoinExeDir(const std::wstring& relativePath) {
    std::wstring dir = GetExeDir();
    if (dir.empty()) {
        return relativePath;
    }
    if (!relativePath.empty() && relativePath.front() == L'\\') {
        return dir + relativePath.substr(1);
    }
    return dir + relativePath;
}

} // namespace utils
