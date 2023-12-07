

#ifndef DISASM_H
#define DISASM_H

#include <string>

#define GEN_INSN_OKAY        0
#define GEN_INSN_IOERROR     1
#define GEN_INSN_ERROR       2

int GenerateInstructions(const std::string & out_filename, bool x64);

#endif

