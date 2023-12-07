#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/core.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/pefile.h"
#include "../core/il.h"
#include "../core/dotnetfile.h"

#include "testfileil.h"

TEST(ILTest, DisasmSmoke)
{
	uint8_t buf[] = {
			/*
			.method public hidebysig static bool  TakesSingleByteArgument(valuetype System.Reflection.Emit.OpCode inst) cil managed
			 SIG: 00 01 02 11 92 88
			{
			  // Method begins at RVA 0x94b60
			  // Code size       45 (0x2D)
			  .maxstack  2
			  .locals init (valuetype System.Reflection.Emit.OperandType V_0)
			  */
		0x2D * 4 + 2, // Tiny header
			  0x0F,   0x00,                     //  IL_0000: ldarga.s   inst
			  0x7B,   0x04, 0x00, 0x16, 0xF8,   //  IL_0002: ldfld      valuetype System.Reflection.Emit.OperandType System.Reflection.Emit.OpCode::m_operand
			  0x0A,                             //  IL_0007: stloc.0
			  0x06,                             //  IL_0008: ldloc.0
			  0x1F,    0x0F,                    //  IL_0009: ldc.i4.s   15
			  0x59,                             //  IL_000b: sub
			  0x45,    0x04, 0x00, 0x00, 0x00,  //  IL_000c: switch     ( 
			           0x02, 0x00, 0x00, 0x00,  //             IL_0023,
					   0x02, 0x00, 0x00, 0x00,  //             IL_0023,
					   0x04, 0x00, 0x00, 0x00,  //             IL_0025,
					   0x02, 0x00, 0x00, 0x00,  //             IL_0023)
			  0x2B,    0x02,                    //  IL_0021: br.s       IL_0025
			  0x17,                             //  IL_0023: ldc.i4.1
			  0x2A,                             //  IL_0024: ret
			  0x16,                             //  IL_0025: ldc.i4.0
			  0xFE, 0x13,						//  IL_0026: volatile.?
			  0x2A,                             //  IL_0027: ret
			//}  end of method OpCodes::TakesSingleByteArgument
	};

	//ILFunction *func;
	//MapFunctionList *map_function_list;
	//MapFunction *map_function;
	//ILCommand *command;

	/*
	TestFile test_file(osDWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = arch.segment_list();
	uint64_t addr = 0x004B58EC;
	TestSegment *segment = segment_list->Add(addr, 0x10000, ".text", mtReadable | mtExecutable);

	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	arch.runtime_function_list()->AddObject(
		new NETRuntimeFunction(arch.runtime_function_list(), addr, addr + (buf[0] >> 2) + 1, addr + 1, NULL, NULL));
	func = reinterpret_cast<ILFunction *>(arch.function_list()->AddByAddress(addr + 1, ctVirtualization, 0, true, NULL));
	ASSERT_EQ(func->count(), 13ul);
	*/
/*
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
	}*/
}
