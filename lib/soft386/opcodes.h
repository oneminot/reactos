/*
 * Soft386 386/486 CPU Emulation Library
 * opcodes.h
 *
 * Copyright (C) 2013 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _OPCODES_H_
#define _OPCODES_H_

/* DEFINES ********************************************************************/

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define SOFT386_NUM_OPCODE_HANDLERS 256
#define SOFT386_OPCODE_WRITE_REG (1 << 1)
#define SOFT386_OPCODE_HANDLER(x)   \
    BOOLEAN FASTCALL x(PSOFT386_STATE State, UCHAR Opcode)

typedef BOOLEAN (FASTCALL *SOFT386_OPCODE_HANDLER_PROC)(PSOFT386_STATE, UCHAR);

extern
SOFT386_OPCODE_HANDLER_PROC
Soft386OpcodeHandlers[SOFT386_NUM_OPCODE_HANDLERS];

SOFT386_OPCODE_HANDLER(Soft386OpcodePrefix);
SOFT386_OPCODE_HANDLER(Soft386OpcodeIncrement);
SOFT386_OPCODE_HANDLER(Soft386OpcodeDecrement);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushReg);
SOFT386_OPCODE_HANDLER(Soft386OpcodePopReg);
SOFT386_OPCODE_HANDLER(Soft386OpcodeNop);
SOFT386_OPCODE_HANDLER(Soft386OpcodeExchangeEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeShortConditionalJmp);
SOFT386_OPCODE_HANDLER(Soft386OpcodeClearCarry);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSetCarry);
SOFT386_OPCODE_HANDLER(Soft386OpcodeComplCarry);
SOFT386_OPCODE_HANDLER(Soft386OpcodeClearInt);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSetInt);
SOFT386_OPCODE_HANDLER(Soft386OpcodeClearDir);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSetDir);
SOFT386_OPCODE_HANDLER(Soft386OpcodeHalt);
SOFT386_OPCODE_HANDLER(Soft386OpcodeInByte);
SOFT386_OPCODE_HANDLER(Soft386OpcodeIn);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOutByte);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOut);
SOFT386_OPCODE_HANDLER(Soft386OpcodeShortJump);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovRegImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovByteRegImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAddEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOrByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOrModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOrAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeOrEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAndByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAndModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAndAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAndEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeXorByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeXorModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeXorAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeXorEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeTestByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeTestModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeTestAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeTestEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeXchgByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeXchgModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushEs);
SOFT386_OPCODE_HANDLER(Soft386OpcodePopEs);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushCs);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAdcEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushSs);
SOFT386_OPCODE_HANDLER(Soft386OpcodePopSs);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSbbEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushDs);
SOFT386_OPCODE_HANDLER(Soft386OpcodePopDs);
SOFT386_OPCODE_HANDLER(Soft386OpcodeDaa);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCmpSubEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeDas);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAaa);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAas);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushAll);
SOFT386_OPCODE_HANDLER(Soft386OpcodePopAll);
SOFT386_OPCODE_HANDLER(Soft386OpcodeBound);
SOFT386_OPCODE_HANDLER(Soft386OpcodeArpl);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeImulModrmImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushByteImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovByteModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovModrm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovStoreSeg);
SOFT386_OPCODE_HANDLER(Soft386OpcodeLea);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovLoadSeg);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCwde);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCdq);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCallAbs);
SOFT386_OPCODE_HANDLER(Soft386OpcodeWait);
SOFT386_OPCODE_HANDLER(Soft386OpcodePushFlags);
SOFT386_OPCODE_HANDLER(Soft386OpcodePopFlags);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSahf);
SOFT386_OPCODE_HANDLER(Soft386OpcodeLahf);
SOFT386_OPCODE_HANDLER(Soft386OpcodeRet);
SOFT386_OPCODE_HANDLER(Soft386OpcodeLdsLes);
SOFT386_OPCODE_HANDLER(Soft386OpcodeEnter);
SOFT386_OPCODE_HANDLER(Soft386OpcodeLeave);
SOFT386_OPCODE_HANDLER(Soft386OpcodeRetFarImm);
SOFT386_OPCODE_HANDLER(Soft386OpcodeRetFar);
SOFT386_OPCODE_HANDLER(Soft386OpcodeInt);
SOFT386_OPCODE_HANDLER(Soft386OpcodeIret);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAam);
SOFT386_OPCODE_HANDLER(Soft386OpcodeAad);
SOFT386_OPCODE_HANDLER(Soft386OpcodeXlat);
SOFT386_OPCODE_HANDLER(Soft386OpcodeLoop);
SOFT386_OPCODE_HANDLER(Soft386OpcodeJecxz);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCall);
SOFT386_OPCODE_HANDLER(Soft386OpcodeJmp);
SOFT386_OPCODE_HANDLER(Soft386OpcodeJmpAbs);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovAlOffset);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovEaxOffset);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovOffsetAl);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovOffsetEax);
SOFT386_OPCODE_HANDLER(Soft386OpcodeSalc);
SOFT386_OPCODE_HANDLER(Soft386OpcodeMovs);
SOFT386_OPCODE_HANDLER(Soft386OpcodeCmps);
SOFT386_OPCODE_HANDLER(Soft386OpcodeStos);
SOFT386_OPCODE_HANDLER(Soft386OpcodeLods);
SOFT386_OPCODE_HANDLER(Soft386OpcodeScas);

#endif // _OPCODES_H_
