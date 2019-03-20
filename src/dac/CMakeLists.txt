add_library(dac INTERFACE)
target_sources(dac INTERFACE
        ${PROJECT_SOURCE_DIR}/include/dac/dac.h
        ${PROJECT_SOURCE_DIR}/include/dac/scheduler.h
        ${PROJECT_SOURCE_DIR}/src/dac/scheduler.cpp
        ${PROJECT_SOURCE_DIR}/src/dac/sync_job_list.cpp
        ${PROJECT_SOURCE_DIR}/src/dac/worker.cpp
)

target_include_directories(dac INTERFACE ${PROJECT_SOURCE_DIR}/include/dac)
