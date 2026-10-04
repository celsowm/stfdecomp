#ifndef STF_RECOVERED_ATTACK_HIT_SOUND_RUNTIME_H
#define STF_RECOVERED_ATTACK_HIT_SOUND_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_sound.h"

enum {
    STF_ATTACK_HIT_SOUND_MAX_IDS = 32u
};

typedef struct stf_attack_hit_sound_resolved {
    size_t id_count;
    uint32_t ids[STF_ATTACK_HIT_SOUND_MAX_IDS];
} stf_attack_hit_sound_resolved;

/*
 * Resolve a recovered attack-hit sound plan against caller-supplied bytes from
 * the selected original table family.
 *
 * SINGLE plans address uint32_t entries by source_index.
 * LIST plans treat source_index as a byte offset from the table base and read a
 * zero-terminated uint32_t ID sequence.
 *
 * This helper deliberately stops at IDs. Playback/timing belongs to the target
 * audio backend.
 */
bool stf_attack_hit_sound_resolve(
    const stf_attack_hit_sound_plan *plan,
    const uint8_t *table_bytes,
    size_t table_size,
    stf_attack_hit_sound_resolved *result
);

#endif
