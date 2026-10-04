#ifndef STF_RECOVERED_ATTACK_HIT_SIDE_EXIT_H
#define STF_RECOVERED_ATTACK_HIT_SIDE_EXIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_SIDE_EXIT_CLASSIFIER_ATTACKER_MIN_SIZE = 0x864u,
    STF_ATTACK_SIDE_EXIT_CLASSIFIER_DEFENDER_MIN_SIZE = 0x004u,
    STF_ATTACK_ABORT_CLEANUP_ATTACKER_MIN_SIZE = 0x1238u,
    STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE = 0x109u
};

typedef enum stf_attack_side_exit_phase {
    STF_ATTACK_SIDE_EXIT_GUARD_2AC74 = 0,
    STF_ATTACK_SIDE_EXIT_NORMAL_2AE40
} stf_attack_side_exit_phase;

typedef enum stf_attack_side_exit_path {
    STF_ATTACK_SIDE_EXIT_CONTINUE = 0,
    STF_ATTACK_SIDE_EXIT_ABORT_2B8C8
} stf_attack_side_exit_path;

typedef struct stf_attack_side_exit_result {
    stf_attack_side_exit_path path;
    bool cleared_defender_bit29;
    bool request_set_kamae;
} stf_attack_side_exit_result;

typedef struct stf_attack_abort_cleanup_result {
    uint32_t next_attacker_counter_1234;
    uint8_t enemy0_108;
    uint8_t enemy1_108;
} stf_attack_abort_cleanup_result;

/*
 * Recover the predicates that feed loc_2B8C8.
 *
 * GUARD_2AC74 models the short precheck at 0x2AC74..0x2AC84.
 * NORMAL_2AE40 models the main side-exit checks through _uk_display_hit_combo.
 * When defender bit 29 is consumed by the normal path, this adapter clears it
 * and reports set_kamae_ram as an explicit external event.
 */
bool stf_attack_hit_side_exit_apply_model2(
    stf_attack_side_exit_phase phase,
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    stf_attack_side_exit_result *result
);

/*
 * Recover loc_2B8C8 exactly: undo the attack_hit +0x1234 lifecycle counter and
 * clear both enemy +0x108 activity markers before returning.
 */
bool stf_attack_hit_abort_cleanup_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *enemy0,
    size_t enemy0_size,
    uint8_t *enemy1,
    size_t enemy1_size,
    stf_attack_abort_cleanup_result *result
);

#endif
