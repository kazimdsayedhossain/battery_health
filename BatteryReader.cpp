#include "BatteryReader.h"

#include <initguid.h>
#include <batclass.h>
#include <setupapi.h>
#include <vector>

namespace battery_logger {

bool BatteryReader::Open() {
    HDEVINFO devices = SetupDiGetClassDevs(&GUID_DEVINTERFACE_BATTERY, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) { last_error_ = GetLastError(); return false; }

    bool opened = false;
    for (DWORD index = 0; !opened; ++index) {
        SP_DEVICE_INTERFACE_DATA interface_data{};
        interface_data.cbSize = sizeof(interface_data);
        if (!SetupDiEnumDeviceInterfaces(devices, nullptr, &GUID_DEVINTERFACE_BATTERY, index, &interface_data)) {
            last_error_ = GetLastError();
            break;
        }

        DWORD bytes = 0;
        SetupDiGetDeviceInterfaceDetail(devices, &interface_data, nullptr, 0, &bytes, nullptr);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)) {
            last_error_ = GetLastError();
            continue;
        }
        std::vector<std::byte> detail_storage(bytes);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(detail_storage.data());
        detail->cbSize = sizeof(*detail);
        if (!SetupDiGetDeviceInterfaceDetail(devices, &interface_data, detail, bytes, nullptr, nullptr)) {
            last_error_ = GetLastError();
            continue;
        }
        device_.reset(CreateFile(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
        if (device_) { opened = ReadMetadata(); }
    }
    SetupDiDestroyDeviceInfoList(devices);
    return opened;
}

bool BatteryReader::QueryInformation(ULONG level, void* output, DWORD output_size) const noexcept {
    BATTERY_QUERY_INFORMATION query{};
    query.BatteryTag = metadata_.tag;
    query.InformationLevel = static_cast<BATTERY_QUERY_INFORMATION_LEVEL>(level);
    DWORD returned = 0;
    if (!DeviceIoControl(device_.get(), IOCTL_BATTERY_QUERY_INFORMATION, &query, sizeof(query),
            output, output_size, &returned, nullptr)) {
        last_error_ = GetLastError();
        return false;
    }
    return true;
}

bool BatteryReader::ReadMetadata() noexcept {
    DWORD returned = 0;
    if (!DeviceIoControl(device_.get(), IOCTL_BATTERY_QUERY_TAG, nullptr, 0, &metadata_.tag,
            sizeof(metadata_.tag), &returned, nullptr) || metadata_.tag == 0) {
        last_error_ = GetLastError();
        return false;
    }
    BATTERY_INFORMATION information{};
    if (!QueryInformation(BatteryInformation, &information, sizeof(information))) { return false; }
    metadata_.full_charge_capacity_mwh = information.FullChargedCapacity;
    metadata_.design_capacity_mwh = information.DesignedCapacity;
    metadata_.cycle_count = information.CycleCount;
    ULONG temperature = 0;
    // Probe optional firmware support once; never repeat an unsupported IOCTL in the loop.
    temperature_supported_ = QueryInformation(BatteryTemperature, &temperature, sizeof(temperature));
    last_error_ = ERROR_SUCCESS;
    return true;
}

bool BatteryReader::ReadSample(BatterySample& sample) const noexcept {
    BATTERY_STATUS status{};
    DWORD returned = 0;
    if (!DeviceIoControl(device_.get(), IOCTL_BATTERY_QUERY_STATUS, &metadata_.tag, sizeof(metadata_.tag),
            &status, sizeof(status), &returned, nullptr)) {
        last_error_ = GetLastError();
        return false;
    }
    sample.voltage_mv = status.Voltage;
    sample.rate_mw = static_cast<std::int32_t>(status.Rate);
    sample.remaining_capacity_mwh = status.Capacity;
    sample.flags = status.PowerState;
    sample.temperature_available = temperature_supported_ && QueryInformation(BatteryTemperature,
        &sample.temperature_deci_kelvin, sizeof(sample.temperature_deci_kelvin));
    return true;
}

} // namespace battery_logger
