#include <stdlib.h>
#include <vector>
#include <set>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <cctype>

#include "disasm.h"
#include "../../third-party/libudis86/extern.h"

const size_t kBufSize = 20;

typedef unsigned char uint8_t;

static void GenerateBuffer(std::vector<uint8_t> * buf)
{
	size_t i;
	buf->resize(kBufSize);
	for (i = 0; i < kBufSize; i++)
		(*buf)[i] = 0x10 + i;
}

typedef unsigned int operand_encoding_t;
#define OP_ENC_REG           0x80000000UL
#define OP_ENC_MEM           0x40000000UL

struct InsnDef {
	unsigned insn_enc;
	size_t count;
	operand_encoding_t enc[3];
	uint8_t	 		pfx_rex;
	uint8_t 		pfx_seg;
	uint8_t 		pfx_opr;
	uint8_t 		pfx_adr;
	uint8_t 		pfx_lock;
	uint8_t 		pfx_rep;
	uint8_t 		pfx_repe;
	uint8_t 		pfx_repne;
	uint8_t 		pfx_insn;

	InsnDef(const struct ud & u);
	friend bool operator == (const InsnDef & left, const InsnDef & right)
	{
		return 0 == memcmp(&left, &right, sizeof(InsnDef));
	}
	friend bool operator < (const InsnDef & left, const InsnDef & right)
	{
		return 0 > memcmp(&left, &right, sizeof(InsnDef));
	}
	friend bool operator > (const InsnDef & left, const InsnDef & right)
	{
		return 0 < memcmp(&left, &right, sizeof(InsnDef));
	}
};

struct InsnDefCompare {
	bool operator() (const InsnDef & left, const InsnDef & right)
	{
		return left < right;
	}
};
typedef std::set<InsnDef, InsnDefCompare> insn_set_t;

InsnDef::InsnDef(const struct ud & u)
	: count(0)
{
	int i;
	const struct ud_operand *op;

	pfx_adr = u.pfx_adr;
	pfx_insn = u.pfx_insn;
	pfx_lock = u.pfx_lock;
	pfx_opr = u.pfx_opr;
	pfx_rep = u.pfx_rep;
	pfx_repe = u.pfx_repe;
	pfx_repne = u.pfx_repne;
	pfx_rex = u.pfx_rex;
	pfx_seg = u.pfx_seg;
	insn_enc = u.mnemonic;
	memset(enc, 0, sizeof(enc));
	/*
	 * Encode registers and operand types. Do not encode offsets and
	 * immediate values.
	 */
	for (i = 0; i < 3; i++) {
		op = &u.operand[i];
		switch (op->type) {
		case UD_OP_REG:
			enc[i] |= OP_ENC_REG;
			enc[i] |= op->base;
			break;
		case UD_OP_MEM:
			/* Encode only registers and scales. */
			enc[i] |= OP_ENC_MEM;
			enc[i] |= op->base | (op->index << 8) | (op->scale << 16);
			break;
		default:
			/* Encode operand type other than OP_ENC_MEM or OP_ENC_REG. */
			enc[i] |= op->type;
			break;
		}
		if (op->type != UD_NONE)
			count++;
	}

}

static std::string ReplaceAll(const std::string & str,
							  const std::string & prev_val,
							  const std::string & new_val)
{
	size_t pos;
	std::string s = str;

	while (true) {
		pos = s.find(prev_val);
		if (std::string::npos == pos)
			break;
		s = s.replace(pos, prev_val.size(), new_val);
	}
	return s;
}

static const struct {
	const char *from;
	const char *to;
} repl[] = {
	{"retn", "ret"},
	{"retnw", "ret"},
	{"iretw", "iret"},
	{"pushfw", "pushf"},
	{"popfw", "popf"},
	{"enterw", "enter"},
	{"cmovae", "cmovnb"},
	{"cmova", "cmovnbe"},
	{"cmovge", "cmovnl"},
	{"cmovg", "cmovnle"},
	{"setae", "setnb"},
	{"seta", "setnbe"},
	{"setge", "setnl"},
	{"setg", "setnle"},
	{"leavew", "leave"}, 
	{"int1", "int 01"},
	{"int3", "int 03"}
};

/* trim from start */
static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

/* trim from end */
static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

/* trim from both ends */
static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}

static const char *pfx[] = {
 "cs", "es", "fs", "gs", "ss", "ds"
};

static std::string FixDisassembly(const std::string & disasm)
{
	size_t i, j;
	std::string s,s2;
	
	s = ReplaceAll(disasm, "0x", "");
	s = ReplaceAll(s, "o16 ", "");
	s = ReplaceAll(s, "a16 ", "");
	s = ReplaceAll(s, "a32 ", "");
	for (i = 0; i < _countof(repl); i++) {
		s2 = ReplaceAll(s, repl[i].from, repl[i].to);
		if (s2 != s) {
			s = s2;
			break;
		}
	}
	s = trim(s);
	for (i = 0; i < 6; i++) {
		if (s.substr(0, 3).compare(std::string(pfx[i]) + " ") == 0) {
			s = s.substr(3);
			break;
		} else {
			j = s.find(" " + std::string(pfx[i]) + " ");
			if (j != s.npos) {
				s = s.substr(0, j) + s.substr(j + 3);
				break;
			}
		}
	}
	return s;
}

static bool IsInsnUnique(const struct ud & u, insn_set_t *is)
{
	InsnDef insn_def(u);

	insn_set_t::iterator it = is->lower_bound(insn_def);
	if (it != is->end() && *it == insn_def) {
		return false;
	}
	is->insert(insn_def);
	return true;
}

static void WriteOutput(FILE * f, const std::vector<uint8_t> & buf, size_t size,
						const char *disasm)
{
	size_t i;
	assert(size <= buf.size());
	for (i = 0; i < size; i++)
		fprintf(f, "%02x", buf[i]);
	fprintf(f, " %s\n", FixDisassembly(disasm).c_str());
	fflush(f);
}

static void GenerateToFile(FILE * f, bool x64)
{
	/* Generate buffer */
	std::vector<uint8_t> buf;
	unsigned int p0, p1, p2;
	insn_set_t is;
	struct ud u;
	unsigned int insn_len, n;
	bool disasm_ok;

	n = 0;
	GenerateBuffer(&buf);
	/*
	for (p0 = 0x10; p0 < 0x110; p0++) {
		for (p1 = 0x10; p1 < 0x110; p1++) {
			for (p2 = 0x10; p2 < 0x110; p2++) {
			*/
	for (p0 = 0x10; p0 < 0x110; p0++) {
		for (p1 = 0x10; p1 < 0x110; p1++) {
			for (p2 = 0x10; p2 < 0x110; p2++) {
				buf[0] = (p0 & 0xff);
				buf[1] = (p1 & 0xff);
				buf[2] = (p2 & 0xff);
				ud_init(&u);
				ud_set_input_buffer(&u, &buf[0], buf.size());
				ud_set_pc(&u, 0x401000);
				ud_set_mode(&u, x64 ? 64 : 32);
				ud_set_syntax(&u, UD_SYN_INTEL);
				ud_set_vendor(&u, UD_VENDOR_INTEL);

				disasm_ok = false;
				if ((insn_len = ud_disassemble(&u)) != 0) {
					char *disasm = ud_insn_asm(&u);
					if (0 != strncmp(disasm, "invalid", 7)) {
						disasm_ok = true;
						if (IsInsnUnique(u, &is)) {
							WriteOutput(f, buf, insn_len, disasm);
							n++;
							if (n % 10000 == 0)
								std::cout << n << " opcodes processed\n";
						}
					}
				} 
				if (!disasm_ok) {
					/* Cannot disassemble. */
					WriteOutput(f, buf, 10, "db");
					n++;
					if (n % 10000 == 0)
						std::cout << n << " opcodes processed\n";
				}
			}
		}
	}
}

int GenerateInstructions(const std::string & out_filename, bool x64)
{
	FILE *f;
	
	f = fopen(out_filename.c_str(), "wt");
	if (f == NULL) {
		std::cerr << "ERROR Cannot open file " << out_filename << "\n";
		return GEN_INSN_IOERROR;
	}

	GenerateToFile(f, x64);
	
	fclose(f);
	return GEN_INSN_OKAY;
}

