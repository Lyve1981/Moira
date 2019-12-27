// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <assert.h>

#include "Moira.h"

namespace moira {

#include "MoiraInit_cpp.h"
#include "MoiraLogic_cpp.h"
#include "MoiraMemory_cpp.h"
#include "MoiraTiming_cpp.h"
#include "MoiraExec_cpp.h"
#include "StringWriter_cpp.h"
#include "MoiraDasm_cpp.h"

Moira::Moira()
{
    createJumpTables();
}

void
Moira::power()
{
    reset();
}

void
Moira::reset()
{
    clock = -40;

    for(int i = 0; i < 8; i++) {
        reg.d[i] = 0;
        reg.a[i] = 0;
    }
    reg.usp = 0;

    sr.t = 0;
    sr.s = 1;
    sr.x = 0;
    sr.n = 0;
    sr.z = 0;
    sr.v = 0;
    sr.c = 0;
    sr.ipl = 7;

    iplPolled = 0;

    sync(16);

    // Read the initial (supervisor) stack pointer from memory
    reg.sp = reg.ssp = readOnReset(0) << 16 | readOnReset(2);

    // Read the initial program counter from memory
    reg.pc = readOnReset(4) << 16 | readOnReset(6);

    // Fill the prefetch queue
    irc = readOnReset(reg.pc);
    prefetch();

    reg.d[0] = 0x10;
    reg.d[1] = 0x20;
    reg.d[2] = 0x30;
    reg.d[3] = 0x40;
    reg.d[4] = 0x50;
    reg.d[5] = 0x60;
    reg.d[6] = 0x70;
    reg.d[7] = 0x80;
    reg.a[0] = 0x90;
    reg.a[1] = 0x10000;
    reg.a[2] = 0x11011;
    reg.a[3] = 0x12033;
    reg.a[4] = 0x13000;
    reg.a[5] = 0x14000000;
    reg.a[6] = 0x80000000;
}

void
Moira::process(u16 reg_ird)
{
    reg.pc += 2;
    (this->*exec[reg_ird])(reg_ird);
}

u16
Moira::getSR()
{
    return sr.t << 15 | sr.s << 13 | sr.ipl << 8 | getCCR();
}

void
Moira::setSR(u16 value)
{
    bool t = (value >> 15) & 1;
    bool s = (value >> 13) & 1;
    u8 ipl = (value >>  8) & 7;

    sr.ipl = ipl;
    sr.t = t;

    setCCR((u8)value);
    setSupervisorMode(s);
}

u8
Moira::getCCR()
{
    return sr.c << 0 | sr.v << 1 | sr.z << 2 | sr.n << 3 | sr.x << 4;
}

void
Moira::setCCR(u8 value)
{
    sr.c = (value >> 0) & 1;
    sr.v = (value >> 1) & 1;
    sr.z = (value >> 2) & 1;
    sr.n = (value >> 3) & 1;
    sr.x = (value >> 4) & 1;
}

void
Moira::setSupervisorMode(bool enable)
{
    if (sr.s == enable) return;

    if (enable) {
        sr.s = 1;
        reg.usp = reg.a[7];
        reg.a[7] = reg.ssp;
    } else {
        sr.s = 0;
        reg.ssp = reg.a[7];
        reg.a[7] = reg.usp;
    }
}

template<bool last> void
Moira::prefetch()
{
    ird = irc;
    irc = readM<Word,last>(reg.pc + 2);
}

template<bool last> void
Moira::fullPrefetch()
{
    irc = readM<Word>(reg.pc);
    prefetch<last>();
}

template<bool skip> void
Moira::readExtensionWord()
{
    reg.pc += 2;
    if (!skip) irc = readM<Word>(reg.pc);
}

void
Moira::dummyRead(u32 pc)
{
    (void)readM<Word>(pc);
}

void
Moira::jumpToVector(u8 nr)
{
    // Update the program counter
    reg.pc = readM<Long>(4 * nr);

    // Update the prefetch queue
    ird = readM<Word>(reg.pc);
    sync(2);
    irc = readM<Word,LAST_BUS_CYCLE>(reg.pc + 2);
}

void
Moira::pollIrq()
{
    iplPolled = readIPL();
}

int
Moira::disassemble(u32 addr, char *str, bool hex)
{
    u32 pc     = addr;
    u16 opcode = read16Dasm(pc);

    StrWriter writer(str, hex);

    (this->*dasm[opcode])(writer, pc, opcode);
    writer << Finish{};

    return pc - addr + 2;
}

}
