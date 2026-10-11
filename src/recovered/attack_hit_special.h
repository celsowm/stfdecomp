#ifndef STF_RECOVERED_ATTACK_HIT_SPECIAL_H
#define STF_RECOVERED_ATTACK_HIT_SPECIAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * sub_19888 -> calc_mht_adr(tag=5) -> sub_1980C, from the special
 * opponent[+0x19F]==0x12 corridor in attack_hit.
 *
 * The caller must resolve the actual tag-5 motion record using the
 * opponent's +0x19C motion id; the first four bytes of the record are
 * enough for the original unaligned little-endian halfword and byte loads.
 * No animation table pointer/address is assumed.
 */
enum {
    STF_ATTACK_HIT_SPECIAL_ATTACKER_MIN_SIZE = 0x840u,
    STF_ATTACK_HIT_SPECIAL_DEFENDER_MIN_SIZE = 0x1F8u,
    STF_ATTACK_HIT_SPECIAL_RECORD_MIN_SIZE = 4u
};

typedef struct stf_attack_hit_special_tgp_query {
    uint32_t opcode; /* 0x2D805B5B */
    int32_t heading_26; /* sign-extended ldis 0x26(g7) */
    uint32_t vector_80;
    uint32_t vector_88;
} stf_attack_hit_special_tgp_query;

typedef bool (*stf_attack_hit_special_tgp_fn)(
    void *user_data,
    const stf_attack_hit_special_tgp_query *request,
    uint32_t response_bits[2]
);

typedef struct stf_attack_hit_special_result {
    uint16_t motion_19c;
    uint16_t animation_halfword;
    uint8_t animation_byte_822;
    bool needed_tgp;
    bool cleared_motion_flag;
    uint32_t attacker_flags_000;
    uint32_t attacker_flags_1a4;
    uint32_t defender_flags_000;
} stf_attack_hit_special_result;

/*
 * Strict input validation before any state writes. If attacker+0x0 bit2
 * is set and +0x83F bit0 is clear, a real 0x2D805B5B response is
 * mandatory; the two result words update attacker +0x18 and +0x20
 * by subtracting the supplied float values. A missing or invalid
 * response rejects the operation without partial mutation.
 *
 * All other updates reproduce the recovered i960 stores in sub_19888
 * and sub_1980C, including signed-byte motion state and bit6 transfer.
 */
bool stf_attack_hit_special_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *tag5_record,
    size_t tag5_record_size,
    stf_attack_hit_special_tgp_fn tgp,
    void *tgp_user_data,
    stf_attack_hit_special_result *result
);

/*
 * Optional integration hook used by collision_combat_runtime. Return an
 * original tag-5 record for a supplied opponent +0x19C motion index.
 * The caller owns record lifetime at least through the apply call.
 */
typedef bool (*stf_attack_hit_special_record_fn)(
    void *user_data,
    uint16_t motion_id,
    const uint8_t **record,
    size_t *record_size
);

#endif
