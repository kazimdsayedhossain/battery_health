cmake_minimum_required(VERSION 3.21)
project(BatteryHealthLogger LANGUAGES CXX)

add_executable(BatteryHealthLogger
    main.cpp
    BatteryReader.cpp
    Logger.cpp
    Analyzer.cpp
    Utils.cpp)

target_compile_features(BatteryHealthLogger PRIVATE cxx_std_17)
target_compile_definitions(BatteryHealthLogger PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(BatteryHealthLogger PRIVATE /W4 /permissive- /EHsc)
target_link_libraries(BatteryHealthLogger PRIVATE setupapi)
