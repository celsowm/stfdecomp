#ifndef STF_RECOVERED_ATTACK_HIT_SOUND_H
#define STF_RECOVERED_ATTACK_HIT_SOUND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_HIT_SOUND_ATTACKER_MIN_SIZE = 0x824u,
    STF_ATTACK_HIT_SOUND_DEFENDER_MIN_SIZE = 0x1B2u
};

typedef enum stf_attack_hit_sound_kind {
    STF_ATTACK_HIT_SOUND_NONE = 0,
    STF_ATTACK_HIT_SOUND_LIST,
    STF_ATTACK_HIT_SOUND_SINGLE
} stf_attack_hit_sound_kind;

typedef enum stf_attack_hit_sound_source {
    STF_ATTACK_HIT_SOUND_SOURCE_NONE = 0,
    STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBE44,
    STF_ATTACK_HIT_SOUND_SOURCE_AUDIO_LIST,
    STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBF4C,
    STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB6F4,
    STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB964
} stf_attack_hit_sound_source;

typedef enum stf_attack_hit_sound_tier {
    STF_ATTACK_HIT_SOUND_TIER_LIGHT = 0,
    STF_ATTACK_HIT_SOUND_TIER_MEDIUM,
    STF_ATTACK_HIT_SOUND_TIER_HEAVY
} stf_attack_hit_sound_tier;

typedef struct stf_attack_hit_sound_plan {
    stf_attack_hit_sound_kind kind;
    stf_attack_hit_sound_source source;
    stf_attack_hit_sound_tier tier;
    uint32_t source_index;
    bool zero_terminated_list;
} stf_attack_hit_sound_plan;

/*
 * Recover the sound-selection logic at 0x2B33C..ah_hit_sd_end (0x2B3FC).
 *
 * No sound table is dereferenced here. The result describes the exact original
 * table family and index the i960 selected, allowing a port/runtime to resolve
 * that event through its own audio backend.
 *
 * For LIST plans source_index is attacker[0x823]-1 and the selected table entry
 * is a zero-terminated uint32_t sound-id list.
 *
 * For SINGLE plans source_index is:
 *   defender[0x1B0] * 3 + tier
 * where tier is light for damage 0..14, medium for 15..29, heavy for 30+.
 */
bool stf_attack_hit_sound_plan_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    stf_attack_hit_sound_plan *result
);

#endif
