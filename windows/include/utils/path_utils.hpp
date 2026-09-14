#ifndef PATH_UTILS_HPP
#define PATH_UTILS_HPP

#include <string>
#include <windows.h>

namespace utils {

std::wstring GetExePath();
std::wstring GetExeDir();
std::wstring JoinExeDir(const std::wstring& relativePath);

} // namespace utils

#endif // PATH_UTILS_HPP
