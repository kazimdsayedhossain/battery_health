#pragma once

#include "BatteryReader.h"
#include <cstdio>
#include <windows.h>

namespace battery_logger {

class Logger {
public:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool OpenNew(const wchar_t* path, const BatteryMetadata& metadata) noexcept;
    bool WriteSample(const BatterySample& sample) noexcept;
    void Close() noexcept;

private:
    FILE* file_ = nullptr;
    BatteryMetadata metadata_{};
    char buffer_[512]{};
};

bool ActiveLogExists(const wchar_t* path) noexcept;
bool ArchiveActiveLog(const wchar_t* active_path, const wchar_t* archive_directory) noexcept;

} // namespace battery_logger
