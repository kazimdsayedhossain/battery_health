# Battery Health Logger

Native Windows 10/11 C++17 battery discharge logger. It reads the Battery Class driver directly, writes one CSV record every seven seconds, and is designed to be asleep virtually all of the time.

## Build

Requirements: Visual Studio 2022 with the Desktop development with C++ workload, a Windows 10 or 11 SDK, and CMake 3.21 or newer.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\BatteryHealthLogger.exe
```

## Workflow

Run the program while unplugged. It opens `BatteryDischarge_active.csv` once and appends a flushed record every seven seconds. Do not close it during the discharge test. Windows shutdown, hibernation, or loss of power is expected. On the next run, it analyzes that active file, writes `BatteryDischarge_report.txt`, archives the CSV as `BatteryDischarge_YYYYMMDD_HHMMSS.csv`, then starts a new test.

The logger uses one process thread (the main thread), a non-periodic waitable timer, no high-resolution timing APIs, and no allocations in the sampling loop. It makes one grouped sampling pass per wake-up: status plus the optional temperature query. Capacity metadata is queried once at startup. It does not alter process priority, timer resolution, power configuration, affinity, registry, or networking.

## CSV fields

`Timestamp` is local wall-clock time. `Tag` identifies the battery instance. `Voltage_mV`, `Rate_mW`, and `RemainingCapacity_mWh` come from `IOCTL_BATTERY_QUERY_STATUS`; negative rate indicates discharge. `Power_mW` is the same driver-provided rate. `FullChargeCapacity_mWh`, `DesignCapacity_mWh`, and `CycleCount` are Battery Class metadata obtained at startup. `Temperature_dK` is tenths of Kelvin when supported, otherwise zero. `PowerStateFlags` is the native Battery Class bitfield; `ACConnected` and `Charging` decode its on-line and charging bits.

## Report methodology

Delivered energy is the sum of monotonic decreases in reported remaining capacity, excluding increases caused by charging or gauge recalibration. Battery health is delivered energy divided by design capacity. Therefore it is a conservative usable-energy estimate and is most meaningful after a mostly complete, uninterrupted discharge. Runtime is based on the fixed seven-second interval; average and peak power use negative (discharging) rates only.

## Notes

Some firmware does not expose temperature, cycle count, or valid capacity information. Those values are recorded as zero or omitted from the report calculation rather than causing a failure. The first detected Battery Class battery is used; this tool is intended for typical single-battery laptops.
