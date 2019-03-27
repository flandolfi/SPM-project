add_library(utils
        ${PROJECT_SOURCE_DIR}/external/DAC/includes/utils.h
        ${PROJECT_SOURCE_DIR}/external/DAC/src/utils.cpp
)

target_include_directories(utils INTERFACE ${PROJECT_SOURCE_DIR}/external/DAC/includes/)

find_package(Threads REQUIRED)

add_executable(mergesort_dac mergesort_dac.cpp)
target_link_libraries(mergesort_dac Threads::Threads dac utils)

add_executable(quicksort_dac quicksort_dac.cpp)
target_link_libraries(quicksort_dac Threads::Threads dac utils)

set(FF_PATH /usr/local/fastflow)

if (EXISTS ${FF_PATH})
    add_executable(mergesort_ff mergesort_dac.cpp ${FF_PATH}/ff/dc.hpp)
    set_target_properties(mergesort_ff PROPERTIES COMPILE_FLAGS "-DUSE_FF -I${FF_PATH}")
    target_link_libraries(mergesort_ff Threads::Threads dac utils)

    add_executable(quicksort_ff quicksort_dac.cpp ${FF_PATH}/ff/dc.hpp)
    set_target_properties(quicksort_ff PROPERTIES COMPILE_FLAGS "-DUSE_FF -I${FF_PATH}")
    target_link_libraries(quicksort_ff Threads::Threads dac utils)
else()
    message("-- FF not found, skipping target")
endif ()

find_library(TBB_FOUND tbb PATH ENV LD_LIBRARY_PATH)

if (TBB_FOUND)
    add_executable(mergesort_tbb mergesort_dac.cpp)
    set_target_properties(mergesort_tbb PROPERTIES COMPILE_FLAGS -DUSE_TBB)
    target_link_libraries(mergesort_tbb Threads::Threads tbb dac utils)

    add_executable(quicksort_tbb quicksort_dac.cpp)
    set_target_properties(quicksort_tbb PROPERTIES COMPILE_FLAGS -DUSE_TBB)
    target_link_libraries(quicksort_tbb Threads::Threads tbb dac utils)
else()
    message("-- TBB not found, skipping target")
endif ()

find_package(OpenMP)

if (OPENMP_FOUND)
    add_executable(mergesort_omp mergesort_dac.cpp)
    set_target_properties(mergesort_omp PROPERTIES COMPILE_FLAGS "-fopenmp -DUSE_OMP")
    target_link_libraries(mergesort_omp Threads::Threads OpenMP::OpenMP_CXX dac utils)

    add_executable(quicksort_omp quicksort_dac.cpp)
    set_target_properties(quicksort_omp PROPERTIES COMPILE_FLAGS "-fopenmp -DUSE_OMP")
    target_link_libraries(quicksort_omp Threads::Threads OpenMP::OpenMP_CXX dac utils)
else()
    message("-- OMP not found, skipping target")
endif ()
