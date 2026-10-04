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
    "${STF_REPO_ROOT}/src/recovered/collision_ball.c"
    "${STF_REPO_ROOT}/src/recovered/collision_support.c"
    "${STF_REPO_ROOT}/src/recovered/collision_no_unit.c"
    "${STF_REPO_ROOT}/src/recovered/collision_attack.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_strength.c"
    "${STF_REPO_ROOT}/src/recovered/copro_scalar.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_guard.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_guard_state.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_guard_common.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_combo.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_damage.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_finish.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_reaction.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_normal_reaction.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_down_reaction.c"
    "${STF_REPO_ROOT}/src/recovered/motion_hit_table.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_motion_prefix.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_motion_vector.c"
    "${STF_REPO_ROOT}/src/recovered/hit_motion_selector.c"
    "${STF_REPO_ROOT}/src/recovered/damage_calculation.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_side_exit.c"
    "${STF_REPO_ROOT}/src/recovered/attack_hit_sound.c"
    "${STF_REPO_ROOT}/src/recovered/skill_accounting.c"
    "${STF_REPO_ROOT}/src/recovered/set_kamae_plan.c"
    "${STF_REPO_ROOT}/src/recovered/get_kamae_value.c"
    "${STF_REPO_ROOT}/src/recovered/ring_scatter_plan.c"
    "${STF_REPO_ROOT}/src/recovered/ring_scatter_runtime.c"
    "${STF_REPO_ROOT}/src/recovered/ring_scatter_damage_flow.c"
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
    target_link_libraries(stf_recovered_game PUBLIC m)
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

add_executable(stf_recovered_collision_ball_tests
    tests/test_recovered_collision_ball.c
)

target_link_libraries(stf_recovered_collision_ball_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_collision_support_tests
    tests/test_recovered_collision_support.c
)

target_link_libraries(stf_recovered_collision_support_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_collision_no_unit_tests
    tests/test_recovered_collision_no_unit.c
)

target_link_libraries(stf_recovered_collision_no_unit_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_collision_attack_tests
    tests/test_recovered_collision_attack.c
)

target_link_libraries(stf_recovered_collision_attack_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_tests
    tests/test_recovered_attack_hit.c
)

target_link_libraries(stf_recovered_attack_hit_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_strength_tests
    tests/test_recovered_attack_hit_strength.c
)

target_link_libraries(stf_recovered_attack_hit_strength_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_copro_scalar_tests
    tests/test_recovered_copro_scalar.c
)

target_link_libraries(stf_recovered_copro_scalar_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_guard_tests
    tests/test_recovered_attack_hit_guard.c
)

target_link_libraries(stf_recovered_attack_hit_guard_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_guard_state_tests
    tests/test_recovered_attack_hit_guard_state.c
)

target_link_libraries(stf_recovered_attack_hit_guard_state_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_guard_common_tests
    tests/test_recovered_attack_hit_guard_common.c
)

target_link_libraries(stf_recovered_attack_hit_guard_common_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_combo_tests
    tests/test_recovered_attack_hit_combo.c
)

target_link_libraries(stf_recovered_attack_hit_combo_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_damage_tests
    tests/test_recovered_attack_hit_damage.c
)

target_link_libraries(stf_recovered_attack_hit_damage_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_finish_tests
    tests/test_recovered_attack_hit_finish.c
)

target_link_libraries(stf_recovered_attack_hit_finish_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_reaction_tests
    tests/test_recovered_attack_hit_reaction.c
)

target_link_libraries(stf_recovered_attack_hit_reaction_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_normal_reaction_tests
    tests/test_recovered_attack_hit_normal_reaction.c
)

target_link_libraries(stf_recovered_attack_hit_normal_reaction_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_down_reaction_tests
    tests/test_recovered_attack_hit_down_reaction.c
)

target_link_libraries(stf_recovered_attack_hit_down_reaction_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_motion_hit_table_tests
    tests/test_recovered_motion_hit_table.c
)

target_link_libraries(stf_recovered_motion_hit_table_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_motion_prefix_tests
    tests/test_recovered_attack_hit_motion_prefix.c
)

target_link_libraries(stf_recovered_attack_hit_motion_prefix_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_motion_vector_tests
    tests/test_recovered_attack_hit_motion_vector.c
)

target_link_libraries(stf_recovered_attack_hit_motion_vector_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_hit_motion_selector_tests
    tests/test_recovered_hit_motion_selector.c
)

target_link_libraries(stf_recovered_hit_motion_selector_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_damage_calculation_tests
    tests/test_recovered_damage_calculation.c
)

target_link_libraries(stf_recovered_damage_calculation_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_side_exit_tests
    tests/test_recovered_attack_hit_side_exit.c
)

target_link_libraries(stf_recovered_attack_hit_side_exit_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_flow_tests
    tests/test_recovered_attack_hit_flow.c
)

target_link_libraries(stf_recovered_attack_hit_flow_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_attack_hit_sound_tests
    tests/test_recovered_attack_hit_sound.c
)

target_link_libraries(stf_recovered_attack_hit_sound_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_skill_accounting_tests
    tests/test_recovered_skill_accounting.c
)

target_link_libraries(stf_recovered_skill_accounting_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_set_kamae_plan_tests
    tests/test_recovered_set_kamae_plan.c
)

target_link_libraries(stf_recovered_set_kamae_plan_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_get_kamae_value_tests
    tests/test_recovered_get_kamae_value.c
)

target_link_libraries(stf_recovered_get_kamae_value_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_ring_scatter_plan_tests
    tests/test_recovered_ring_scatter_plan.c
)

target_link_libraries(stf_recovered_ring_scatter_plan_tests PRIVATE stf_recovered_game)

add_executable(stf_recovered_ring_scatter_runtime_tests
    tests/test_recovered_ring_scatter_runtime.c
)

target_link_libraries(stf_recovered_ring_scatter_runtime_tests PRIVATE stf_recovered_game)

enable_testing()
add_test(NAME stf_recovery_i960_tests COMMAND stf_recovery_i960_tests)
add_test(NAME stf_i960_corridor_help COMMAND stf_i960_corridor --help)
add_test(NAME stf_recovery_bus_trace_tests COMMAND stf_recovery_bus_trace_tests)
add_test(NAME stf_recovery_model2b_memory_tests COMMAND stf_recovery_model2b_memory_tests)
add_test(NAME stf_recovered_coli_tests COMMAND stf_recovered_coli_tests)
add_test(NAME stf_recovered_collision_tests COMMAND stf_recovered_collision_tests)
add_test(NAME stf_recovered_collision_ball_tests COMMAND stf_recovered_collision_ball_tests)
add_test(NAME stf_recovered_collision_support_tests COMMAND stf_recovered_collision_support_tests)
add_test(NAME stf_recovered_collision_no_unit_tests COMMAND stf_recovered_collision_no_unit_tests)
add_test(NAME stf_recovered_collision_attack_tests COMMAND stf_recovered_collision_attack_tests)
add_test(NAME stf_recovered_attack_hit_tests COMMAND stf_recovered_attack_hit_tests)
add_test(NAME stf_recovered_attack_hit_strength_tests COMMAND stf_recovered_attack_hit_strength_tests)
add_test(NAME stf_recovered_copro_scalar_tests COMMAND stf_recovered_copro_scalar_tests)
add_test(NAME stf_recovered_attack_hit_guard_tests COMMAND stf_recovered_attack_hit_guard_tests)
add_test(NAME stf_recovered_attack_hit_guard_state_tests COMMAND stf_recovered_attack_hit_guard_state_tests)
add_test(NAME stf_recovered_attack_hit_guard_common_tests COMMAND stf_recovered_attack_hit_guard_common_tests)
add_test(NAME stf_recovered_attack_hit_combo_tests COMMAND stf_recovered_attack_hit_combo_tests)
add_test(NAME stf_recovered_attack_hit_damage_tests COMMAND stf_recovered_attack_hit_damage_tests)
add_test(NAME stf_recovered_attack_hit_finish_tests COMMAND stf_recovered_attack_hit_finish_tests)
add_test(NAME stf_recovered_attack_hit_reaction_tests COMMAND stf_recovered_attack_hit_reaction_tests)
add_test(NAME stf_recovered_attack_hit_normal_reaction_tests COMMAND stf_recovered_attack_hit_normal_reaction_tests)
add_test(NAME stf_recovered_attack_hit_down_reaction_tests COMMAND stf_recovered_attack_hit_down_reaction_tests)
add_test(NAME stf_recovered_motion_hit_table_tests COMMAND stf_recovered_motion_hit_table_tests)
add_test(NAME stf_recovered_attack_hit_motion_prefix_tests COMMAND stf_recovered_attack_hit_motion_prefix_tests)
add_test(NAME stf_recovered_attack_hit_motion_vector_tests COMMAND stf_recovered_attack_hit_motion_vector_tests)
add_test(NAME stf_recovered_hit_motion_selector_tests COMMAND stf_recovered_hit_motion_selector_tests)
add_test(NAME stf_recovered_damage_calculation_tests COMMAND stf_recovered_damage_calculation_tests)
add_test(NAME stf_recovered_attack_hit_side_exit_tests COMMAND stf_recovered_attack_hit_side_exit_tests)
add_test(NAME stf_recovered_attack_hit_flow_tests COMMAND stf_recovered_attack_hit_flow_tests)
add_test(NAME stf_recovered_attack_hit_sound_tests COMMAND stf_recovered_attack_hit_sound_tests)
add_test(NAME stf_recovered_skill_accounting_tests COMMAND stf_recovered_skill_accounting_tests)
add_test(NAME stf_recovered_set_kamae_plan_tests COMMAND stf_recovered_set_kamae_plan_tests)
add_test(NAME stf_recovered_get_kamae_value_tests COMMAND stf_recovered_get_kamae_value_tests)
add_test(NAME stf_recovered_ring_scatter_plan_tests COMMAND stf_recovered_ring_scatter_plan_tests)
add_test(NAME stf_recovered_ring_scatter_runtime_tests COMMAND stf_recovered_ring_scatter_runtime_tests)
