add_library(utils INTERFACE)
target_sources(utils INTERFACE
        ${PROJECT_SOURCE_DIR}/external/DAC/includes/utils.h
        ${PROJECT_SOURCE_DIR}/external/DAC/src/utils.cpp
)

target_include_directories(utils INTERFACE ${PROJECT_SOURCE_DIR}/external/DAC/includes/)
