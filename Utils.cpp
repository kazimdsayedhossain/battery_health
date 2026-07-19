#include "Utils.h"

#include <cstdio>

namespace battery_logger {

void UniqueHandle::reset(HANDLE value) noexcept {
    if (*this) { CloseHandle(handle_); }
    handle_ = value;
}

bool FormatLocalTimestamp(char* buffer, std::size_t buffer_size) noexcept {
    SYSTEMTIME time{};
    GetLocalTime(&time);
    return std::snprintf(buffer, buffer_size, "%04u-%02u-%02u %02u:%02u:%02u",
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond) > 0;
}

bool FormatTimestampForFile(wchar_t* buffer, std::size_t buffer_size) noexcept {
    SYSTEMTIME time{};
    GetLocalTime(&time);
    return _snwprintf_s(buffer, buffer_size, _TRUNCATE, L"%04u%02u%02u_%02u%02u%02u",
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond) >= 0;
}

} // namespace battery_logger
