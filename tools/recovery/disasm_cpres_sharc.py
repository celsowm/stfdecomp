#!/usr/bin/env python3
"""Small evidence-focused SHARC disassembler for STF cpres recovery.

This intentionally implements only instruction classes used by the recovered
STF cpres handlers.  It is not a general ADSP-2106x disassembler.

Input is the raw 0x741C-byte upload emitted by extract_cpres_program.py.
Program memory base is 0x20000 and each packet is one 48-bit SHARC instruction.
"""

from __future__ import annotations

import argparse
from pathlib import Path

PACKET_BYTES = 6
PM_BASE = 0x20000

COND = {
    0x00: "EQ", 0x01: "LT", 0x02: "LE", 0x03: "AC",
    0x04: "AV", 0x05: "MV", 0x06: "MS", 0x07: "SV",
    0x08: "SZ", 0x09: "FLAG0_IN", 0x0A: "FLAG1_IN",
    0x0B: "FLAG2_IN", 0x0C: "FLAG3_IN", 0x0D: "TF",
    0x0E: "BM", 0x0F: "NOT_LCE", 0x10: "NE", 0x11: "GE",
    0x12: "GT", 0x13: "NOT_AC", 0x14: "NOT_AV", 0x15: "NOT_MV",
    0x16: "NOT_MS", 0x17: "NOT_SV", 0x18: "NOT_SZ",
    0x19: "NOT_FLAG0_IN", 0x1A: "NOT_FLAG1_IN",
    0x1B: "NOT_FLAG2_IN", 0x1C: "NOT_FLAG3_IN",
    0x1D: "NOT_TF", 0x1E: "NBM", 0x1F: "ALWAYS",
}


def ureg_name(value: int) -> str:
    if 0x00 <= value <= 0x0F:
        return f"R{value}"
    if 0x10 <= value <= 0x1F:
        return f"I{value - 0x10}"
    if 0x20 <= value <= 0x2F:
        return f"M{value - 0x20}"
    if 0x30 <= value <= 0x3F:
        return f"L{value - 0x30}"
    if 0x40 <= value <= 0x4F:
        return f"B{value - 0x40}"
    special = {
        0x60: "FADDR", 0x61: "DADDR", 0x63: "PC", 0x64: "PCSTK",
        0x65: "PCSTKP", 0x66: "LADDR", 0x67: "CURLCNTR",
        0x68: "LCNTR", 0x70: "USTAT1", 0x71: "USTAT2",
        0x79: "IRPTL", 0x7A: "MODE2", 0x7B: "MODE1", 0x7C: "ASTAT",
        0x7D: "IMASK", 0x7E: "STKY", 0x7F: "IMASKP",
        0xDB: "PX", 0xDC: "PX1", 0xDD: "PX2", 0xDE: "TPERIOD",
        0xDF: "TCOUNT",
    }
    return special.get(value, f"UREG[{value:02X}]")


def cond_name(opcode: int) -> str:
    return COND.get((opcode >> 33) & 0x1F, "?")


def decode_compute(comp: int) -> str:
    if comp == 0:
        return ""
    op = (comp >> 12) & 0xFF
    cu = (comp >> 20) & 0x3
    rn = (comp >> 8) & 0xF
    rx = (comp >> 4) & 0xF
    ry = comp & 0xF
    if comp & 0x400000:
        multi = (comp >> 16) & 0x3F
        rxm, rym = (comp >> 6) & 3, ((comp >> 4) & 3) + 4
        rxa, rya = ((comp >> 2) & 3) + 8, (comp & 3) + 12
        rm = (comp >> 12) & 0xF
        ra = rn
        if 0x30 <= multi <= 0x3F:
            return (
                f"F{rm}=F{rxm}*F{rym}; "
                f"F{ra}=F{rxa}+F{rya}; "
                f"F{(comp >> 16) & 0xF}=F{rxa}-F{rya}"
            )
        if multi == 0x18:
            return f"F{rm}=F{rxm}*F{rym}; F{ra}=F{rxa}+F{rya}"
        if multi == 0x19:
            return f"F{rm}=F{rxm}*F{rym}; F{ra}=F{rxa}-F{rya}"
        return f"COMPUTE_MULTI[0x{multi:02X}]"
    if cu == 0:
        float_ops = {
            0x81: "+", 0x82: "-", 0x8A: "CMP",
            0xA2: "NEG", 0xB0: "ABS", 0xA1: "PASS",
            0xC4: "RECIPS", 0xC5: "RSQRTS",
            0xE1: "MIN", 0xE2: "MAX",
        }
        if op == 0x81:
            return f"F{rn}=F{rx}+F{ry}"
        if op == 0x82:
            return f"F{rn}=F{rx}-F{ry}"
        if op == 0x8A:
            return f"COMP(F{rx},F{ry})"
        if op == 0xA2:
            return f"F{rn}=-F{rx}"
        if op == 0xB0:
            return f"F{rn}=ABS(F{rx})"
        if op == 0xA1:
            return f"F{rn}=F{rx}"
        if op == 0xC4:
            return f"F{rn}=RECIPS(F{rx})"
        if op == 0xC5:
            return f"F{rn}=RSQRTS(F{rx})"
        if 0xF0 <= op <= 0xFF:
            rs = (comp >> 12) & 0xF
            return f"F{rn}=F{rx}+F{ry}; F{rs}=F{rx}-F{ry}"
        if op in float_ops:
            return f"FLOAT_OP[0x{op:02X}]"
        return f"ALU[0x{op:02X}] R{rn},R{rx},R{ry}"
    if cu == 1 and op == 0x30:
        return f"F{rn}=F{rx}*F{ry}"
    return f"COMPUTE[cu={cu},op=0x{op:02X}]"


def decode(opcode: int, pc: int) -> str:
    top = (opcode >> 40) & 0xFF
    if top == 0x00:
        return "IDLE" if opcode & 0x8000000000 else "NOP"
    if top == 0x0F:
        return f"{ureg_name((opcode >> 32) & 0xFF)}=0x{opcode & 0xFFFFFFFF:08X}"
    if 0x90 <= top <= 0x9F:
        g = (opcode >> 37) & 1
        i = (opcode >> 41) & 7
        m = (opcode >> 38) & 7
        mem = "PM" if g else "DM"
        return f"{mem}(I{i},M{m})=0x{opcode & 0xFFFFFFFF:08X}"
    if 0xA0 <= top <= 0xBF:
        d = (opcode >> 40) & 1
        g = (opcode >> 44) & 1
        i = (opcode >> 41) & 7
        reg = ureg_name((opcode >> 32) & 0xFF)
        mem = "PM" if g else "DM"
        addr = opcode & 0xFFFFFFFF
        return (
            f"{mem}(0x{addr:08X},I{i})={reg}"
            if d else
            f"{reg}={mem}(0x{addr:08X},I{i})"
        )
    if top in (0x06, 0x07):
        cond = cond_name(opcode)
        is_call = bool(opcode & 0x8000000000)
        relative = bool(opcode & 0x10000000000)
        raw = opcode & 0xFFFFFF
        if relative and raw & 0x800000:
            raw -= 0x1000000
        target = pc + raw if relative else raw
        return f"{'CALL' if is_call else 'JUMP'} {cond} 0x{target:05X}"
    if top in (0x0A, 0x0B):
        comp = decode_compute(opcode & 0x7FFFFF)
        return f"RTS {cond_name(opcode)}" + (f"; {comp}" if comp else "")
    if top in (0x0C, 0x0D):
        return (
            f"DO_COUNTER target=0x{pc + ((opcode & 0xFFFFFF) if "
            f"(opcode & 0x800000) == 0 else (opcode & 0xFFFFFF)-0x1000000):05X}"
        )
    if top == 0x0E:
        return f"DO_UNTIL target=0x{pc + (opcode & 0xFFFFFF):05X}"
    if 0x40 <= top <= 0x5F:
        cond = cond_name(opcode)
        g = (opcode >> 32) & 1
        d = (opcode >> 31) & 1
        i = (opcode >> 41) & 7
        m = (opcode >> 38) & 7
        reg = ureg_name((opcode >> 23) & 0xFF)
        mem = "PM" if g else "DM"
        move = f"{mem}(I{i},M{m})={reg}" if d else f"{reg}={mem}(I{i},M{m})"
        comp = decode_compute(opcode & 0x7FFFFF)
        return f"{cond}: " + (f"{comp}; " if comp else "") + move
    if top == 0x01:
        return f"{cond_name(opcode)}: {decode_compute(opcode & 0x7FFFFF)}"
    if 0x20 <= top <= 0x3F:
        return f"COMPUTE_DUAL {decode_compute(opcode & 0x7FFFFF)}"
    if 0x70 <= top <= 0x7F:
        src = ureg_name((opcode >> 36) & 0xFF)
        dst = ureg_name((opcode >> 23) & 0xFF)
        comp = decode_compute(opcode & 0x7FFFFF)
        return f"{cond_name(opcode)}: {comp + '; ' if comp else ''}{dst}={src}"
    if top == 0x02:
        return f"SHIFT cond={cond_name(opcode)} raw=0x{opcode:012X}"
    if top in (0x08, 0x09) or 0xC0 <= top <= 0xFF:
        return f"INDIRECT_JUMP raw=0x{opcode:012X}"
    return f"RAW class=0x{top:02X} 0x{opcode:012X}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upload", type=Path)
    parser.add_argument("--start", type=lambda v: int(v, 0), default=0x20B1F)
    parser.add_argument("--count", type=lambda v: int(v, 0), default=0x80)
    args = parser.parse_args()

    data = args.upload.read_bytes()
    start_index = args.start - PM_BASE
    if start_index < 0:
        raise SystemExit("start before PM base")
    for index in range(start_index, start_index + args.count):
        offset = index * PACKET_BYTES
        if offset + PACKET_BYTES > len(data):
            break
        opcode = int.from_bytes(data[offset:offset + PACKET_BYTES], "little")
        pc = PM_BASE + index
        print(f"{pc:05X}: {opcode:012X}  {decode(opcode, pc)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
