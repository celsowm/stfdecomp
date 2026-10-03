cmake_minimum_required(VERSION 3.16)

project(stf_recovery_i960 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(stf_recovery_i960 STATIC
    src/status.c
    src/instruction.c
    src/decoder.c
    src/formatter.c
    src/bus.c
    src/model2b_bus.c
    src/executor.c
    src/executor_arch.c
)

target_include_directories(stf_recovery_i960
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(stf_recovery_i960 PRIVATE /W4)
else()
    target_compile_options(stf_recovery_i960 PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(stf_recovery_i960 PUBLIC m)
endif()

add_executable(stf_i960_probe
    tools/stf_i960_probe.c
)

target_link_libraries(stf_i960_probe PRIVATE stf_recovery_i960)

add_executable(stf_recovery_i960_tests
    tests/test_i960_core.c
)

target_link_libraries(stf_recovery_i960_tests PRIVATE stf_recovery_i960)

enable_testing()
add_test(NAME stf_recovery_i960_tests COMMAND stf_recovery_i960_tests)
