// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <Mode M, Size S> bool
Moira::addressErrorDeprecated(u32 addr)
{
    if (MOIRA_EMULATE_ADDRESS_ERROR) {

        if ((addr & 1) && S != Byte && isMemMode(M)) {
            execAddressError(addr);
            return true;
        }
    }
    return false;
}

template <Size S> bool
Moira::addressError(u32 addr)
{
    if (MOIRA_EMULATE_ADDRESS_ERROR) {

        if ((addr & 1) && S != Byte) {
            execAddressError(addr);
            return true;
        }
    }
    return false;
}

template<> u32
Moira::readM<Byte>(u32 addr)
{
    sync(2);
    u32 result = memory->moiraRead8(addr & 0xFFFFFF);
    sync(2);

    return result;
}

template<> u32
Moira::readM<Byte>(u32 addr, bool &error)
{
    error = false;

    return readM<Byte>(addr);
}

template<> u32
Moira::readM<Word>(u32 addr)
{
    sync(2);
    u32 result = memory->moiraRead16(addr & 0xFFFFFF);
    sync(2);

    return result;
}

template<> u32
Moira::readM<Word>(u32 addr, bool &error)
{
    sync(2);
    if ((error = addressError<Word>(addr))) { return 0; }
    u32 result = memory->moiraRead16(addr & 0xFFFFFF);
    sync(2);

    return result;
}

template<> u32
Moira::readM<Long>(u32 addr)
{
    u32 hi = readM<Word>(addr);
    u32 lo = readM<Word>(addr);

    return hi << 16 | lo;
}

template<> u32
Moira::readM<Long>(u32 addr, bool &error)
{
    u32 hi = readM<Word>(addr, error);
    if (error) return 0;
    u32 lo = readM<Word>(addr + 2);

    return hi << 16 | lo;
}

bool
Moira::read8(u32 addr, u8 &value)
{
    sync(2);

    value = memory->moiraRead8(addr & 0xFFFFFF);

    sync(2);

    return true;
}

bool
Moira::read16(u32 addr, u16 &value)
{
    sync(2);

    if (addressError<Word>(addr)) return false;
    value = (u32)memory->moiraRead16(addr & 0xFFFFFF);

    sync(2);

    return true;
}

bool
Moira::read32(u32 addr, u32 &value)
{
    u16 hi, lo;

    if (!read16(addr, hi)) return false;
    if (!read16(addr + 2, lo)) return false;

    value = (u32)hi << 16 | (u32)lo;
    return true;
}

u32
Moira::readOnReset(u32 addr)
{
    sync(2);
    u32 result = memory->moiraReadAfterReset16(addr & 0xFFFFFF);
    sync(2);

    return result;
}

template<> u32
Moira::readMDeprecated<Byte>(u32 addr)
{
    u8 v; read8(addr, v); return v;
}

template<> u32
Moira::readMDeprecated<Word>(u32 addr)
{
    u16 v; read16(addr, v); return v;
}

template<> u32
Moira::readMDeprecated<Long>(u32 addr)
{
    u32 v; read32(addr, v); return v;
}

template<> void
Moira::writeMDeprecated<Byte>(u32 addr, u32 value)
{
    write8(addr, value);
}

template<> void
Moira::writeMDeprecated<Word>(u32 addr, u32 value)
{
    write16(addr, value);
}

template<> void
Moira::writeMDeprecated<Long>(u32 addr, u32 value)
{
    write32(addr, value);
}

template<> bool
Moira::readMDeprecated<Byte>(u32 addr, u32 &value)
{
    u8 v;
    if (!read8(addr, v)) { value = v; return true; }
    return false;
}

template<> bool
Moira::readMDeprecated<Word>(u32 addr, u32 &value)
{
    u16 v;
    if (!read16(addr, v)) { value = v; return true; }
    return false;
}

template<> bool
Moira::readMDeprecated<Long>(u32 addr, u32 &value)
{
    u32 v;
    if (!read32(addr, v)) { value = v; return true; }
    return false;
}

bool
Moira::write8(u32 addr, u8 value)
{
    sync(2);

    memory->moiraWrite8(addr & 0xFFFFFF, value);

    sync(2);

    return true;
}

bool
Moira::write16(u32 addr, u16 value)
{
    sync(2);

    if (addressError<Word>(addr)) return false;
    memory->moiraWrite16(addr & 0xFFFFFF, value);

    sync(2);

    return true;
}

bool
Moira::write32(u32 addr, u32 value)
{
    u16 hi = value >> 16;
    u16 lo = value & 0xFFFF;

    if (!write16(addr, hi)) return false;
    if (!write16(addr, lo)) return false;

    return true;
}

template<> u32
Moira::read<Byte>(u32 addr)
{
    return memory->moiraRead8(addr);
}

template<> u32
Moira::read<Word>(u32 addr)
{
    return memory->moiraRead16(addr);
}

template<> u32
Moira::read<Long>(u32 addr)
{
    return memory->moiraRead16(addr) << 16 | memory->moiraRead16(addr + 2);
}

template<> void
Moira::write<Byte>(u32 addr, u32 value)
{
    memory->moiraWrite8(addr & 0xFFFFFF, (u8)value);
}

template<> void
Moira::write<Word>(u32 addr, u32 value)
{
    memory->moiraWrite16(addr & 0xFFFFFF, (u16)value);
}

template<> void
Moira::write<Long>(u32 addr, u32 value)
{
    memory->moiraWrite16(addr & 0xFFFFFF, (u16)(value >> 16));
    memory->moiraWrite16((addr + 2) & 0xFFFFFF, (u16)value);
}

void
Moira::push(u32 value)
{
    reg.sp -= 4;
    write<Long>(reg.sp, value);
}

template<Mode M, Size S, u8 flags> u32
Moira::computeEA(u32 n) {

    assert(n < 8);

    u32 result;

    switch (M) {

        case 0:  // Dn
        case 1:  // An
        {
            result = n;
            break;
        }
        case 2:  // (An)
        {
            result = readA(n);
            break;
        }
        case 3:  // (An)+
        {
            result = readA(n);
            if (!(flags & SKIP_POST_PRE)) postIncPreDec<M,S>(n);
            break;
        }
        case 4:  // -(An)
        {
            result = readA(n) - ((n == 7 && S == Byte) ? 2 : S);
            if (!(flags & SKIP_POST_PRE)) postIncPreDec<M,S>(n);
            break;
        }
        case 5: // (d,An)
        {
            u32 an = readA(n);
            i16  d = (i16)irc;

            result = d + an;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 6: // (d,An,Xi)
        {
            i8   d = (i8)irc;
            i32 an = readA(n);
            i32 xi = readR((irc >> 12) & 0b1111);

            result = d + an + ((irc & 0x800) ? xi : (i16)xi);

            sync(2);
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 7: // ABS.W
        {
            result = irc;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 8: // ABS.L
        {
            result = irc << 16;
            readExtensionWord();
            result |= irc;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 9: // (d,PC)
        {
            i16  d = (i16)irc;

            result = reg.pc + d;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 10: // (d,PC,Xi)
        {
            i8   d = (i8)irc;
            i32 xi = readR((irc >> 12) & 0b1111);

            result = d + reg.pc + ((irc & 0x800) ? xi : (i16)xi);

            sync(2);
            readExtensionWord();
            break;
        }
        case 11: // Im
        {
            result = readImm<S>();
            break;
        }
        default:
        {
            assert(false);
        }
    }
    return result;
}

template<Mode M, Size S> void
Moira::postIncPreDec(int n)
{
    if (M == 3) { // (An)+
        sync(2);
        reg.a[n] += (n == 7 && S == Byte) ? 2 : S;
    }
    if (M == 4) { // (-(An)
        sync(2);
        reg.a[n] -= (n == 7 && S == Byte) ? 2 : S;
    }
}

template<Mode M, Size S> bool
Moira::readOperand(int n, u32 &ea, u32 &result)
{
    switch (M) {

        case 0:  // Dn
        {
            result = readD<S>(n);
            return true;
        }
        case 1:  // An
        {
            result = readA<S>(n);
            return true;
        }
        case 11: // Im
        {
            result = readImm<S>();
            return true;
        }
        default:
        {
            ea = computeEA<M,S,SKIP_POST_PRE>(n);
            if (addressErrorDeprecated<M,S>(ea)) return false;

            postIncPreDec<M,S>(n);
            result = readMDeprecated<S>(ea);
            return true;
        }
    }
}

template<Mode M, Size S> bool
Moira::writeOperand(int n, u32 value)
{
    switch (M) {

        case 0:  // Dn
        {
            writeD<S>(n, value);
            return true;
        }
        case 1:  // An
        {
            writeA<S>(n, value);
            return true;
        }
        case 11: // Im
        {
            assert(false);
            return false;
        }
        default:
        {
            u32 ea = computeEA<M,S,SKIP_POST_PRE>(n);
            if (addressErrorDeprecated<M,S>(ea)) return false;

            postIncPreDec<M,S>(n);
            write<S>(ea, value);
            return true;
        }
    }
}

template<Mode M, Size S> void
Moira::writeOperand(int n, u32 ea, u32 value)
{
    assert(M < 11);

    switch (M) {

        case 0:  // Dn
        {
            writeD<S>(n, value);
            break;
        }
        case 1:  // An
        {
            writeA<S>(n, value);
            break;
        }
        case 11: // Im
        {
            assert(false);
            break;
        }
        default:
        {
            write<S>(ea, value);
            break;
        }
    }
}

template<Size S> u32
Moira::readImm()
{
    u32 result;

    switch (S) {
        case Byte:
            result = (u8)irc;
            readExtensionWord();
            break;
        case Word:
            result = irc;
            readExtensionWord();
            break;
        case Long:
            result = irc << 16;
            readExtensionWord();
            result |= irc;
            readExtensionWord();
            break;
    }

    return result;
}
