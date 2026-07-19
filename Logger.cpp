#include "Logger.h"
#include "Utils.h"

#include <batclass.h>
#include <cstdio>

namespace battery_logger {

Logger::~Logger() { Close(); }

bool Logger::OpenNew(const wchar_t* path, const BatteryMetadata& metadata) noexcept {
    metadata_ = metadata;
    file_ = _wfopen(path, L"wb");
    if (file_ == nullptr) { return false; }
    // A caller-owned buffer prevents the CRT from allocating during sampling.
    static char file_buffer[4096];
    setvbuf(file_, file_buffer, _IOFBF, sizeof(file_buffer));
    const int written = std::snprintf(buffer_, sizeof(buffer_),
        "Timestamp,Tag,Voltage_mV,Rate_mW,Power_mW,RemainingCapacity_mWh,FullChargeCapacity_mWh,DesignCapacity_mWh,CycleCount,Temperature_dK,PowerStateFlags,ACConnected,Charging\r\n");
    return written > 0 && std::fwrite(buffer_, 1, static_cast<size_t>(written), file_) == static_cast<size_t>(written)
        && std::fflush(file_) == 0;
}

bool Logger::WriteSample(const BatterySample& sample) noexcept {
    char timestamp[32]{};
    if (!FormatLocalTimestamp(timestamp, sizeof(timestamp))) { return false; }
    const unsigned ac_connected = (sample.flags & BATTERY_POWER_ON_LINE) != 0 ? 1U : 0U;
    const unsigned charging = (sample.flags & BATTERY_CHARGING) != 0 ? 1U : 0U;
    const int written = std::snprintf(buffer_, sizeof(buffer_),
        "%s,%lu,%lu,%ld,%ld,%lu,%lu,%lu,%lu,%lu,%lu,%u,%u\r\n",
        timestamp, metadata_.tag, sample.voltage_mv, sample.rate_mw, sample.rate_mw,
        sample.remaining_capacity_mwh, metadata_.full_charge_capacity_mwh,
        metadata_.design_capacity_mwh, metadata_.cycle_count,
        sample.temperature_available ? sample.temperature_deci_kelvin : 0UL,
        sample.flags, ac_connected, charging);
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(buffer_)) { return false; }
    return std::fwrite(buffer_, 1, static_cast<size_t>(written), file_) == static_cast<size_t>(written)
        && std::fflush(file_) == 0;
}

void Logger::Close() noexcept {
    if (file_ != nullptr) { std::fclose(file_); file_ = nullptr; }
}

bool ActiveLogExists(const wchar_t* path) noexcept {
    const DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool ArchiveActiveLog(const wchar_t* active_path, const wchar_t* archive_directory) noexcept {
    wchar_t timestamp[32]{};
    wchar_t archive_path[MAX_PATH]{};
    return FormatTimestampForFile(timestamp, sizeof(timestamp) / sizeof(timestamp[0])) &&
        swprintf_s(archive_path, L"%s\\BatteryDischarge_%s.csv", archive_directory, timestamp) > 0 &&
        MoveFileEx(active_path, archive_path, MOVEFILE_WRITE_THROUGH) != FALSE;
}

} // namespace battery_logger
