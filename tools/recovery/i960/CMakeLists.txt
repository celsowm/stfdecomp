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
    src/model2b_map.c
    src/executor.c
    src/executor_arch.c
)

target_include_directories(stf_recovery_i960
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

get_filename_component(
    STF_REPO_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../.."
    ABSOLUTE
)

add_library(stf_recovered_game STATIC
    "${STF_REPO_ROOT}/src/recovered/coli_init.c"
    "${STF_REPO_ROOT}/src/recovered/collision_finish.c"
)

target_include_directories(stf_recovered_game
    PUBLIC
        "${STF_REPO_ROOT}/src/recovered"
)

if(MSVC)
    target_compile_options(stf_recovery_i960 PRIVATE /W4)
    target_compile_options(stf_recovered_game PRIVATE /W4)
else()
    target_compile_options(stf_recovery_i960 PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(stf_recovered_game PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(stf_recovery_i960 PUBLIC m)
endif()

add_executable(stf_i960_probe
    tools/stf_i960_probe.c
)

target_link_libraries(stf_i960_probe PRIVATE stf_recovery_i960)


add_executable(stf_i960_corridor
    tools/stf_i960_corridor.c
)

target_link_libraries(stf_i960_corridor PRIVATE stf_recovery_i960)

add_executable(stf_recovery_i960_tests
    tests/test_i960_core.c
)

target_link_libraries(stf_recovery_i960_tests PRIVATE stf_recovery_i960)

add_executable(stf_recovery_bus_trace_tests
    tests/test_bus_trace.c
)

target_link_libraries(stf_recovery_bus_trace_tests PRIVATE stf_recovery_i960)

add_executable(stf_recovery_model2b_memory_tests
    tests/test_model2b_memory.c
)

target_link_libraries(stf_recovery_model2b_memory_tests PRIVATE stf_recovery_i960)

add_executable(stf_recovered_coli_tests
    tests/test_recovered_coli.c
)

target_link_libraries(stf_recovered_coli_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_collision_tests
    tests/test_recovered_collision.c
)

target_link_libraries(stf_recovered_collision_tests PRIVATE stf_recovered_game)

enable_testing()
add_test(NAME stf_recovery_i960_tests COMMAND stf_recovery_i960_tests)
add_test(NAME stf_i960_corridor_help COMMAND stf_i960_corridor --help)
add_test(NAME stf_recovery_bus_trace_tests COMMAND stf_recovery_bus_trace_tests)
add_test(NAME stf_recovery_model2b_memory_tests COMMAND stf_recovery_model2b_memory_tests)
add_test(NAME stf_recovered_coli_tests COMMAND stf_recovered_coli_tests)
add_test(NAME stf_recovered_collision_tests COMMAND stf_recovered_collision_tests)
