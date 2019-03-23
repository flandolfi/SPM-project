add_library(utils
        ${PROJECT_SOURCE_DIR}/external/DAC/includes/utils.h
        ${PROJECT_SOURCE_DIR}/external/DAC/src/utils.cpp
        )

target_include_directories(utils INTERFACE ${PROJECT_SOURCE_DIR}/external/DAC/includes/)

find_package(Threads REQUIRED)
add_executable(mergesort mergesort_dac.cpp)
target_link_libraries(mergesort Threads::Threads dac utils)