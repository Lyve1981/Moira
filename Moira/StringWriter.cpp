// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "StringWriter.h"
#include "assert.h"
#include <math.h>
#include <stdio.h>

namespace moira {

static int decDigits(u64 value) { return value ? 1 + log10(value) : 1; }
static int binDigits(u64 value) { return value ? 1 + log2(value) : 1; }
static int hexDigits(u64 value) { return (binDigits(value) + 3) / 4; }

static void sprintd(char *&s, u64 value, int digits)
{
    for (int i = digits - 1; i >= 0; i--) {
        u8 digit = value % 10;
        s[i] = '0' + digit;
        value /= 10;
    }
    s += digits;
}

static void sprintd(char *&s, u64 value)
{
    sprintd(s, value, decDigits(value));
}

static void sprintd_signed(char *&s, i64 value)
{
    if (value < 0) { *s++ = '-'; value *= -1; }
    sprintd(s, value, decDigits(value));
}

static void sprintx(char *&s, u64 value, int digits)
{
    *s++ = '$';
    for (int i = digits - 1; i >= 0; i--) {
        u8 digit = value % 16;
        s[i] = (digit <= 9) ? ('0' + digit) : ('a' + digit - 10);
        value /= 16;
    }
    s += digits;
}

static void sprintx(char *&s, u64 value)
{
    sprintx(s, value, hexDigits(value));
}

static void sprintx_signed(char *&s, i64 value)
{
    if (value < 0) { *s++ = '-'; value *= -1; }
    sprintx(s, value, hexDigits(value));
}

StrWriter&
StrWriter::operator<<(const char *str)
{
    while (*str) { *ptr++ = *str++; };
    return *this;
}

StrWriter&
StrWriter::operator<<(int value)
{
    sprintd(ptr, value);
    return *this;
}

StrWriter&
StrWriter::operator<<(Int i)
{
    hex ? sprintx_signed(ptr, i.raw) : sprintd_signed(ptr, i.raw);
    return *this;
}

StrWriter&
StrWriter::operator<<(UInt u)
{
    hex ? sprintx(ptr, u.raw) : sprintd(ptr, u.raw);
    return *this;
}

StrWriter&
StrWriter::operator<<(UInt8 u)
{
    hex ? sprintx(ptr, u.raw, 2) : sprintd(ptr, u.raw, 3);
    return *this;
}

StrWriter&
StrWriter::operator<<(UInt16 u)
{
    hex ? sprintx(ptr, u.raw, 4) : sprintd(ptr, u.raw, 5);
    return *this;
}

StrWriter&
StrWriter::operator<<(UInt32 u)
{
    hex ? sprintx(ptr, u.raw, 8) : sprintd(ptr, u.raw, 10);
    return *this;
}

StrWriter&
StrWriter::operator<<(Dn dn)
{
    *ptr++ = 'D';
    *ptr++ = '0' + dn.raw;
    return *this;
}

StrWriter&
StrWriter::operator<<(An an)
{
    *ptr++ = 'A';
    *ptr++ = '0' + an.raw;
    return *this;
}

StrWriter&
StrWriter::operator<<(Rn rn)
{
    if (rn.raw < 8) {
        *this << Dn{rn.raw};
    } else {
        *this << An{rn.raw - 8};
    }
    return *this;
}

StrWriter&
StrWriter::operator<<(Imu im)
{
    *ptr++ = '#';
    *this << UInt(im.raw);
    return *this;
}

StrWriter&
StrWriter::operator<<(Ims im)
{
    *ptr++ = '#';
    *this << Int(im.raw);
    return *this;
}

StrWriter&
StrWriter::operator<<(Imd im)
{
    *ptr++ = '#';
    sprintd(ptr, im.raw);
    return *this;
}

StrWriter&
StrWriter::operator<<(Scale s)
{
    if (s.raw) {
        *ptr++ = '*';
        *ptr++ = '0' + (1 << s.raw);
    }
    return *this;
}

StrWriter&
StrWriter::operator<<(Align align)
{
    while (ptr < base + align.raw) *ptr++ = ' ';
    return *this;
}

StrWriter&
StrWriter::operator<<(RegList l)
{
    int r[16];

    // Step 1: Fill array r with the register list bits, e.g., 11101101
    for (int i = 0; i <= 15; i++) { r[i] = !!(l.raw & (1 << i)); }

    // Step 2: Convert 11101101 to 12301201
    for (int i = 1; i <= 15; i++) { if (r[i]) r[i] = r[i-1] + 1; }

    // Step 3: Convert 12301201 to 33302201
    for (int i = 14; i >= 0; i--) { if (r[i] && r[i+1]) r[i] = r[i+1]; }

    // Step 4: Convert 33302201 to "D0-D2/D4/D5/D7"
    bool first = true;
    for (int i = 0; i <= 15; i += r[i] + 1) {

        if (r[i] == 0) continue;

        // Print delimiter
        if (first) { first = false; } else { *this << "/"; }

        // Format variant 1: Single register
        if (r[i] == 1) { *this << Rn{i}; }

        // Format variant 2: Register range
        else { *this << Rn{i} << "-" << Rn{i+r[i]-1}; }
    }
    return *this;
}

StrWriter&
StrWriter::operator<<(RegRegList l)
{
    u16 regsD = l.raw & 0x00FF;
    u16 regsA = l.raw & 0xFF00;

    *this << RegList { regsD };
    if (regsD && regsA) *this << "/";
    *this << RegList { regsA };

    return *this;
}

template <Instr I> StrWriter&
StrWriter::operator<<(Ins<I> i)
{
    *this << instrStr[I];
    return *this;
}

template <Cond C> StrWriter&
StrWriter::operator<<(Cnd<C> c)
{
    *this << condStr[C];
    return *this;
}

template <Size S> StrWriter&
StrWriter::operator<<(Sz<S>)
{
    *this << ((S == Byte) ? ".b" : (S == Word) ? ".w" : ".l");
    return *this;
}

template <Mode M, Size S> StrWriter&
StrWriter::operator<<(const Ea<M,S> &ea)
{
    switch (M) {

        case 0: // Dn
        {
            *this << Dn{ea.reg};
            break;
        }
        case 1: // An
        {
            *this << An{ea.reg};
            break;
        }
        case 2: // (An)
        {
            *this << "(" << An{ea.reg} << ")";
            break;
        }
        case 3:  // (An)+
        {
            *this << "(" << An{ea.reg} << ")+";
            break;
        }
        case 4: // -(An)
        {
            *this << "-(" << An{ea.reg} << ")";
            break;
        }
        case 5: // (d,An)
        {
            *this << "(" << Int{(i16)ea.ext1};
            *this << "," << An{ea.reg} << ")";
            break;
        }
        case 6: // (d,An,Xi)
        {
            (ea.ext1 & 0x100) ? fullExtension(ea) : briefExtension(ea);
            break;
        }
        case 7: // ABS.W
        {
            *this << UInt(ea.ext1);
            *this << ".w";
            break;
        }
        case 8: // ABS.L
        {
            *this << UInt(ea.ext1);
            *this << ".l";
            break;
        }
        case 9: // (d,PC)
        {
            *this << "(" << Int{(i16)ea.ext1} << ",PC)";
            auto resolved = UInt(ea.pc + (i16)ea.ext1 + 2);
            StrWriter(comment, hex) << "; (" << resolved << ")" << Finish{};
            break;
        }
        case 10: // (d,PC,Xi)
        {
            (ea.ext1 & 0x100) ? fullExtension(ea) : briefExtension(ea);
            break;
        }
        case 11: // Imm
        {
            *this << Imu(ea.ext1);
            break;
        }
    }
    return *this;
}

StrWriter&
StrWriter::operator<<(Finish)
{
    for (int i = 0; comment[i] != 0; i++) *ptr++ = comment[i];
    *ptr = 0;
    return *this;
}

template <Mode M, Size S> void
StrWriter::briefExtension(const Ea<M,S> &ea)
{
    assert(M == 6 || M == 10);

    //   15 - 12    11   10   09   08   07   06   05   04   03   02   01   00
    // -----------------------------------------------------------------------
    // | REGISTER | LW | SCALE   | 0  | DISPLACEMENT                         |
    // -----------------------------------------------------------------------

    u16 reg   = xxxx____________ (ea.ext1);
    u16 lw    = ____x___________ (ea.ext1);
    u16 scale = _____xx_________ (ea.ext1);
    u16 disp  = ________xxxxxxxx (ea.ext1);

    *this << "(";
    if (disp) *this << Int{(i8)disp} << ",";
    M == 10 ? *this << "PC" : *this << An{ea.reg};
    *this << "," << Rn{reg};
    lw ? *this << Sz<Long>{} : *this << Sz<Word>{};
    *this << Scale{scale} << ")";
}

template <Mode M, Size S> void
StrWriter::fullExtension(const Ea<M,S> &ea)
{
    assert(M == 6 || M == 10);

    //   15 - 12    11   10   09   08   07   06   05   04   03   02   01   00
    // -----------------------------------------------------------------------
    // | REGISTER | LW | SCALE   | 1  | BS | IS | BD SIZE  | 0  | IIS        |
    // -----------------------------------------------------------------------

    u16  reg   = xxxx____________ (ea.ext1);
    u16  lw    = ____x___________ (ea.ext1);
    u16  scale = _____xx_________ (ea.ext1);
    u16  bs    = ________x_______ (ea.ext1);
    u16  is    = _________x______ (ea.ext1);
    u16  iis   = _____________xxx (ea.ext1);
    u32  base  = ea.ext2;
    u32  outer = ea.ext3;

    bool preindex      = (iis > 0 && iis < 4);
    bool postindex     = (iis > 4);
    bool effectiveZero = (ea.ext1 & 0xe4) == 0xC4 || (ea.ext1 & 0xe2) == 0xC0;

    if (effectiveZero) {
        *this << "0";
        return;
    }

    *this << "(";

    bool comma = false;
    if (preindex || postindex)
    {
        *this << "[";
    }
    if (base)
    {
        *this << Int{(i16)base};
        comma = true;
    }
    if (!bs)
    {
        if (comma) *this << ",";
        M == 10 ? *this << "PC" : *this << An{ea.reg};
        comma = true;
    }
    if (postindex)
    {
        *this << "]";
        comma = true;
    }
    if (!is)
    {
        if (comma) *this << ",";
        *this << Rn{reg};
        lw ? (*this << Sz<Long>{}) : (*this << Sz<Word>{});
        *this << Scale{scale};
        comma = true;
    }
    if (preindex)
    {
        *this << "]";
        comma = true;
    }
    if(outer)
    {
        if (comma) *this << ",";
        *this << Int((i16)outer);
    }

    *this << ")";
}

}

