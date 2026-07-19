#pragma once

#include <windows.h>
#include <cstddef>

namespace battery_logger {

class UniqueHandle {
public:
    explicit UniqueHandle(HANDLE handle = INVALID_HANDLE_VALUE) noexcept : handle_(handle) {}
    ~UniqueHandle() { reset(); }
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;
    UniqueHandle(UniqueHandle&& other) noexcept : handle_(other.release()) {}
    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) { reset(other.release()); }
        return *this;
    }
    HANDLE get() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr; }
    HANDLE release() noexcept { HANDLE value = handle_; handle_ = INVALID_HANDLE_VALUE; return value; }
    void reset(HANDLE value = INVALID_HANDLE_VALUE) noexcept;

private:
    HANDLE handle_;
};

// Produces a sortable local timestamp without allocating memory.
bool FormatLocalTimestamp(char* buffer, std::size_t buffer_size) noexcept;
bool FormatTimestampForFile(wchar_t* buffer, std::size_t buffer_size) noexcept;

} // namespace battery_logger
