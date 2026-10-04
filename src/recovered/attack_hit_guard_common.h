#ifndef STF_RECOVERED_ATTACK_HIT_GUARD_COMMON_H
#define STF_RECOVERED_ATTACK_HIT_GUARD_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_GUARD_COMMON_ATTACKER_MIN_SIZE = 0x124Cu,
    STF_GUARD_COMMON_DEFENDER_MIN_SIZE = 0xC74u,
    STF_GUARD_COMMON_WORKSPACE_MIN_SIZE = 0x270u,
    STF_GUARD_COMMON_ENEMY_MIN_SIZE = 0x10Au
};

typedef struct stf_guard_common_result {
    bool skipped;
    uint32_t attacker_counter_1234;
    uint32_t attacker_counter_1238;
    uint8_t enemy_109;
    uint32_t skill_amount;
    uint32_t attacker_194;
    uint32_t defender_198;
    int16_t defender_6d8;
    int16_t defender_5de;
    uint32_t workspace_26c;
    bool copy_122x;
    bool copy_124x;
    bool requires_sub_2b94c;
} stf_guard_common_result;

/*
 * Recover attack_hit common guard path 0x2AC74..0x2AE28.
 *
 * sub_2B94C remains an external table/motion lookup; its resulting g0 is
 * supplied by the caller. Sound/skill calls are represented as result fields.
 */
bool stf_attack_hit_guard_common_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *enemy_slot,
    size_t enemy_slot_size,
    uint8_t hit_flags_50fe03,
    uint8_t guard_limit,
    uint32_t motion_g0,
    stf_guard_common_result *result
);

#endif
