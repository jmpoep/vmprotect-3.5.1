#ifndef IL_H
#define IL_H

enum ILCommandType {
	icUnknown, icByte, icWord, icDword, icQword, icComment, icData, icCase,
	icNop, icBreak, icLdarg_0, icLdarg_1, icLdarg_2, icLdarg_3, icLdloc_0, icLdloc_1, icLdloc_2, icLdloc_3, icStloc_0, icStloc_1, 
	icStloc_2, icStloc_3, icLdarg_s, icLdarga_s, icStarg_s, icLdloc_s, icLdloca_s, icStloc_s, icLdnull, icLdc_i4_m1, icLdc_i4_0, 
	icLdc_i4_1, icLdc_i4_2, icLdc_i4_3, icLdc_i4_4, icLdc_i4_5, icLdc_i4_6, icLdc_i4_7, icLdc_i4_8, icLdc_i4_s, icLdc_i4, icLdc_i8, 
	icLdc_r4, icLdc_r8, icDup, icPop, icJmp, icCall, icCalli, icRet, icBr_s, icBrfalse_s, icBrtrue_s, icBeq_s, icBge_s, icBgt_s, icBle_s, 
	icBlt_s, icBne_un_s, icBge_un_s, icBgt_un_s, icBle_un_s, icBlt_un_s, icBr, icBrfalse, icBrtrue, icBeq, icBge, icBgt, icBle, icBlt, 
	icBne_un, icBge_un, icBgt_un, icBle_un, icBlt_un, icSwitch, icLdind_i1, icLdind_u1, icLdind_i2, icLdind_u2, icLdind_i4, icLdind_u4, 
	icLdind_i8, icLdind_i, icLdind_r4, icLdind_r8, icLdind_ref, icStind_ref, icStind_i1, icStind_i2, icStind_i4, icStind_i8, icStind_r4,
	icStind_r8, icAdd, icSub, icMul, icDiv, icDiv_un, icRem, icRem_un, icAnd, icOr, icXor, icShl, icShr, icShr_un, icNeg, icNot, icConv_i1, icConv_i2, 
	icConv_i4, icConv_i8, icConv_r4, icConv_r8, icConv_u4, icConv_u8, icCallvirt, icCpobj, icLdobj, icLdstr, icNewobj, icCastclass, 
	icIsinst, icConv_r_un, icUnbox, icThrow, icLdfld, icLdflda, icStfld, icLdsfld, icLdsflda, icStsfld, icStobj, icConv_ovf_i1_un, 
	icConv_ovf_i2_un, icConv_ovf_i4_un, icConv_ovf_i8_un, icConv_ovf_u1_un, icConv_ovf_u2_un, icConv_ovf_u4_un, icConv_ovf_u8_un, 
	icConv_ovf_i_un, icConv_ovf_u_un, icBox, icNewarr, icLdlen, icLdelema, icLdelem_i1, icLdelem_u1, icLdelem_i2, icLdelem_u2, 
	icLdelem_i4, icLdelem_u4, icLdelem_i8, icLdelem_i, icLdelem_r4, icLdelem_r8, icLdelem_ref, icStelem_i, icStelem_i1, icStelem_i2, 
	icStelem_i4, icStelem_i8, icStelem_r4, icStelem_r8, icStelem_ref, icLdelem, icStelem, icUnbox_any, icConv_ovf_i1, icConv_ovf_u1, 
	icConv_ovf_i2, icConv_ovf_u2, icConv_ovf_i4, icConv_ovf_u4, icConv_ovf_i8, icConv_ovf_u8, icRefanyval, icCkfinite, icMkrefany, 
	icLdtoken, icConv_u2, icConv_u1, icConv_i, icConv_ovf_i, icConv_ovf_u, icAdd_ovf, icAdd_ovf_un, icMul_ovf, icMul_ovf_un, icSub_ovf,
	icSub_ovf_un, icEndfinally, icLeave, icLeave_s, icStind_i, icConv_u, icArglist, icCeq, icCgt, icCgt_un, icClt, icClt_un, icLdftn, icLdvirtftn, 
	icLdarg, icLdarga, icStarg, icLdloc, icLdloca, icStloc, icLocalloc, icEndfilter, icUnaligned, icVolatile, icTail, icInitobj, icConstrained, 
	icCpblk, icInitblk, icNo, icRethrow, icSizeof, icRefanytype, icReadonly,
	icConv, icConv_ovf, icConv_ovf_un, icLdmem_i4, icInitarg, icInitcatchblock, icEntertry, icCallvm, icCallvmvirt,
	icCmp, icCmp_un,
	icCNT
};

enum ILFlowControl : uint8_t
{
	Branch, 
	Break, 
	Call, 
	Cond_Branch, 
	Meta,			// Provides information about a subsequent instruction. For example, the Unaligned instruction of Reflection.Emit.Opcodes has FlowControl.Meta and specifies that the subsequent pointer instruction might be unaligned. 
	Next,			// Normal flow of control
	Phi,			// Obsolete. This enumerator value is reserved and should not be used.
	Return, 
	Throw
};

enum ILOpCodeType : uint8_t
{
	Experimental,	// Reserved (ECMA) instruction
	Macro,			// synonym for other MSIL instructions (ldarg.0 - ldarg)
	Reserved,		// Reserved (ECMA) instruction
	Objmodel,		// Instruction that applies to objects
	Prefix,			// Prefix instruction that modifies the behavior of the following instruction
	Primitive		// Built-in instruction
};

enum ILOperandType : uint8_t// The operand is a ...
{
	InlineBrTarget,			// 32-bit integer branch target
	InlineField,			// 32-bit metadata token
	InlineI,				// 32-bit integer
	InlineI8,				// 64-bit integer
	InlineMethod,			// 32-bit metadata token
	InlineNone,				// No operand
	InlinePhi,				// Obsolete. The operand is reserved and should not be used.
	InlineR,				// 64-bit IEEE floating point number
	Inline8,				// not used
	InlineSig,				// 32-bit metadata signature token
	InlineString,			// 32-bit metadata string token
	InlineSwitch,			// 32-bit integer argument to a switch instruction
	InlineTok,				// FieldRef, MethodRef, or TypeRef token
	InlineType,				// 32-bit metadata token
	InlineVar,				// 16-bit integer containing the ordinal of a local variable or an argument
	ShortInlineBrTarget,	// 8-bit integer branch target
	ShortInlineI,			// 8-bit integer
	ShortInlineR,			// 32-bit IEEE floating point number
	ShortInlineVar,			// 8-bit integer containing the ordinal of a local variable or an argument
};

enum ILStackBehaviour : uint8_t
{
	Pop0,				// No values are popped off the stack.
	Pop1,				// Pops one value off the stack.
	Pop1_pop1,			// Pops 1 value off the stack for the first operand, and 1 value of the stack for the second operand.
	Popi,				// Pops a 32-bit integer off the stack.
	Popi_pop1,			// Pops a 32-bit integer off the stack for the first operand, and a value off the stack for the second operand.
	Popi_popi,			// Pops a 32-bit integer off the stack for the first operand, and a 32-bit integer off the stack for the second operand.
	Popi_popi8,			// Pops a 32-bit integer off the stack for the first operand, and a 64-bit integer off the stack for the second operand.
	Popi_popi_popi,		// Pops a 32-bit integer off the stack for the first operand, a 32-bit integer off the stack for the second operand, and a 32-bit integer off the stack for the third operand.
	Popi_popr4,			// Pops a 32-bit integer off the stack for the first operand, and a 32-bit floating point number off the stack for the second operand.
	Popi_popr8,			// Pops a 32-bit integer off the stack for the first operand, and a 64-bit floating point number off the stack for the second operand.
	Popref,				// Pops a reference off the stack.
	Popref_pop1,		// Pops a reference off the stack for the first operand, and a value off the stack for the second operand.
	Popref_popi,		// Pops a reference off the stack for the first operand, and a 32-bit integer off the stack for the second operand.
	Popref_popi_popi,	// Pops a reference off the stack for the first operand, a value off the stack for the second operand, and a value off the stack for the third operand.
	Popref_popi_popi8,	// Pops a reference off the stack for the first operand, a value off the stack for the second operand, and a 64-bit integer off the stack for the third operand.
	Popref_popi_popr4,	// Pops a reference off the stack for the first operand, a value off the stack for the second operand, and a 32-bit integer off the stack for the third operand.
	Popref_popi_popr8,	// Pops a reference off the stack for the first operand, a value off the stack for the second operand, and a 64-bit floating point number off the stack for the third operand.
	Popref_popi_popref,	// Pops a reference off the stack for the first operand, a value off the stack for the second operand, and a reference off the stack for the third operand.
	Push0,				// No values are pushed onto the stack.
	Push1,				// Pushes one value onto the stack.
	Push1_push1,		// Pushes 1 value onto the stack for the first operand, and 1 value onto the stack for the second operand.
	Pushi,				// Pushes a 32-bit integer onto the stack.
	Pushi8,				// Pushes a 64-bit integer onto the stack.
	Pushr4,				// Pushes a 32-bit floating point number onto the stack.
	Pushr8,				// Pushes a 64-bit floating point number onto the stack.
	Pushref,			// Pushes a reference onto the stack.
	Varpop,				// Pops a variable off the stack.
	Varpush,			// Pushes a variable onto the stack.
	Popref_popi_pop1	// Pops a reference off the stack for the first operand, a value off the stack for the second operand, and a 32-bit integer off the stack for the third operand.
};

struct ILOpCode {
	const char *name;
	ILStackBehaviour pop;
	ILStackBehaviour push;
	ILOperandType operand_type;
	ILOpCodeType opcode_type;
	ILFlowControl flow_type;
	bool is_ret;
	int stack_change;
} const ILOpCodes[icCNT] = {
	{".byte ??", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{".byte", Pop0, Push0, ShortInlineI, Primitive, Next, false, 0},
	{".word", Pop0, Push0, InlineVar, Primitive, Next, false, 0},
	{".dword", Pop0, Push0, InlineI, Primitive, Next, false, 0},
	{".qword", Pop0, Push0, InlineI8, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{".byte", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineI, Primitive, Next, false, 0},
	{"nop", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"break", Pop0, Push0, InlineNone, Primitive, Break, false, 0},
	{"ldarg.0", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"ldarg.1", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"ldarg.2", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"ldarg.3", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"ldloc.0", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"ldloc.1", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"ldloc.2", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"ldloc.3", Pop0, Push1, InlineNone, Macro, Next, false, 1},
	{"stloc.0", Pop1, Push0, InlineNone, Macro, Next, false, -1},
	{"stloc.1", Pop1, Push0, InlineNone, Macro, Next, false, -1},
	{"stloc.2", Pop1, Push0, InlineNone, Macro, Next, false, -1},
	{"stloc.3", Pop1, Push0, InlineNone, Macro, Next, false, -1},
	{"ldarg.s", Pop0, Push1, ShortInlineVar, Macro, Next, false, 1},
	{"ldarga.s", Pop0, Pushi, ShortInlineVar, Macro, Next, false, 1},
	{"starg.s", Pop1, Push0, ShortInlineVar, Macro, Next, false, -1},
	{"ldloc.s", Pop0, Push1, ShortInlineVar, Macro, Next, false, 1},
	{"ldloca.s", Pop0, Pushi, ShortInlineVar, Macro, Next, false, 1},
	{"stloc.s", Pop1, Push0, ShortInlineVar, Macro, Next, false, -1},
	{"ldnull", Pop0, Pushref, InlineNone, Primitive, Next, false, 1},
	{"ldc.i4.m1", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.0", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.1", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.2", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.3", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.4", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.5", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.6", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.7", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.8", Pop0, Pushi, InlineNone, Macro, Next, false, 1},
	{"ldc.i4.s", Pop0, Pushi, ShortInlineI, Macro, Next, false, 1},
	{"ldc.i4", Pop0, Pushi, InlineI, Primitive, Next, false, 1},
	{"ldc.i8", Pop0, Pushi8, InlineI8, Primitive, Next, false, 1},
	{"ldc.r4", Pop0, Pushr4, ShortInlineR, Primitive, Next, false, 1},
	{"ldc.r8", Pop0, Pushr8, InlineR, Primitive, Next, false, 1},
	{"dup", Pop1, Push1_push1, InlineNone, Primitive, Next, false, 1},
	{"pop", Pop1, Push0, InlineNone, Primitive, Next, false, -1},
	{"jmp", Pop0, Push0, InlineMethod, Primitive, Call, true, 0},
	{"call", Varpop, Varpush, InlineMethod, Primitive, Call, false, 0},
	{"calli", Varpop, Varpush, InlineSig, Primitive, Call, false, 0},
	{"ret", Varpop, Push0, InlineNone, Primitive, Return, true, 0},
	{"br.s", Pop0, Push0, ShortInlineBrTarget, Macro, Branch, true, 0},
	{"brfalse.s", Popi, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -1},
	{"brtrue.s", Popi, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -1},
	{"beq.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bge.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bgt.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"ble.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"blt.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bne.un.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bge.un.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bgt.un.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"ble.un.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"blt.un.s", Pop1_pop1, Push0, ShortInlineBrTarget, Macro, Cond_Branch, false, -2},
	{"br", Pop0, Push0, InlineBrTarget, Primitive, Branch, true, 0},
	{"brfalse", Popi, Push0, InlineBrTarget, Primitive, Cond_Branch, false, -1},
	{"brtrue", Popi, Push0, InlineBrTarget, Primitive, Cond_Branch, false, -1},
	{"beq", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bge", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bgt", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"ble", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"blt", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bne.un", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bge.un", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"bgt.un", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"ble.un", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"blt.un", Pop1_pop1, Push0, InlineBrTarget, Macro, Cond_Branch, false, -2},
	{"switch", Popi, Push0, InlineSwitch, Primitive, Cond_Branch, false, -1},
	{"ldind.i1", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"ldind.u1", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"ldind.i2", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"ldind.u2", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"ldind.i4", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"ldind.u4", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"ldind.i8", Popi, Pushi8, InlineNone, Primitive, Next, false, 0},
	{"ldind.i", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"ldind.r4", Popi, Pushr4, InlineNone, Primitive, Next, false, 0},
	{"ldind.r8", Popi, Pushr8, InlineNone, Primitive, Next, false, 0},
	{"ldind.ref", Popi, Pushref, InlineNone, Primitive, Next, false, 0},
	{"stind.ref", Popi_popi, Push0, InlineNone, Primitive, Next, false, -2},
	{"stind.i1", Popi_popi, Push0, InlineNone, Primitive, Next, false, -2},
	{"stind.i2", Popi_popi, Push0, InlineNone, Primitive, Next, false, -2},
	{"stind.i4", Popi_popi, Push0, InlineNone, Primitive, Next, false, -2},
	{"stind.i8", Popi_popi8, Push0, InlineNone, Primitive, Next, false, -2},
	{"stind.r4", Popi_popr4, Push0, InlineNone, Primitive, Next, false, -2},
	{"stind.r8", Popi_popr8, Push0, InlineNone, Primitive, Next, false, -2},
	{"add", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"sub", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"mul", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"div", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"div.un", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"rem", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"rem.un", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"and", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"or", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"xor", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"shl", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"shr", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"shr.un", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"neg", Pop1, Push1, InlineNone, Primitive, Next, false, 0},
	{"not", Pop1, Push1, InlineNone, Primitive, Next, false, 0},
	{"conv.i1", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.i2", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.i4", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.i8", Pop1, Pushi8, InlineNone, Primitive, Next, false, 0},
	{"conv.r4", Pop1, Pushr4, InlineNone, Primitive, Next, false, 0},
	{"conv.r8", Pop1, Pushr8, InlineNone, Primitive, Next, false, 0},
	{"conv.u4", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.u8", Pop1, Pushi8, InlineNone, Primitive, Next, false, 0},
	{"callvirt", Varpop, Varpush, InlineMethod, Objmodel, Call, false, 0},
	{"cpobj", Popi_popi, Push0, InlineType, Objmodel, Next, false, -2},
	{"ldobj", Popi, Push1, InlineType, Objmodel, Next, false, 0},
	{"ldstr", Pop0, Pushref, InlineString, Objmodel, Next, false, 1},
	{"newobj", Varpop, Pushref, InlineMethod, Objmodel, Call, false, 1},
	{"castclass", Popref, Pushref, InlineType, Objmodel, Next, false, 0},
	{"isinst", Popref, Pushi, InlineType, Objmodel, Next, false, 0},
	{"conv.r.un", Pop1, Pushr8, InlineNone, Primitive, Next, false, 0},
	{"unbox", Popref, Pushi, InlineType, Primitive, Next, false, 0},
	{"throw", Popref, Push0, InlineNone, Objmodel, Throw, true, -1},
	{"ldfld", Popref, Push1, InlineField, Objmodel, Next, false, 0},
	{"ldflda", Popref, Pushi, InlineField, Objmodel, Next, false, 0},
	{"stfld", Popref_pop1, Push0, InlineField, Objmodel, Next, false, -2},
	{"ldsfld", Pop0, Push1, InlineField, Objmodel, Next, false, 1},
	{"ldsflda", Pop0, Pushi, InlineField, Objmodel, Next, false, 1},
	{"stsfld", Pop1, Push0, InlineField, Objmodel, Next, false, -1},
	{"stobj", Popi_pop1, Push0, InlineType, Primitive, Next, false, -2},
	{"conv.ovf.i1.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i2.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i4.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i8.un", Pop1, Pushi8, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u1.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u2.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u4.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u8.un", Pop1, Pushi8, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u.un", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"box", Pop1, Pushref, InlineType, Primitive, Next, false, 0},
	{"newarr", Popi, Pushref, InlineType, Objmodel, Next, false, 0},
	{"ldlen", Popref, Pushi, InlineNone, Objmodel, Next, false, 0},
	{"ldelema", Popref_popi, Pushi, InlineType, Objmodel, Next, false, -1},
	{"ldelem.i1", Popref_popi, Pushi, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.u1", Popref_popi, Pushi, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.i2", Popref_popi, Pushi, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.u2", Popref_popi, Pushi, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.i4", Popref_popi, Pushi, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.u4", Popref_popi, Pushi, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.i8", Popref_popi, Pushi8, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.i", Popref_popi, Pushi, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.r4", Popref_popi, Pushr4, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.r8", Popref_popi, Pushr8, InlineNone, Objmodel, Next, false, -1},
	{"ldelem.ref", Popref_popi, Pushref, InlineNone, Objmodel, Next, false, -1},
	{"stelem.i", Popref_popi_popi, Push0, InlineNone, Objmodel, Next, false, -3},
	{"stelem.i1", Popref_popi_popi, Push0, InlineNone, Objmodel, Next, false, -3},
	{"stelem.i2", Popref_popi_popi, Push0, InlineNone, Objmodel, Next, false, -3},
	{"stelem.i4", Popref_popi_popi, Push0, InlineNone, Objmodel, Next, false, -3},
	{"stelem.i8", Popref_popi_popi8, Push0, InlineNone, Objmodel, Next, false, -3},
	{"stelem.r4", Popref_popi_popr4, Push0, InlineNone, Objmodel, Next, false, -3},
	{"stelem.r8", Popref_popi_popr8, Push0, InlineNone, Objmodel, Next, false, -3},
	{"stelem.ref", Popref_popi_popref, Push0, InlineNone, Objmodel, Next, false, -3},
	{"ldelem", Popref_popi, Push1, InlineType, Objmodel, Next, false, -1},
	{"stelem", Popref_popi_pop1, Push0, InlineType, Objmodel, Next, false, 0},
	{"unbox.any", Popref, Push1, InlineType, Objmodel, Next, false, 0},
	{"conv.ovf.i1", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u1", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i2", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u2", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i4", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u4", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i8", Pop1, Pushi8, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u8", Pop1, Pushi8, InlineNone, Primitive, Next, false, 0},
	{"refanyval", Pop1, Pushi, InlineType, Primitive, Next, false, 0},
	{"ckfinite", Pop1, Pushr8, InlineNone, Primitive, Next, false, 0},
	{"mkrefany", Popi, Push1, InlineType, Primitive, Next, false, 0},
	{"ldtoken", Pop0, Pushi, InlineTok, Primitive, Next, false, 1},
	{"conv.u2", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.u1", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.i", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.i", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"conv.ovf.u", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"add.ovf", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"add.ovf.un", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"mul.ovf", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"mul.ovf.un", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"sub.ovf", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"sub.ovf.un", Pop1_pop1, Push1, InlineNone, Primitive, Next, false, -1},
	{"endfinally", Pop0, Push0, InlineNone, Primitive, Return, true, 0},
	{"leave", Pop0, Push0, InlineBrTarget, Primitive, Branch, true, 0},
	{"leave.s", Pop0, Push0, ShortInlineBrTarget, Primitive, Branch, true, 0},
	{"stind.i", Popi_popi, Push0, InlineNone, Primitive, Next, false, -2},
	{"conv.u", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"arglist", Pop0, Pushi, InlineNone, Primitive, Next, false, 1},
	{"ceq", Pop1_pop1, Pushi, InlineNone, Primitive, Next, false, -1},
	{"cgt", Pop1_pop1, Pushi, InlineNone, Primitive, Next, false, -1},
	{"cgt.un", Pop1_pop1, Pushi, InlineNone, Primitive, Next, false, -1},
	{"clt", Pop1_pop1, Pushi, InlineNone, Primitive, Next, false, -1},
	{"clt.un", Pop1_pop1, Pushi, InlineNone, Primitive, Next, false, -1},
	{"ldftn", Pop0, Pushi, InlineMethod, Primitive, Next, false, 1},
	{"ldvirtftn", Popref, Pushi, InlineMethod, Primitive, Next, false, 0},
	{"ldarg", Pop0, Push1, InlineVar, Primitive, Next, false, 1},
	{"ldarga", Pop0, Pushi, InlineVar, Primitive, Next, false, 1},
	{"starg", Pop1, Push0, InlineVar, Primitive, Next, false, -1},
	{"ldloc", Pop0, Push1, InlineVar, Primitive, Next, false, 1},
	{"ldloca", Pop0, Pushi, InlineVar, Primitive, Next, false, 1},
	{"stloc", Pop1, Push0, InlineVar, Primitive, Next, false, -1},
	{"localloc", Popi, Pushi, InlineNone, Primitive, Next, false, 0},
	{"endfilter", Popi, Push0, InlineNone, Primitive, Return, true, -1},
	{"unaligned.", Pop0, Push0, ShortInlineI, Prefix, Meta, false, 0},
	{"volatile.", Pop0, Push0, InlineNone, Prefix, Meta, false, 0},
	{"tail.", Pop0, Push0, InlineNone, Prefix, Meta, false, 0},
	{"initobj", Popi, Push0, InlineType, Objmodel, Next, false, -1},
	{"constrained.", Pop0, Push0, InlineType, Prefix, Meta, false, 0},
	{"cpblk", Popi_popi_popi, Push0, InlineNone, Primitive, Next, false, -3},
	{"initblk", Popi_popi_popi, Push0, InlineNone, Primitive, Next, false, -3},
	{"no.", Pop0, Push0, InlineNone, Prefix, Meta, false, 0},
	{"rethrow", Pop0, Push0, InlineNone, Objmodel, Throw, true, 0},
	{"sizeof", Pop0, Pushi, InlineType, Primitive, Next, false, 1},
	{"refanytype", Pop1, Pushi, InlineNone, Primitive, Next, false, 0},
	{"readonly.", Pop0, Push0, InlineNone, Prefix, Meta, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0},
	{"", Pop0, Push0, InlineNone, Primitive, Next, false, 0}
};

class TokenReference;
class ILCommand;

class ILVMCommand : public BaseVMCommand
{
public:
	explicit ILVMCommand(ILCommand *owner, ILCommandType command_type, uint64_t value, TokenReference *token_reference);
	virtual uint64_t address() const { return address_; }
	virtual void set_address(uint64_t address) { address_ = address; }
	virtual void Compile();
	ILCommandType command_type() const { return command_type_; }
	uint64_t value() const { return value_; }
	void set_value(uint64_t value) { value_ = value; }
	void set_dump(const Data &dump) { dump_ = dump; }
	bool is_data() const { return (command_type_ == icByte || command_type_ == icWord || command_type_ == icDword); }
	virtual bool is_end() const { return false; }
	virtual void WriteToFile(IArchitecture &file);
	virtual size_t dump_size() const { return dump_.size(); }
	TokenReference *token_reference() const { return token_reference_; }
	ILCommandType crypt_command() const { return crypt_command_; }
	OperandSize crypt_size() const { return crypt_size_; }
	uint64_t crypt_key() const { return crypt_key_; }
	ILVMCommand *link_command() const { return link_command_; }
	void set_token_reference(TokenReference *token_reference) { token_reference_ = token_reference; }
	void set_crypt_command(ILCommandType crypt_command, OperandSize crypt_size, uint64_t crypt_key) { crypt_command_ = crypt_command; crypt_size_ = crypt_size; crypt_key_ = crypt_key; }
	void set_link_command(ILVMCommand *command) { link_command_ = command; }
private:
	uint64_t address_;
	ILCommandType command_type_;
	uint64_t value_;
	Data dump_;
	TokenReference *token_reference_;
	ILCommandType crypt_command_;
	OperandSize crypt_size_;
	uint64_t crypt_key_;
	ILVMCommand *link_command_;
};

class ILCommand: public BaseCommand
{
public:
	explicit ILCommand(IFunction *owner, OperandSize size, ILCommandType type, uint64_t operand_value, IFixup *fixup = NULL);
	explicit ILCommand(IFunction *owner, OperandSize size, uint64_t address = 0);
	explicit ILCommand(IFunction *owner, OperandSize size, const std::string &value);
	explicit ILCommand(IFunction *owner, OperandSize size, const Data &value);
	explicit ILCommand(IFunction *owner, const ILCommand &source);
	void Init(ILCommandType type, uint64_t operand_value = 0, TokenReference *token_reference = NULL);
	virtual void clear();
	ILVMCommand *item(size_t index) const { return reinterpret_cast<ILVMCommand *>(BaseCommand::item(index)); }
	virtual uint64_t ext_vm_address() const { return (ext_vm_entry_) ? ext_vm_entry_->address() : vm_address(); }
	virtual ISEHandler *seh_handler() const;
	virtual void set_seh_handler(ISEHandler *handler);
	virtual uint64_t address() const { return address_; }
	virtual CommandType type() const { return type_; }
	virtual std::string text() const;
	virtual uint32_t section_options() const { return section_options_; }
	virtual void include_section_option(SectionOption option) { section_options_ |= option; } 
	virtual void exclude_section_option(SectionOption option) { section_options_ &= ~option; } 
	virtual size_t original_dump_size() const { return (original_dump_size_) ? original_dump_size_ : dump_size(); }
	virtual void CompileToNative();
	virtual void CompileLink(const CompileContext &ctx);
	virtual void PrepareLink(const CompileContext &ctx);
	virtual void set_operand_value(size_t operand_index, uint64_t value);
	virtual void set_link_value(size_t link_index, uint64_t value);
	virtual void set_jmp_value(size_t link_index, uint64_t value) {};
	virtual void set_address(uint64_t address);
	void set_operand_fixup(IFixup *fixup);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void Rebase(uint64_t delta_base);
	virtual ILCommand *Clone(IFunction *owner) const;
	virtual bool Merge(ICommand *command);
	void InitUnknown();
	void InitComment(const std::string &comment);
	virtual bool is_data() const;
	virtual bool is_end() const;
	bool is_prefix() const;
	virtual std::string display_address() const;
	virtual std::string dump_str() const;
	OperandSize size() const { return size_; }
	virtual CommentInfo comment();
	size_t ReadFromFile(IArchitecture & file);
	void WriteToFile(IArchitecture &file);
	uint64_t operand_value() const { return operand_value_; }
	size_t operand_pos() const { return operand_pos_; }
	uint64_t ReadValueFromFile(IArchitecture &file, OperandSize value_size, bool is_token = false);
	void ReadString(IArchitecture &file, size_t len);
	void ReadCaseCommand(IArchitecture &file);
	TokenReference *token_reference() const { return token_reference_; }
	void set_token_reference(TokenReference *token_reference) { token_reference_ = token_reference; }
	void CompileToVM(const CompileContext &ctx);
	ILVMCommand *AddVMCommand(const CompileContext &ctx, ILCommandType command_type, uint64_t value, uint32_t options = 0, TokenReference *token_reference = NULL, ILCommand *to_command = NULL);
	void AddExtSection(const CompileContext &ctx);
	void set_param(uint32_t param) { param_ = param; }
	int GetStackLevel(size_t *pop_ref = NULL) const;
private:
	void AddCmpSection(const CompileContext &ctx, ILCommandType jmp_command);
	void AddJmpWithFlagSection(const CompileContext &ctx, bool is_true);
	void AddCryptorSection(const CompileContext &ctx, ValueCryptor *cryptor, bool is_decrypt);
	uint64_t address_;
	OperandSize size_;
	ILCommandType type_;
	uint64_t operand_value_;
	size_t original_dump_size_;
	uint32_t section_options_;
	size_t operand_pos_;
	TokenReference *token_reference_;
	std::vector<ILVMCommand *> vm_links_;
	InternalLinkList internal_links_;
	ILVMCommand *ext_vm_entry_;
	uint32_t param_;
	IFixup *fixup_;
};

class ILFunction : public BaseFunction
{
public:
	explicit ILFunction(IFunctionList *owner, const FunctionName &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder);
	explicit ILFunction(IFunctionList *owner = NULL);
	explicit ILFunction(IFunctionList *owner, OperandSize cpu_address_size, IFunction *parent = NULL);
	explicit ILFunction(IFunctionList *owner, const ILFunction &src);
	virtual ILFunction *Clone(IFunctionList *owner) const;
	ILCommand *item(size_t index) const { return reinterpret_cast<ILCommand *>(IFunction::item(index)); }
	virtual bool Compile(const CompileContext &ctx);
	virtual void AfterCompile(const CompileContext &ctx);
	virtual void CompileLinks(const CompileContext &ctx);
	virtual bool Prepare(const CompileContext &ctx);
	virtual void CompileInfo(const CompileContext &ctx);
	ILCommand *AddCommand(OperandSize value_size, uint64_t value);
	ILCommand *AddCommand(const std::string &value);
	ILCommand *AddCommand(const Data &value);
	ILCommand *AddCommand(ILCommandType type, uint64_t operand_value, IFixup *fixup = NULL);
	ILCommand *Add(uint64_t address);
	ILCommand *GetCommandByAddress(uint64_t address) const;
	ILCommand *GetCommandByNearAddress(uint64_t address) const;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual ILCommand *ParseCommand(IArchitecture &file, uint64_t address, bool dump_mode = false);
	void CreateBlocks();
protected:
	virtual ILCommand *CreateCommand();
	virtual ILCommand *ParseString(IArchitecture &file, uint64_t address, size_t len);
	virtual void ParseBeginCommands(IArchitecture &file);
	virtual void ParseEndCommands(IArchitecture &file);
	virtual IFunction *CreateFunction(IFunction *parent) { return new ILFunction(NULL, cpu_address_size(), parent); }
	void CalcStack(std::map<ILCommand *, int> &stack_map);
	void Mutate(const CompileContext &ctx);
	void CompileToNative(const CompileContext &ctx);
	void CompileToVM(const CompileContext &ctx);
};

class ILFileHelper : public IObject
{
public:
	explicit ILFileHelper();
	~ILFileHelper();
	void Parse(NETArchitecture &file);
private:
	void AddString(NETArchitecture &file, uint32_t token, uint64_t reference);

	std::vector<MapFunction *> string_list_;
	MapFunctionList *marker_name_list_;
	size_t marker_index_;

	// no copy ctr or assignment op
	ILFileHelper(const ILFileHelper &);
	ILFileHelper &operator =(const ILFileHelper &);
};

class ILCommandBlock;
class ILToken;
class ILMethodDef;

class ILCommandNode : public IObject
{
public:
	ILCommandNode(ILCommandBlock *owner, ILCommand *command);
	~ILCommandNode();
	ILCommand *command() const { return command_; }
	std::vector<ILCommandNode*> stack() const { return stack_; }
	void set_stack(std::vector<ILCommandNode*> &stack) { stack_ = stack; }
	std::string token_name() const;
private:
	ILCommandBlock *owner_;
	ILCommand *command_;
	std::vector<ILCommandNode*> stack_;
};

class ILCommandBlock : public ObjectList<ILCommandNode>
{
public:
	ILCommandBlock();
	bool Parse(ILFunction &func);
	ILCommandNode *GetNodeByCommand(ILCommand *command) const;
	ILToken *GetTypeOf(ILCommandNode *node) const;
	ILToken *GetTypeFromStack(ILCommandNode *node) const;
protected:
	void AddObject(ILCommandNode *node);
private:
	ILCommandNode *Add(ILCommand *command);
	std::map<ILCommand *, ILCommandNode *> map_;
	ILMethodDef *method_;
};

class NETLoader : public ILFunction
{
public:
	explicit NETLoader(IFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	virtual size_t WriteToFile(IArchitecture &file);
	ILCommand *file_crc_entry() const { return file_crc_entry_; }
	uint32_t file_crc_size() const { return file_crc_size_; }
	ILCommand *file_crc_size_entry() const { return file_crc_size_entry_; }
	ILCommand *loader_crc_entry() const { return loader_crc_entry_; }
	uint32_t loader_crc_size() const { return loader_crc_size_; }
	ILCommand *loader_crc_size_entry() const { return loader_crc_size_entry_; }
	ILCommand *loader_crc_hash_entry() const { return loader_crc_hash_entry_; }
	ILCommand *import_entry() const { return import_entry_; }
	uint32_t import_size() const { return import_size_; }
	ILCommand *iat_entry() const { return iat_entry_; }
	uint32_t iat_size() const { return iat_size_; }
	ILCommand *pe_entry() const { return pe_entry_; }
	ILCommand *strong_name_signature_entry() const { return strong_name_signature_entry_; }
	ILCommand *vtable_fixups_entry() const { return vtable_fixups_entry_; }
	ILCommand *tls_entry() const { return tls_entry_; }
	uint32_t tls_size() const { return tls_size_; }
private:
	void AddAVBuffer(const CompileContext &ctx);

	struct ImportInfo {
		ILCommand *original_first_thunk;
		ILCommand *name;
		ILCommand *first_thunk;
		//IntelCommand *loader_name;
	};

	struct ImportFunctionInfo {
		PEImportFunction *import_function;
		ILCommand *name;
		ILCommand *thunk;
		ILCommand *loader_name;
		ImportFunctionInfo(PEImportFunction *import_function_)
			: import_function(import_function_), name(NULL), thunk(NULL), loader_name(NULL) {}
		bool operator == (PEImportFunction *import_function_) const
		{
			return (import_function == import_function_);
		}
	};

	struct LoaderInfo {
		ILCommand *data;
		size_t size;
		LoaderInfo(ILCommand *data_, size_t size_) 
			: data(data_), size(size_) {}
	};

	struct PackerInfo {
		PESegment *section;
		uint64_t address;
		size_t size;
		ILCommand *data;
		bool operator == (PESegment *section_) const
		{
			return (section == section_);
		}
	};

	ILCommand *import_entry_;
	uint32_t import_size_;
	ILCommand *iat_entry_;
	uint32_t iat_size_;
	ILCommand *file_crc_entry_;
	ILCommand *file_crc_size_entry_;
	uint32_t file_crc_size_;
	ILCommand *loader_crc_entry_;
	ILCommand *loader_crc_size_entry_;
	ILCommand *loader_crc_hash_entry_;
	uint32_t loader_crc_size_;
	ILCommand *pe_entry_;
	ILCommand *strong_name_signature_entry_;
	ILCommand *vtable_fixups_entry_;
	ILCommand *tls_entry_;
	ILCommand *tls_call_back_entry_;
	uint32_t tls_size_;

};

class ILSDK : public ILFunction
{
public:
	explicit ILSDK(IFunctionList *owner, OperandSize cpu_address_size);
	bool Init(const CompileContext &ctx);
};

class ILCRCTable : public ILFunction
{
public:
	explicit ILCRCTable(IFunctionList *owner, OperandSize cpu_address_size);
	bool Init(const CompileContext &ctx);
	size_t table_size() const { return (count() - 2) * OperandSizeToValue(osDWord); }
	ILCommand *table_entry() const { return item(0); }
	ILCommand *size_entry() const { return size_entry_; }
	ILCommand *hash_entry() const { return hash_entry_; }
private:
	ILCommand *size_entry_;
	ILCommand *hash_entry_;
};

class ILRuntimeData : public ILFunction
{
public:
	explicit ILRuntimeData(IFunctionList *owner, OperandSize cpu_address_size);
	bool Init(const CompileContext &ctx);
	virtual size_t WriteToFile(IArchitecture &file);
private:
	struct CommandCompareHelper {
		bool operator () (const ILCommand *left, ILCommand *right) const;
	};

	RC5Key rc5_key_;
	ILCommand *resources_entry_;
	uint32_t resources_size_;
	ILCommand *strings_entry_;
	uint32_t strings_size_;
	ILCommand *trial_hwid_entry_;
	uint32_t trial_hwid_size_;
#ifdef ULTIMATE
	ILCommand *license_data_entry_;
	uint32_t license_data_size_;
#endif
};

class ILRuntimeCRCTable : public ILFunction
{
public:
	explicit ILRuntimeCRCTable(IFunctionList *owner, OperandSize cpu_address_size);
	virtual void clear();
	virtual bool Compile(const CompileContext &ctx);
	virtual size_t WriteToFile(IArchitecture &file);
	size_t region_count() const { return region_info_list_.size(); }
private:
	struct RegionInfo {
		uint64_t address;
		uint32_t size;
		bool is_self_crc;

		RegionInfo(uint64_t address_, uint32_t size_, bool is_self_crc_)
			: address(address_), size(size_), is_self_crc(is_self_crc_)
		{
		}
	};
	std::vector<RegionInfo> region_info_list_;
	ValueCryptor *cryptor_;
};

class ILToken;

struct ImportDelegateInfo
{
	ILToken *method;
	ILToken *invoke;
	uint32_t call_type;
	ImportDelegateInfo(ILToken *method_, ILToken *invoke_, uint32_t call_type_)
		: method(method_), invoke(invoke_), call_type(call_type_)
	{

	}
};

class ILImport : public ILFunction
{
public:
	explicit ILImport(IFunctionList *owner, OperandSize cpu_address_size);
	bool Init(const CompileContext &ctx);
	std::vector<ImportDelegateInfo> info_list() const { return info_list_; }
private:
	std::vector<ImportDelegateInfo> info_list_;
};

class ILVirtualMachineProcessor;

class ILFunctionList : public BaseFunctionList
{
public:
	explicit ILFunctionList(IArchitecture *owner);
	explicit ILFunctionList(IArchitecture *owner, const ILFunctionList &src);
	~ILFunctionList();
	ILFunction *item(size_t index) const;
	ILFunction *GetFunctionByAddress(uint64_t address) const;
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	virtual void CompileLinks(const CompileContext &ctx);
	virtual void CompileInfo(const CompileContext &ctx);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual ILFunction *Add(const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder);
	virtual ILFunctionList *Clone(IArchitecture *owner) const;
	virtual ILCRCTable *crc_table() const { return crc_table_; }
	virtual ValueCryptor *crc_cryptor() const { return crc_cryptor_; }
	ILImport *import() const { return import_; }
	ILRuntimeCRCTable *runtime_crc_table() const { return runtime_crc_table_; }
	virtual IFunction *CreateFunction(OperandSize cpu_address_size = osDefault);
	virtual bool GetRuntimeOptions() const { return true; }
	ILVirtualMachineProcessor *AddProcessor(OperandSize cpu_address_size);
protected:
	ILSDK *AddSDK(OperandSize cpu_address_size);
	ILRuntimeData *AddRuntimeData(OperandSize cpu_address_size);
	ILCRCTable *AddCRCTable(OperandSize cpu_address_size);
	ILFunction *AddWatermark(OperandSize cpu_address_size, Watermark *watermark, int copy_count);
	ILRuntimeCRCTable *AddRuntimeCRCTable(OperandSize cpu_address_size);
	ILImport *AddImport(OperandSize cpu_address_size);

	ValueCryptor *crc_cryptor_;
	ILCRCTable *crc_table_;
	ILRuntimeCRCTable *runtime_crc_table_;
	ILImport *import_;

	// no copy ctr or assignment op
	ILFunctionList(const ILFunctionList &);
	ILFunctionList &operator =(const ILFunctionList &);
};

class ILVirtualMachineProcessor : public ILFunction
{
public:
	ILVirtualMachineProcessor(ILFunctionList *owner, OperandSize cpu_address_size);
	void InitCommands(const CompileContext &ctx);
	virtual void AfterCompile(const CompileContext &ctx);
	virtual void CompileLinks(const CompileContext &ctx);
	virtual void CompileInfo(const CompileContext &ctx);
	virtual size_t WriteToFile(IArchitecture &file);
};

class ILVirtualMachineList : public IVirtualMachineList
{
public:
	virtual IVirtualMachineList * Clone() const;
	virtual void Prepare(const CompileContext &ctx);
	virtual IFunction *processor() const { return NULL; }
};

class ILOpcodeList;
class ILToken;

class ILOpcodeInfo : public IObject
{
public:
	ILOpcodeInfo(ILOpcodeList *owner, ILCommandType command_type, ILToken *entry);
	~ILOpcodeInfo();
	ILCommandType command_type() const { return command_type_; }
	ILToken *entry() const { return entry_; }
	uint8_t opcode() const { return opcode_; }
	void set_opcode(uint8_t opcode) { opcode_ = opcode; }
	uint64_t Key() const { return command_type_; }
	class circular_queue : public std::vector<ILOpcodeInfo *>
	{
		size_t position_;
	public:
		circular_queue() : std::vector<ILOpcodeInfo *>(), position_(0) {}
		ILOpcodeInfo *Next();
	};
private:
	ILOpcodeList *owner_;
	ILCommandType command_type_;
	ILToken *entry_;
	uint8_t opcode_;
};

class ILOpcodeList : public ObjectList<ILOpcodeInfo>
{
public:
	ILOpcodeList();
	ILOpcodeInfo *Add(ILCommandType command_type, ILToken *entry);
	ILOpcodeInfo *GetOpcodeInfo(ILCommandType command_type) const;
};

class ILMethodDef;

class ILVirtualMachine : public BaseVirtualMachine
{
public:
	ILVirtualMachine(ILVirtualMachineList *owner, uint8_t id, ILVirtualMachineProcessor *processor);
	void Init(const CompileContext &ctx);
	virtual ByteList *registr_order() { return NULL; }
	virtual bool backward_direction() const { return false; }
	virtual ILFunction *processor() const { return processor_; }
	ILMethodDef *ctor() const { return ctor_; }
	ILMethodDef *invoke() const { return invoke_; }
	void CompileCommand(ILVMCommand &vm_command);
private:
	ILOpcodeInfo *GetOpcode(ILCommandType command_type);
	ILVirtualMachineProcessor *processor_;
	ILMethodDef *ctor_;
	ILMethodDef *invoke_;
	ILOpcodeList opcode_list_;
	std::unordered_map<uint64_t, ILOpcodeInfo::circular_queue> opcode_stack_;
};

#endif