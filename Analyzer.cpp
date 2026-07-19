#include "Analyzer.h"

#include <cstdio>
#include <cstring>
#include <cmath>

namespace battery_logger {

namespace {
struct Summary {
    unsigned long samples = 0;
    double delivered_mwh = 0.0;
    double rate_sum_mw = 0.0;
    unsigned long discharge_rate_samples = 0;
    double peak_discharge_mw = 0.0;
    unsigned long min_voltage_mv = 0;
    unsigned long max_temperature_dk = 0;
    unsigned long design_capacity_mwh = 0;
};
}

bool AnalyzeLog(const wchar_t* csv_path, const wchar_t* report_path) noexcept {
    FILE* input = nullptr;
    if (_wfopen_s(&input, csv_path, L"rb") != 0) { return false; }
    char line[512]{};
    std::fgets(line, sizeof(line), input); // Header.
    Summary summary{};
    unsigned long previous_capacity = 0;
    bool have_previous = false;
    while (std::fgets(line, sizeof(line), input) != nullptr) {
        char timestamp[32]{};
        unsigned long tag, voltage, remaining, full, design, cycles, temperature, flags, ac, charging;
        long rate, power;
        const int parsed = sscanf_s(line, "%31[^,],%lu,%lu,%ld,%ld,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu",
            timestamp, static_cast<unsigned>(sizeof(timestamp)), &tag, &voltage, &rate, &power, &remaining, &full, &design, &cycles,
            &temperature, &flags, &ac, &charging);
        if (parsed != 13) { continue; }
        if (!have_previous) { summary.min_voltage_mv = voltage; have_previous = true; }
        summary.min_voltage_mv = (voltage < summary.min_voltage_mv) ? voltage : summary.min_voltage_mv;
        summary.max_temperature_dk = (temperature > summary.max_temperature_dk) ? temperature : summary.max_temperature_dk;
        if (rate < 0) {
            const double discharge = static_cast<double>(-rate);
            summary.rate_sum_mw += discharge;
            ++summary.discharge_rate_samples;
            summary.peak_discharge_mw = discharge > summary.peak_discharge_mw ? discharge : summary.peak_discharge_mw;
        }
        if (have_previous && previous_capacity > remaining) { summary.delivered_mwh += previous_capacity - remaining; }
        previous_capacity = remaining;
        summary.design_capacity_mwh = design;
        ++summary.samples;
    }
    std::fclose(input);
    if (summary.samples < 2) { return false; }
    FILE* report = nullptr;
    if (_wfopen_s(&report, report_path, L"wb") != 0) { return false; }
    const double hours = (summary.samples - 1) * 7.0 / 3600.0;
    const double average_power = summary.discharge_rate_samples == 0 ? 0.0 :
        summary.rate_sum_mw / summary.discharge_rate_samples;
    const double health = summary.design_capacity_mwh == 0 ? 0.0 : (summary.delivered_mwh / summary.design_capacity_mwh) * 100.0;
    const double celsius = summary.max_temperature_dk == 0 ? 0.0 : (summary.max_temperature_dk / 10.0) - 273.15;
    std::fprintf(report,
        "Battery discharge report\r\n"
        "========================\r\n"
        "Samples: %lu\r\nTotal runtime: %.2f hours\r\nEnergy delivered: %.2f Wh\r\n"
        "Average discharge power: %.0f mW\r\nPeak discharge power: %.0f mW\r\n"
        "Minimum voltage: %lu mV\r\nMaximum temperature: %.1f C\r\n"
        "Average discharge rate: %.0f mW\r\nBattery health: %.1f%%\r\n\r\n"
        "Method: energy delivered is the sum of decreases in reported remaining capacity. Health is delivered energy divided by design capacity. Charging intervals and rises in capacity are excluded.\r\n",
        summary.samples, hours, summary.delivered_mwh / 1000.0, average_power, summary.peak_discharge_mw,
        summary.min_voltage_mv, celsius, average_power, health);
    return std::fclose(report) == 0;
}

} // namespace battery_logger
