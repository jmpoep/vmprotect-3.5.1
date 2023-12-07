#ifndef INTEL_H
#define INTEL_H

enum IntelCommandType : uint16_t
{
	cmUnknown,cmPush,cmPop,cmMov,cmAdd,cmXor,cmTest,cmLea,
	cmUd0, cmRet,cmNor,cmNand,cmCrc,cmCall,cmJmp,cmFstsw,cmFsqrt,cmFchs,cmFstcw,cmFldcw,
	cmFild,cmFist,cmFistp,cmFld,cmFstp,cmFst,
	cmFadd,cmFsub,cmFsubr,cmFisub,cmFisubr,cmFdiv,cmFcomp,cmFmul,
	cmRepe,cmRepne,cmRep,cmDB,cmDW,cmDD,cmDQ,
	cmMovs,cmCmps,cmScas,
	cmMovzx,cmMovsx,

	cmInc,cmDec,
	cmLes,cmLds,cmLfs,cmLgs,cmLss,
	cmXadd,cmBswap,
	cmJmpWithFlag,
	cmAnd,cmSub,cmStos,cmLods,cmNop,cmXchg,
	cmPushf,cmPopf,cmSahf,cmLahf,cmShl,cmShr,cmSal,cmSar,cmRcl,cmRcr,cmRol,cmRor,cmShld,cmShrd,
	cmLoope,cmLoopne,cmLoop,cmJCXZ,
	cmIn,cmIns,cmOut,cmOuts,cmWait,
	cmCbw,cmCwde,cmCdqe,cmCwd,cmCdq,cmCqo,
	cmClc,cmStc,cmCli,cmSti,cmCld,cmStd,
	cmNot,cmNeg,cmDiv,cmImul,cmIdiv,cmMul,
	cmOr,cmAdc,cmCmp,cmSbb,
	cmPusha,cmPopa,

	cmClflush,cmPause,

	cmBound,cmArpl,cmDaa,cmDas,cmAaa,cmAam,cmAad,cmAas,cmEnter,cmLeave,cmInt,cmInto,cmIret,
	cmSetXX,cmCmov,

	cmAddpd,cmAddps,cmAddsd,cmAddss,
	cmAndpd,cmAndps,cmAndnpd,cmAndnps,
	cmCmppd,cmCmpps,cmCmpsd,cmCmpss,
	cmComisd,cmComiss,
	cmCvtdq2ps,cmCvtpd2dq,cmCvtdq2pd,cmCvtpd2pi,cmCvtps2pi,
	cmCvtpd2ps,cmCvtps2pd,cmCvtpi2pd,cmCvtpi2ps,cmCvtps2dq,
	cmCvtsd2si,cmCvtss2si,cmCvtsd2ss,cmCvtss2sd,
	cmCvttpd2pi,cmCvttps2pi,cmCvttpd2dq,cmCvttps2dq,
	cmCvttsd2si,cmCvttss2si,
	cmDivpd,cmDivps,cmDivsd,cmDivss,
	cmMaxpd,cmMaxps,cmMaxsd,cmMaxss,
	cmMinpd,cmMinps,cmMinsd,cmMinss,
	cmMulpd,cmMulps,cmMulsd,cmMulss,
	cmOrpd,cmOrps,

	cmMovd,cmMovq,cmMovntq,cmMovapd,cmMovaps,cmMovdqa,cmMovdqu,
	cmMovdq2q,cmMovq2dq,
	cmMovhlps,cmMovhpd,cmMovhps,cmMovlhps,cmMovlpd,cmMovlps,
	cmMovmskpd,cmMovmskps,
	cmMovnti,
	cmMovntpd,cmMovntps,
	cmMovsd,cmMovss,
	cmMovupd,cmMovups,

	cmPmovmskb,cmPsadbw,
	cmPshufw,cmPshufd,cmPshuflw,cmPshufhw,
	cmPsubb,cmPsubw,cmPsubd,cmPsubq,
	cmPsubsb,cmPsubsw,
	cmPsubusb,cmPsubusw,
	cmPaddb,cmPaddw,cmPaddd,cmPaddq,
	cmPaddsb,cmPaddsw,
	cmPaddusb,cmPaddusw,
	cmPavgb,cmPavgw,
	cmPinsrb,cmPinsrw,cmPinsrd,cmPinsrq,cmPextrw,
	cmPmaxsb,cmPmaxsw,cmPmaxsd,cmPmaxub,cmPmaxuw,cmPmaxud,
	cmPminsb,cmPminsw,cmPminsd,cmPminub,cmPminuw,cmPminud,
	cmPmulhuw,cmPmulhw,cmPmullw,cmPmuludq,cmPmulld,
	cmPsllw,cmPslld,cmPsllq,cmPslldq,
	cmPsraw,cmPsrad,
	cmPsrlw,cmPsrld,cmPsrlq,cmPsrldq,

	cmPunpcklbw,cmPunpcklwd,cmPunpckldq,cmPunpcklqdq,cmPunpckhqdq,

	cmPackusdw,cmPcmpgtb,cmPcmpgtw,cmPcmpgtd,cmPcmpeqb,cmPcmpeqw,cmPcmpeqd,cmEmms,
	cmPacksswb,cmPackuswb,cmPunpckhbw,cmPunpckhwd,cmPunpckhdq,cmPackssdw,cmPand,cmPandn,cmPor,cmPxor,cmPmaddwd,
	cmRcpps,cmRcpss,
	cmRsqrtss,cmMovsxd,
	cmShufps,cmShufpd,
	cmSqrtpd,cmSqrtps,cmSqrtsd,cmSqrtss,
	cmSubpd,cmSubps,cmSubsd,cmSubss,
	cmUcomisd,cmUcomiss,
	cmUnpckhpd,cmUnpckhps,
	cmUnpcklpd,cmUnpcklps,
	cmXorpd,cmXorps,

	cmBt,cmBts,cmBtr,cmBtc,cmXlat,cmCpuid,cmRsm,cmBsf,cmBsr,cmCmpxchg,cmCmpxchg8b,
	cmHlt,cmCmc,
	cmLgdt,cmSgdt,cmLidt,cmSidt,cmSmsw,cmLmsw,cmInvlpg,
	cmLar,cmLsl,cmClts,cmInvd,cmWbinvd,cmUd2,cmWrmsr,cmRdtsc,cmRdmsr,cmRdpmc,

	cmFcom,cmFdivr,
	cmFiadd,cmFimul,cmFicom,cmFicomp,cmFidiv,cmFidivr,
	cmFaddp,cmFmulp,cmFsubp,cmFsubrp,cmFdivp,cmFdivrp,
	cmFbld,cmFbstp,

	cmFfree,cmFrstor,cmFsave,cmFucom,cmFucomp,

	cmFldenv,cmFstenvm,
	cmFxch,cmFabs,cmFxam,
	cmFld1,cmFldl2t,cmFldl2e,cmFldpi,cmFldlg2,cmFldln2,

	cmFldz,cmFyl2x,cmFptan,cmFpatan,cmFxtract,cmFprem1,cmFdecstp,cmFincstp,
	cmFprem,cmFyl2xp1,cmFsincos,cmFrndint,cmFscale,cmFsin,cmFcos,cmFtst,
	cmFstenv,cmF2xm1,cmFnop,cmFinit,cmFclex,cmFcompp,

	cmSysenter,cmSysexit,cmSldt,cmStr,cmLldt,cmLtr,cmVerr,cmVerw,
	cmSfence,cmLfence,cmMfence,cmPrefetchnta,cmPrefetcht0,cmPrefetcht1,cmPrefetcht2,cmPrefetch,cmPrefetchw,
	cmFxrstor,cmFxsave,cmLdmxcsr,cmStmxcsr,

	cmFcmovb, cmFcmove, cmFcmovbe, cmFcmovu, cmFcmovnb, cmFcmovne, cmFcmovnbe, cmFcmovnu,

	cmFucomi,cmFcomi,
	cmFucomip,cmFcomip,cmFucompp,

	cmVmcall, cmVmlaunch, cmVmresume, cmVmxoff, cmMonitor, cmMwait, cmXgetbv, cmXsetbv, cmVmrun, cmVmmcall, 
	cmVmload, cmVmsave, cmStgi, cmClgi, cmSkinit, cmInvlpga, cmSwapgs, cmRdtscp, cmSyscall, cmSysret, cmFemms, cmGetsec,
	cmPshufb, cmPhaddw, cmPhaddd, cmPhaddsw, cmPmaddubsw, cmPhsubw, cmPhsubd, cmPhsubsw, cmPsignb, cmPsignw, cmPsignd, cmPmulhrsw,
	cmPabsb, cmPabsw, cmPabsd, cmMovbe, cmPalignr, cmRsqrtps, cmVmread, cmVmwrite, cmSvldt, cmRsldt, cmSvts, cmRsts,
	cmXsave, cmXrstor, cmVmptrld, cmVmptrst, cmMaskmovq, cmFnstenv, cmFnstcw, cmFstp1, cmFneni, cmFndisi, cmFnclex, cmFninit, 
	cmFsetpm, cmFisttp, cmFnsave, cmFnstsw, cmFxch4, cmFcomp5, cmFfreep, cmFxch7, cmFstp8, cmFstp9, cmHaddpd, cmHsubpd,
	cmAddsubpd, cmAddsubps, cmMovntdq, cmFcom2, cmFcomp3, cmHaddps, cmHsubps, cmMovddup, cmMovsldup, cmCvtsi2sd, cmCvtsi2ss, 
	cmMovntsd, cmMovntss, cmLddqu, cmMovshdup, cmPopcnt, cmTzcnt, cmLzcnt,
	cmPblendvb, cmPblendps, cmPblendpd, cmPblendw, cmPtest, cmPmovsxbw, cmPmovsxbd, cmPmovsxbq, cmPmovsxwd, cmPmovsxwq, cmPmovsxdq, cmPmuldq,
	cmPcmpeqq, cmMovntdqa, cmXsaveopt, cmMaskmovdqu, cmUd1, cmPcmpgtq, 
	cmAesdec, cmAesdeclast, cmAesenc, cmAesenclast, cmAesimc, cmAeskeygenassist,
	cmRdrand, cmRdseed,
	cmPmovzxbw, cmPmovzxbd, cmPmovzxbq, cmPmovzxwd, cmPmovzxwq, cmPmovzxdq,

	cmFnmadd132sd, cmFnmadd213sd, cmFnmadd231sd,
	cmFnmadd132ss, cmFnmadd213ss, cmFnmadd231ss,

	cmUleb,cmSleb,cmDC,
	cmVbroadcastss, cmVbroadcastsd, cmVbroadcastf128, cmVperm2f128, cmVpermilpd, cmVpermilps, cmRoundpd, cmRoundps, cmCrc32, cmPextrb, cmPextrd, cmPextrq, cmVzeroupper,
	cmVzeroall, cmBlendpd, cmBlendps, cmBlendvpd, cmBlendvps, cmDpps, cmExtractf128, cmInsertf128, cmMaskmovpd, cmMaskmovps,
	cmVtestps, cmVtestpd, cmPcmpistri
};

static const char *intel_command_name[] = {
	"db ??","push","pop","mov","add","xor","test","lea",
	"ud0", "ret","nor","nand","crc","call","jmp","fstsw","fsqrt","fchs","fstcw","fldcw",
	"fild","fist","fistp","fld","fstp","fst",
	"fadd","fsub","fsubr","fisub","fisubr","fdiv","fcomp","fmul",
	"repe","repne","rep","db","dw","dd","dq",
	"movs","cmps","scas",
	"movzx","movsx",

	"inc","dec",
	"les","lds","lfs","lgs","lss",
	"xadd","bswap",
	"j",
	"and","sub","stos","lods","nop","xchg",
	"pushf","popf","sahf","lahf","shl","shr","sal","sar","rcl","rcr","rol","ror","shld","shrd",
	"loope","loopne","loop","jcxz",
	"in","ins","out","outs","wait",
	"cbw","cwde","cdqe","cwd","cdq","cqo",
	"clc","stc","cli","sti","cld","std",
	"not","neg","div","imul","idiv","mul",
	"or","adc","cmp","sbb",
	"pusha","popa",

	"clflush","pause",

	"bound","arpl","daa","das","aaa","aam","aad","aas","enter","leave","int","into","iret",
	"set","cmov",
	"addpd","addps","addsd","addss",
	"andpd","andps","andnpd","andnps",
	"cmppd","cmpps","cmpsd","cmpss",
	"comisd","comiss",
	"cvtdq2ps","cvtpd2dq","cvtdq2pd","cvtpd2pi","cvtps2pi",
	"cvtpd2ps","cvtps2pd","cvtpi2pd","cvtpi2ps","cvtps2dq",
	"cvtsd2si","cvtss2si","cvtsd2ss","cvtss2sd",
	"cvttpd2pi","cvttps2pi","cvttpd2dq","cvttps2dq",
	"cvttsd2si","cvttss2si",
	"divpd","divps","divsd","divss",
	"maxpd","maxps","maxsd","maxss",
	"minpd","minps","minsd","minss",
	"mulpd","mulps","mulsd","mulss",
	"orpd","orps",

	"movd","movq","movntq","movapd","movaps","movdqa","movdqu",
	"movdq2q","movq2dq",
	"movhlps","movhpd","movhps","movlhps","movlpd","movlps",
	"movmskpd","movmskps",
	"movnti",
	"movntpd","movntps",
	"movsd","movss",
	"movupd","movups",

	"pmovmskb","psadbw",
	"pshufw","pshufd","pshuflw","pshufhw",
	"psubb","psubw","psubd","psubq",
	"psubsb","psubsw",
	"psubusb","psubusw",
	"paddb","paddw","paddd","paddq",
	"paddsb","paddsw",
	"paddusb","paddusw",
	"pavgb","pavgw",
	"pinsrb","pinsrw","pinsrd","pinsrq","pextrw",
	"pmaxsb","pmaxsw","pmaxsd","pmaxub","pmaxuw","pmaxud",
	"pminsb","pminsw","pminsd","pminub","pminuw","pminud",
	"pmulhuw","pmulhw","pmullw","pmuludq","pmulld",
	"psllw","pslld","psllq","pslldq",
	"psraw","psrad",
	"psrlw","psrld","psrlq","psrldq",
	"punpcklbw","punpcklwd","punpckldq","punpcklqdq","punpckhqdq",

	"packusdw","pcmpgtb","pcmpgtw","pcmpgtd","pcmpeqb","pcmpeqw","pcmpeqd","emms",
	"packsswb","packuswb","punpckhbw","punpckhwd","punpckhdq","packssdw","pand","pandn","por","pxor","pmaddwd",
	"rcpps","rcpss",
	"rsqrtss","movsxd",
	"shufps","shufpd",
	"sqrtpd","sqrtps","sqrtsd","sqrtss",
	"subpd","subps","subsd","subss",
	"ucomisd","ucomiss",
	"unpckhpd","unpckhps",
	"unpcklpd","unpcklps",
	"xorpd","xorps",

	"bt","bts","btr","btc","xlat","cpuid","rsm","bsf","bsr","cmpxchg","cmpxchg8b",
	"hlt","cmc",
	"lgdt","sgdt","lidt","sidt","smsw","lmsw","invlpg",
	"lar","lsl","clts","invd","wbinvd","ud2","wrmsr","rdtsc","rdmsr","rdpmc",
	"fcom","fdivr",
	"fiadd","fimul","ficom","ficomp","fidiv","fidivr",
	"faddp","fmulp","fsubp","fsubrp","fdivp","fdivrp",
	"fbld","fbstp",

	"ffree","frstor","fsave","fucom","fucomp",

	"fldenv","fstenvm",
	"fxch","fabs","fxam",
	"fld1","fldl2t","fldl2e","fldpi","fldlg2","fldln2",

	"fldz","fyl2x","fptan","fpatan","fxtract","fprem1","fdecstp","fincstp",
	"fprem","fyl2xp1","fsincos","frndint","fscale","fsin","fcos","ftst",

	"fstenv","f2xm1","fnop","finit","fclex","fcompp",

	"sysenter","sysexit","sldt","str","lldt","ltr","verr","verw",
	"sfence","lfence","mfence","prefetchnta","prefetcht0","prefetcht1","prefetcht2","prefetch","prefetchw",
	"fxrstor","fxsave","ldmxcsr","stmxcsr",

	"fcmovb", "fcmove", "fcmovbe", "fcmovu", "fcmovnb", "fcmovne", "fcmovnbe", "fcmovnu",
	"fucomi","fcomi",
	"fucomip","fcomip","fucompp",
	"vmcall", "vmlaunch", "vmresume", "vmxoff", "monitor", "mwait", "xgetbv", "xsetbv", "vmrun", "vmmcall",
	"vmload", "vmsave", "stgi", "clgi", "skinit", "invlpga", "swapgs", "rdtscp", "syscall", "sysret", "femms", "getsec",
	"pshufb", "phaddw", "phaddd", "phaddsw", "pmaddubsw", "phsubw", "phsubd", "phsubsw", "psignb", "psignw", "psignd", "pmulhrsw",
	"pabsb", "pabsw", "pabsd", "movbe", "palignr", "rsqrtps", "vmread", "vmwrite", "svldt", "rsldt", "svts", "rsts",
	"xsave", "xrstor", "vmptrld", "vmptrst", "maskmovq", "fnstenv", "fnstcw", "fstp1", "fneni", "fndisi", "fnclex", "fninit", 
	"fsetpm", "fisttp", "fnsave", "fnstsw", "fxch4", "fcomp5", "ffreep", "fxch7", "fstp8", "fstp9", "haddpd", "hsubpd",
	"addsubpd", "addsubps", "movntdq", "fcom2", "fcomp3", "haddps", "hsubps", "movddup", "movsldup", "cvtsi2sd", "cvtsi2ss",
	"movntsd", "movntss", "lddqu", "movshdup", "popcnt", "tzcnt", "lzcnt",
	"pblendvb", "pblendps", "pblendpd", "pblendw", "ptest", "pmovsxbw", "pmovsxbd", "pmovsxbq", "pmovsxwd", "pmovsxwq", "pmovsxdq", "pmuldq",
	"pcmpeqq", "movntdqa", "xsaveopt", "maskmovdqu", "ud1", "pcmpgtq", 
	"aesdec", "aesdeclast", "aesenc", "aesenclast", "aesimc", "aeskeygenassist",
	"rdrand", "rdseed",
	"pmovzxbw", "pmovzxbd", "pmovzxbq", "pmovzxwd", "pmovzxwq", "pmovzxdq",

	"fnmadd132sd", "fnmadd213sd", "fnmadd231sd",
	"fnmadd132ss", "fnmadd213ss", "fnmadd231ss",

	"uleb","sleb","dc",
	"broadcastss", "broadcastsd", "broadcastf128", "perm2f128", "permilpd", "permilps", "roundpd", "roundps", "crc32", "pextrb", "pextrd", "pextrq", "zeroupper",
	"zeroall", "blendpd", "blendps", "blendvpd", "blendvps", "dpps", "extractf128", "insertf128", "maskmovpd", "maskmovps",
	"testps", "testpd", "pcmpistri"
};

enum IntelFlags : uint16_t {
	fl_C = 0x0001,
	fl_P = 0x0004,
	fl_A = 0x0010,
	fl_Z = 0x0040,
	fl_S = 0x0080,
	fl_T = 0x0100,
	fl_I = 0x0200,
	fl_D = 0x0400,
	fl_O = 0x0800,
	fl_OS = fl_S | fl_O
};

enum IntelRegistr : uint8_t {
	regEAX,
	regECX,
	regEDX,
	regEBX,
	regESP,
	regEBP,
	regESI,
	regEDI,
	regR8,
	regR9,
	regR10,
	regR11,
	regR12,
	regR13,
	regR14,
	regR15,
	regEIP
};

enum IntelSegment : uint8_t {
	segES,
	segCS,
	segSS,
	segDS,
	segFS,
	segGS,
	segDefault = 0xff
};

enum IntelRexFlags : uint8_t {
	rexB = 0x01,
	rexX = 0x02,
	rexR = 0x04,
	rexW = 0x08,
	vexL = 0x80
};

struct IntelOperand {
	uint64_t value;
	IFixup *fixup;
	IRelocation *relocation;
	uint16_t type;
	OperandSize size;
	uint8_t registr;
	uint8_t base_registr;
	uint8_t scale_registr;
	uint8_t value_pos;
	OperandSize address_size;
	OperandSize value_size;
	bool show_size;
	bool is_large_value;

	IntelOperand() 
	{ 
		Clear();
	}

	void Clear() 
	{
		type = otNone;
		fixup = NULL;
		relocation = NULL;
		registr = 0;
		base_registr = 0;
		scale_registr = 0;
		size = osDefault;
		value_size = address_size = osDefault;
		value_pos = 0;
		value = 0;
		show_size = false;
		is_large_value = false;
	}

	IntelSegment effective_base_segment(IntelSegment base_segment) const
	{
		if (base_segment == segDefault && 
				(((type & otBaseRegistr) && (base_registr == regESP || base_registr == regEBP)) ||
				((type & otRegistr) && (registr == regESP || registr == regEBP)))) {
				return segSS;
		} else {
			return (base_segment == segDefault) ? segDS : base_segment;
		}
	}

	IntelOperand(uint32_t type_, OperandSize size_, uint8_t registr_ = 0, uint64_t value_ = 0, IFixup *fixup_ = NULL);

	uint64_t encode() const
	{
		uint64_t res = static_cast<uint64_t>(type) << 48;
		if (type & (otRegistr | otSegmentRegistr | otControlRegistr | otDebugRegistr | otFPURegistr | otHiPartRegistr | otMMXRegistr | otXMMRegistr))
			res |= static_cast<uint64_t>(registr) << 44;
		if (type & otBaseRegistr)
			res |= static_cast<uint64_t>(base_registr) << 40;
		if (type & otValue)
			res |= static_cast<uint32_t>(value);
		return res;
	}

	void decode(uint64_t value_)
	{
		type = (value_ >> 48) & 0xffff;
		if (type & (otRegistr | otSegmentRegistr | otControlRegistr | otDebugRegistr | otFPURegistr | otHiPartRegistr | otMMXRegistr | otXMMRegistr))
			registr = (value_ >> 44) & 0xf;
		if (type & otBaseRegistr)
			base_registr = (value_ >> 40) & 0xf;
		if (type & otValue)
			value = static_cast<uint32_t>(value_);
	}

	bool operator == (const IntelOperand &operand) const
	{
		if (type != operand.type)
			return false;
		if (type & (otRegistr | otSegmentRegistr | otControlRegistr | otDebugRegistr | otFPURegistr | otHiPartRegistr | otMMXRegistr | otXMMRegistr)) {
			if (registr != operand.registr)
				return false;
		}
		if ((type & (otMemory | otRegistr)) == (otMemory | otRegistr)) {
			if (scale_registr != operand.scale_registr)
				return false;
		}
		if (type & otBaseRegistr) {
			if (base_registr != operand.base_registr)
				return false;
		}
		if (type & otValue) {
			if (value != operand.value)
				return false;
		}
		return true;
	}

	bool operator != (const IntelOperand &operand) const
	{
		return !(operator == (operand));
	}
};

class IntelCommand;
class IntelOpcodeInfo;

class IntelVMCommand : public BaseVMCommand
{
public:
	explicit IntelVMCommand(IntelCommand *owner, IntelCommandType command_type, OperandType operand_type, OperandSize size, uint64_t value, uint32_t options);
	explicit IntelVMCommand(IntelCommand *owner, const IntelVMCommand &src);
	virtual void WriteToFile(IArchitecture &file);
	virtual void Compile();
	IntelVMCommand *Clone(IntelCommand *owner);
	virtual uint64_t address() const { return address_; }
	virtual size_t dump_size() const { return dump_.size(); }
	virtual void set_address(uint64_t address) { address_ = address; }
	virtual void set_value(uint64_t value) { value_ = value; }
	void set_sub_value(uint64_t sub_value) { sub_value_ = sub_value; }
	void set_dump(const Data &dump) { dump_ = dump; }
	uint32_t options() const { return options_; }
	void include_option(VMCommandOption option) { options_ |= option; }
	int GetStackLevel() const;
	IntelCommandType crypt_command() const { return crypt_command_; }
	OperandSize crypt_size() const { return crypt_size_; }
	uint64_t crypt_key() const { return crypt_key_; }
	IntelVMCommand *link_command() const { return link_command_; }
	void set_crypt_command(IntelCommandType crypt_command, OperandSize crypt_size, uint64_t crypt_key) { crypt_command_ = crypt_command; crypt_size_ = crypt_size; crypt_key_ = crypt_key; }
	void set_link_command(IntelVMCommand *command) { link_command_ = command; }
	IntelCommandType command_type() const { return command_type_; }
	OperandType operand_type() const { return operand_type_; }
	uint8_t registr() const { return registr_; }
	OperandSize size() const { return size_; }
	uint64_t value() const { return value_; }
	uint64_t sub_value() const { return sub_value_; }
	IntelSegment base_segment() const { return base_segment_; }
	uint8_t subtype() const { return subtype_; }
	uint8_t dump(size_t pos) const { return dump_[pos]; }
	uint64_t dump_value(OperandSize size, size_t pos) const;
	void set_dump_value(OperandSize size, size_t pos, uint64_t value);
	void set_dump(size_t pos, uint8_t value) { dump_[pos] = value; }
	bool can_merge(CommandInfoList &command_info_list) const;
	IntelOpcodeInfo *opcode() const { return opcode_; }
	void set_opcode(IntelOpcodeInfo *opcode) { opcode_ = opcode; }
	virtual bool is_end() const;
	bool is_data() const { return (command_type_ == cmDD || command_type_ == cmDQ); }
	IFixup *fixup() const { return fixup_; }
	void set_fixup(IFixup *fixup) { fixup_ = fixup; }
private:
	uint64_t CorrectDumpValue(OperandSize size, uint64_t value) const;
	uint64_t address_;
	uint64_t crypt_key_;
	uint64_t sub_value_;
	uint64_t value_;

	IntelVMCommand *link_command_;
	IntelOpcodeInfo *opcode_;

	uint32_t options_;
	IntelCommandType command_type_;
	IntelCommandType crypt_command_;

	Data dump_;

	OperandType operand_type_;
	uint8_t registr_;
	uint8_t subtype_;
	OperandSize size_;
	IntelSegment base_segment_;
	OperandSize crypt_size_;
	IFixup *fixup_;
};

struct DisasmContext {
	IArchitecture *file;
	bool lower_reg;
	bool lower_address;
	uint8_t rex_prefix;
	uint8_t vex_registr;
	bool use_last_byte;
};

struct AsmContext {
	uint8_t rex_prefix;
};

enum CompileOperandOption {
	coSaveResult = 0x0001,
	coAsPointer = 0x0002,
	coInverse = 0x0004,
	coFixup = 0x0008,
	coAsWord = 0x0010
};

class IntelFunction;
class SectionCryptor;
class AddressRange;

class IntelCommandInfoList : public CommandInfoList
{
public:
	explicit IntelCommandInfoList(OperandSize cpu_address_size);
	virtual void Add(AccessType access_type, uint8_t value, OperandType operand_type, OperandSize size);
	void AddOperand(const IntelOperand &operand, AccessType access_type);
	void set_base_segment(IntelSegment base_segment) { base_segment_ = base_segment; }
private:
	OperandSize cpu_address_size_;
	IntelSegment base_segment_;
};

class EncodedData;

class IntelCommand: public BaseCommand
{
public:
	explicit IntelCommand(IFunction *owner, OperandSize size, uint64_t address = 0);
	explicit IntelCommand(IFunction *owner, OperandSize size, IntelCommandType type, IntelOperand operand1 = IntelOperand(), IntelOperand operand2 = IntelOperand(), IntelOperand operand3 = IntelOperand());
	explicit IntelCommand(IFunction *owner, OperandSize size, const std::string &value);
	explicit IntelCommand(IFunction *owner, OperandSize size, const os::unicode_string &value);
	explicit IntelCommand(IFunction *owner, OperandSize size, const Data &value);
	explicit IntelCommand(IFunction *owner, const IntelCommand &source);
	~IntelCommand();
	virtual void clear();
	virtual IntelCommand *Clone(IFunction *owner) const;
	virtual void CompileToNative();
	virtual void CompileLink(const CompileContext &ctx);
	virtual void PrepareLink(const CompileContext &ctx);
	void CompileToVM(const CompileContext &ctx);
	virtual uint64_t address() const { return address_; }
	virtual CommandType type() const { return type_; }
	IntelOperand operand(size_t index) const {
		if (index >= _countof(operand_))
			throw std::runtime_error("subscript out of range");
		return operand_[index];
	}
	const IntelOperand *operand_ptr(size_t index) const {
		if (index >= _countof(operand_))
			throw std::runtime_error("subscript out of range");
		return &operand_[index];
	}
	virtual std::string text() const;
	virtual CommentInfo comment();
	virtual uint32_t section_options() const { return section_options_; }
	virtual void set_address(uint64_t address);
	virtual size_t original_dump_size() const { return (original_dump_size_) ? original_dump_size_ : dump_size(); }
	IntelSegment base_segment() const { return base_segment_; }
	CommandType preffix_command() const { return preffix_command_; }
	OperandSize size() const { return size_; }
	uint32_t flags() const { return flags_; }
	void set_flags(uint32_t flags) { flags_ = flags; }
	virtual ISEHandler *seh_handler() const { return seh_handler_; }
	void set_seh_handler(ISEHandler *handler) { seh_handler_ = handler; }
	size_t command_pos() const { return command_pos_; }
	size_t ReadFromFile(IArchitecture &file);
	uint64_t ReadValueFromFile(IArchitecture &file, OperandSize size);
	void ReadArray(IArchitecture &file, size_t len);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void WriteToFile(IArchitecture &file);
	virtual void set_operand_value(size_t operand_index, uint64_t value);
	virtual void set_link_value(size_t link_index, uint64_t value);
	virtual void set_jmp_value(size_t link_index, uint64_t value);
	void set_operand_fixup(size_t operand_index, IFixup *fixup);
	void set_operand_relocation(size_t operand_index, IRelocation *relocation);
	void set_operand_scale(size_t operand_index, uint8_t value);
	void set_base_segment(IntelSegment base_segment) { base_segment_ = base_segment; }
	void set_preffix_command(IntelCommandType preffix_command) { preffix_command_ = preffix_command; }
	void Init(IntelCommandType type, IntelOperand operand1 = IntelOperand(), IntelOperand operand2 = IntelOperand(), IntelOperand operand3 = IntelOperand());
	void Init(const Data &data);
	void InitUnknown();
	virtual bool is_data() const;
	virtual bool is_end() const;
	virtual void Rebase(uint64_t delta_base);
	virtual void include_section_option(SectionOption option) { section_options_ |= option; }
	virtual void exclude_section_option(SectionOption option) { section_options_ &= ~option; }
	IntelVMCommand *AddVMCommand(const CompileContext &ctx, IntelCommandType command_type, OperandType operand_type, OperandSize size, uint64_t value, uint32_t options = 0, IFixup *fixup = NULL);
	void AddBeginSection(const CompileContext &ctx, uint32_t options = 0);
	void AddEndSection(const CompileContext &ctx, IntelCommandType end_command, uint8_t end_value = 0, uint32_t options = 0);
	void AddExtSection(const CompileContext &ctx, IntelCommand *command);
	uint64_t AddStoreEIPSection(const CompileContext &ctx, uint64_t prev_eip);
	void AddStoreExtRegistrSection(const CompileContext &ctx, uint8_t registr);
	void AddStoreExtRegistersSection(const CompileContext &ctx);
	void AddCryptorSection(const CompileContext &ctx, ValueCryptor *cryptor, bool is_decrypt);
	IntelVMCommand *item(size_t index) const { return reinterpret_cast<IntelVMCommand *>(BaseCommand::item(index)); }
	virtual uint64_t ext_vm_address() const { return (ext_vm_entry_) ? ext_vm_entry_->address() : vm_address(); }
	virtual std::string dump_str() const;
	virtual std::string display_address() const;
	bool is_equal(const IntelCommand &command) const;
	IntelVMCommand *ext_vm_entry() const { return ext_vm_entry_; }
	bool GetCommandInfo(IntelCommandInfoList &command_info_list) const;
	virtual bool Merge(ICommand *command);
	uint8_t ReadDataByte(EncodedData &data, size_t *pos);
	uint16_t ReadDataWord(EncodedData &data, size_t *pos);
	uint32_t ReadDataDWord(EncodedData &data, size_t *pos);
	uint64_t ReadDataQWord(EncodedData &data, size_t *pos);
	uint64_t ReadUleb128(EncodedData &data, size_t *pos);
	int64_t ReadSleb128(EncodedData &data, size_t *pos);
	uint64_t ReadEncoding(EncodedData &data, uint8_t encoding, size_t *pos);
	std::string ReadString(EncodedData &data, size_t *pos);
	void ReadData(EncodedData &data, size_t size, size_t *pos);
	int32_t ReadCompressedValue(IArchitecture &file);
#ifdef CHECKED
	virtual bool check_hash() const;
	void update_hash();
#endif
private:
	bool GetOperandText(std::string &str, size_t index) const;
	IntelOperand *GetFreeOperand();
	// disasm methods
	void ReadCommand(IntelCommandType type, uint32_t of_1, uint32_t of_2, uint32_t of_3, DisasmContext &ctx);
	void ReadRegFromRM(uint8_t code, OperandSize operand_size, OperandType operand_type, const DisasmContext &ctx);
	void ReadRM(uint8_t code, OperandSize operand_size, OperandType operand_type, bool show_size, const DisasmContext &ctx);
	void ReadReg(uint8_t code, OperandSize operand_size, OperandType operand_type, const DisasmContext &ctx);
	IntelOperand *ReadValue(OperandSize operand_size, OperandSize value_size, const DisasmContext &ctx);
	void ReadValueAddAddress(OperandSize operand_size, OperandSize value_size, const DisasmContext &ctx);
	void ReadFlags(uint8_t code);
	// asm methods
	void PushReg(size_t operand_index, uint8_t add_code, AsmContext &ctx);
	void PushRM(size_t operand_index, uint8_t add_code, AsmContext &ctx);
	void PushRegAndRM(size_t reg_operand_index, size_t rm_operand_index, AsmContext &ctx);
	void PushFlags(uint8_t add_code);
	void PushPrefix(AsmContext &ctx);
	void PushBytePrefix(uint8_t prefix);
	void PushWordPrefix();
	// virtualization methods
	void CompileOperand(const CompileContext &ctx, size_t operand_index, uint32_t options = 0);
	void AddCorrectOperandSizeSection(const CompileContext &ctx, OperandSize src, OperandSize dst);
	void AddCombineFlagsSection(const CompileContext &ctx, uint16_t mask);
	void AddRegistrAndValueSection(const CompileContext &ctx, uint8_t registr, OperandSize registr_size, uint64_t value, bool need_pushf = false);
	void AddRegistrOrValueSection(const CompileContext &ctx, uint8_t registr, OperandSize registr_size, uint64_t value, bool need_pushf = false);
	void AddCorrectFlagSection(const CompileContext &ctx, uint16_t flags);
	void AddExtractFlagSection(const CompileContext &ctx, uint16_t flags, bool is_inverse, uint8_t extract_to);
	void AddJmpWithFlagSection(const CompileContext &ctx, IntelCommandType command_type);
	void AddCorrectESPSection(const CompileContext &ctx, OperandSize operand_size, size_t value);
	void AddCheckBreakpointSection(const CompileContext &ctx, OperandSize address_size);
	void AddCheckCRCSection(const CompileContext &ctx, OperandSize address_size);
#ifdef CHECKED
	uint32_t calc_hash() const;
	uint32_t hash_;
#endif

	uint64_t address_;
	IntelOperand operand_[3];
	uint32_t vex_operand_;
	uint32_t flags_;

	std::vector<IntelVMCommand *> vm_links_;
	InternalLinkList internal_links_;
	std::vector<IntelVMCommand *> jmp_links_;

	IntelVMCommand *ext_vm_entry_;
	SectionCryptor *begin_section_cryptor_;
	SectionCryptor *end_section_cryptor_;
	IntelCommandInfoList *vm_command_info_list_;

	size_t command_pos_;
	size_t original_dump_size_;
	uint32_t section_options_;

	IntelCommandType type_;
	IntelCommandType preffix_command_;

	OperandSize size_;
	IntelSegment base_segment_;

	ISEHandler *seh_handler_;

	// no copy ctr or assignment op
	IntelCommand(const IntelCommand &);
	IntelCommand &operator =(const IntelCommand &);
};

class SectionCryptorList;

class SectionCryptor : public IObject
{
public:
	explicit SectionCryptor(SectionCryptorList *owner, OperandSize cpu_address_size);
	~SectionCryptor();
	ByteList *registr_order() { return &registr_order_; }
	SectionCryptor *end_cryptor();
	void set_end_cryptor(SectionCryptor *cryptor);
private:
	SectionCryptorList *owner_;
	ByteList registr_order_;
	SectionCryptor *parent_cryptor_;
};

class SectionCryptorList : public ObjectList<SectionCryptor>
{
public:
	explicit SectionCryptorList(IFunction *owner);
	SectionCryptor *Add();
private:
	IFunction *owner_;
};

enum ReadMarkerOption {
	moNone,
	moNeedParam = 0x1,
	moForward = 0x2,
	moSkipLastCall = 0x4
};

class IntelFunction : public BaseFunction
{
public:
	explicit IntelFunction(IFunctionList *owner, const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder);
	explicit IntelFunction(IFunctionList *owner = NULL);
	explicit IntelFunction(IFunctionList *owner, OperandSize cpu_address_size, IFunction *parent = NULL);
	explicit IntelFunction(IFunctionList *owner, const IntelFunction &src);
	virtual ~IntelFunction();
	virtual void clear();
	virtual IntelFunction *Clone(IFunctionList *owner) const;
	IntelCommand *item(size_t index) const { return reinterpret_cast<IntelCommand *>(IFunction::item(index)); }
	IntelCommand *ReadValidCommand(IArchitecture &file, uint64_t address);
	void ReadMarkerCommands(IArchitecture &file, MarkerCommandList &command_list, uint64_t address, uint32_t options);
	virtual bool Compile(const CompileContext &ctx);
	virtual void AfterCompile(const CompileContext &ctx);
	virtual void CompileLinks(const CompileContext &ctx);
	virtual bool Init(const CompileContext &ctx);
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool PrepareExtCommands(const CompileContext &ctx);
	virtual void CompileInfo(const CompileContext &ctx);
	IntelCommand *AddCommand(OperandSize value_size, uint64_t value);
	IntelCommand *AddCommand(const std::string &value);
	IntelCommand *AddCommand(const os::unicode_string &value);
	IntelCommand *AddCommand(const Data &value);
	IntelCommand *Add(uint64_t address);
	IntelCommand *AddCommand(IntelCommandType type, IntelOperand operand1 = IntelOperand(), IntelOperand operand2 = IntelOperand(), IntelOperand operand3 = IntelOperand());
	SectionCryptorList *section_cryptor_list() { return section_cryptor_list_; }
	void AddWatermarkReference(uint64_t address, const std::string &value);
	IntelCommand *GetCommandByAddress(uint64_t address) const;
	IntelCommand *GetCommandByNearAddress(uint64_t address) const;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	bool ParseNewSEH(IArchitecture &file, uint64_t address);
	bool ParseCxxSEH(IArchitecture &file, uint64_t address);
	bool ParseCompressedCxxSEH(IArchitecture &file, uint64_t address, uint64_t begin);
	bool ParseScopeSEH(IArchitecture &file, uint64_t address, uint32_t table_count);
	virtual IntelCommand *ParseCommand(IArchitecture &file, uint64_t address, bool dump_mode = false);
	uint64_t ParseParam(IArchitecture &file, size_t index, uint64_t &param_reference);
protected:
	virtual IntelCommand *CreateCommand();
	virtual IntelCommand *ParseString(IArchitecture &file, uint64_t address, size_t len);
	virtual void ParseBeginCommands(IArchitecture &file);
	virtual void ParseEndCommands(IArchitecture &file);
	virtual uint64_t GetNextAddress(IArchitecture &file);
	virtual IntelFunction *CreateFunction(IFunction *parent = NULL) { return new IntelFunction(NULL, cpu_address_size(), parent); }
	void CompileToNative(const CompileContext &ctx);
	void CompileToVM(const CompileContext &ctx);
	void CreateBlocks();
private:
	bool ParseFilterSEH(IArchitecture &file, uint64_t address);
	bool ParseSwitch(IArchitecture &file, uint64_t address, OperandSize value_size, uint64_t add_value, IntelCommand *parent_command, size_t mode, size_t max_table_count);
	bool ParseSEH3(IArchitecture &file, uint64_t address);
	bool ParseSEH4(IArchitecture &file, uint64_t address);
	bool ParseVB6SEH(IArchitecture &file, uint64_t address);
	bool ParseBCBSEH(IArchitecture &file, uint64_t address, uint64_t next_address, uint8_t version);
	bool ParseDelphiSEH(IArchitecture &file, uint64_t address);
	CompilerFunction *ParseCompilerFunction(IArchitecture &file, uint64_t address);
	IntelCommand *AddGate(ICommand *to_command, AddressRange *address_range);
	IntelCommand *AddShortGate(ICommand *to_command, AddressRange *address_range);

	uint64_t GetRegistrValue(uint8_t reg, size_t end_index);
	uint64_t GetRegistrMaxValue(uint8_t reg, size_t end_index, IArchitecture &file);
	void GetFreeRegisters(size_t index, CommandInfoList &command_info_list) const;
	void Mutate(const CompileContext &ctx, bool for_virtualization, int index = 0);

	SectionCryptorList *section_cryptor_list_;
	std::set<uint64_t> break_case_list_;

	// no copy ctr or assignment op
	IntelFunction(const IntelFunction &);
	IntelFunction &operator =(const IntelFunction &);
};

class IntelStack;
class IntelFlagsValue;

enum ValueType {
	vtNone = 0,
	vtValue = 1,
	vtRegistr = 2,
	vtReturnAddress = 4
};

class IntelStackValue : public IObject
{
public:
	IntelStackValue(IntelStack *owner, ValueType type, uint64_t value);
	~IntelStackValue();
	ValueType type() const { return type_; }
	uint64_t value() const { return value_; }
	void set_value(uint64_t value) { value_ = value; }
	bool is_modified() const { return is_modified_; }
	void set_is_modified(bool value) { is_modified_ = value; }
	void Calc(IntelCommandType command_type, uint16_t command_flags, bool inverse_flags, OperandSize size, uint64_t op2, IntelFlagsValue *flags);
private:
	IntelStack *owner_;
	ValueType type_;
	uint64_t value_;
	bool is_modified_;
};

class IntelStack : public ObjectList<IntelStackValue>
{
public:
	IntelStack();
	IntelStackValue *Add(ValueType type, uint64_t value);
	IntelStackValue *Insert(size_t index, ValueType type, uint64_t value);
	IntelStackValue *GetRegistr(uint8_t reg) const;
	IntelStackValue *GetRandom(uint32_t types);
};

class IntelRegistrValue : public IntelStackValue
{
public:
	IntelRegistrValue(IntelStack *owner, uint8_t registr, uint64_t value)
		: IntelStackValue(owner, vtValue, value), registr_(registr) { }
	uint8_t registr() const { return registr_; }
private:
	uint8_t registr_;
};

class IntelFlagsValue : public IObject
{
public:
	IntelFlagsValue();
	uint32_t mask() const { return mask_; }
	uint32_t value() const { return value_; }
	void Calc(IntelCommandType command_type, OperandSize size, uint64_t op1, uint64_t op2, uint64_t result);
	uint16_t GetRandom() const;
	bool Check(uint16_t flags) const;
	void clear() {
		mask_ = 0;
		value_ = 0;
	}
private:
	void exclude(uint16_t mask);

	uint32_t mask_;
	uint32_t value_;
};

class IntelRegistrStorage : public IntelStack
{
public:
	IntelRegistrStorage();
	IntelRegistrValue *item(size_t index) const;
	IntelRegistrValue *GetRegistr(uint8_t reg) const;
	IntelRegistrValue *Add(uint8_t reg, uint64_t value);
};

class IntelObfuscation : public IObject
{
public:
	explicit IntelObfuscation();
	void Compile(IntelFunction *func, size_t index, size_t end_index = -1, bool for_virtualization = false);
private:
	IntelCommand *AddCommand(IntelCommandType type, IntelOperand operand1 = IntelOperand(), IntelOperand operand2 = IntelOperand(), IntelOperand operand3 = IntelOperand());
	void AddRandomCommands();
	void AddRestoreStack(size_t to_index);
	void AddRestoreRegistr(uint8_t reg);
	void AddRestoreStackItem(IntelStackValue *stack_item);
	void CompileOperand(IntelOperand *operand);

	IntelFunction *func_;
	AddressRange *address_range_;
	std::vector<IntelCommand *> command_list_;
	IntelStack stack_;
	IntelRegistrStorage registr_values_;
	IntelFlagsValue flags_;
	std::map<IntelCommand *, size_t> jmp_command_list_;
};

class IntelFileHelper : public IObject
{
public:
	explicit IntelFileHelper();
	~IntelFileHelper();
	void Parse(IArchitecture &file);
private:
	void AddMarker(IArchitecture &file, uint64_t address, uint64_t name_reference, uint64_t name_address, ObjectType type, uint8_t tag, bool is_unicode);
	void AddString(IArchitecture &file, uint64_t address, uint64_t reference, bool is_unicode);
	void AddEndMarker(IArchitecture &file, uint64_t address, uint64_t next_address, ObjectType type);

	std::vector<MapFunction *> string_list_;
	MapFunctionList *marker_name_list_;
	size_t marker_index_;
	
	// no copy ctr or assignment op
	IntelFileHelper(const IntelFileHelper &);
	IntelFileHelper &operator =(const IntelFileHelper &);
};

class IntelSDK : public IntelFunction
{
public:
	explicit IntelSDK(IFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Init(const CompileContext &ctx);
};

class PEIntelSDK : public IntelSDK
{
public:
	explicit PEIntelSDK(IFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Init(const CompileContext &ctx);
};

class PEIntelExport : public IntelFunction
{
public:
	explicit PEIntelExport(IFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Init(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	uint32_t size() const { return size_; }
private:
	uint32_t size_;
};

class PEImportFunction;
class IntelImport : public IntelFunction
{
public:
	explicit IntelImport(IFunctionList *owner, OperandSize cpu_address_size);
	bool Init(const CompileContext &ctx);
	IntelCommand *GetIATCommand(PEImportFunction *import_function) const;
private:
	struct IATInfo {
		PEImportFunction *import_function;
		IntelCommand *command;
		bool from_runtime;
	};
	std::vector<IATInfo> iat_info_list_;
};

class IntelCRCTable : public IntelFunction
{
public:
	explicit IntelCRCTable(IFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Init(const CompileContext &ctx);
	size_t table_size() const { return (count() - 2) * OperandSizeToValue(osDWord); }
	IntelCommand *table_entry() const { return item(0); }
	IntelCommand *size_entry() const { return size_entry_; }
	IntelCommand *hash_entry() const { return hash_entry_; }
private:
	IntelCommand *size_entry_;
	IntelCommand *hash_entry_;
};

class IntelRuntimeData : public IntelFunction
{
public:
	explicit IntelRuntimeData(IFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Init(const CompileContext &ctx);
	virtual size_t WriteToFile(IArchitecture &file);
private:
	RC5Key rc5_key_;
	uint32_t data_key_;
	IntelCommand *strings_entry_;
	uint32_t strings_size_;
	IntelCommand *resources_entry_;
	uint32_t resources_size_;
	IntelCommand *trial_hwid_entry_;
	uint32_t trial_hwid_size_;
#ifdef ULTIMATE
	IntelCommand *license_data_entry_;
	uint32_t license_data_size_;
	IntelCommand *files_entry_;
	uint32_t files_size_;
	IntelCommand *registry_entry_;
	uint32_t registry_size_;
#endif

	struct CommandCompareHelper {
		bool operator () (const IntelCommand *left, IntelCommand *right) const;
	};
};

class IntelLoaderData : public IntelFunction
{
public:
	explicit IntelLoaderData(IFunctionList *owner, OperandSize cpu_address_size);
	bool Init(const CompileContext &ctx);
};

class IntelWatermark : public IntelFunction
{
public:
	explicit IntelWatermark(IFunctionList *owner, OperandSize cpu_address_size);
	bool Init(const CompileContext &ctx);
};

class IntelRuntimeCRCTable : public IntelFunction
{
public:
	explicit IntelRuntimeCRCTable(IFunctionList *owner, OperandSize cpu_address_size);
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

class IntelVirtualMachineProcessor;

class IntelFunctionList : public BaseFunctionList
{
public:
	explicit IntelFunctionList(IArchitecture *owner);
	explicit IntelFunctionList(IArchitecture *owner, const IntelFunctionList &src);
	~IntelFunctionList();
	virtual IntelFunctionList *Clone(IArchitecture *owner) const;
	virtual IntelFunction *Add(const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder);
	IntelFunction *item(size_t index) const;
	IntelFunction *GetFunctionByAddress(uint64_t address) const;
	virtual bool Prepare(const CompileContext &ctx);
	virtual void CompileLinks(const CompileContext &ctx);
	IntelImport *import() const { return import_; }
	virtual IntelCRCTable *crc_table() const { return crc_table_; }
	IntelLoaderData *loader_data() const { return loader_data_; }
	IntelRuntimeCRCTable *runtime_crc_table() const { return runtime_crc_table_; }
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual ValueCryptor *crc_cryptor() const { return crc_cryptor_; }
	virtual IntelFunction *CreateFunction(OperandSize cpu_address_size = osDefault);
	virtual bool GetRuntimeOptions() const;
	IntelVirtualMachineProcessor *AddProcessor(OperandSize cpu_address_size);
protected:
	virtual IntelSDK *AddSDK(OperandSize cpu_address_size);
private:
	IntelImport *AddImport(OperandSize cpu_address_size);
	IntelRuntimeData *AddRuntimeData(OperandSize cpu_address_size);
	IntelCRCTable *AddCRCTable(OperandSize cpu_address_size);
	IntelLoaderData *AddLoaderData(OperandSize cpu_address_size);
	IntelFunction *AddWatermark(OperandSize cpu_address_size, Watermark *watermark, int copy_count);
	IntelRuntimeCRCTable *AddRuntimeCRCTable(OperandSize cpu_address_size);

	ValueCryptor *crc_cryptor_;
	IntelImport *import_;
	IntelCRCTable *crc_table_;
	IntelLoaderData *loader_data_;
	IntelRuntimeCRCTable *runtime_crc_table_;

	// no copy ctr or assignment op
	IntelFunctionList(const IntelFunctionList &);
	IntelFunctionList &operator =(const IntelFunctionList &);
};

class PEIntelFunctionList : public IntelFunctionList
{
public:
	explicit PEIntelFunctionList(IArchitecture *owner);
	explicit PEIntelFunctionList(IArchitecture *owner, const PEIntelFunctionList &src);
	virtual PEIntelFunctionList *Clone(IArchitecture *owner) const;
	virtual bool Prepare(const CompileContext &ctx);
	PEIntelExport *AddExport(OperandSize cpu_address_size);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
protected:
	virtual IntelSDK *AddSDK(OperandSize cpu_address_size);
};

class BaseIntelLoader : public IntelFunction
{
public:
	BaseIntelLoader(IntelFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	virtual IVirtualMachine *virtual_machine(IVirtualMachineList *virtual_machine_list, ICommand *command) const;
	uint64_t import_segment_address() const { return import_segment_address_; }
	uint64_t data_segment_address() const { return data_segment_address_; }
protected:
	struct LoaderInfo {
		IntelCommand *data;
		size_t size;
		LoaderInfo(IntelCommand *data_, size_t size_) 
			: data(data_), size(size_) {}
	};
	void AddAVBuffer(const CompileContext &ctx);
private:
	std::set<ICommand *> command_group_;
	uint64_t import_segment_address_;
	uint64_t data_segment_address_;
};

class PESegment;

class PEIntelLoader : public BaseIntelLoader
{
public:
	PEIntelLoader(IntelFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Prepare(const CompileContext &ctx);
	IntelCommand *import_entry() const { return import_entry_; }
	uint32_t import_size() const { return import_size_; }
	IntelCommand *iat_entry() const { return iat_entry_; }
	uint32_t iat_size() const { return iat_size_; }
	IntelCommand *name_entry() const { return name_entry_; }
	uint32_t name_size() const { return iat_size_; }
	IntelCommand *export_entry() const { return export_entry_; }
	uint32_t export_size() const { return export_size_; }
	IntelCommand *tls_entry() const { return tls_entry_; }
	uint32_t tls_size() const { return tls_size_; }
	IntelCommand *delay_import_entry() const { return delay_import_entry_; }
	uint32_t delay_import_size() const { return delay_import_size_; }
	IntelCommand *resource_section_info() const { return resource_section_info_; }
	IntelCommand *resource_packer_info() const { return resource_packer_info_; }
	IntelCommand *file_crc_entry() const { return file_crc_entry_; }
	uint32_t file_crc_size() const { return file_crc_size_; }
	IntelCommand *file_crc_size_entry() const { return file_crc_size_entry_; }
	IntelCommand *loader_crc_entry() const { return loader_crc_entry_; }
	uint32_t loader_crc_size() const { return loader_crc_size_; }
	IntelCommand *loader_crc_size_entry() const { return loader_crc_size_entry_; }
	IntelCommand *loader_crc_hash_entry() const { return loader_crc_hash_entry_; }
	IntelCommand *cfg_check_function_entry() const { return cfg_check_function_entry_; }
	void set_security_cookie(uint64_t value) { security_cookie_ = value; }
	void set_iat_address(uint64_t value) { iat_address_ = value; }
	std::vector<uint64_t> cfg_address_list() const;
private:
	IntelCommand *import_entry_;
	uint32_t import_size_;
	IntelCommand *iat_entry_;
	uint32_t iat_size_;
	IntelCommand *name_entry_;
	IntelCommand *resource_section_info_;
	IntelCommand *resource_packer_info_;
	IntelCommand *export_entry_;
	uint32_t export_size_;
	IntelCommand *tls_entry_;
	IntelCommand *tls_call_back_entry_;
	uint32_t tls_size_;
	IntelCommand *file_crc_entry_;
	uint32_t file_crc_size_;
	IntelCommand *file_crc_size_entry_;
	IntelCommand *loader_crc_entry_;
	uint32_t loader_crc_size_;
	IntelCommand *loader_crc_size_entry_;
	IntelCommand *loader_crc_hash_entry_;
	IntelCommand *delay_import_entry_;
	uint32_t delay_import_size_;
	uint64_t security_cookie_;
	uint64_t iat_address_;
	IntelCommand *cfg_check_function_entry_;

	struct ImportInfo {
		IntelCommand *original_first_thunk;
		IntelCommand *name;
		IntelCommand *first_thunk;
		IntelCommand *loader_name;
	};

	struct ImportFunctionInfo {
		PEImportFunction *import_function;
		IntelCommand *name;
		IntelCommand *thunk;
		IntelCommand *loader_name;
		ImportFunctionInfo(PEImportFunction *import_function_)
			: import_function(import_function_), name(NULL), thunk(NULL), loader_name(NULL) {}
		bool operator == (PEImportFunction *import_function_) const
		{
			return (import_function == import_function_);
		}
	};

	struct PackerInfo {
		PESegment *section;
		uint64_t address;
		size_t size;
		IntelCommand *data;
		bool operator == (PESegment *section_) const
		{
			return (section == section_);
		}
	};
};

class MacArchitecture;
class MacImportFunction;
class MacFixup;
class MacSegment;
class MacSymbol;

class MacIntelSDK : public IntelSDK
{
public:
	explicit MacIntelSDK(IFunctionList *owner, OperandSize cpu_address_size);
};

class MacIntelFunctionList : public IntelFunctionList
{
public:
	explicit MacIntelFunctionList(IArchitecture *owner);
	explicit MacIntelFunctionList(IArchitecture *owner, const MacIntelFunctionList &src);
	virtual MacIntelFunctionList *Clone(IArchitecture *owner) const;
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
protected:
	virtual IntelSDK *AddSDK(OperandSize cpu_address_size);
private:
	std::map<MacImportFunction *, IntelCommand *> relocation_list_;
};

class MacIntelLoader : public BaseIntelLoader
{
public:
	MacIntelLoader(IntelFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	IntelCommand *import_entry() const { return import_entry_; }
	uint32_t import_size() const { return import_size_; }
	IntelCommand *jmp_table_entry() const { return jmp_table_entry_; }
	uint32_t jmp_table_size() const { return jmp_table_size_; }
	IntelCommand *lazy_import_entry() const { return lazy_import_entry_; }
	uint32_t lazy_import_size() const { return lazy_import_size_; }
	IntelCommand *init_entry() const { return init_entry_; }
	uint32_t init_size() const { return init_size_; }
	IntelCommand *term_entry() const { return term_entry_; }
	uint32_t term_size() const { return term_size_; }
	IntelCommand *thread_variables_entry() const { return thread_variables_entry_; }
	uint32_t thread_variables_size() const { return thread_variables_size_; }
	IntelCommand *thread_data_entry() const { return thread_data_entry_; }
	uint32_t thread_data_size() const { return thread_data_size_; }
	IntelCommand *file_crc_entry() const { return file_crc_entry_; }
	uint32_t file_crc_size() const { return file_crc_size_; }
	IntelCommand *file_crc_size_entry() const { return file_crc_size_entry_; }
	IntelCommand *loader_crc_entry() const { return loader_crc_entry_; }
	uint32_t loader_crc_size() const { return loader_crc_size_; }
	IntelCommand *loader_crc_size_entry() const { return loader_crc_size_entry_; }
	IntelCommand *loader_crc_hash_entry() const { return loader_crc_hash_entry_; }
	IntelCommand *file_entry() const { return file_entry_; }
	std::vector<MacSegment *> packed_segment_list() const { return packed_segment_list_; }
	IntelCommand *patch_section_entry() const { return patch_section_entry_; }
private:
	IntelCommand *import_entry_;
	uint32_t import_size_;
	IntelCommand *jmp_table_entry_;
	uint32_t jmp_table_size_;
	IntelCommand *lazy_import_entry_;
	uint32_t lazy_import_size_;
	IntelCommand *init_entry_;
	uint32_t init_size_;
	IntelCommand *term_entry_;
	uint32_t term_size_;
	IntelCommand *thread_variables_entry_;
	uint32_t thread_variables_size_;
	IntelCommand *thread_data_entry_;
	uint32_t thread_data_size_;
	std::map<MacImportFunction *, IntelCommand *> import_function_info_;
	std::map<MacFixup *, IntelCommand *> relocation_info_;
	IntelCommand *file_crc_entry_;
	uint32_t file_crc_size_;
	IntelCommand *file_crc_size_entry_;
	IntelCommand *loader_crc_entry_;
	uint32_t loader_crc_size_;
	IntelCommand *loader_crc_size_entry_;
	IntelCommand *loader_crc_hash_entry_;
	IntelCommand *file_entry_;
	IntelCommand *patch_section_entry_;
	std::vector<MacSegment *> packed_segment_list_;

	struct PackerInfo {
		uint64_t address;
		size_t size;
		MacSegment *segment;
		IntelCommand *data;
		PackerInfo()
			: address(0), size(0), segment(NULL), data(NULL)
		{
		}

		PackerInfo(MacSegment *segment_, uint64_t address_, size_t size_)
			: address(address_), size(size_), segment(segment_), data(NULL)
		{
		}

		bool operator == (MacSegment *segment_) const
		{
			return (segment == segment_);
		}
	};
};

class ELFImportFunction;
class ELFSegment;
class ELFFixup;
class ELFArchitecture;

class ELFIntelSDK : public IntelSDK
{
public:
	explicit ELFIntelSDK(IFunctionList *owner, OperandSize cpu_address_size);
};

class ELFIntelFunctionList : public IntelFunctionList
{
public:
	explicit ELFIntelFunctionList(IArchitecture *owner);
	explicit ELFIntelFunctionList(IArchitecture *owner, const ELFIntelFunctionList &src);
	virtual ELFIntelFunctionList *Clone(IArchitecture *owner) const;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
protected:
	virtual IntelSDK *AddSDK(OperandSize cpu_address_size);
};

class ELFIntelLoader : public BaseIntelLoader
{
public:
	ELFIntelLoader(IntelFunctionList *owner, OperandSize cpu_address_size);
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	IntelCommand *import_entry() const { return import_entry_; }
	uint32_t import_size() const { return import_size_; }
	IntelCommand *file_crc_entry() const { return file_crc_entry_; }
	uint32_t file_crc_size() const { return file_crc_size_; }
	IntelCommand *file_crc_size_entry() const { return file_crc_size_entry_; }
	IntelCommand *loader_crc_entry() const { return loader_crc_entry_; }
	uint32_t loader_crc_size() const { return loader_crc_size_; }
	IntelCommand *loader_crc_size_entry() const { return loader_crc_size_entry_; }
	IntelCommand *loader_crc_hash_entry() const { return loader_crc_hash_entry_; }
	IntelCommand *term_entry() const { return term_entry_ ? reinterpret_cast<IntelCommand *>(term_entry_->link()->to_command()) : NULL; }
	IntelCommand *preinit_entry() const { return preinit_entry_; }
	uint32_t preinit_size() const { return preinit_size_; }
	IntelCommand *init_entry() const { return init_entry_; }
	IntelCommand *tls_entry() const { return tls_entry_; }
	uint32_t GetPackedSize(ELFArchitecture *file) const;
private:
	IntelCommand *import_entry_;
	uint32_t import_size_;
	IntelCommand *file_crc_entry_;
	uint32_t file_crc_size_;
	IntelCommand *file_crc_size_entry_;
	IntelCommand *loader_crc_entry_;
	uint32_t loader_crc_size_;
	IntelCommand *loader_crc_size_entry_;
	IntelCommand *loader_crc_hash_entry_;
	IntelCommand *preinit_entry_;
	uint32_t preinit_size_;
	IntelCommand *term_entry_;
	IntelCommand *init_entry_;
	IntelCommand *tls_entry_;
	IntelCommand *relro_entry_;

	struct PackerInfo {
		uint64_t address;
		size_t size;
		ELFSegment *segment;
		IntelCommand *data;
		PackerInfo()
			: address(0), size(0), segment(NULL), data(NULL)
		{
		}

		PackerInfo(ELFSegment *segment_, uint64_t address_, size_t size_)
			: address(address_), size(size_), segment(segment_), data(NULL)
		{
		}

		bool operator == (ELFSegment *segment_) const
		{
			return (segment == segment_);
		}
	};
};

class IntelOpcodeList;

class IntelOpcodeInfo : public IObject
{
public:
	IntelOpcodeInfo(IntelOpcodeList *owner, IntelCommandType command_type, OperandType operand_type, OperandSize size, uint8_t value, IntelCommand *entry, OpcodeCryptor *value_cryptor = NULL, OpcodeCryptor *end_cryptor = NULL);
	~IntelOpcodeInfo();
	IntelCommandType command_type() const { return command_type_; }
	OperandType operand_type() const { return operand_type_; }
	OperandSize size() const { return size_; }
	uint8_t value() const { return value_; }
	IntelCommand *entry() const { return entry_; }
	uint8_t opcode() const { return opcode_; }
	void set_opcode(uint8_t opcode) { opcode_ = opcode; }
	OpcodeCryptor *value_cryptor() const { return value_cryptor_; }
	OpcodeCryptor *end_cryptor() const { return end_cryptor_; }
	uint64_t Key();
	static uint64_t Key(IntelCommandType command_type, OperandType operand_type, OperandSize size, uint8_t value);
	class circular_queue : public std::vector<IntelOpcodeInfo *>
	{
		size_t position_;
	public:
		circular_queue() : std::vector<IntelOpcodeInfo *>(), position_(0) {}
		IntelOpcodeInfo *Next();
	};
private:
	IntelOpcodeList *owner_;
	IntelCommand *entry_;
	IntelCommandType command_type_;
	OperandType operand_type_;
	OperandSize size_;
	uint8_t value_;
	uint8_t opcode_;
	OpcodeCryptor *value_cryptor_;
	OpcodeCryptor *end_cryptor_;
};

class IntelOpcodeList : public ObjectList<IntelOpcodeInfo>
{
public:
	IntelOpcodeList();
	IntelOpcodeInfo *Add(IntelCommandType command_type, OperandType operand_type, OperandSize size, uint8_t value, IntelCommand *entry = NULL, OpcodeCryptor *value_cryptor = NULL, OpcodeCryptor *end_cryptor = NULL);
	IntelOpcodeInfo *GetOpcodeInfo(IntelCommandType command_type, OperandType operand_type, OperandSize size, uint8_t value) const;
};

class IntelRegistrList : public std::vector<uint8_t>
{
public:
	IntelRegistrList() : std::vector<uint8_t>() {}
	uint8_t GetRandom(bool no_solid = false)
	{
		if (empty())
			return regEmpty;
		uint8_t res;
		while (true) {
			size_t i = rand() % size();
			res = at(i);
			if (no_solid && (res == regESI || res == regEDI || res == regEBP))
				continue;
			erase(begin() + i);
			break;
		}
		return res;
	}
	void remove(uint8_t reg)
	{
		iterator it = std::find(begin(), end(), reg);
		if (it != end())
			erase(it);
	}
	void remove(const IntelRegistrList &reg_list)
	{
		for (size_t i = 0; i < reg_list.size(); i++) {
			remove(reg_list[i]);
		}
	}
};

enum VirtualMachineType {
	vtClassic,
	vtAdvanced
};

struct IntelVirtualMachineObfuscation
{
	IntelCommand *begin_;
	IntelCommand *end_;

	IntelVirtualMachineObfuscation(IntelCommand *begin, IntelCommand *end) : begin_(begin), end_(end) {
	}
};

class IntelVirtualMachineProcessor : public IntelFunction
{
public:
	IntelVirtualMachineProcessor(IntelFunctionList *parent, OperandSize cpu_address_size);
	virtual bool Prepare(const CompileContext &ctx);
	void AddExceptionHandler(const CompileContext &ctx);
	void AddObfuscation(size_t old_count);
	void AddObfuscationHandler(IntelCommand *begin);

	std::vector<std::unique_ptr<IntelVirtualMachineObfuscation>> obfuscation_list_;
};

class IntelVirtualMachineList;

class IntelVirtualMachine : public BaseVirtualMachine
{
public:
	IntelVirtualMachine(IntelVirtualMachineList *owner, VirtualMachineType type, uint8_t id, IntelVirtualMachineProcessor *processor);
	~IntelVirtualMachine();
	void Init(const CompileContext &ctx, const IntelOpcodeList &visible_opcode_list);
	void Prepare(const CompileContext &ctx);
	ByteList *registr_order() { return &registr_order_; }
	virtual bool backward_direction() const { return backward_direction_; }
	void CompileCommand(IntelVMCommand &vm_command);
	void CompileBlock(CommandBlock &block, bool need_encrypt);
	void AddExtJmpCommand(uint8_t id);
	ValueCryptor *entry_cryptor() const { return &const_cast<ValueCryptor &>(entry_cryptor_); }
	VirtualMachineType type() const { return type_; }
	IntelCommand *entry_command() const { return entry_command_; }
	IntelCommand *init_command() const { return init_command_; }
	virtual IntelFunction *processor() const { return processor_; }
private:
	IntelCommand *AddReadCommand(OperandSize size, OpcodeCryptor *command_cryptor, uint8_t registr);
	void AddValueCommand(ValueCommand &value_command, bool is_decrypt, uint8_t registr);
	void AddEndHandlerCommands(IntelCommand *to_command, OpcodeCryptor *command_cryptor);
	IntelCommand *CloneHandler(IntelCommand *handler);
	void InitCommands(const CompileContext &ctx, const IntelOpcodeList &visible_opcode_list);
	void AddCallCommands(CallingConvention calling_convention, IntelCommand *call_entry, uint8_t registr);
	IntelOpcodeInfo *GetOpcode(IntelCommandType command_type, OperandType operand_type, OperandSize size, uint8_t value);
	bool IsRegistrUsed(uint8_t registr);
	std::vector<OpcodeCryptor *> GetOpcodeCryptorList(IntelVMCommand *command);
	VirtualMachineType type_; 
	IntelVirtualMachineProcessor *processor_;
	IntelRegistrList registr_list_;
	IntelOpcodeList opcode_list_;
	std::unordered_map<uint64_t, IntelOpcodeInfo::circular_queue> opcode_stack_;
	ByteList registr_order_;
	ValueCryptor entry_cryptor_;
	OpcodeCryptor *command_cryptor_;
	std::vector<OpcodeCryptor *> cryptor_list_;
	IntelCommand *entry_command_;
	IntelCommand *init_command_;
	IntelCommand *ext_jmp_command_;
	bool backward_direction_;
	std::vector<IntelCommand *> vm_links_;
	uint8_t stack_registr_;
	uint8_t pcode_registr_;
	uint8_t jmp_registr_;
	uint8_t crypt_registr_;
	IntelRegistrList free_registr_list_;
	
	// no copy ctr or assignment op
	IntelVirtualMachine(const IntelVirtualMachine &);
	IntelVirtualMachine &operator =(const IntelVirtualMachine &);
};

class IntelVirtualMachineList : public IVirtualMachineList
{
public:
	IntelVirtualMachineList();
	~IntelVirtualMachineList();
	virtual void Prepare(const CompileContext &ctx);
	IntelVirtualMachine *item(size_t index) const { return reinterpret_cast<IntelVirtualMachine *>(IVirtualMachineList::item(index)); }
	virtual IntelVirtualMachineList *Clone() const;
	uint64_t GetCRCValue(uint64_t &address, size_t size);
	void ClearCRCMap();
private:
	MemoryManager *crc_manager_;
	std::map<uint64_t, ICommand *> map_;

	// no copy ctr or assignment op
	IntelVirtualMachineList(const IntelVirtualMachineList &);
	IntelVirtualMachineList &operator =(const IntelVirtualMachineList &);
};

#endif