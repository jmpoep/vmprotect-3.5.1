#include "precompiled.h"
#include "macfile.h"
#include "arm.h"
#include "testfile.h"

TEST(ArmFunctionTest, Arm_Test)
{
	return;

	MacFile pf(NULL);
	ArmFunction command_list(NULL, NULL);

	ASSERT_TRUE(pf.Open("test-binaries/ios-app-test2-arm", foRead) == osSuccess);
	ASSERT_TRUE(pf.count() == 1);
	IArchitecture &arch = *pf.item(0);
	ASSERT_TRUE(command_list.ReadFromFile(arch, 0x20B6) == 115);
	ASSERT_TRUE(command_list.ReadFromFile(arch, 0x345E) == 41);
	ASSERT_TRUE(command_list.ReadFromFile(arch, 0x1DCD8) == 2);
}

TEST(ArmFunctionTest, ARM)
{

	return;

	uint32_t buf[] = {
		0xe0006fce, // AND R6, R0, LR,ASR#31
		0xe0007064, // AND R7, R0, R4,RRX
		0xe0000000, // AND R0, R0, R0
		0xe0000001, // AND R0, R0, R1
		0xe0000002, // AND R0, R0, R2
		0xe0000003, // AND R0, R0, R3
		0xe0000004, // AND R0, R0, R4
		0xe0000005, // AND R0, R0, R5
		0xe0000006, // AND R0, R0, R6
		0xe0000007, // AND R0, R0, R7
		0xe0000008, // AND R0, R0, R8
		0xe0000009, // AND R0, R0, R9
		0xe000000a, // AND R0, R0, R10
		0xe000000b, // AND R0, R0, R11
		0xe000000c, // AND R0, R0, R12
		0xe000000d, // AND R0, R0, SP
		0xe000000e, // AND R0, R0, LR
		0xe000000f, // AND R0, R0, PC
		0xe0000010, // AND R0, R0, R0,LSL R0
		0xe0000011, // AND R0, R0, R1,LSL R0
		0xe0000012, // AND R0, R0, R2,LSL R0
		0xe0000013, // AND R0, R0, R3,LSL R0
		0xffffffff // INVALID
	};

	//ArmFunction command_list(NULL, NULL);
	TestFile test_file(ptArm, osDWord);
	IArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	TestSegment *segment = segment_list->Add(0x00400000, 0x10000, ".text", mtReadable | mtExecutable);
	segment->set_physical_size(sizeof(buf));
	test_file.OpenFromMemory(buf, sizeof(buf));
	//command_list.ReadFromFile(arch, segment_list->item(0)->address());
}