#ifndef STF_RECOVERED_SKILL_ACCOUNTING_H
#define STF_RECOVERED_SKILL_ACCOUNTING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct stf_skill_accounting_result {
    bool applied;
    uint32_t selected_flag;
    uint32_t total_skill_after;
} stf_skill_accounting_result;

/*
 * Portable recovery of total_skill_adder_g7/total_skill_adder_g8.
 *
 * The caller supplies fighter_slot as the value at fighter + 4. Slot 0 selects
 * select0_flag; every non-zero slot selects select1_flag, exactly as the i960
 * code does.
 *
 * Skill is accumulated only when:
 *   rank_mode bit 0 is set,
 *   rank_mode bit 7 is clear, and
 *   selected select{0,1}_flag bit 2 is set.
 *
 * amount and total_skill_before are represented as raw 32-bit register values;
 * addition therefore intentionally follows modulo-2^32 register arithmetic.
 */
bool stf_total_skill_add(
    uint16_t rank_mode,
    uint32_t select0_flag,
    uint32_t select1_flag,
    uint8_t fighter_slot,
    uint32_t amount,
    uint32_t total_skill_before,
    stf_skill_accounting_result *result
);

#endif
