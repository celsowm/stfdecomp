#ifndef STF_RECOVERED_ATTACK_HIT_GUARD_STATE_H
#define STF_RECOVERED_ATTACK_HIT_GUARD_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum stf_guard_sound {
    STF_GUARD_SOUND_NONE = 0,
    STF_GUARD_SOUND_KNOCK_9,
    STF_GUARD_SOUND_KNOCK_3
} stf_guard_sound;

typedef enum stf_guard_block_kind {
    STF_GUARD_BLOCK_A = 0,
    STF_GUARD_BLOCK_B = 1
} stf_guard_block_kind;

enum {
    STF_GUARD_STATE_ATTACKER_MIN_SIZE = 0x124Cu,
    STF_GUARD_STATE_DEFENDER_MIN_SIZE = 0xC74u,
    STF_GUARD_STATE_WORKSPACE_MIN_SIZE = 0x270u
};

typedef struct stf_guard_state_result {
    uint32_t workspace_26c;
    int32_t defender_c70;
    uint32_t attacker_194;
    uint32_t defender_198;
    int16_t defender_6d8;
    int16_t defender_5de;
    stf_guard_sound sound;
    bool copy_122x;
    bool copy_124x;
    bool requires_sub_2b94c;
} stf_guard_state_result;

/*
 * Recover the CPU-side state mutations shared by attack_hit guard blocks
 * 0x2AA70 and 0x2AB54. Sound is reported as an event instead of played.
 * Block B reports (but does not execute) the external sub_2B94C call.
 */
bool stf_attack_hit_guard_apply_model2(
    stf_guard_block_kind kind,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint16_t hit_flags_50fe00,
    stf_guard_state_result *result
);

#endif
