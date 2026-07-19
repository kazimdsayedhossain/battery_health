#include "Analyzer.h"
#include "BatteryReader.h"
#include "Logger.h"
#include "Utils.h"

#include <windows.h>
#include <cstdio>

namespace {
constexpr wchar_t kActiveLog[] = L"BatteryDischarge_active.csv";
constexpr wchar_t kReport[] = L"BatteryDischarge_report.txt";
constexpr DWORD kSampleIntervalMs = 7000;
}

int wmain() {
    using namespace battery_logger;
    if (ActiveLogExists(kActiveLog)) {
        std::puts("Previous battery test found; generating its report.");
        const bool report_written = AnalyzeLog(kActiveLog, kReport);
        if (!ArchiveActiveLog(kActiveLog, L".")) {
            std::fputs("Could not finalize the previous test. Resolve the file error and run again.\n", stderr);
            return 1;
        }
        if (report_written) {
            std::puts("Previous test finalized. See BatteryDischarge_report.txt.");
        } else {
            std::puts("Previous log had too few valid samples for a report and was archived.");
        }
        return 0;
    }

    BatteryReader reader;
    if (!reader.Open()) {
        std::fprintf(stderr, "No accessible battery was found (Windows error %lu).\n", reader.last_error());
        return 1;
    }
    Logger logger;
    if (!logger.OpenNew(kActiveLog, reader.metadata())) {
        std::fputs("Could not create the battery log.\n", stderr);
        return 1;
    }
    UniqueHandle timer(CreateWaitableTimer(nullptr, FALSE, nullptr));
    if (!timer) {
        std::fprintf(stderr, "Could not create the waitable timer (Windows error %lu).\n", GetLastError());
        return 1;
    }
    std::puts("Battery discharge logging started. Leave this program running until the next boot.");
    // Take one startup reading, then let the kernel schedule all subsequent wakes.
    BatterySample initial_sample{};
    if (!reader.ReadSample(initial_sample) || !logger.WriteSample(initial_sample)) {
        std::fprintf(stderr, "Initial sample failed (Windows error %lu).\n", reader.last_error());
        return 1;
    }
    LARGE_INTEGER due_time{};
    due_time.QuadPart = -static_cast<LONGLONG>(kSampleIntervalMs) * 10000LL;
    if (!SetWaitableTimer(timer.get(), &due_time, kSampleIntervalMs, nullptr, nullptr, FALSE)) {
        std::fprintf(stderr, "Could not arm the waitable timer (Windows error %lu).\n", GetLastError());
        return 1;
    }
    for (;;) {
        if (WaitForSingleObject(timer.get(), INFINITE) != WAIT_OBJECT_0) {
            std::fprintf(stderr, "Timer wait failed (Windows error %lu).\n", GetLastError());
            return 1;
        }
        BatterySample sample{};
        if (!reader.ReadSample(sample) || !logger.WriteSample(sample)) {
            std::fprintf(stderr, "Sampling stopped (Windows error %lu).\n", reader.last_error());
            return 1;
        }
    }
}
