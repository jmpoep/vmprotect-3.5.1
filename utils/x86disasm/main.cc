#include <iostream>
#include "disasm.h"

int main(int argc, char **argv)
{
	int rc;
	bool x64;
	std::cout << "x86 Disassembly Generator (C) 2012\n";
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <output_file_name> [x64]\n";
		return 1;
	}
	if (argc >= 3) {
		x64 = (0 == _strnicmp(argv[2], "x64", 3));
	} else {
		x64 = false;
	}
	rc = GenerateInstructions(argv[1], x64);
	if (rc == GEN_INSN_OKAY)
		std::cout << "Finished\n";
	else
		std::cerr << "ERROR Failed with error " << rc << "\n";
	return rc;
}
