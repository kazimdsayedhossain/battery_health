#pragma once

#include "Utils.h"
#include <windows.h>
#include <cstdint>

namespace battery_logger {

struct BatteryMetadata {
    std::uint32_t tag = 0;
    std::uint32_t full_charge_capacity_mwh = 0;
    std::uint32_t design_capacity_mwh = 0;
    std::uint32_t cycle_count = 0;
};

struct BatterySample {
    std::uint32_t voltage_mv = 0;
    std::int32_t rate_mw = 0;
    std::uint32_t remaining_capacity_mwh = 0;
    std::uint32_t temperature_deci_kelvin = 0;
    std::uint32_t flags = 0;
    bool temperature_available = false;
};

class BatteryReader {
public:
    BatteryReader() = default;
    ~BatteryReader() = default;
    BatteryReader(const BatteryReader&) = delete;
    BatteryReader& operator=(const BatteryReader&) = delete;

    bool Open();
    bool ReadSample(BatterySample& sample) const noexcept;
    const BatteryMetadata& metadata() const noexcept { return metadata_; }
    DWORD last_error() const noexcept { return last_error_; }

private:
    bool ReadMetadata() noexcept;
    bool QueryInformation(ULONG level, void* output, DWORD output_size) const noexcept;

    UniqueHandle device_;
    BatteryMetadata metadata_{};
    bool temperature_supported_ = false;
    mutable DWORD last_error_ = ERROR_SUCCESS;
};

} // namespace battery_logger
