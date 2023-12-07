#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/core.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/pefile.h"
#include "../core/intel.h"

#include "testfileintel.h"

TEST(IntelTest, x86_AsmMarkers)
{
	uint8_t buf[] = {
		0x55,							// 004B58EC push ebp
		0x8B, 0xEC,						// 004B58ED mov ebp, esp
		0x6A, 0x00,						// 004B58EF push 00
		0x6A, 0x00,						// 004B58F1 push 00
		0x6A, 0x00,						// 004B58F3 push 00
		0x53,							// 004B58F5 push ebx
		0x8B, 0xD8,						// 004B58F6 mov ebx, eax
		0x33, 0xC0,						// 004B58F8 xor eax, eax
		0x55,							// 004B58FA push ebp
		0x68, 0xDA, 0x59, 0x4B, 0x00,   // 004B58FB push 004B59DA
		0x64, 0xFF, 0x30,				// 004B5900 push dword ptr fs:[eax]
		0x64, 0x89, 0x20,				// 004B5903 mov fs:[eax], esp
		0xEB, 0x10,						// 004B5906 jmp 004B5918
		0x56, 0x4D, 0x50, 0x72, 0x6F, 0x74, 0x65, 0x63, 0x74, 0x20, 0x62, 0x65, 0x67, 0x69, 0x6E, 0x00, // db 'VMProtect begin', 0
		0x8D, 0x55, 0xFC,				// 004B5918 lea edx, [ebp-04]
		0x8B, 0x83, 
		0x8C, 0x03, 0x00, 0x00,			// 004B591A mov eax, [ebx+0000038C]
		0xE8, 0x0A, 0xE0, 0xFC, 0xFF,	// 004B5920 call 00483930
		0x8B, 0x45, 0xFC,				// 004B5925 mov eax, [ebp-04]
		0x33, 0xD2,						// 004B5928 xor edx, edx
		0xE8, 0xD4, 0xCB, 0xF5, 0xFF,   // 004B592A call 00412504
		0xB9, 0x11, 0x00, 0x00, 0x00,	// 004B592F mov ecx, 00000011
		0x99,							// 004B5930 cdq
		0xF7, 0xF9,						// 004B5931 idiv ecx
		0x83, 0xFA, 0x0D,				// 004B5933 cmp edx, 0D
		0x75, 0x2F,						// 004B5936 jnz 004B596C
		0x68, 0xE8, 0x59, 0x4B, 0x00,	// 004B5938 push 004B59E8
		0xE8, 0x81, 0xFC, 0xFF, 0xFF,	// 004B593D call 004B55C8
		0x8B, 0xD0,						// 004B5942 mov edx, eax
		0x8D, 0x45, 0xF8,				// 004B5944 lea eax, [ebp-08]
		0xE8, 0x6F, 0x10, 0xF5, 0xFF,	// 004B5947 call 004069C0
		0x8B, 0x45, 0xF8,				// 004B594C mov eax, [ebp-08]
		0x6A, 0x00,						// 004B594F push 00
		0x6A, 0xFF,						// 004B5951 push -01
		0x6A, 0xFF,						// 004B5953 push -01
		0x6A, 0x00,						// 004B5955 push 00
		0x0F, 0xB7, 0x0D, 
		0xFC, 0x59, 0x4B, 0x00,			// 004B5957 movzx ecx, word ptr [004B59FC]
		0xB2, 0x02,						// 004B595E mov dl, 02
		0xE8, 0xAA, 0x4A, 0xFB, 0xFF,	// 004B5960 call 0046A414
		0xEB, 0x4B,						// 004B5965 jmp 004B59B7
		0x68, 0x00, 0x5A, 0x4B, 0x00,	// 004B5967 push 004B5A00
		0xE8, 0x52, 0xFC, 0xFF, 0xFF,	// 004B596C call 004B55C8
		0x8B, 0xD0,						// 004B5971 mov edx, eax
		0x8D, 0x45, 0xF4,				// 004B5973 lea eax, [ebp-0C]
		0xE8, 0x40, 0x10, 0xF5, 0xFF,	// 004B5976 call 004069C0
		0x8B, 0x45, 0xF4,				// 004B597B mov eax, [ebp-0C]
		0x6A, 0x00,						// 004B597E push 00
		0x6A, 0xFF,						// 004B5980 push -01
		0x6A, 0xFF,						// 004B5982 push -01
		0x6A, 0x00,						// 004B5984 push 00
		0x0F, 0xB7, 0x0D, 
		0xFC, 0x59, 0x4B, 0x00,			// 004B5986 movzx ecx, word ptr [004B59FC]
		0xB2, 0x01,						// 004B598D mov dl, 01
		0xE8, 0x7B, 0x4A, 0xFB, 0xFF,	// 004B598F call 0046A414
		0x8B, 0x83, 
		0x8C, 0x03, 0x00, 0x00,			// 004B5994 mov eax, [ebx+0000038C]
		0x8B, 0x10,						// 004B599A mov edx, [eax]
		0xFF, 0x92, 
		0xE8, 0x00, 0x00, 0x00,			// 004B599C call dword ptr [edx+000000E8]
		0xEB, 0x0E,						// 004B59A2 jmp 004B59B7
		0x56, 0x4D, 0x50, 0x72, 0x6F, 0x74, 0x65, 0x63, 0x74, 0x20, 0x65, 0x6E, 0x64, 0x00, // db 'VMProtect end', 00 
		0x33, 0xC0,						// 004B59B2 xor eax, eax
		0x5A,							// 004B59B4 pop edx
		0x59,							// 004B59B5 pop ecx
		0x59,							// 004B59B6 pop ecx
		0x64, 0x89, 0x10,				// 004B59B7 mov fs:[eax], edx
		0x68, 0xE1, 0x59, 0x4B, 0x00,	// 004B59BA push 004B59E1
		0x8D, 0x45, 0xF4,				// 004B59BF lea eax, [ebp-0C]
		0xBA, 0x02, 0x00, 0x00, 0x00,	// 004B59C2 mov edx, 00000002
		0xE8, 0x6B, 0x0E, 0xF5, 0xFF,	// 004B59C7 call 0040683C
		0x8D, 0x45, 0xFC,				// 004B59CC lea eax, [ebp-04]
		0xE8, 0x5B, 0x0E, 0xF5, 0xFF,	// 004B59CF call 00406834
		0xC3,							// 004B59D4 ret
		0xE9, 0xC1, 0xFD, 0xF4, 0xFF,	// 004B59D5 jmp 004057A0
		0xEB, 0xE3,						// 004B59DA jmp 004B59C4
		0x5B,							// 004B59DC pop ebx
		0x8B, 0xE5,						// 004B59DD mov esp, ebp
		0x5D,							// 004B59DF pop ebp
		0xC3,							// 004B59E0 ret
		0x90, 0x90, 0x90, 0x90
	};

	IntelFunction *func;
	MapFunctionList *map_function_list;
	MapFunction *map_function;
	IntelCommand *command;

	TestFile test_file(osDWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x004B58EC, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 73ul);
	map_function_list = arch.map_function_list();
	map_function = map_function_list->GetFunctionByName("VMProtectMarker1");
	ASSERT_TRUE(map_function != NULL);
	EXPECT_EQ(map_function->address(), 0x004B5906ull);
	EXPECT_EQ(func->ReadFromFile(arch, map_function->address()), 45ul);
	// Check CompileToNative
	std::vector<uint8_t> data;
	size_t i, j;
	for (i = 0; i < func->count(); i++) {
		command = func->item(i);
		if ((command->type() == cmJmpWithFlag || command->type() == cmJmp || command->type() == cmCall) && command->dump_size() == 2)
			continue;

		data.clear();
		for (j = 0; j < command->dump_size(); j++) {
			data.push_back(command->dump(j));
		}
		command->CompileToNative();
		EXPECT_EQ(command->dump_size(), data.size());
		for (j = 0; j < command->dump_size(); j++) {
			EXPECT_EQ(command->dump(j), data[j]);
		}
	}
}

TEST(IntelTest, CompilerFunction_Delphi)
{
	PEFile pf(NULL);

	EXPECT_EQ(pf.Open("test-binaries/win32-app-delphi-i386", foRead), osSuccess);
	EXPECT_EQ(pf.count(), 1ul);
	PEArchitecture &arch = *pf.arch_pe();
	IntelFunction command_list;
	EXPECT_EQ(command_list.ReadFromFile(arch, 0x004BBC48), 17ul);
	EXPECT_EQ(command_list.ReadFromFile(arch, 0x004B5A34), 2ul);
}

uint8_t char2hex(char d)
{
	if ((d >= '0') && (d <= '9')) {
		return (d - '0');
	}
	if ((d >= 'A') && (d <= 'F')) {
		return (d - '7');
	}
	if ((d >= 'a') && (d <= 'f')) {
		return (d - 'W');
	}
	return 0;
}

void ascii2hex(const char *inp, uint8_t *out, size_t size)
{
	size_t i, j;

	for (i=0, j=0; j<size/2; j++, i+=2) {
		out[j] = (char2hex(inp[i]) << 4) + char2hex(inp[i+1]);
	}
}

TEST(IntelTest, x86)
{
//	TestFile test_file(osDWord);
	TestFile test_file(osQWord);
	IArchitecture &arch = *test_file.item(0);

	IntelCommand intel_command(NULL, arch.cpu_address_size(), 0x401000);
	size_t i, k, n;
	FileStream fs;
	bool status;
	std::string line, udisstr, intel_str;
	uint8_t dump[40], sse_preffix;

	return;

	status = fs.Open("test-binaries/intel-x64-opcodes.txt", fmOpenRead | fmShareDenyNone); //-V779

	ASSERT_TRUE(status);
	for (;;) {
		if (!fs.ReadLine(line))
			break;
		i = line.find(' ');
		if (i > 0) {
			ascii2hex(line.c_str(), dump, i);

			n = 0;
			sse_preffix = 0;
			for (k = 0; k < i/2; k++) {
				if (dump[k] == 0xF2 || dump[k] == 0xF3 || (dump[k] == 0x66 && sse_preffix == 0))
					sse_preffix = dump[k];
				if (dump[k] == 0x26 || 
					dump[k] == 0x2e || 
					dump[k] == 0x36 || 
					dump[k] == 0x3e || 
					dump[k] == 0x64 || 
					dump[k] == 0x65 || 
					dump[k] == 0x66 || 
					dump[k] == 0x67 || 
					dump[k] == 0xF2 || 
					dump[k] == 0xF3
					) {
					n++;
				} else {
					break;
				}
			}

			// VMRUN is not decoded by udis when ud_set_vendor(&u, UD_VENDOR_INTEL) is called; not a bug
			if (dump[n] == 0x0f && dump[n+1] == 0x01 && dump[n+2] >= 0xc0) 
				continue;

			// Workarounds for UDIS bugs (low priority of fixing)
			// UDIS workaround: mov invalid_CR, reg, mov reg, invalid_CR
			if (dump[n] == 0x0f && (dump[n+1] == 0x20 || dump[n + 1] == 0x22)) {
				unsigned cr = (dump[n+2] & 0x38) >> 3;
				if (cr == 1 || (cr >= 5 && cr <= 7) || cr >= 9)
					continue;
			}

			if (arch.cpu_address_size() == osQWord) {
				if ((dump[n] >= 0x48 && dump[n] <= 0x4f && dump[n+1] == 0x6d)
					|| (dump[n] == 0x48 && dump[n+1] == 0x6f)
					|| (dump[n] == 0x9c)
					|| (dump[n] == 0x9d)
					)
					continue;
			}

			//dprint((line.c_str()));
			//dprint(("\n"));

			test_file.OpenFromMemory(dump, (uint32_t)(i / 2));
			udisstr = line.substr(i + 1);
			try {
			if (intel_command.ReadFromFile(arch) != i / 2) {
				if (intel_command.type() == cmDB && (line.substr(i + 1, 2).compare("db") == 0 || udisstr.find("invalid") != udisstr.npos))
					continue;

				FAIL() << "Cannot disassemble:\nudis: '" << udisstr 
					<< "'\nintel_command: '" << intel_command.text() << "'\n" <<
					"binary: " << line.substr(0, i);
			}
			} catch (std::runtime_error & e) {
				FAIL() << "Exception '" << e.what() << "'\n" << "Input: " << line;
			}
			intel_str = intel_command.text();
			std::transform(intel_str.begin(), intel_str.end(), intel_str.begin(), ::tolower);
			if (intel_str.compare(udisstr) != 0) {
				if (udisstr.find("invalid") != udisstr.npos && intel_command.type() == cmDB)
					continue;

				if (udisstr.find("rip+") != udisstr.npos) {
					//dprint((line.c_str()));
					//dprint(("\n"));
					continue;
				}

				if (intel_command.type() == cmLoop ||
					intel_command.type() == cmLoope ||
					intel_command.type() == cmLoopne ||
					intel_command.type() == cmJCXZ ||
					intel_command.type() == cmJmp ||
					intel_command.type() == cmCall ||
					intel_command.type() == cmPusha ||
					intel_command.type() == cmPopa ||
					intel_command.type() == cmJmpWithFlag)
					continue;
				FAIL() << "Disasm error:\nudis: '" << udisstr 
					<< "'\nintel_command: '" << intel_str << "'\n" <<
					"binary: " << line.substr(0, i);
			}
		}
	}
};

TEST(IntelTest, x86_Disasm)
{
	PEFile pf(NULL);
	IntelFunction command_list;

	EXPECT_EQ(pf.Open("test-binaries/win32-app-test1-i386", foRead), osSuccess);
	EXPECT_EQ(pf.count(), 1ul);
	IArchitecture &arch = *pf.item(0);
	EXPECT_EQ(command_list.ReadFromFile(arch, 0x00401198), 53ul);
	EXPECT_TRUE(command_list.GetCommandByNearAddress(0x00401235) != NULL);
	EXPECT_EQ(command_list.item(0)->address(), 0x00401198ull);
	EXPECT_EQ(command_list.item(command_list.count() - 1)->next_address(), 0x00401240ull);
}

TEST(IntelTest, x86_DelphiSEH)
{
	uint8_t buf[] = {
		0x55,							// push ebp
		0x8B, 0xEC,						// mov ebp, esp
		0x83, 0xC4, 0xEC,				// add esp, -14
		0x53,							// push ebx
		0x56,							// push esi
		0x57,							// push edi
		0x33, 0xC0,						// xor eax, eax
		0x89, 0x45, 0xEC,				// mov [ebp-14], eax
		0xA1, 0x9C, 0x1D, 0x41, 0x00,	// mov eax, [00411D9C]
		0xC6, 0x00, 0x01,				// mov byte ptr [eax], 01
		0xB8, 0xB4, 0xE4, 0x40, 0x00,	// mov eax, 0040E4B4
		0xE8, 0x10, 0x89, 0xFF, 0xFF,	// call 00408AA0
		0x33, 0xC0,						// xor eax, eax
		0x55,							// push ebp
		0x68, 0x37, 0x02, 0x41, 0x00,	// push 00410237
		0x64, 0xFF, 0x30,				// push dword ptr fs:[eax]
		0x64, 0x89, 0x20,				// mov fs:[eax], esp
		0x33, 0xC0,						// xor eax, eax
		0x55,							// push ebp
		0x68, 0xD6, 0x01, 0x41, 0x00,	// push 004101D6
		0x64, 0xFF, 0x30,				// push dword ptr fs:[eax]
		0x64, 0x89, 0x20,				// mov fs:[eax], esp
		0xB8, 0x32, 0x00, 0x00, 0x00,	// mov eax, 00000032
		0xE8, 0x06, 0x3B, 0xFF, 0xFF,	// call 00403CBC
		0x8B, 0xD0,						// mov edx, eax
		0xA1, 0xE0, 0x1C, 0x41, 0x00,	// mov eax, [00411CE0]
		0xE8, 0x72, 0x42, 0xFF, 0xFF,	// call 00404434
		0xE8, 0x99, 0x42, 0xFF, 0xFF,	// call 00404460
		0xE8, 0xA0, 0x39, 0xFF, 0xFF,	// call 00403B6C
		0x33, 0xC0,						// xor eax, eax
		0x5A,							// pop edx
		0x59,							// pop ecx
		0x59,							// pop ecx
		0x64, 0x89, 0x10,				// mov fs:[eax], edx
		0xEB, 0x4B,						// jmp 00410221
		0xE9, 0x21, 0x4F, 0xFF, 0xFF,	// jmp 004050FC
		0x01, 0x00, 0x00, 0x00,			// dd 00000001
		0x54, 0x90, 0x41, 0x00,			// dd 00419054
		0xE7, 0x01, 0x41, 0x00,			// dd 004101E7
		0x8B, 0xD8,						// mov ebx, eax
		0x8D, 0x55, 0xEC,				// lea edx, [ebp-14]
		0x8B, 0x03,						// mov eax, [ebx]
		0xE8, 0xBD, 0x45, 0xFF, 0xFF,	// call 004047B0
		0x8B, 0x55, 0xEC,				// mov edx, [ebp-14]
		0xA1, 0xE0, 0x1C, 0x41, 0x00,	// mov eax, [00411CE0]
		0xE8, 0x8C, 0x64, 0xFF, 0xFF,	// call 0040668C
		0xBA, 0x54, 0x02, 0x41, 0x00,	// mov edx, 00410254
		0xE8, 0x82, 0x64, 0xFF, 0xFF,	// call 0040668C
		0x8B, 0x53, 0x04,				// mov edx, [ebx+04]
		0xE8, 0x7A, 0x64, 0xFF, 0xFF,	// call 0040668C
		0xE8, 0x49, 0x42, 0xFF, 0xFF,	// call 00404460
		0xE8, 0x50, 0x39, 0xFF, 0xFF,	// call 00403B6C
		0xE8, 0x07, 0x52, 0xFF, 0xFF,	// call 00405428
		0x33, 0xC0,						// xor eax, eax
		0x5A,							// pop edx
		0x59,							// pop ecx
		0x59,							// pop ecx
		0x64, 0x89, 0x10,				// mov fs:[eax], edx
		0x68, 0x3E, 0x02, 0x41, 0x00,	// push 0041023E
		0x8D, 0x45, 0xEC,				// lea eax, [ebp-14]
		0xE8, 0x66, 0x5E, 0xFF, 0xFF,	// call 0040609C
		0xC3,							// ret
		0xE9, 0x48, 0x50, 0xFF, 0xFF,	// jmp 00405284
		0xEB, 0xF0,						// jmp 0041022E
		0x5F,							// pop edi
		0x5E,							// pop esi
		0x5B,							// pop ebx
		0xE8, 0xF6, 0x56, 0xFF, 0xFF,	// call 0040593C
		0xC3							// ret
	};

	ICommand *command;
	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00410170, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	EXPECT_EQ(func->count(), 69ul);
	command = func->GetCommandByAddress(0x004101a1);
	EXPECT_EQ(command->link()->type(), ltFilterSEHBlock);
	command = func->GetCommandByAddress(0x004101DB);
	EXPECT_EQ(command->text(), "dd 00000001");
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 00419054");
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 004101E7");
	EXPECT_EQ(command->link()->type(), ltMemSEHBlock);
	command = func->GetCommandByAddress(0x00410229);
	EXPECT_EQ(command->link()->type(), ltFinallyBlock);
	// Check CompileToNative
	std::vector<uint8_t> data;
	size_t i, j;
	for (i = 0; i < func->count(); i++) {
		command = func->item(i);
		if ((command->type() == cmJmpWithFlag || command->type() == cmJmp || command->type() == cmCall) && command->dump_size() == 2)
			continue;

		data.clear();
		for (j = 0; j < command->dump_size(); j++) {
			data.push_back(command->dump(j));
		}
		command->CompileToNative();
		EXPECT_EQ(command->dump_size(), data.size());
		for (j = 0; j < command->dump_size(); j++) {
			EXPECT_EQ(command->dump(j), data[j]);
		}
	}
	// Check the compilation prepare
	MemoryManager manager(&arch);
	CompileContext ctx;
	ctx.file = &arch;
	ctx.manager = &manager;
	ASSERT_TRUE(func->Prepare(ctx));
	ASSERT_TRUE(func->PrepareExtCommands(ctx));
	ASSERT_EQ(manager.count(), 2ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 0x00410175ull);
	EXPECT_EQ(region->size(), 0x66ul);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
	region = manager.item(1);
	EXPECT_EQ(region->address(), 0x004101e7ull);
	EXPECT_EQ(region->size(), 0x60ul);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
}

TEST(IntelTest, x86_ParseSDK)
{
	PEFile pf(NULL);
	IntelFunction *func;
	MapFunctionList *map_function_list;
	MapFunction *map_function;
	IntelCommand *command;

	EXPECT_EQ(pf.Open("test-binaries/win32-app-test1-i386", foRead), osSuccess);
	EXPECT_EQ(pf.count(), 1ul);
	IArchitecture &arch = *pf.item(0);
	map_function_list = arch.map_function_list();
	map_function = map_function_list->GetFunctionByAddress(0x004011A3);
	EXPECT_EQ(map_function->name().compare("VMProtectMarker \"Test marker\""), 0);
	map_function = map_function_list->GetFunctionByAddress(0x00403049);
	EXPECT_EQ(map_function->name().compare("string \"Correct password\""), 0);
	map_function = map_function_list->GetFunctionByAddress(0x00403066);
	EXPECT_EQ(map_function->name().compare("string \"Incorrect password\""), 0);
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(0x004011A3, ctVirtualization, 0, true, NULL));
	EXPECT_EQ(func->count(), 47ul);
	// Check CompileToNative
	std::vector<uint8_t> data;
	size_t i, j;
	for (i = 0; i < func->count(); i++) {
		command = func->item(i);
		if ((command->type() == cmJmpWithFlag || command->type() == cmJmp || command->type() == cmCall) && command->dump_size() == 2)
			continue;

		data.clear();
		for (j = 0; j < command->dump_size(); j++) {
			data.push_back(command->dump(j));
		}
		command->CompileToNative();
		EXPECT_EQ(command->dump_size(), data.size());
		for (j = 0; j < command->dump_size(); j++) {
			EXPECT_EQ(command->dump(j), data[j]);
		}
	}
}

TEST(IntelTest, Clone)
{
	PEFile pf(NULL);

	ASSERT_EQ(pf.Open("test-binaries/win32-app-test1-i386", foRead), osSuccess);
	EXPECT_EQ(pf.count(), 1ul);
	IFunctionList *function_list = pf.item(0)->function_list();
	Folder *f1 = pf.folder_list()->Add("Folder1");
	Folder *f2 = f1->Add("Folder2");
	IFunction *func = function_list->AddByAddress(0x004011A3, ctUltra, 0, false, f2);
	size_t command_count = func->count();
	size_t link_count = func->link_list()->count();
	uint64_t link_address = func->link_list()->item(2)->from_command()->address();
	IFunctionList *clone_function_list = function_list->Clone(NULL);
	pf.Close();
	// Check clone function list.
	//EXPECT_EQ(clone_function_list->root_folder()->count(), 1);
	//EXPECT_EQ(clone_function_list->root_folder()->item(0)->count(), 1);
	//EXPECT_EQ(clone_function_list->root_folder()->item(0)->name().compare("Folder1"), 0);

	ASSERT_EQ(clone_function_list->count(), 1ul);
	func = clone_function_list->item(0);
	EXPECT_EQ(func->compilation_type(), ctUltra);
	EXPECT_EQ(func->need_compile(), false);
	//EXPECT_EQ(func->folder()->name().compare("Folder2"), 0);
	ASSERT_EQ(func->count(), command_count);
	ASSERT_EQ(func->link_list()->count(), link_count);
	EXPECT_EQ(func->link_list()->item(2)->from_command()->address(), link_address);
	delete clone_function_list;
}

TEST(IntelTest, x86_Switch)
{
	uint8_t buf[] = {
		0x83, 0xEC, 0x0C,  // sub esp, 0C
		0x53,  // push ebx
		0x56,  // push esi
		0x57,  // push edi
		0x89, 0x65, 0xE8,  // mov [ebp-18], esp
		0x8B, 0xF1,  // mov esi, ecx
		0x33, 0xC0,  // xor eax, eax
		0x89, 0x45, 0xE4,  // mov [ebp-1C], eax
		0x89, 0x45, 0xFC,  // mov [ebp-04], eax
		0x52,  // push edx
		0x51,  // push ecx
		0x53,  // push ebx
		0xB8, 0x68, 0x58, 0x4D, 0x56,  // mov eax, 564D5868
		0x33, 0xDB,  // xor ebx, ebx
		0xB9, 0x0A, 0x00, 0x00, 0x00,  // mov ecx, 0000000A
		0xBA, 0x58, 0x56, 0x00, 0x00,  // mov edx, 00005658
		0xED,  // in eax, dx
		0x81, 0xFB, 0x68, 0x58, 0x4D, 0x56,  // cmp ebx, 564D5868
		0x75, 0x03,  // jnz 00403050
		0x89, 0x4D, 0xE4,  // mov [ebp-1C], ecx
		0x5B,  // pop ebx
		0x59,  // pop ecx
		0x5A,  // pop edx
		0x8B, 0x45, 0xE4,  // mov eax, [ebp-1C]
		0x48,  // dec eax
		0x83, 0xF8, 0x03,  // cmp eax, 03
		0x0F, 0x87, 0x97, 0x00, 0x00, 0x00,  // jnbe 004030F7
		0xFF, 0x24, 0x85, 0x40, 0x31, 0x40, 0x00,  // jmp dword ptr [eax*4+00403140]
		0x68, 0x74, 0xD8, 0x4E, 0x00,  // push 004ED874
		0xE8, 0xDF, 0x00, 0x00, 0x00,  // call 00403150
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0xB0, 0x01,  // mov al, 01
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0x68, 0x94, 0xD8, 0x4E, 0x00,  // push 004ED894
		0xE8, 0xBB, 0x00, 0x00, 0x00,  // call 00403150
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0xB0, 0x01,  // mov al, 01
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0x68, 0xB8, 0xD8, 0x4E, 0x00,  // push 004ED8B8
		0xE8, 0x97, 0x00, 0x00, 0x00,  // call 00403150
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0xB0, 0x01,  // mov al, 01
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0x68, 0xDC, 0xD8, 0x4E, 0x00,  // push 004ED8DC
		0xE8, 0x73, 0x00, 0x00, 0x00,  // call 00403150
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0xB0, 0x01,  // mov al, 01
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0x68, 0x04, 0xD9, 0x4E, 0x00,  // push 004ED904
		0xE8, 0x4F, 0x00, 0x00, 0x00,  // call 00403150
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0xB0, 0x01,  // mov al, 01
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0xB8, 0x01, 0x00, 0x00, 0x00,  // mov eax, 00000001
		0xC3,  // ret
		0x8B, 0x65, 0xE8,  // mov esp, [ebp-18]
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0x32, 0xC0,  // xor al, al
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0xCD, 0xCD,  // alignment
		0x67, 0x30, 0x40, 0x00,  // dd 00403067
		0x8B, 0x30, 0x40, 0x00,  // dd 0040308B
		0xAF, 0x30, 0x40, 0x00,  // dd 004030AF
		0xD3, 0x30, 0x40, 0x00,  // dd 004030D3
		0x00, 0x00, 0x00, 0x00   // dd 0
	};

	ICommand *command;
	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x0040301D, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 92ul);
	command = func->GetCommandByAddress(0x00403140);
	EXPECT_EQ(command->text(), "dd 00403067");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 0040308B");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 004030AF");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 004030D3");
	EXPECT_EQ(command->link()->type(), ltCase);
	// Check CompileToNative
	std::vector<uint8_t> data;
	size_t i, j;
	for (i = 0; i < func->count(); i++) {
		command = func->item(i);
		if ((command->type() == cmJmpWithFlag || command->type() == cmJmp || command->type() == cmCall) && command->dump_size() == 2)
			continue;

		data.clear();
		for (j = 0; j < command->dump_size(); j++) {
			data.push_back(command->dump(j));
		}
		command->CompileToNative();
		EXPECT_EQ(command->dump_size(), data.size());
		for (j = 0; j < command->dump_size(); j++) {
			EXPECT_EQ(command->dump(j), data[j]);
		}
	}
	// Check the compilation prepare
	MemoryManager manager(&arch);
	CompileContext ctx;
	ctx.file = &arch;
	ctx.manager = &manager;
	ASSERT_TRUE(func->Prepare(ctx));
	ASSERT_TRUE(func->PrepareExtCommands(ctx));
	ASSERT_EQ(manager.count(), 2ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 0x00403022ull);
	EXPECT_EQ(region->size(), 0xF9ul);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
	region = manager.item(1);
	EXPECT_EQ(region->address(), 0x00403140ull);
	EXPECT_EQ(region->size(), 0x10ul);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
}

TEST(IntelTest, x86_MsvcSEH)
{
	uint8_t buf[] = {
		0x55,  // push ebp
		0x8B, 0xEC,  // mov ebp, esp
		0x6A, 0xFF,  // push -01
		0x68, 0x48, 0x32, 0x40, 0x00,  // push 00403248
		0x68, 0xE4, 0x7A, 0x4D, 0x00,  // push 004D7AE4
		0x64, 0xA1, 0x00, 0x00, 0x00, 0x00,  // mov eax, fs:[00000000]
		0x50,  // push eax
		0x64, 0x89, 0x25, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], esp
		0x83, 0xEC, 0x08,  // sub esp, 08
		0x53,  // push ebx
		0x56,  // push esi
		0x57,  // push edi
		0x89, 0x65, 0xE8,  // mov [ebp-18], esp
		0xC7, 0x45, 0xFC, 0x00, 0x00, 0x00, 0x00,  // mov dword ptr [ebp-04], 00000000
		0xB8, 0x01, 0x00, 0x00, 0x00,  // mov eax, 00000001
		0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, // nop
		0x68, 0x14, 0xD9, 0x4E, 0x00,  // push 004ED914
		0x8B, 0xF1,  // mov esi, ecx
		0xE8, 0x47, 0xFF, 0xFF, 0xFF,  // call 00403150
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0xB0, 0x01,  // mov al, 01
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0xB8, 0x01, 0x00, 0x00, 0x00,  // mov eax, 00000001
		0xC3,  // ret
		0x8B, 0x65, 0xE8,  // mov esp, [ebp-18]
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0x32, 0xC0,  // xor al, al
		0x8B, 0x4D, 0xF0,  // mov ecx, [ebp-10]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0xCD, 0xCD,  // alignment
		0xFF, 0xFF, 0xFF, 0xFF,  // dd FFFFFFFF
		0x23, 0x32, 0x40, 0x00,  // dd 00403223
		0x29, 0x32, 0x40, 0x00,  // dd 00403229
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00   // dd 0
	};

	ICommand *command;
	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x004031C0, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 55ul);
	command = func->GetCommandByAddress(0x00403248);
	EXPECT_EQ(command->text(), "dd FFFFFFFF");
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 00403223");
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 00403229");
	// Check CompileToNative
	std::vector<uint8_t> data;
	size_t i, j;
	for (i = 0; i < func->count(); i++) {
		command = func->item(i);
		if ((command->type() == cmJmpWithFlag || command->type() == cmJmp || command->type() == cmCall) && command->dump_size() == 2)
			continue;

		data.clear();
		for (j = 0; j < command->dump_size(); j++) {
			data.push_back(command->dump(j));
		}
		command->CompileToNative();
		EXPECT_EQ(command->dump_size(), data.size());
		for (j = 0; j < command->dump_size(); j++) {
			EXPECT_EQ(command->dump(j), data[j]);
		}
	}
	// Check the compilation prepare
	MemoryManager manager(&arch);
	CompileContext ctx;
	ctx.file = &arch;
	ctx.manager = &manager;
	ASSERT_TRUE(func->Prepare(ctx));
	ASSERT_TRUE(func->PrepareExtCommands(ctx));
	ASSERT_EQ(manager.count(), 1ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 0x004031C5ull);
	EXPECT_EQ(region->size(), 0x81ul);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
}

TEST(IntelTest, x64_Switch)
{
	uint8_t buf[] = {
		0x83, 0xFB, 0x06,  // cmp ebx, 06
		0x74, 0x0A,  // jz 000000010001874A
		0x83, 0xFB, 0x07,  // cmp ebx, 07
		0x74, 0x05,  // jz 000000010001874A
		0x83, 0xFB, 0x08,  // cmp ebx, 08
		0x75, 0x35,  // jnz 000000010001877F
		0xBA, 0x04, 0xE8, 0x00, 0x00,  // mov edx, 0000E804
		0xE8, 0xBA, 0x2B, 0x01, 0x00,  // call 000000010002B30E
		0x48, 0x85, 0xC0,  // test rax, rax
		0x74, 0x0A,  // jz 0000000100018763
		0x33, 0xD2,  // xor edx, edx
		0x48, 0x8B, 0xC8,  // mov rcx, rax
		0xE8, 0x17, 0x20, 0x01, 0x00,  // call 000000010002A77A
		0xBA, 0x05, 0xE8, 0x00, 0x00,  // mov edx, 0000E805
		0x48, 0x8B, 0xCF,  // mov rcx, rdi
		0xE8, 0x9E, 0x2B, 0x01, 0x00,  // call 000000010002B30E
		0x48, 0x85, 0xC0,  // test rax, rax
		0x74, 0x0A,  // jz 000000010001877F
		0x33, 0xD2,  // xor edx, edx
		0x48, 0x8B, 0xC8,  // mov rcx, rax
		0xE8, 0xFB, 0x1F, 0x01, 0x00,  // call 000000010002A77A
		0x8D, 0x43, 0xFF,  // lea eax, [rbx-01]
		0x83, 0xF8, 0x09,  // cmp eax, 09
		0x77, 0x27,  // jnbe 00000001000187AE
		0x48, 0x8D, 0x15, 0x72, 0x78, 0xFE, 0xFF,  // lea rdx, [0000000100000000]
		0x48, 0x98,  // cdqe
		0x8B, 0x8C, 0x82, 0xF8, 0x87, 0x01, 0x00,  // mov ecx, [rdx+rax*4+00000000000187F8]
		0x48, 0x03, 0xCA,  // add rcx, rdx
		0xFF, 0xE1,  // jmp rcx
		0x4C, 0x8B, 0x8F, 0x58, 0x01, 0x00, 0x00,  // mov r9, [rdi+0000000000000158]
		0xEB, 0x10,  // jmp 00000001000187B5
		0x4C, 0x8B, 0x8F, 0x60, 0x01, 0x00, 0x00,  // mov r9, [rdi+0000000000000160]
		0xEB, 0x07,  // jmp 00000001000187B5
		0x4C, 0x8B, 0x8F, 0x50, 0x01, 0x00, 0x00,  // mov r9, [rdi+0000000000000150]
		0x48, 0x8B, 0x4F, 0x40,  // mov rcx, [rdi+40]
		0xBA, 0x80, 0x00, 0x00, 0x00,  // mov edx, 00000080
		0x44, 0x8D, 0x42, 0x81,  // lea r8d, [rdx-7F]
		0xFF, 0x15, 0xA8, 0xA2, 0xFE, 0xFF,  // call qword ptr [0000000100002A70]
		0x48, 0x8D, 0x0D, 0x81, 0x77, 0x02, 0x00,  // lea rcx, [000000010003FF50]
		0x41, 0xB8, 0x01, 0x00, 0x00, 0x00,  // mov r8d, 00000001
		0x8B, 0xD3,  // mov edx, ebx
		0xE8, 0xD4, 0x77, 0x00, 0x00,  // call 000000010001FFB0
		0x48, 0x8B, 0xCF,  // mov rcx, rdi
		0x48, 0x8B, 0xD0,  // mov rdx, rax
		0xE8, 0x87, 0x28, 0x01, 0x00,  // call 000000010002B06E
		0x48, 0x8B, 0x7C, 0x24, 0x48,  // mov rdi, [rsp+48]
		0x48, 0x8B, 0x5C, 0x24, 0x40,  // mov rbx, [rsp+40]
		0x33, 0xC0,  // xor eax, eax
		0x48, 0x83, 0xC4, 0x28,  // add rsp, 28
		0xC3,  // ret
		0xAE, 0x87, 0x01, 0x00,  // dd 000187AE
		0xAE, 0x87, 0x01, 0x00,  // dd 000187AE
		0xAE, 0x87, 0x01, 0x00,  // dd 000187AE
		0xA5, 0x87, 0x01, 0x00,  // dd 000187A5
		0xAE, 0x87, 0x01, 0x00,  // dd 000187AE
		0x9C, 0x87, 0x01, 0x00,  // dd 0001879C
		0x9C, 0x87, 0x01, 0x00,  // dd 0001879C
		0xAE, 0x87, 0x01, 0x00,  // dd 000187AE
		0xAE, 0x87, 0x01, 0x00,  // dd 000187AE
		0xAE, 0x87, 0x01, 0x00,  // dd 000187AE
		0x00, 0x00, 0x00, 0x00   // dd 0
	};

	ICommand *command;
	IntelFunction *func;
	TestFile test_file(osQWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x010001873Bull, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	EXPECT_EQ(func->count(), 60ul);
	command = func->GetCommandByAddress(0x01000187F8ull);
	EXPECT_EQ(command->text(), "dd 000187AE");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 000187AE");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 000187AE");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 000187A5");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 000187AE");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 0001879C");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 0001879C");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 000187AE");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 000187AE");
	EXPECT_EQ(command->link()->type(), ltCase);
	command = func->GetCommandByAddress(command->next_address());
	EXPECT_EQ(command->text(), "dd 000187AE");
	EXPECT_EQ(command->link()->type(), ltCase);
	// Check CompileToNative
	std::vector<uint8_t> data;
	size_t i, j;
	for (i = 0; i < func->count(); i++) {
		command = func->item(i);
		if ((command->type() == cmJmpWithFlag || command->type() == cmJmp || command->type() == cmCall) && command->dump_size() == 2)
			continue;

		data.clear();
		for (j = 0; j < command->dump_size(); j++) {
			data.push_back(command->dump(j));
		}
		command->CompileToNative();
		EXPECT_EQ(command->dump_size(), data.size());
		for (j = 0; j < command->dump_size(); j++) {
			EXPECT_EQ(command->dump(j), data[j]);
		}
	}
	// Check the compilation prepare
	MemoryManager manager(&arch);
	CompileContext ctx;
	ctx.file = &arch;
	ctx.manager = &manager;
	ASSERT_TRUE(func->Prepare(ctx));
	ASSERT_TRUE(func->PrepareExtCommands(ctx));
	ASSERT_EQ(manager.count(), 1ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 0x0100018740ull);
	EXPECT_EQ(region->size(), 0xe0ul);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
}

TEST(IntelTest, x86_MsvcSEH2)
{
	uint8_t buf[] = {
		0x55,  // push ebp
		0x8B, 0xEC,  // mov ebp, esp
		0x6A, 0xFF,  // push -01
		0x68, 0x7E, 0x10, 0x40, 0x00,  // push 0040107E
		0x64, 0xA1, 0x00, 0x00, 0x00, 0x00,  // mov eax, fs:[00000000]
		0x50,  // push eax
		0x51,  // push ecx
		0x53,  // push ebx
		0x56,  // push esi
		0x57,  // push edi
		0xA1, 0x00, 0x30, 0x40, 0x00,  // mov eax, [00403000]
		0x33, 0xC5,  // xor eax, ebp
		0x50,  // push eax
		0x8D, 0x45, 0xF4,  // lea eax, [ebp-0C]
		0x64, 0xA3, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], eax
		0x89, 0x65, 0xF0,  // mov [ebp-10], esp
		0xC7, 0x45, 0xFC, 0x00, 0x00, 0x00, 0x00,  // mov dword ptr [ebp-04], 00000000
		0x8B, 0x45, 0x08,  // mov eax, [ebp+08]
		0x99,  // cdq
		0x33, 0xC9,  // xor ecx, ecx
		0xF7, 0xF9,  // idiv ecx
		0x85, 0xC0,  // test eax, eax
		0x75, 0x0E,  // jnz 0040104A
		0x68, 0xF4, 0x20, 0x40, 0x00,  // push 004020F4
		0xFF, 0x15, 0xA0, 0x20, 0x40, 0x00,  // call dword ptr [004020A0]
		0x83, 0xC4, 0x04,  // add esp, 04
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0x8B, 0x4D, 0xF4,  // mov ecx, [ebp-0C]
		0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00,  // mov fs:[00000000], ecx
		0x59,  // pop ecx
		0x5F,  // pop edi
		0x5E,  // pop esi
		0x5B,  // pop ebx
		0x8B, 0xE5,  // mov esp, ebp
		0x5D,  // pop ebp
		0xC3,  // ret
		0x68, 0x00, 0x21, 0x40, 0x00,  // push 00402100
		0xFF, 0x15, 0xA0, 0x20, 0x40, 0x00,  // call dword ptr [004020A0]
		0x83, 0xC4, 0x04,  // add esp, 04
		0xC7, 0x45, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF,  // mov dword ptr [ebp-04], FFFFFFFF
		0xB8, 0x51, 0x10, 0x40, 0x00,  // mov eax, 00401051
		0xC3,  // ret
		0x8B, 0x54, 0x24, 0x08,  // mov edx, [esp+08]
		0x8D, 0x42, 0x0C,  // lea eax, [edx+0C]
		0x8B, 0x4A, 0xEC,  // mov ecx, [edx-14]
		0x33, 0xC8,  // xor ecx, eax
		0xE8, 0xE3, 0xF7, 0xFF, 0xFF,  // call 00401094
		0xB8, 0xD0, 0x10, 0x40, 0x00,  // mov eax, 004010D0
		0xE9, 0xD3, 0xFF, 0xFF, 0xFF,  // jmp 0040188E
		0xCD, 0xCD, 0xCD,  // alignment
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x63, 0x10, 0x40, 0x00,  // dd 00401063
		0xFF, 0xFF, 0xFF, 0xFF,  // dd FFFFFFFF
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0xFF, 0xFF, 0xFF, 0xFF,  // dd FFFFFFFF
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x01, 0x00, 0x00, 0x00,  // dd 00000001
		0x01, 0x00, 0x00, 0x00,  // dd 00000001
		0x9C, 0x10, 0x40, 0x00,  // dd 0040109C
		0x22, 0x05, 0x93, 0x19,  // dd 19930522
		0x02, 0x00, 0x00, 0x00,  // dd 00000002
		0xAC, 0x10, 0x40, 0x00,  // dd 004010AC
		0x01, 0x00, 0x00, 0x00,  // dd 00000001
		0xBC, 0x10, 0x40, 0x00,  // dd 004010BC
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
		0x00, 0x00, 0x00, 0x00,  // dd 00000000
	};

	ICommand *command;
	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00401000, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 64ul);
	command = func->GetCommandByAddress(0x004010a8);
	EXPECT_EQ(command->text(), "dd 00401063");
	EXPECT_EQ(command->link()->type(), ltExtSEHHandler);
	// Check CompileToNative
	std::vector<uint8_t> data;
	size_t i, j;
	for (i = 0; i < func->count(); i++) {
		command = func->item(i);
		if ((command->type() == cmJmpWithFlag || command->type() == cmJmp || command->type() == cmCall) && command->dump_size() == 2)
			continue;

		data.clear();
		for (j = 0; j < command->dump_size(); j++) {
			data.push_back(command->dump(j));
		}
		command->CompileToNative();
		EXPECT_EQ(command->dump_size(), data.size());
		for (j = 0; j < command->dump_size(); j++) {
			EXPECT_EQ(command->dump(j), data[j]);
		}
	}
	// Check the compilation prepare
	MemoryManager manager(&arch);
	CompileContext ctx;
	ctx.file = &arch;
	ctx.manager = &manager;
	ASSERT_TRUE(func->Prepare(ctx));
	ASSERT_TRUE(func->PrepareExtCommands(ctx));
	ASSERT_EQ(manager.count(), 1ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 0x00401005ull);
	EXPECT_EQ(region->size(), 0x79ul);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
}

TEST(IntelTest, x86_FPU)
{
	uint8_t buf[] = {
		0x9B, 0xD9, 0x7D, 0xFC,  // fstcw [ebp - 4]
		0x9B, 0xDD, 0x7D, 0xFC,  // fstsw [ebp - 4]
		0x9B, // wait
		0x90, // nop
		0xD9, 0x7D, 0xFC,  // fnstcw [ebp - 4]
		0xDD, 0x7D, 0xFC,  // fnstsw [ebp - 4]
		0xC3,  // ret
		0xCD, 0xCD, 0xCD, 0xCD,  // alignment
	};

	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00401000, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 7ul);
	EXPECT_EQ(func->item(0)->text(), "fstcw word ptr [ebp-04]");
	EXPECT_EQ(func->item(1)->text(), "fstsw word ptr [ebp-04]");
	EXPECT_EQ(func->item(4)->text(), "fnstcw word ptr [ebp-04]");
	EXPECT_EQ(func->item(5)->text(), "fnstsw word ptr [ebp-04]");
}

TEST(IntelTest, x86_DisasmBranch1)
{
	uint8_t buf[] = {
		0x90, // nop
		0xE9, 0x01, 0x00, 0x00, 0x00, // jmp +1
		0xCC, // --
		0xE9, 0x01, 0x00, 0x00, 0x00, // jmp +1
		0xCC, // --
		0xC3, // ret
	};

	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00401000, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 2ul);
}

TEST(IntelTest, x86_DisasmBranch2)
{
	uint8_t buf[] = {
		0x90, // nop
		0xE9, 0x01, 0x00, 0x00, 0x00, // jmp +1
		0xCC, // --
		0x90, // nop
		0x90, // nop
		0x90, // nop
		0x90, // nop
		0xE9, 0x01, 0x00, 0x00, 0x00, // jmp +1
		0xCC, // --
		0x0F, 0x84, 0xF3, 0xFF, 0xFF, 0xFF, // jz -13
		0x90, // nop
		0xC3, // ret
	};

	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00401000, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 2ul);
}

TEST(IntelTest, x86_DisasmBranch3)
{
	uint8_t buf[] = {
		0x90, // nop
		0xE9, 0x01, 0x00, 0x00, 0x00, // jmp +1
		0xCC, // --
		0x90, // nop
		0x0F, 0x84, 0xF2, 0xFF, 0xFF, 0xFF, // jz -14
		0x90, // nop
		0x90, // nop
		0xE9, 0x01, 0x00, 0x00, 0x00, // jmp +1
		0xCC, // --
		0x0F, 0x84, 0xF3, 0xFF, 0xFF, 0xFF, // jz -13
		0x90, // nop
		0xC3, // ret
	};

	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00401000, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 10ul);
}

TEST(IntelTest, x86_DisasmBranch4)
{
	uint8_t buf[] = {
		0x90, // nop
		0xE9, 0x03, 0x00, 0x00, 0x00, // jmp +3
		0xCC, // --
		0x90, // nop
		0x90, // nop
		0x90, // nop
		0x90, // nop
		0x0F, 0x84, 0xF6, 0xFF, 0xFF, 0xFF, // jz -10
		0x90, // nop
		0x90, // nop
		0xE9, 0x01, 0x00, 0x00, 0x00, // jmp +1
		0xCC, // --
		0x0F, 0x84, 0xF3, 0xFF, 0xFF, 0xFF, // jz -13
		0x90, // nop
		0xC3, // ret
	};

	IntelFunction *func;
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00401000, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment_list->item(0)->address(), ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 13ul);
}

#ifdef _WIN32
void CompileFunction(void *src, void *dest)
{
	uint8_t *buf = reinterpret_cast<uint8_t *>(src);
	if (buf[0] == 0xe9) {
		// jmp
		uint32_t dw = *reinterpret_cast<uint32_t*>(&buf[1]);
		buf = &buf[dw + 5];
	}
	TestFile test_file(osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(reinterpret_cast<uint64_t>(dest), 0x1000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(static_cast<uint32_t>(segment->size()));
	test_file.OpenFromMemory(buf, static_cast<uint32_t>(0x1000 - 0x100));
	IntelFunction *func = reinterpret_cast<IntelFunction *>(arch.function_list()->AddByAddress(segment->address(), ctVirtualization, 0, true, NULL));
	for (size_t i = 0; i < func->count(); i++) {
		IntelCommand *command = func->item(i);

		if (command->type() == cmCall)
			command->set_operand_value(0, command->operand(0).value + reinterpret_cast<uint64_t>(buf) - reinterpret_cast<uint64_t>(dest));
	}
	CompileOptions options;
	options.flags = cpEncryptBytecode;
	//int r = GetTickCount();
	//printf("%d\n", r);
	//srand(r);
	arch.Compile(options, NULL);

	arch.Seek(0);
	arch.Read(dest, (size_t)arch.size());
}

void DoTestFunctionDWord(void *func_)
{
	void *address = VirtualAlloc(NULL, 0x20000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	ASSERT_TRUE(address != NULL);
	CompileFunction(func_, address);

	uint32_t orig_flags, flags;
	uint32_t param1, param2, orig_res, res;
	for (size_t i = 0; i < 100; i++) {
		param1 = rand32();
		param2 = rand32();
		typedef uint32_t (tFunc)(uint32_t param1, uint32_t param2, uint32_t *flags);
		orig_res = reinterpret_cast<tFunc *>(func_)(param1, param2, &orig_flags);
		res = reinterpret_cast<tFunc *>(address)(param1, param2, &flags);
		EXPECT_EQ(res, orig_res);
		EXPECT_EQ(flags, orig_flags);
	}

	VirtualFree(address, 0, MEM_RELEASE);
}

void DoTestFunctionWord(void *func)
{
	void *address = VirtualAlloc(NULL, 0x20000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	ASSERT_TRUE(address != NULL);
	CompileFunction(func, address);

	uint32_t orig_flags, flags;
	uint16_t param1, param2, orig_res, res;
	for (size_t i = 0; i < 100; i++) {
		param1 = rand();
		param2 = rand();
		typedef uint16_t (tFunc)(uint16_t param1, uint16_t param2, uint32_t *flags);
		orig_res = reinterpret_cast<tFunc *>(func)(param1, param2, &orig_flags);
		res = reinterpret_cast<tFunc *>(address)(param1, param2, &flags);
		EXPECT_EQ(res, orig_res);
		EXPECT_EQ(flags, orig_flags);
	}

	VirtualFree(address, 0, MEM_RELEASE);
}

void DoTestFunctionByte(void *func)
{
	void *address = VirtualAlloc(NULL, 0x20000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	ASSERT_TRUE(address != NULL);
	CompileFunction(func, address);

	uint32_t orig_flags, flags;
	uint8_t param1, param2, orig_res;
	for (size_t i = 0; i < 100; i++) {
		param1 = rand();
		param2 = rand();
		typedef uint8_t (tFunc)(uint8_t param1, uint8_t param2, uint32_t *flags);
		orig_res = reinterpret_cast<tFunc *>(func)(param1, param2, &orig_flags);
		EXPECT_EQ(reinterpret_cast<tFunc *>(address)(param1, param2, &flags), orig_res);
		EXPECT_EQ(flags, orig_flags);
	}

	VirtualFree(address, 0, MEM_RELEASE);
}

#ifndef _WIN64
uint32_t AddDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		add eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t AddWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		add ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t AddByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		add al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationAdd)
{
	DoTestFunctionDWord(&AddDWord);
	DoTestFunctionWord(&AddWord);
	DoTestFunctionByte(&AddByte);
}

uint32_t AdcDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		adc eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t AdcWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		adc ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t AdcByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		adc al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationAdc)
{
	DoTestFunctionDWord(&AdcDWord);
	DoTestFunctionWord(&AdcWord);
	DoTestFunctionByte(&AdcByte);
}

uint32_t SubDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		sub eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t SubWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		sub ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t SubByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		sub al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationSub)
{
	DoTestFunctionDWord(&SubDWord);
	DoTestFunctionWord(&SubWord);
	DoTestFunctionByte(&SubByte);
}

uint32_t SbbDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		sbb eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t SbbWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		sbb ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t SbbByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		sbb al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationSbb)
{
	DoTestFunctionDWord(&SbbDWord);
	DoTestFunctionWord(&SbbWord);
	DoTestFunctionByte(&SbbByte);
}

uint32_t CmpDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		cmp eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t CmpWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		cmp ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t CmpByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		cmp al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationCmp)
{
	DoTestFunctionDWord(&CmpDWord);
	DoTestFunctionWord(&CmpWord);
	DoTestFunctionByte(&CmpByte);
}

uint32_t ShlDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		shl eax, cl

		mov res, eax
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t ShlWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		shl ax, cl

		mov res, ax
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t ShlByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		shl al, cl

		mov res, al
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationShl)
{
	DoTestFunctionDWord(&ShlDWord);
	DoTestFunctionWord(&ShlWord);
	DoTestFunctionByte(&ShlByte);
}

uint32_t ShrDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		shr eax, cl

		mov res, eax
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t ShrWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		shr ax, cl

		mov res, ax
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t ShrByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		shr al, cl

		mov res, al
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationShr)
{
	DoTestFunctionDWord(&ShrDWord);
	DoTestFunctionWord(&ShrWord);
	DoTestFunctionByte(&ShrByte);
}

uint32_t RolDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		rol eax, cl

		mov res, eax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t RolWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		rol ax, cl

		mov res, ax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t RolByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		rol al, cl

		mov res, al
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationRol)
{
	DoTestFunctionDWord(&RolDWord);
	DoTestFunctionWord(&RolWord);
	DoTestFunctionByte(&RolByte);
}

uint32_t RorDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		ror eax, cl

		mov res, eax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t RorWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		ror ax, cl

		mov res, ax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t RorByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		ror al, cl

		mov res, al
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationRor)
{
	DoTestFunctionDWord(&RorDWord);
	DoTestFunctionWord(&RorWord);
	DoTestFunctionByte(&RorByte);
}

uint32_t SarDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		sar eax, cl

		mov res, eax
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t SarWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		sar ax, cl

		mov res, ax
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t SarByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		sar al, cl

		mov res, al
		pushfd
		pop eax
		and eax, ~fl_A
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationSar)
{
	DoTestFunctionDWord(&SarDWord);
	DoTestFunctionWord(&SarWord);
	DoTestFunctionByte(&SarByte);
}

uint32_t RclDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		rcl eax, cl

		mov res, eax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t RclWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		rcl ax, cl

		mov res, ax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t RclByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		rcl al, cl
		
		mov res, al
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationRcl)
{
	DoTestFunctionDWord(&RclDWord);
	DoTestFunctionWord(&RclWord);
	DoTestFunctionByte(&RclByte);
}

uint32_t RcrDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		rcr eax, cl

		mov res, eax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t RcrWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		rcr ax, cl

		mov res, ax
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t RcrByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		rcr al, cl
		
		mov res, al
		pushfd
		pop eax
		and cl, 31
		jnz label_1
		xor eax, eax
label_1:
		cmp cl, 1
		jz label_2
		and eax, ~fl_O
label_2:
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationRcr)
{
	DoTestFunctionDWord(&RcrDWord);
	DoTestFunctionWord(&RcrWord);
	DoTestFunctionByte(&RcrByte);
}

uint32_t XorDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		xor eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t XorWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		xor ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t XorByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		xor al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationXor)
{
	DoTestFunctionDWord(&XorDWord);
	DoTestFunctionWord(&XorWord);
	DoTestFunctionByte(&XorByte);
}

uint32_t OrDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		or eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t OrWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		or ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t OrByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		or al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationOr)
{
	DoTestFunctionDWord(&OrDWord);
	DoTestFunctionWord(&OrWord);
	DoTestFunctionByte(&OrByte);
}

uint32_t AndDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		and eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t AndWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		and ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t AndByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		and al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationAnd)
{
	DoTestFunctionDWord(&AndDWord);
	DoTestFunctionWord(&AndWord);
	DoTestFunctionByte(&AndByte);
}

uint32_t TestDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		test eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t TestWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		test ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t TestByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		test al, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationTest)
{
	DoTestFunctionDWord(&TestDWord);
	DoTestFunctionWord(&TestWord);
	DoTestFunctionByte(&TestByte);
}

uint32_t NotDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		not eax

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t NotWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		not ax

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t NotByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		not al

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationNot)
{
	DoTestFunctionDWord(&NotDWord);
	DoTestFunctionWord(&NotWord);
	DoTestFunctionByte(&NotByte);
}

uint32_t NegDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		neg eax

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t NegWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		neg ax

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t NegByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		neg al

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationNeg)
{
	DoTestFunctionDWord(&NegDWord);
	DoTestFunctionWord(&NegWord);
	DoTestFunctionByte(&NegByte);
}

uint32_t XaddDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax

		xadd res, ecx
		
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t XaddWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		mov res, ax

		xadd res, cx

		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t XaddByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		mov res, al

		xadd res, cl

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationXadd)
{
	DoTestFunctionDWord(&XaddDWord);
	DoTestFunctionWord(&XaddWord);
	DoTestFunctionByte(&XaddByte);
}

uint32_t XchgDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		xchg eax, ecx

		mov res, eax
		cmp eax, param1
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t XchgWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2

		xchg ax, cx

		mov res, ax
		cmp ax, param1
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t XchgByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		xchg al, cl

		mov res, al
		cmp al, param1
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationXchg)
{
	DoTestFunctionDWord(&XchgDWord);
	DoTestFunctionWord(&XchgWord);
	DoTestFunctionByte(&XchgByte);
}

uint32_t IncDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		inc eax

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t IncWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		inc ax

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t IncByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		inc al

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationInc)
{
	DoTestFunctionDWord(&IncDWord);
	DoTestFunctionWord(&IncWord);
	DoTestFunctionByte(&IncByte);
}

uint32_t DecDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		dec eax

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t DecWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		dec ax

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t DecByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		dec al

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationDec)
{
	DoTestFunctionDWord(&DecDWord);
	DoTestFunctionWord(&DecWord);
	DoTestFunctionByte(&DecByte);
}

uint32_t MulDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		push edx
		mov eax, param1
		mov ecx, param2

		mul ecx

		pushfd
		add eax, edx
		mov res, eax
		pop eax
		and eax, ~(fl_S | fl_Z | fl_A | fl_P)
		mov ecx, flags
		mov [ecx], eax
		pop edx
		pop ecx
		pop eax
	}
	return res;
}

uint16_t MulWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		push edx
		mov ax, param1

		mul param2

		pushfd
		add ax, dx
		mov res, ax
		pop eax
		and eax, ~(fl_S | fl_Z | fl_A | fl_P)
		mov ecx, flags
		mov [ecx], eax
		pop edx
		pop ecx
		pop eax
	}
	return res;
}

uint8_t MulByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		mul cl

		pushfd
		add al, ah
		mov res, al
		pop eax
		and eax, ~(fl_S | fl_Z | fl_A | fl_P)
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationMul)
{
	DoTestFunctionDWord(&MulDWord);
	DoTestFunctionWord(&MulWord);
	DoTestFunctionByte(&MulByte);
}

uint32_t ImulDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		push edx
		mov eax, param1
		mov ecx, param2

		imul ecx

		pushfd
		add eax, edx
		mov res, eax
		pop eax
		and eax, ~(fl_S | fl_Z | fl_A | fl_P)
		mov ecx, flags
		mov [ecx], eax
		pop edx
		pop ecx
		pop eax
	}
	return res;
}

uint16_t ImulWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		push edx
		mov ax, param1

		imul param2

		pushfd
		add ax, dx
		mov res, ax
		pop eax
		and eax, ~(fl_S | fl_Z | fl_A | fl_P)
		mov ecx, flags
		mov [ecx], eax
		pop edx
		pop ecx
		pop eax
	}
	return res;
}

uint8_t ImulByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2

		imul cl

		pushfd
		add al, ah
		mov res, al
		pop eax
		and eax, ~(fl_S | fl_Z | fl_A | fl_P)
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationImul)
{
	DoTestFunctionDWord(&ImulDWord);
	DoTestFunctionWord(&ImulWord);
	DoTestFunctionByte(&ImulByte);
}

uint32_t BtDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		bt eax, cl

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t BtWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		bt ax, cl

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t BtMemDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		and ecx, 0x1f

		bt param1, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t BtMemWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		and ecx, 0x0f

		bt param1, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t BtValDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		and ecx, 0x1f

		bt param1, 0x18

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t BtValWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		and ecx, 0x0f

		bt param1, 8

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationBt)
{
	DoTestFunctionDWord(&BtDWord);
	DoTestFunctionWord(&BtWord);
	DoTestFunctionDWord(&BtMemDWord);
	DoTestFunctionWord(&BtMemWord);
	DoTestFunctionDWord(&BtValDWord);
	DoTestFunctionWord(&BtValWord);
}

uint32_t BtsDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		bts eax, cl

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t BtsWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		bts ax, cl

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t BtsMemDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		and ecx, 0x1f

		bts param1, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t BtsMemWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		and ecx, 0x0f

		bts param1, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationBts)
{
	DoTestFunctionDWord(&BtsDWord);
	DoTestFunctionWord(&BtsWord);
	DoTestFunctionDWord(&BtsMemDWord);
	DoTestFunctionWord(&BtsMemWord);
}

uint32_t BtrDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		btr eax, cl

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t BtrWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		btr ax, cl

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationBtr)
{
	DoTestFunctionDWord(&BtrDWord);
	DoTestFunctionWord(&BtrWord);
}

uint32_t BtcDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		btc eax, cl

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t BtcWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		btc ax, cl

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationBtc)
{
	DoTestFunctionDWord(&BtcDWord);
	DoTestFunctionWord(&BtcWord);
}

uint32_t CmovDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		cmp al, cl

		cmovc eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t CmovWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		cmp al, cl

		cmovna ax, cx

		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationCmov)
{
	DoTestFunctionDWord(&CmovDWord);
	DoTestFunctionWord(&CmovWord);
}

uint32_t LockAddDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax

		lock add res, ecx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint16_t LockAddWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx
		mov ax, param1
		mov cx, param2
		mov res, ax

		lock add res, cx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t LockAddByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		mov res, al

		lock add res, cl

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LockSubDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax

		lock sub res, ecx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LockXorDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax

		lock xor res, ecx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LockAndDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax

		lock and res, ecx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LockOrDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax

		lock or res, ecx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LockXchgDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax
		cmp eax, ecx

		lock xchg res, ecx
		mov res, ecx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LockXaddDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		mov res, eax

		lock xadd res, ecx
		xor res, ecx

		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationLock)
{
	DoTestFunctionDWord(&LockAddDWord);
	DoTestFunctionWord(&LockAddWord);
	DoTestFunctionByte(&LockAddByte);
	DoTestFunctionDWord(&LockAndDWord);
	DoTestFunctionDWord(&LockSubDWord);
	DoTestFunctionDWord(&LockXorDWord);
	DoTestFunctionDWord(&LockOrDWord);
	DoTestFunctionDWord(&LockXchgDWord);
	DoTestFunctionDWord(&LockXaddDWord);
}

uint32_t CbxDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1

		cbw
		movzx ecx, ax
		mov eax, param2
		cwd
		add eax, ecx

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t MovzxDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx

		lea ecx, param1
		movzx eax, byte ptr [ecx + 1]
		movzx ecx, word ptr [ecx + 2]

		add eax, ecx
		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t MovsxDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		movsx ecx, cl
		movsx eax, ax

		add eax, ecx
		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t LahfByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov al, param1
		mov cl, param2
		cmp al, cl

		lahf

		mov res, ah
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t SahfByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov ah, param1

		sahf

		mov res, ah
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t StcByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		xor eax, eax

		stc

		setc res
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax

		pop ecx
		pop eax
	}
	return res;
}

uint8_t ClcByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov eax, -1
		inc eax

		clc

		setc res
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint8_t CmcByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	_asm {
		push eax
		push ecx
		mov eax, -1
		inc eax

		cmc

		setc res
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}


uint8_t XlatByte(uint8_t param1, uint8_t param2, uint32_t *flags)
{
	uint8_t res;
	uint8_t xlat_table[0x100];
	for (size_t i = 0; i < 0x100; i++)
		xlat_table[(uint8_t)i] = 0x100 - (uint8_t)i;
	_asm {
		push eax
		push ecx

		lea ebx, xlat_table
		mov al, param1
		xlat
		add al, param2
		xlat

		mov res, al
		pushfd
		pop eax
		mov ecx, flags
		mov[ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t BswapDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1

		bswap eax

		mov res, eax
		add eax, param2
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LeaDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2

		lea eax, [ecx+eax*4+0x10000]

		mov res, eax
		xor eax, param2
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t ESPDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push eax
		push ecx

		mov eax, param1
		mov [esp+4], eax
		cmp eax, [esp+4]

		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
		pop eax
	}
	return res;
}

uint16_t SegWord(uint16_t param1, uint16_t param2, uint32_t *flags)
{
	uint16_t res;
	_asm {
		push eax
		push ecx

		push ss
		pop eax

		mov cx, ds
		cmp ax, cx
		
		mov res, ax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LodsDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		push edi

		lea esi, param1
		lodsd
		mov res, eax

		mov ecx, flags
		mov [ecx], esi
		pop edi
		pop ecx
		pop eax
	}
	return res;
}

uint32_t RepStosDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		push edi

		mov eax, param1
		mov ecx, 4
		lea edi, res
		rep stosb

		mov eax, ecx
		mov ecx, flags
		mov [ecx], eax
		pop edi
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LdsDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	uint8_t mem[6];

	_asm {
		push eax
		push ecx
		mov eax, param1
		lea ecx, mem
		mov [ecx], eax
		mov ax, ds
		mov [ecx+4], ax

		lds eax, mem

		mov res, ecx
		cmp eax, ecx
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t PopESPDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		push edi

		mov edi, esp
		mov eax, param1
		mov ecx, param2
		push 0
		push 0
		xchg [esp], eax
		pop dword ptr [esp]
		pop eax
		sub edi, esp

		mov res, eax
		mov ecx, flags
		mov [ecx], edi
		pop edi
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationOther)
{
	DoTestFunctionDWord(&CbxDWord);
	DoTestFunctionDWord(&MovzxDWord);
	DoTestFunctionDWord(&MovsxDWord);
	DoTestFunctionByte(&LahfByte);
	DoTestFunctionByte(&SahfByte);
	DoTestFunctionByte(&StcByte);
	DoTestFunctionByte(&ClcByte);
	DoTestFunctionByte(&CmcByte);
	DoTestFunctionDWord(&BswapDWord);
	DoTestFunctionDWord(&LeaDWord);
	DoTestFunctionDWord(&ESPDWord);
	DoTestFunctionWord(&SegWord);
	DoTestFunctionDWord(&LodsDWord);
	DoTestFunctionDWord(&RepStosDWord);
	DoTestFunctionDWord(&LdsDWord);
	DoTestFunctionDWord(&PopESPDWord);
	DoTestFunctionByte(&XlatByte);
}

uint32_t WriteStackDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		push edi

		mov edi, esp
		push ecx
		mov [edi-4], eax
		mov eax, [edi-4]
		mov esp, edi

		mov res, eax
		mov ecx, flags
		mov [ecx], edi
		pop edi
		pop ecx
		pop eax
	}
	return res;
}

uint32_t ReadStackDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		push edi

		mov edi, esp
		push ecx
		mov eax, [edi-4]
		mov esp, edi

		mov res, eax
		mov ecx, flags
		mov [ecx], edi
		pop edi
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationStack)
{
	DoTestFunctionDWord(&ReadStackDWord);
	DoTestFunctionDWord(&WriteStackDWord);
}

uint32_t JxxDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1

		cmp eax, param2
		jnl label_0
		not eax
label_0:
		test eax, 1
		jnz label_1
		add eax, param2
		jc label_2
		or al, 1
		jmp label_0
label_2:
		rol eax, 1

label_1:
		mov res, eax
		pushfd
		pop eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LoopDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		movzx ecx, cl

		jecxz label_1
label_0:
		add eax, eax
		loop label_0

label_1:
		mov res, eax
		mov eax, ecx
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LoopeDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		movzx ecx, cl

		jecxz label_1
label_0:
		add eax, eax
		test ah, 1
		loope label_0

label_1:
		mov res, eax
		mov eax, ecx
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t LoopneDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx
		mov eax, param1
		mov ecx, param2
		movzx ecx, cl

		jecxz label_1
label_0:
		add eax, eax
		test ah, 1
		loopne label_0

label_1:
		mov res, eax
		mov eax, ecx
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

uint32_t RetDWord(uint32_t param1, uint32_t param2, uint32_t *flags)
{
	uint32_t res;
	_asm {
		push eax
		push ecx

		push cs
		mov eax, offset(label_1)
		push eax
		retf

label_1:
		mov eax, param1
		mov res, eax
		mov ecx, flags
		mov [ecx], eax
		pop ecx
		pop eax
	}
	return res;
}

TEST(IntelTest, VirtualizationBranches)
{
	DoTestFunctionDWord(&JxxDWord);
	DoTestFunctionDWord(&LoopDWord);
	DoTestFunctionDWord(&LoopeDWord);
	DoTestFunctionDWord(&LoopneDWord);
	DoTestFunctionDWord(&RetDWord);
	DoTestFunctionDWord(&CmovDWord);
}
#endif //_WIN64
#endif //_WIN32
