#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "attack_hit_guard_vector.h"
#include "attack_hit_motion_vector.h"
#include "tgp_command_trace.h"

enum { F_SIZE = 0x900u };

static uint32_t bits(float value)
{
    uint32_t raw = 0u;
    memcpy(&raw, &value, sizeof(raw));
    return raw;
}

static void wr32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
        ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

/* Test-only deterministic fake. It is NOT a hardware capture or
 * an emulator implementation of the original coprocessor. */
typedef struct fixture {
    unsigned calls;
    unsigned reject_on_call;
    unsigned nonfinite_on_call;
    float reply_base;
} fixture;

static bool source(
    void *userdata, const stf_attack_hit_guard_vector_query *request,
    uint32_t *response
)
{
    fixture *f = (fixture *)userdata;
    if (f == NULL || request == NULL || response == NULL) return false;
    ++f->calls;
    if (request->argument_count < 1u || request->argument_count > 2u ||
        f->calls == f->reject_on_call) return false;
    *response = f->calls == f->nonfinite_on_call
        ? UINT32_C(0x7FC00000)
        : bits(f->reply_base + (float)f->calls);
    return true;
}

static int guard_trace_test(void)
{
    uint8_t attacker[F_SIZE], defender[F_SIZE];
    stf_tgp_command_record recorded[5], modified[5];
    stf_tgp_command_trace trace;
    stf_attack_hit_guard_vector_result result;
    fixture fake;
    uint32_t preserved[3];
    size_t difference = 0u;
    uint32_t reply = UINT32_C(0xABCDEF12);

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(recorded, 0, sizeof(recorded));
    memset(&fake, 0, sizeof(fake));
    wr32(defender + 0x5D8u, bits(60.0f));
    wr32(defender + 0x5E0u, bits(91.0f));
    wr32(defender + 0x5E4u, bits(92.0f));
    wr32(defender + 0x5E8u, bits(93.0f));
    fake.reply_base = 1.0f;
    if (!stf_tgp_command_trace_begin_capture(
            &trace, recorded, 5u, source, &fake) ||
        !stf_attack_hit_guard_vector_apply_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            32, bits(16.0f), 1u, NULL, 0u,
            stf_tgp_command_trace_callback, &trace, &result) ||
        trace.count != 5u || trace.position != 5u ||
        fake.calls != 5u || result.completed_commands != 5u ||
        recorded[0].request.opcode != UINT32_C(0x0D001A1A) ||
        recorded[0].request.argument_count != 1u ||
        recorded[1].request.opcode != UINT32_C(0x12002424) ||
        recorded[1].request.words[1] != bits(2.0f) ||
        recorded[2].request.opcode != UINT32_C(0x12802525) ||
        recorded[3].request.words[0] != UINT32_C(0xFFFFFFE0) ||
        recorded[3].request.words[1] != bits(4.0f) ||
        rd32(defender + 0x5E0u) != bits(5.0f) ||
        rd32(defender + 0x5E4u) != bits(3.0f) ||
        rd32(defender + 0x5E8u) != bits(6.0f)) return 1;

    preserved[0] = rd32(defender + 0x5E0u);
    preserved[1] = rd32(defender + 0x5E4u);
    preserved[2] = rd32(defender + 0x5E8u);
    if (!stf_tgp_command_trace_begin_replay(&trace, recorded, 5u) ||
        !stf_attack_hit_guard_vector_apply_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            32, bits(16.0f), 1u, NULL, 0u,
            stf_tgp_command_trace_callback, &trace, &result) ||
        !stf_tgp_command_trace_fully_consumed(&trace) ||
        fake.calls != 5u ||
        rd32(defender + 0x5E0u) != preserved[0] ||
        rd32(defender + 0x5E4u) != preserved[1] ||
        rd32(defender + 0x5E8u) != preserved[2]) return 2;

    /* No extra command is allowed after a fully matched capture. */
    if (stf_tgp_command_trace_callback(
            &trace, &recorded[0].request, &reply) ||
        trace.error != STF_TGP_TRACE_EXHAUSTED ||
        trace.error_position != 5u ||
        reply != UINT32_C(0xABCDEF12) ||
        stf_tgp_command_trace_fully_consumed(&trace)) return 3;

    memcpy(modified, recorded, sizeof(recorded));
    modified[2].request.words[1] ^= 1u;
    if (!stf_tgp_command_trace_begin_replay(&trace, modified, 5u) ||
        stf_attack_hit_guard_vector_apply_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            32, bits(16.0f), 1u, NULL, 0u,
            stf_tgp_command_trace_callback, &trace, &result) ||
        trace.error != STF_TGP_TRACE_MISMATCH ||
        trace.error_position != 2u || trace.position != 2u ||
        result.completed_commands != 2u ||
        rd32(defender + 0x5E0u) != preserved[0] ||
        rd32(defender + 0x5E4u) != preserved[1] ||
        rd32(defender + 0x5E8u) != preserved[2]) return 4;

    if (stf_tgp_command_trace_compare(
            recorded, 5u, recorded, 5u, &difference) !=
            STF_TGP_TRACE_IDENTICAL || difference != 5u ||
        stf_tgp_command_trace_compare(
            recorded, 5u, modified, 5u, &difference) !=
            STF_TGP_TRACE_INPUT_DIFFERENCE || difference != 2u)
        return 5;

    memcpy(modified, recorded, sizeof(recorded));
    modified[3].response_bits ^= 1u;
    if (stf_tgp_command_trace_compare(
            recorded, 5u, modified, 5u, &difference) !=
            STF_TGP_TRACE_REPLY_DIFFERENCE || difference != 3u ||
        stf_tgp_command_trace_compare(
            recorded, 5u, modified, 4u, &difference) !=
            STF_TGP_TRACE_REPLY_DIFFERENCE || difference != 3u ||
        stf_tgp_command_trace_compare(
            recorded, 5u, recorded, 4u, &difference) !=
            STF_TGP_TRACE_LENGTH_DIFFERENCE || difference != 4u)
        return 6;

    memset(&fake, 0, sizeof(fake));
    fake.reply_base = 1.0f;
    if (!stf_tgp_command_trace_begin_capture(
            &trace, recorded, 2u, source, &fake) ||
        stf_attack_hit_guard_vector_apply_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            32, bits(16.0f), 1u, NULL, 0u,
            stf_tgp_command_trace_callback, &trace, &result) ||
        trace.error != STF_TGP_TRACE_CAPACITY ||
        trace.count != 2u || fake.calls != 2u ||
        result.completed_commands != 2u ||
        rd32(defender + 0x5E0u) != preserved[0]) return 7;

    memset(&fake, 0, sizeof(fake));
    fake.reply_base = 1.0f;
    fake.nonfinite_on_call = 3u;
    if (!stf_tgp_command_trace_begin_capture(
            &trace, recorded, 5u, source, &fake) ||
        stf_attack_hit_guard_vector_apply_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            32, bits(16.0f), 1u, NULL, 0u,
            stf_tgp_command_trace_callback, &trace, &result) ||
        trace.error != STF_TGP_TRACE_NONFINITE_RESPONSE ||
        trace.count != 2u || result.completed_commands != 2u ||
        rd32(defender + 0x5E0u) != preserved[0]) return 8;

    memset(&fake, 0, sizeof(fake));
    fake.reply_base = 1.0f;
    fake.reject_on_call = 4u;
    if (!stf_tgp_command_trace_begin_capture(
            &trace, recorded, 5u, source, &fake) ||
        stf_attack_hit_guard_vector_apply_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            32, bits(16.0f), 1u, NULL, 0u,
            stf_tgp_command_trace_callback, &trace, &result) ||
        trace.error != STF_TGP_TRACE_BACKEND_FAILED ||
        trace.count != 3u || result.completed_commands != 3u ||
        rd32(defender + 0x5E0u) != preserved[0]) return 9;

    return 0;
}

static int knockback_trace_test(void)
{
    stf_tgp_command_record records[4];
    stf_tgp_command_trace trace;
    stf_motion_prefix_result prefix;
    stf_motion_vector_inputs inputs;
    stf_motion_vector_result captured;
    stf_motion_vector_result replayed;
    fixture fake;
    uint8_t completed = 0u;
    size_t difference = 0u;

    memset(&prefix, 0, sizeof(prefix));
    memset(&inputs, 0, sizeof(inputs));
    memset(&fake, 0, sizeof(fake));
    memset(records, 0, sizeof(records));
    prefix.angle_r6 = 0;
    prefix.sqrt_r4_bits = bits(2.0f);
    fake.reply_base = 0.0f;

    if (!stf_tgp_command_trace_begin_capture(
            &trace, records, 4u, source, &fake) ||
        !stf_attack_hit_motion_vector_compute_with_tgp(
            &prefix, &inputs, stf_tgp_command_trace_callback, &trace,
            &completed, &captured) ||
        trace.count != 4u || completed != 4u ||
        captured.x_5e0_bits != bits(3.0f) ||
        captured.y_5e4_bits != bits(1.0f) ||
        captured.z_5e8_bits != bits(4.0f) ||
        records[0].request.opcode != UINT32_C(0x12002424) ||
        records[1].request.opcode != UINT32_C(0x12802525) ||
        records[2].request.words[1] != bits(2.0f) ||
        records[3].request.words[1] != bits(2.0f)) return 1;

    if (!stf_tgp_command_trace_begin_replay(&trace, records, 4u) ||
        !stf_attack_hit_motion_vector_compute_with_tgp(
            &prefix, &inputs, stf_tgp_command_trace_callback, &trace,
            &completed, &replayed) ||
        !stf_tgp_command_trace_fully_consumed(&trace) ||
        captured.x_5e0_bits != replayed.x_5e0_bits ||
        captured.y_5e4_bits != replayed.y_5e4_bits ||
        captured.z_5e8_bits != replayed.z_5e8_bits ||
        stf_tgp_command_trace_compare(
            records, 4u, records, 4u, &difference) !=
            STF_TGP_TRACE_IDENTICAL) return 2;

    /* The same captured replies cannot be silently applied to a
     * different horizontal heading. */
    inputs.horizontal_angle_r10 = 64;
    replayed.z_5e8_bits = UINT32_C(0xDEADBEEF);
    if (!stf_tgp_command_trace_begin_replay(&trace, records, 4u) ||
        stf_attack_hit_motion_vector_compute_with_tgp(
            &prefix, &inputs, stf_tgp_command_trace_callback, &trace,
            &completed, &replayed) ||
        trace.error != STF_TGP_TRACE_MISMATCH ||
        trace.error_position != 2u || completed != 2u ||
        replayed.z_5e8_bits != UINT32_C(0xDEADBEEF)) return 3;

    return 0;
}

int main(void)
{
    int rc = guard_trace_test();
    if (rc != 0) return rc;
    rc = knockback_trace_test();
    return rc == 0 ? 0 : 100 + rc;
}
