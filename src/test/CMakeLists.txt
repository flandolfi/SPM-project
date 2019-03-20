find_package(Threads REQUIRED)
add_executable(mergesort mergesort_dac.cpp)
target_link_libraries(mergesort Threads::Threads dac utils)