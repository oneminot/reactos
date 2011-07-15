	OpAddr = 0x03,	/* 1 op, const addr */
	OpDeref = 0x06,
	OpConst1u = 0x08,	/* 1 op, 1 byte const */
	OpConst1s = 0x09,	/*	" signed */
	OpConst2u = 0x0A,	/* 1 op, 2 byte const  */
	OpConst2s = 0x0B,	/*	" signed */
	OpConst4u = 0x0C,	/* 1 op, 4 byte const */
	OpConst4s = 0x0D,	/*	" signed */
	OpConst8u = 0x0E,	/* 1 op, 8 byte const */
	OpConst8s = 0x0F,	/*	" signed */
	OpConstu = 0x10,	/* 1 op, LEB128 const */
	OpConsts = 0x11,	/*	" signed */
	OpDup = 0x12,
	OpDrop = 0x13,
	OpOver = 0x14,
	OpPick = 0x15,		/* 1 op, 1 byte stack index */
	OpSwap = 0x16,
	OpRot = 0x17,
	OpXderef = 0x18,
	OpAbs = 0x19,
	OpAnd = 0x1A,
	OpDiv = 0x1B,
	OpMinus = 0x1C,
	OpMod = 0x1D,
	OpMul = 0x1E,
	OpNeg = 0x1F,
	OpNot = 0x20,
	OpOr = 0x21,
	OpPlus = 0x22,
	OpPlusUconst = 0x23,	/* 1 op, ULEB128 addend */
	OpShl = 0x24,
	OpShr = 0x25,
	OpShra = 0x26,
	OpXor = 0x27,
	OpSkip = 0x2F,		/* 1 op, signed 2-byte constant */
	OpBra = 0x28,		/* 1 op, signed 2-byte constant */
	OpEq = 0x29,
	OpGe = 0x2A,
	OpGt = 0x2B,
	OpLe = 0x2C,
	OpLt = 0x2D,
	OpNe = 0x2E,
	OpLit0 = 0x30,
		/* OpLitN = OpLit0 + N for N = 0..31 */
	OpReg0 = 0x50,
		/* OpRegN = OpReg0 + N for N = 0..31 */
	OpBreg0 = 0x70,	/* 1 op, signed LEB128 constant */
		/* OpBregN = OpBreg0 + N for N = 0..31 */
	OpRegx = 0x90,	/* 1 op, ULEB128 register */
	OpFbreg = 0x91,	/* 1 op, SLEB128 offset */
	OpBregx = 0x92,	/* 2 op, ULEB128 reg, SLEB128 off */
	OpPiece = 0x93,	/* 1 op, ULEB128 size of piece */
	OpDerefSize = 0x94,	/* 1-byte size of data retrieved */
	OpXderefSize = 0x95,	/* 1-byte size of data retrieved */
	OpNop = 0x96,
	/* next four new in Dwarf v3 */
	OpPushObjAddr = 0x97,
	OpCall2 = 0x98,	/* 2-byte offset of DIE */
	OpCall4 = 0x99,	/* 4-byte offset of DIE */
	OpCallRef = 0x9A,	/* 4- or 8- byte offset of DIE */
	/* 0xE0-0xFF reserved for user-specific */
