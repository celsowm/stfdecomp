#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stf/recovery/set_obj_i960_bridge.h"
#include "stf/recovery/set_obj_i960_diff.h"

#define CHECK(cond,num) do { if (!(cond)) { \
    fprintf(stderr,"set_obj i960 bridge case %d failed at line %d\n",(num),__LINE__); \
    return (num); } } while (0)

enum { DATA_SIZE = 0xE0100u };
typedef struct event {
    stf_i960_bus_access_kind kind;
    uint32_t address;
    uint32_t word;
    stf_status status;
} event;
typedef struct test_host {
    stf_model2b_bus bus;
    stf_i960_cpu cpu;
    uint8_t *data_rom;
    uint8_t program[16];
    stf_set_obj_i960_bridge_result result;
    event bus_events[64];
    event device_events[32];
    unsigned bus_count;
    unsigned device_count;
    unsigned fail_device_at;
    unsigned fifo_reply_count;
    bool quad_fifo_mode;
    uint32_t quad_replies[12];
} test_host;

static void little32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static uint32_t bus_word(const stf_i960_bus_trace_event *e)
{
    if (e->byte_count != 4u) return 0u;
    return (uint32_t)e->bytes[0] |
        ((uint32_t)e->bytes[1] << 8u) |
        ((uint32_t)e->bytes[2] << 16u) |
        ((uint32_t)e->bytes[3] << 24u);
}

static void trace(const stf_i960_bus_trace_event *e,void *ctx)
{
    test_host *f=(test_host *)ctx;
    if(f->bus_count >= 64u)return;
    f->bus_events[f->bus_count].kind=e->kind;
    f->bus_events[f->bus_count].address=e->address;
    f->bus_events[f->bus_count].word=bus_word(e);
    f->bus_events[f->bus_count].status=e->status;
    ++f->bus_count;
}

static stf_status io_read(void *ctx, uint32_t addr, void *out, size_t len)
{
    test_host *f=(test_host *)ctx;
    uint32_t reply=0u;
    const uint32_t replies[]={
        UINT32_C(0xABCD0000),UINT32_C(0x123),UINT32_C(0x456)
    };
    if(len!=4u || f->device_count>=32u)return STF_ERROR_UNSUPPORTED;
    if(f->fail_device_at==f->device_count+1u)
        return STF_ERROR_UNSUPPORTED;
    if(addr==UINT32_C(0x00802008)) {
        reply=UINT32_C(0x7000);
    } else if(f->quad_fifo_mode &&
              addr >= UINT32_C(0x00884000) &&
              addr <= UINT32_C(0x0088400C)) {
        if(f->fifo_reply_count>=12u)return STF_ERROR_UNSUPPORTED;
        reply=f->quad_replies[f->fifo_reply_count++];
    } else if(addr==UINT32_C(0x00884000)) {
        if(f->fifo_reply_count>=3u)return STF_ERROR_UNSUPPORTED;
        reply=replies[f->fifo_reply_count++];
    } else return STF_ERROR_UNSUPPORTED;
    f->device_events[f->device_count].kind=STF_I960_BUS_ACCESS_READ;
    f->device_events[f->device_count].address=addr;
    f->device_events[f->device_count].word=reply;
    f->device_events[f->device_count].status=STF_OK;
    ++f->device_count;
    little32((uint8_t *)out,reply);
    return STF_OK;
}

static stf_status io_write(void *ctx,uint32_t addr,
                           const void *data,size_t len)
{
    test_host *f=(test_host *)ctx;
    const uint8_t *bytes=(const uint8_t *)data;
    uint32_t word=0u;
    if(len!=4u || f->device_count>=32u)return STF_ERROR_UNSUPPORTED;
    if(f->fail_device_at==f->device_count+1u)
        return STF_ERROR_UNSUPPORTED;
    word=(uint32_t)bytes[0] | ((uint32_t)bytes[1]<<8u) |
         ((uint32_t)bytes[2]<<16u) | ((uint32_t)bytes[3]<<24u);
    f->device_events[f->device_count].kind=STF_I960_BUS_ACCESS_WRITE;
    f->device_events[f->device_count].address=addr;
    f->device_events[f->device_count].word=word;
    f->device_events[f->device_count].status=STF_OK;
    ++f->device_count;
    return STF_OK;
}

static bool init(test_host *f,bool attach_data,bool enable_device)
{
    stf_i960_bus *bus;
    memset(f,0,sizeof(*f));
    if(stf_model2b_bus_init(&f->bus)!=STF_OK)return false;
    f->data_rom=(uint8_t *)calloc(1u,DATA_SIZE);
    if(f->data_rom==NULL) {
        stf_model2b_bus_destroy(&f->bus);
        return false;
    }
    if(attach_data) {
        if(stf_model2b_bus_attach_main_data(&f->bus,f->data_rom,DATA_SIZE)!=STF_OK)
            return false;
        little32(f->data_rom+0xE0000u, 3u);
        little32(f->data_rom+0xE0024u, UINT32_C(0x11111111));
        little32(f->data_rom+0xE0028u, UINT32_C(0x22222222));
        little32(f->data_rom+0xE002Cu, UINT32_C(0x33333333));
        little32(f->data_rom+0xE0030u, UINT32_C(0x44440005));
    }
    /* A genuine i960 executor instruction (synthetic ROM data,
     * NOT proprietary STF instructions): lda 0x500000,r14. */
    little32(f->program, UINT32_C(0x8C703000));
    little32(f->program+4u, UINT32_C(0x00500000));
    if(stf_model2b_bus_attach_program(&f->bus,f->program,
                                     sizeof(f->program))!=STF_OK) return false;
    stf_i960_cpu_reset(&f->cpu,0u,0u,0u);
    f->cpu.registers[16u]=2u;
    f->cpu.registers[18u]=UINT32_C(0xBEEFF00D);
    f->cpu.registers[26u]=UINT32_C(0x00800000);
    f->cpu.registers[27u]=UINT32_C(0x00880000);
    f->cpu.registers[28u]=UINT32_C(0x4000);
    bus=stf_model2b_bus_i960(&f->bus);
    if(stf_i960_bus_write_u32(bus,UINT32_C(0x501018),100u)!=STF_OK ||
       stf_i960_bus_write_u32(bus,UINT32_C(0x50101C),17u)!=STF_OK ||
       stf_i960_bus_write_u32(bus,UINT32_C(0x5010D0),5u)!=STF_OK ||
       stf_i960_bus_write_u32(bus,UINT32_C(0x501010),33u)!=STF_OK ||
       stf_i960_bus_write_u32(bus,UINT32_C(0x50FFFC),77u)!=STF_OK)
        return false;
    if(enable_device)
        stf_model2b_bus_set_device_callbacks(&f->bus,f,io_read,io_write);
    /* Start the trace after setup; capture every bus access from bridge. */
    stf_i960_bus_set_trace(bus,trace,f);
    return true;
}

static void destroy(test_host *f)
{
    stf_model2b_bus_destroy(&f->bus);
    free(f->data_rom);
}

static uint32_t ram(test_host *f,uint32_t address)
{
    uint32_t word=0u;
    (void)stf_i960_bus_read_u32(stf_model2b_bus_i960(&f->bus),address,&word);
    return word;
}

static bool ev(const test_host *f,unsigned ix,
               stf_i960_bus_access_kind kind,uint32_t addr,uint32_t word)
{
    return ix<f->device_count &&
        f->device_events[ix].kind==kind &&
        f->device_events[ix].address==addr &&
        f->device_events[ix].word==word;
}

static int normal_bridge(void)
{
    test_host f;
    const stf_i960_cpu *cpu;
    uint32_t before_regs[STF_I960_REGISTER_COUNT];
    unsigned i;
    CHECK(init(&f,true,true),1);
    cpu=&f.cpu;

    /* Run a real decoder/executor step before taking CPU boundary
     * snapshot: proves this bridge accepts the existing CPU object. */
    CHECK(stf_i960_step(&f.cpu,stf_model2b_bus_i960(&f.bus),NULL)==STF_OK &&
          f.cpu.ip==8u && f.cpu.registers[14u]==UINT32_C(0x00500000),2);
    memcpy(before_regs,cpu->registers,sizeof(before_regs));
    f.bus_count=0u;
    CHECK(stf_set_obj_i960_bridge_execute(
            cpu,&f.bus,STF_SET_OBJ_I960_NORMAL,&f.result),3);
    CHECK(f.result.invoked && f.result.completed && f.result.committed &&
          f.result.recovery.submitted &&
          f.result.recovery.completed_operations==17u &&
          f.result.descriptor_reads==5u &&
          f.result.work_ram_reads==5u &&
          f.result.work_ram_writes==4u &&
          f.result.descriptor_address==UINT32_C(0x020E0024) &&
          f.result.geo_bus_address==UINT32_C(0x00800000) &&
          f.result.fifo_bus_address==UINT32_C(0x00884000) &&
          f.device_count==17u && f.bus_count==31u &&
          memcmp(before_regs,cpu->registers,sizeof(before_regs))==0,4);
    CHECK(ev(&f,0u,STF_I960_BUS_ACCESS_WRITE,0x884000u,UINT32_C(0x09801313)) &&
          ev(&f,1u,STF_I960_BUS_ACCESS_WRITE,0x884000u,UINT32_C(0x09801313)) &&
          ev(&f,2u,STF_I960_BUS_ACCESS_WRITE,0x884000u,UINT32_C(0x09801313)) &&
          ev(&f,4u,STF_I960_BUS_ACCESS_WRITE,0x884000u,UINT32_C(0x3C007878)) &&
          ev(&f,6u,STF_I960_BUS_ACCESS_WRITE,0x801008u,UINT32_C(0x7048)) &&
          ev(&f,9u,STF_I960_BUS_ACCESS_WRITE,0x884000u,UINT32_C(0x11111111)) &&
          ev(&f,10u,STF_I960_BUS_ACCESS_WRITE,0x884000u,UINT32_C(0x22222222)) &&
          ev(&f,14u,STF_I960_BUS_ACCESS_WRITE,0x884000u,17u),5);
    for(i=0u;i<5u;++i)
        CHECK(f.bus_events[i].kind==STF_I960_BUS_ACCESS_READ &&
              f.bus_events[i].address >= UINT32_C(0x500000),6);
    CHECK(f.bus_events[10].address==UINT32_C(0x884000) &&
          f.bus_events[30].address==UINT32_C(0x50101C) &&
          ram(&f,UINT32_C(0x501010))==34u &&
          ram(&f,UINT32_C(0x50FFFC))==2u &&
          ram(&f,UINT32_C(0x5010D0))==UINT32_C(0x123) &&
          ram(&f,UINT32_C(0x50101C))==UINT32_C(0x456),7);
    destroy(&f);
    return 0;
}

static int closed_gate_without_rom(void)
{
    test_host f;
    CHECK(init(&f,false,false),8);
    f.bus_count=0u;
    CHECK(stf_i960_bus_write_u32(stf_model2b_bus_i960(&f.bus),
              UINT32_C(0x501018),16u)==STF_OK,9);
    f.bus_count=0u;
    f.cpu.registers[16u]=UINT32_C(0x8123); /* no table available */
    CHECK(stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_NORMAL,&f.result),10);
    CHECK(f.result.recovery.gate_closed &&
          f.result.invoked && f.result.completed &&
          f.result.committed &&
          f.result.descriptor_reads==0u &&
          f.result.work_ram_reads==5u &&
          f.result.work_ram_writes==0u &&
          f.bus_count==5u && f.device_count==0u,11);
    destroy(&f);
    return 0;
}

static int missing_hardware_fails_closed(void)
{
    test_host f;
    const stf_model2b_fault *fault;
    CHECK(init(&f,true,false),12);
    CHECK(!stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_NORMAL,&f.result),13);
    fault=stf_model2b_bus_last_fault(&f.bus);
    CHECK(f.result.invoked && !f.result.completed &&
          f.result.last_bus_status==STF_ERROR_UNSUPPORTED &&
          f.result.recovery.completed_operations==0u &&
          f.result.work_ram_writes==0u &&
          f.result.descriptor_reads==5u &&
          fault!=NULL && fault->valid && fault->write &&
          fault->address==UINT32_C(0x884000) &&
          ram(&f,UINT32_C(0x501010))==33u,14);
    destroy(&f);
    return 0;
}

static int partial_bus_failure_keeps_cpu_state(void)
{
    test_host f;
    CHECK(init(&f,true,true),15);
    f.fail_device_at=5u; /* three FIFO writes and one read passed */
    CHECK(!stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_NORMAL,&f.result),16);
    CHECK(f.result.invoked && !f.result.completed && f.result.committed &&
          f.result.recovery.completed_operations==4u &&
          f.result.work_ram_writes==2u &&
          f.result.last_bus_status==STF_OK /* RAM commits succeeded */ &&
          f.device_count==4u &&
          ram(&f,UINT32_C(0x501010))==34u &&
          ram(&f,UINT32_C(0x50FFFC))==2u &&
          ram(&f,UINT32_C(0x50101C))==17u,17);
    destroy(&f);
    return 0;
}

static int common_and_special(void)
{
    test_host f;
    CHECK(init(&f,true,true),18);
    CHECK(stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_COMMON,&f.result),19);
    CHECK(f.result.recovery.submitted &&
          f.result.recovery.completed_operations==5u &&
          f.device_count==5u &&
          ram(&f,UINT32_C(0x501010))==34u &&
          ram(&f,UINT32_C(0x50101C))==22u &&
          ev(&f,0u,STF_I960_BUS_ACCESS_WRITE,0x800010u,0x101u) &&
          ev(&f,4u,STF_I960_BUS_ACCESS_WRITE,0x80400Cu,UINT32_MAX),20);
    destroy(&f);

    CHECK(init(&f,true,true),21);
    CHECK(stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_THD,&f.result),22);
    CHECK(f.result.recovery.submitted &&
          f.result.recovery.completed_operations==11u &&
          f.device_count==11u &&
          ev(&f,0u,STF_I960_BUS_ACCESS_WRITE,0x8000B0u,0xB0Bu) &&
          ev(&f,1u,STF_I960_BUS_ACCESS_WRITE,0x884000u,UINT32_C(0x1A003434)) &&
          ev(&f,8u,STF_I960_BUS_ACCESS_WRITE,0x804004u,UINT32_C(0xBEEFF00D)) &&
          ram(&f,UINT32_C(0x501010))==34u &&
          ram(&f,UINT32_C(0x50101C))==22u,23);
    destroy(&f);
    return 0;
}

static int invalid_config_and_missing_tables(void)
{
    test_host f;
    CHECK(init(&f,false,false),24);
    f.cpu.registers[26u]=0u;
    CHECK(!stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_NORMAL,&f.result) &&
          f.result.last_bus_status==STF_ERROR_INVALID_ARGUMENT &&
          !f.result.invoked && f.bus_count==0u,25);
    f.cpu.registers[26u]=UINT32_C(0x00800000);
    CHECK(!stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_NORMAL,&f.result) &&
          !f.result.invoked &&
          f.result.work_ram_reads==5u &&
          f.result.descriptor_reads==0u,26);
    destroy(&f);
    return 0;
}

static int trace_diff_and_fail_closed(void)
{
    test_host original, recovered;
    stf_i960_bus_trace_event original_buffer[32], recovered_buffer[32];
    stf_i960_bus_trace_event limited_buffer[3];
    stf_set_obj_i960_mmio_capture one, two, limited;
    stf_set_obj_i960_mmio_difference difference;

    CHECK(init(&original,true,true),27);
    CHECK(init(&recovered,true,true),28);
    CHECK(stf_set_obj_i960_mmio_capture_init(
              &one,original_buffer,32u) &&
          stf_set_obj_i960_mmio_capture_init(
              &two,recovered_buffer,32u) &&
          stf_set_obj_i960_mmio_capture_init(
              &limited,limited_buffer,3u),29);
    stf_i960_bus_set_trace(stf_model2b_bus_i960(&original.bus),
                          stf_set_obj_i960_mmio_capture_callback,&one);
    stf_i960_bus_set_trace(stf_model2b_bus_i960(&recovered.bus),
                          stf_set_obj_i960_mmio_capture_callback,&two);
    CHECK(stf_set_obj_i960_bridge_execute(&original.cpu,&original.bus,
              STF_SET_OBJ_I960_NORMAL,&original.result) &&
          stf_set_obj_i960_bridge_execute(&recovered.cpu,&recovered.bus,
              STF_SET_OBJ_I960_NORMAL,&recovered.result),30);
    CHECK(one.count==17u && two.count==17u &&
          stf_set_obj_i960_mmio_compare(&one,&two,&difference) &&
          difference.equal && difference.capture_complete &&
          difference.first_difference==17u,31);

    /* Same command sequence but one changed read reply is NOT equal. */
    two.events[15].bytes[0] ^= 1u;
    CHECK(stf_set_obj_i960_mmio_compare(&one,&two,&difference) &&
          !difference.equal && difference.first_difference==15u,32);
    two.events[15].bytes[0] ^= 1u;
    two.count=16u;
    CHECK(stf_set_obj_i960_mmio_compare(&one,&two,&difference) &&
          !difference.equal && difference.first_difference==16u,33);

    /* Capacity exhaustion explicitly INVALIDATES comparison, instead
     * of letting the truncated trace appear equivalent. */
    /* Restore the input snapshot; the prior successful call raised
     * POLYGON above its gate so replaying without a reset is invalid. */
    CHECK(stf_i960_bus_write_u32(stf_model2b_bus_i960(&recovered.bus),
              UINT32_C(0x50101C),17u)==STF_OK,35);
    stf_i960_bus_set_trace(stf_model2b_bus_i960(&recovered.bus),
                          stf_set_obj_i960_mmio_capture_callback,&limited);
    recovered.device_count=0u;
    recovered.fifo_reply_count=0u;
    CHECK(stf_set_obj_i960_bridge_execute(&recovered.cpu,&recovered.bus,
              STF_SET_OBJ_I960_NORMAL,&recovered.result) &&
          limited.count==3u && limited.overflow &&
          !stf_set_obj_i960_mmio_compare(&one,&limited,&difference) &&
          !difference.capture_complete,34);

    destroy(&original);
    destroy(&recovered);
    return 0;
}

static void setup_quad_responses(test_host *f)
{
    unsigned i;
    f->quad_fifo_mode=true;
    f->quad_replies[0]=UINT32_C(0x3F800000);
    for(i=1u;i<11u;++i)
        f->quad_replies[i]=UINT32_C(0x10000000)+i;
    f->quad_replies[11]=UINT32_C(0x3E4CCCCD);
}

static int fifo_bridge_and_shared_tail(void)
{
    test_host f;
    unsigned i;
    stf_i960_bus *bus;
    CHECK(init(&f,true,true),36);
    setup_quad_responses(&f);
    bus=stf_model2b_bus_i960(&f.bus);
    f.cpu.registers[18u]=0u; /* input g2, not a RAM pointer */
    f.cpu.registers[19u]=UINT32_C(0x501400); /* scratch g3 */
    f.cpu.registers[23u]=UINT32_C(0x501600); /* flags structure g7 */
    CHECK(stf_i960_bus_write_u32(bus,UINT32_C(0x5017A4),UINT32_C(1)<<26)==STF_OK &&
          stf_i960_bus_write_u32(bus,UINT32_C(0x501D0C),0u)==STF_OK &&
          stf_i960_bus_write_u32(bus,UINT32_C(0x5017A8),UINT32_C(0x00040000))==STF_OK,37);
    /* The halfword at g7+0x1AA is the upper half of g7+0x1A8. */
    CHECK(stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_FIFO,&f.result),38);
    CHECK(f.result.invoked && f.result.completed &&
          f.result.recovery.submitted &&
          f.result.fifo_quads_consumed==3u &&
          f.result.fifo_quads_saved==3u &&
          f.result.fifo_tail==STF_SET_OBJ_FIFO_THD_FLAG26_TAIL &&
          f.result.g3_rewind_bytes==0u &&
          f.result.caller_buffer_writes==12u &&
          f.result.descriptor_reads==5u &&
          f.result.recovery.completed_operations==30u &&
          f.device_count==30u &&
          ram(&f,UINT32_C(0x50101C))==22u &&
          ram(&f,UINT32_C(0x501010))==34u,39);
    for(i=0u;i<12u;++i)
        CHECK(ram(&f,UINT32_C(0x501400)+i*4u)==f.quad_replies[i],40);
    CHECK(ev(&f,12u,STF_I960_BUS_ACCESS_WRITE,
              UINT32_C(0x8000B0),UINT32_C(0xB0B)) &&
          ev(&f,29u,STF_I960_BUS_ACCESS_WRITE,
              UINT32_C(0x80400C),UINT32_MAX) &&
          ev(&f,27u,STF_I960_BUS_ACCESS_WRITE,
              UINT32_C(0x804004),UINT32_C(0x800000)),41);
    destroy(&f);
    return 0;
}

static int three_quad_save_load_bridge(void)
{
    test_host f;
    stf_i960_bus *bus;
    uint32_t saved[12];
    unsigned i;
    CHECK(init(&f,true,true),42);
    setup_quad_responses(&f);
    f.cpu.registers[18u]=UINT32_C(0x501500);
    bus=stf_model2b_bus_i960(&f.bus);
    CHECK(stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_SAVE,&f.result),43);
    CHECK(f.result.recovery.submitted &&
          f.result.recovery.completed_operations==31u &&
          f.result.caller_buffer_writes==12u &&
          f.result.caller_buffer_reads==0u &&
          f.result.fifo_quads_saved==3u &&
          f.result.g2_advanced_bytes==48u &&
          f.device_count==31u &&
          ram(&f,UINT32_C(0x501010))==34u &&
          ram(&f,UINT32_C(0x50101C))==22u,44);
    for(i=0u;i<12u;++i) {
        saved[i]=ram(&f,UINT32_C(0x501500)+i*4u);
        CHECK(saved[i]==f.quad_replies[i],45);
    }
    destroy(&f);

    CHECK(init(&f,true,true),46);
    f.cpu.registers[18u]=UINT32_C(0x501500);
    bus=stf_model2b_bus_i960(&f.bus);
    for(i=0u;i<12u;++i)
        CHECK(stf_i960_bus_write_u32(
                  bus,UINT32_C(0x501500)+i*4u,saved[i])==STF_OK,47);
    CHECK(stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_LOAD,&f.result),48);
    CHECK(f.result.recovery.submitted &&
          f.result.recovery.completed_operations==18u &&
          f.result.caller_buffer_reads==12u &&
          f.result.caller_buffer_writes==0u &&
          f.result.g2_advanced_bytes==48u &&
          f.device_count==18u &&
          ram(&f,UINT32_C(0x501010))==34u &&
          ram(&f,UINT32_C(0x50101C))==22u,49);
    destroy(&f);
    return 0;
}

static int invalid_caller_buffer_gate(void)
{
    test_host f;
    CHECK(init(&f,true,true),50);
    f.cpu.registers[19u]=0u;
    CHECK(!stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_FIFO,&f.result) &&
          f.result.last_bus_status==STF_ERROR_OUT_OF_BOUNDS &&
          !f.result.invoked && f.device_count==0u,51);
    f.cpu.registers[18u]=UINT32_C(0x5FFFF0);
    CHECK(!stf_set_obj_i960_bridge_execute(
              &f.cpu,&f.bus,STF_SET_OBJ_I960_SAVE,&f.result) &&
          !f.result.invoked && f.device_count==0u,52);
    destroy(&f);
    return 0;
}

int main(void)
{
    int rc=normal_bridge();
    if(rc)return rc;
    rc=closed_gate_without_rom();
    if(rc)return rc;
    rc=missing_hardware_fails_closed();
    if(rc)return rc;
    rc=partial_bus_failure_keeps_cpu_state();
    if(rc)return rc;
    rc=common_and_special();
    if(rc)return rc;
    rc=invalid_config_and_missing_tables();
    if(rc)return rc;
    rc=trace_diff_and_fail_closed();
    if(rc)return rc;
    rc=fifo_bridge_and_shared_tail();
    if(rc)return rc;
    rc=three_quad_save_load_bridge();
    if(rc)return rc;
    return invalid_caller_buffer_gate();
}
