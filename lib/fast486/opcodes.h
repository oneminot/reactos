/*
 * Fast486 386/486 CPU Emulation Library
 * opcodes.h
 *
 * Copyright (C) 2014 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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

#pragma once

/* DEFINES ********************************************************************/

#define FAST486_NUM_OPCODE_HANDLERS 256
#define FAST486_OPCODE_WRITE_REG (1 << 1)
#define FAST486_OPCODE_HANDLER(x)   \
    BOOLEAN FASTCALL x(PFAST486_STATE State, UCHAR Opcode)

typedef BOOLEAN (FASTCALL *FAST486_OPCODE_HANDLER_PROC)(PFAST486_STATE, UCHAR);

extern
FAST486_OPCODE_HANDLER_PROC
Fast486OpcodeHandlers[FAST486_NUM_OPCODE_HANDLERS];

FAST486_OPCODE_HANDLER(Fast486OpcodeInvalid);

FAST486_OPCODE_HANDLER(Fast486OpcodePrefix);
FAST486_OPCODE_HANDLER(Fast486OpcodeIncrement);
FAST486_OPCODE_HANDLER(Fast486OpcodeDecrement);
FAST486_OPCODE_HANDLER(Fast486OpcodePushReg);
FAST486_OPCODE_HANDLER(Fast486OpcodePopReg);
FAST486_OPCODE_HANDLER(Fast486OpcodeNop);
FAST486_OPCODE_HANDLER(Fast486OpcodeExchangeEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeShortConditionalJmp);
FAST486_OPCODE_HANDLER(Fast486OpcodeClearCarry);
FAST486_OPCODE_HANDLER(Fast486OpcodeSetCarry);
FAST486_OPCODE_HANDLER(Fast486OpcodeComplCarry);
FAST486_OPCODE_HANDLER(Fast486OpcodeClearInt);
FAST486_OPCODE_HANDLER(Fast486OpcodeSetInt);
FAST486_OPCODE_HANDLER(Fast486OpcodeClearDir);
FAST486_OPCODE_HANDLER(Fast486OpcodeSetDir);
FAST486_OPCODE_HANDLER(Fast486OpcodeHalt);
FAST486_OPCODE_HANDLER(Fast486OpcodeInByte);
FAST486_OPCODE_HANDLER(Fast486OpcodeIn);
FAST486_OPCODE_HANDLER(Fast486OpcodeOutByte);
FAST486_OPCODE_HANDLER(Fast486OpcodeOut);
FAST486_OPCODE_HANDLER(Fast486OpcodeShortJump);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovRegImm);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovByteRegImm);
FAST486_OPCODE_HANDLER(Fast486OpcodeAddByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeAddModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeAddAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeAddEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeOrByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeOrModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeOrAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeOrEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeAndByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeAndModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeAndAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeAndEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeXorByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeXorModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeXorAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeXorEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeTestByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeTestModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeTestAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeTestEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeXchgByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeXchgModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodePushEs);
FAST486_OPCODE_HANDLER(Fast486OpcodePopEs);
FAST486_OPCODE_HANDLER(Fast486OpcodePushCs);
FAST486_OPCODE_HANDLER(Fast486OpcodeAdcByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeAdcModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeAdcAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeAdcEax);
FAST486_OPCODE_HANDLER(Fast486OpcodePushSs);
FAST486_OPCODE_HANDLER(Fast486OpcodePopSs);
FAST486_OPCODE_HANDLER(Fast486OpcodeSbbByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeSbbModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeSbbAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeSbbEax);
FAST486_OPCODE_HANDLER(Fast486OpcodePushDs);
FAST486_OPCODE_HANDLER(Fast486OpcodePopDs);
FAST486_OPCODE_HANDLER(Fast486OpcodeDaa);
FAST486_OPCODE_HANDLER(Fast486OpcodeCmpSubByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeCmpSubModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeCmpSubAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeCmpSubEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeDas);
FAST486_OPCODE_HANDLER(Fast486OpcodeAaa);
FAST486_OPCODE_HANDLER(Fast486OpcodeAas);
FAST486_OPCODE_HANDLER(Fast486OpcodePushAll);
FAST486_OPCODE_HANDLER(Fast486OpcodePopAll);
FAST486_OPCODE_HANDLER(Fast486OpcodeBound);
FAST486_OPCODE_HANDLER(Fast486OpcodeArpl);
FAST486_OPCODE_HANDLER(Fast486OpcodePushImm);
FAST486_OPCODE_HANDLER(Fast486OpcodeImulModrmImm);
FAST486_OPCODE_HANDLER(Fast486OpcodePushByteImm);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovByteModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovModrm);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovStoreSeg);
FAST486_OPCODE_HANDLER(Fast486OpcodeLea);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovLoadSeg);
FAST486_OPCODE_HANDLER(Fast486OpcodeCwde);
FAST486_OPCODE_HANDLER(Fast486OpcodeCdq);
FAST486_OPCODE_HANDLER(Fast486OpcodeCallAbs);
FAST486_OPCODE_HANDLER(Fast486OpcodeWait);
FAST486_OPCODE_HANDLER(Fast486OpcodePushFlags);
FAST486_OPCODE_HANDLER(Fast486OpcodePopFlags);
FAST486_OPCODE_HANDLER(Fast486OpcodeSahf);
FAST486_OPCODE_HANDLER(Fast486OpcodeLahf);
FAST486_OPCODE_HANDLER(Fast486OpcodeRet);
FAST486_OPCODE_HANDLER(Fast486OpcodeLdsLes);
FAST486_OPCODE_HANDLER(Fast486OpcodeEnter);
FAST486_OPCODE_HANDLER(Fast486OpcodeLeave);
FAST486_OPCODE_HANDLER(Fast486OpcodeRetFarImm);
FAST486_OPCODE_HANDLER(Fast486OpcodeRetFar);
FAST486_OPCODE_HANDLER(Fast486OpcodeInt);
FAST486_OPCODE_HANDLER(Fast486OpcodeIret);
FAST486_OPCODE_HANDLER(Fast486OpcodeAam);
FAST486_OPCODE_HANDLER(Fast486OpcodeAad);
FAST486_OPCODE_HANDLER(Fast486OpcodeXlat);
FAST486_OPCODE_HANDLER(Fast486OpcodeLoop);
FAST486_OPCODE_HANDLER(Fast486OpcodeJecxz);
FAST486_OPCODE_HANDLER(Fast486OpcodeCall);
FAST486_OPCODE_HANDLER(Fast486OpcodeJmp);
FAST486_OPCODE_HANDLER(Fast486OpcodeJmpAbs);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovAlOffset);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovEaxOffset);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovOffsetAl);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovOffsetEax);
FAST486_OPCODE_HANDLER(Fast486OpcodeSalc);
FAST486_OPCODE_HANDLER(Fast486OpcodeMovs);
FAST486_OPCODE_HANDLER(Fast486OpcodeCmps);
FAST486_OPCODE_HANDLER(Fast486OpcodeStos);
FAST486_OPCODE_HANDLER(Fast486OpcodeLods);
FAST486_OPCODE_HANDLER(Fast486OpcodeScas);
FAST486_OPCODE_HANDLER(Fast486OpcodeIns);
FAST486_OPCODE_HANDLER(Fast486OpcodeOuts);

#endif // _OPCODES_H_

/* EOF */
