#ifndef STF_RECOVERED_ATTACK_HIT_H
#define STF_RECOVERED_ATTACK_HIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_HIT_FIGHTER_MODEL2_MIN_SIZE = 0x1238u,
    STF_ATTACK_HIT_OPPONENT_MODEL2_MIN_SIZE = 0x1A0u,
    STF_ATTACK_HIT_WORKSPACE_MODEL2_MIN_SIZE = 0x270u,
    STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE = 0x109u
};

typedef struct stf_attack_hit_prefix_result {
    int8_t hit_kind_843;
    int8_t hit_kind_83e;
    int16_t hit_flags_828;
    uint32_t next_workspace_26c;
    uint32_t next_fighter_counter_1234;
    uint8_t enemy_slot_108;
    bool opponent_override;
    bool special_opponent_path;
} stf_attack_hit_prefix_result;

/*
 * Recover the hardware-independent prefix of attack_hit beginning at the
 * original loc_2A858. The earlier game-mode gate and the special sub_19888
 * call are deliberately outside this contract.
 */
bool stf_attack_hit_prefix_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *enemy_slot,
    size_t enemy_slot_size,
    stf_attack_hit_prefix_result *result
);

#endif
