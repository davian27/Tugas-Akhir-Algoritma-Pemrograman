cmake_minimum_required(VERSION 3.16)
project(PowerQualityAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

set(ANALYSIS_SOURCES
    src/AnalysisDatabase.cpp
    src/DataLoader.cpp
    src/SignalAnalysis.cpp
    src/PowerAnalysis.cpp
    src/DisturbanceDetector.cpp
    src/ReportManager.cpp
)

if(WIN32)
    add_executable(PowerQualityAnalyzer WIN32 ${ANALYSIS_SOURCES} src/gui_main.cpp)
    target_link_libraries(PowerQualityAnalyzer PRIVATE comdlg32 SQLite::SQLite3)
    if(MINGW)
        target_link_options(PowerQualityAnalyzer PRIVATE -municode)
    endif()
else()
    add_executable(PowerQualityAnalyzer ${ANALYSIS_SOURCES} src/Config.cpp src/Tests.cpp src/main.cpp)
    target_link_libraries(PowerQualityAnalyzer PRIVATE SQLite::SQLite3)
endif()
