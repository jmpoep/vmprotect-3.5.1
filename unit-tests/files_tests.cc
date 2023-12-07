#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/intel.h"
#include "../core/pefile.h"

#include "testfileintel.h"

TEST(MapFunctionListTest, ReadFromMapFile_Gcc_Test)
{
	TestFile test_file(osDWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	MapFunctionList *fl = arch.map_function_list();
	segment_list->Add(0x00401000, 0x000083DC, ".text", mtReadable | mtExecutable);
	segment_list->Add(0x0040A000, 0x00004A6C, ".data", mtReadable | mtWritable);
	segment_list->Add(0x0040F000, 0x00002968, ".rdata", mtReadable);
	segment_list->Add(0x00412000, 0x00000068, ".bss", mtReadable | mtWritable);
	segment_list->Add(0x00413000, 0x00001AF8, ".idata", mtReadable | mtWritable);
	segment_list->Add(0x00415000, 0x00000660, "/4", mtDiscardable);
	segment_list->Add(0x00416000, 0x00001517, "/19", mtDiscardable);
	segment_list->Add(0x00418000, 0x0007D3B4, "/35", mtDiscardable);
	segment_list->Add(0x00496000, 0x000055C2, "/47", mtDiscardable);
	segment_list->Add(0x0049C000, 0x0000518E, "/61", mtDiscardable);
	segment_list->Add(0x004A2000, 0x000017CC, "/73", mtDiscardable);
	segment_list->Add(0x004A4000, 0x00003FEB, "/86", mtDiscardable);
	segment_list->Add(0x004A8000, 0x00007534, "/97", mtDiscardable);
	segment_list->Add(0x004B0000, 0x00001C78, "/108", mtDiscardable);

	ASSERT_NO_THROW(arch.ReadTestMapFile("test-binaries/gcc-linux-map-1"));
	ASSERT_EQ(fl->count(), 493ul);
	EXPECT_EQ(fl->item(0)->name().compare("WinMainCRTStartup"), 0);
	EXPECT_EQ(fl->item(100)->name().compare("__chkstk"), 0);
	EXPECT_EQ(fl->item(200)->name().compare("__gnu_cxx::__concurrence_lock_error::~__concurrence_lock_error()"), 0);
}

TEST(MapFunctionListTest, ReadFromMapFile_Gcc_Test2)
{
	TestFile test_file(osQWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	MapFunctionList *fl = arch.map_function_list();
	segment_list->Add(0x0000000008048134,       0x13, ".interp", mtReadable);
	segment_list->Add(0x0000000008048148,       0x20, ".note.ABI-tag", mtReadable);
	segment_list->Add(0x0000000008048168,       0x20, ".gnu.hash", mtReadable);
	segment_list->Add(0x0000000008048188,       0x50, ".dynsym", mtReadable);
	segment_list->Add(0x00000000080481d8,       0x4a, ".dynstr", mtReadable);
	segment_list->Add(0x0000000008048222,        0xa, ".gnu.version", mtReadable);
	segment_list->Add(0x000000000804822c,       0x20, ".gnu.version_r", mtReadable);
	segment_list->Add(0x000000000804824c,        0x8, ".rel.dyn", mtReadable);
	segment_list->Add(0x0000000008048254,       0x18, ".rel.plt", mtReadable);
	segment_list->Add(0x000000000804826c,       0x17, ".init", mtReadable);
	segment_list->Add(0x0000000008048284,       0x40, ".plt", mtReadable);
	segment_list->Add(0x00000000080482d0,      0x1a8, ".text", mtReadable | mtExecutable);
	segment_list->Add(0x0000000008048478,       0x1c, ".fini", mtReadable);
	segment_list->Add(0x0000000008048494,       0x12, ".rodata", mtReadable);
	segment_list->Add(0x00000000080484a8,       0x1c, ".eh_frame_hdr", mtReadable);
	segment_list->Add(0x00000000080484c4,       0x58, ".eh_frame", mtReadable);
	segment_list->Add(0x000000000804951c,        0x8, ".ctors", mtReadable);
	segment_list->Add(0x0000000008049524,        0x8, ".dtors", mtReadable);
	segment_list->Add(0x000000000804952c,        0x4, ".jcr", mtReadable);
	segment_list->Add(0x0000000008049530,       0xc8, ".dynamic", mtReadable);
	segment_list->Add(0x00000000080495f8,        0x4, ".got", mtReadable);
	segment_list->Add(0x00000000080495fc,       0x18, ".got.plt", mtReadable);
	segment_list->Add(0x0000000008049614,        0x4, ".data", mtReadable);
	segment_list->Add(0x0000000008049618,        0x8, ".bss", mtReadable);

	ASSERT_NO_THROW(arch.ReadTestMapFile("test-binaries/gcc-linux-map-2"));
	ASSERT_EQ(fl->count(), 17ul);
	EXPECT_EQ(fl->item(0)->name().compare("_init"), 0);
	EXPECT_EQ(fl->item(10)->name().compare("_IO_stdin_used"), 0);
	EXPECT_EQ(fl->item(16)->name().compare("__data_start"), 0);
}

TEST(MapFunctionListTest, ReadFromMapFile_GccApple_Test)
{
	TestFile test_file(osDWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	MapFunctionList *fl = arch.map_function_list();
	segment_list->Add(0x00000000, 0x00001000, "__PAGEZERO", mtNone);
	segment_list->Add(0x00001000, 0x00001000, "__TEXT", mtReadable | mtExecutable);
	segment_list->Add(0x00002000, 0x00001000, "__DATA", mtReadable | mtWritable);

	ASSERT_NO_THROW(arch.ReadTestMapFile("test-binaries/gcc-apple-map"));
	ASSERT_EQ(fl->count(), 53ul);
	EXPECT_EQ(fl->item(0)->name().compare("start"), 0);
	EXPECT_EQ(fl->item(20)->name().compare("_exit"), 0);
	EXPECT_EQ(fl->item(40)->name().compare("non-lazy-pointer-to: ___gxx_personality_v0"), 0);
}

TEST(MapFunctionListTest, ReadFromMapFile_Deplhi_Test)
{
	TestFile test_file(osDWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	MapFunctionList *fl = arch.map_function_list();
	segment_list->Add(0x00401000, 0x000B9510, ".text", mtReadable | mtExecutable);
	segment_list->Add(0x004BB000, 0x00000C90, ".itext", mtReadable | mtExecutable);
	segment_list->Add(0x004BC000, 0x000025B0, ".data", mtReadable | mtWritable);
	segment_list->Add(0x004BF000, 0x000052E8, ".bss", mtReadable | mtWritable);
	segment_list->Add(0x004C5000, 0x00003284, ".idata", mtReadable | mtWritable);
	segment_list->Add(0x004C9000, 0x00000326, ".didata", mtReadable | mtWritable);
	segment_list->Add(0x004CA000, 0x0000003C, ".tls", mtReadable | mtWritable);
	segment_list->Add(0x004CB000, 0x00000018, ".rdata", mtReadable);
	segment_list->Add(0x004CC000, 0x00010AB0, ".reloc", mtReadable | mtDiscardable);
	segment_list->Add(0x004DD000, 0x0000D000, ".rsrc", mtReadable);

	ASSERT_NO_THROW(arch.ReadTestMapFile("test-binaries/delphi-map"));
	ASSERT_EQ(fl->count(), 5702ul);
	EXPECT_EQ(fl->item(0)->name().compare("System..TObject"), 0);
	EXPECT_EQ(fl->item(1000)->name().compare("SysUtils.DecodeDate"), 0);
	EXPECT_EQ(fl->item(2000)->name().compare("Classes.TWriter.DefineBinaryProperty"), 0);
}

TEST(MapFunctionListTest, ReadFromMapFile_Msvc_Test)
{
	TestFile test_file(osDWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	MapFunctionList *fl = arch.map_function_list();
	segment_list->Add(0x00401000, 0x00023FC9, ".textbss", mtReadable | mtWritable | mtExecutable);
	segment_list->Add(0x00425000, 0x0004BD1F, ".text", mtReadable | mtExecutable);
	segment_list->Add(0x00471000, 0x0000EF7A, ".rdata", mtReadable);
	segment_list->Add(0x00480000, 0x00003898, ".data", mtReadable | mtExecutable);
	segment_list->Add(0x00484000, 0x00000D35, ".idata", mtReadable | mtExecutable);
	segment_list->Add(0x00485000, 0x000005C0, ".rsrc", mtReadable);
	segment_list->Add(0x00486000, 0x000035D3, ".reloc", mtReadable);

	ASSERT_NO_THROW(arch.ReadTestMapFile("test-binaries/msvc-map"));
	ASSERT_EQ(fl->count(), 2055ul);
	EXPECT_EQ(fl->item(0)->name().compare("__enc$textbss$begin"), 0);
	EXPECT_EQ(fl->item(1000)->name().compare("`string'"), 0);
	EXPECT_EQ(fl->item(2000)->name().compare("__imp__TlsGetValue@4"), 0);
	// Check static symbol
	ASSERT_TRUE(fl->GetFunctionByName("char * UnDecorator::outputString") != NULL);
}

TEST(MapFunctionListTest, ReadFromMapFile_Msvc_Test2)
{
	TestFile test_file(osQWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	MapFunctionList *fl = arch.map_function_list();
	segment_list->Add(0x00401000, 0x00042578, ".text", mtReadable | mtExecutable);
	segment_list->Add(0x00444000, 0x0000C4F6, ".rdata", mtReadable);
	segment_list->Add(0x00451000, 0x00000B08, ".data", mtReadable | mtWritable);
	segment_list->Add(0x00452000, 0x00001788, ".pdata", mtReadable);
	segment_list->Add(0x00454000, 0x00002003, ".idata", mtReadable | mtWritable);
	segment_list->Add(0x00457000, 0x00000C09, ".rsrc", mtReadable);

	ASSERT_NO_THROW(arch.ReadTestMapFile("test-binaries/msvc-x64-map-2"));
	ASSERT_EQ(fl->count(), 732ul);
	EXPECT_EQ(fl->item(0)->name().compare("test_header(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)"), 0);
	EXPECT_EQ(fl->item(300)->name().compare("isxdigit"), 0);
	EXPECT_EQ(fl->item(600)->name().compare("__imp_UnhandledExceptionFilter"), 0);
}

TEST(MapFunctionListTest, Map_Test)
{
	TestFile test_file(osQWord);
	TestArchitecture &arch = *test_file.item(0);
	TestSegmentList *segment_list = reinterpret_cast<TestSegmentList *>(arch.segment_list());
	MapFunctionList *fl = arch.map_function_list();
	MapFunction *mf = NULL;
	segment_list->Add(0x00401000, 0x00042578, ".text", mtReadable | mtExecutable);
	segment_list->Add(0x00444000, 0x0000C4F6, ".rdata", mtReadable);
	segment_list->Add(0x00451000, 0x00000B08, ".data", mtReadable | mtWritable);
	segment_list->Add(0x00452000, 0x00001788, ".pdata", mtReadable);
	segment_list->Add(0x00454000, 0x00002003, ".idata", mtReadable | mtWritable);
	segment_list->Add(0x00457000, 0x00000C09, ".rsrc", mtReadable);

	ASSERT_NO_THROW(arch.ReadTestMapFile("test-binaries/msvc-x64-map-2"));
	ASSERT_NO_THROW(mf = fl->GetFunctionByName("main"));
	ASSERT_TRUE(mf != NULL);
	EXPECT_EQ(mf->name().compare("main"), 0);
	EXPECT_EQ(mf->address(), 0x402ef0ull);
	ASSERT_NO_THROW(mf = fl->GetFunctionByAddress(0x40b460));
	ASSERT_TRUE(mf != NULL);
	EXPECT_EQ(mf->name().compare("pcre_exec"), 0);
	EXPECT_EQ(mf->address(), 0x40b460ull);
}

TEST(MemoryManager, Alloc)
{
	MemoryManager manager(NULL);

	manager.Add(0x1000, 0x1000, mtReadable, NULL);
	manager.Add(0x2000, 0x300, mtReadable | mtExecutable, NULL);
	manager.Add(0x3000, 0x100, mtReadable | mtWritable, NULL);
	EXPECT_EQ(manager.Alloc(0x10, mtReadable | mtExecutable), 0x2000ull);
	EXPECT_EQ(manager.Alloc(0x10, mtReadable | mtExecutable), 0x2010ull);
	EXPECT_EQ(manager.Alloc(0x10, mtReadable | mtExecutable, 0, 0x200), 0x2200ull);
	EXPECT_EQ(manager.Alloc(0x20, mtReadable | mtWritable), 0x3000ull);
	EXPECT_EQ(manager.Alloc(0x20, mtReadable | mtWritable), 0x3020ull);
	EXPECT_EQ(manager.Alloc(0x200, mtNone, 0x1800), 0x1800ull);
	EXPECT_EQ(manager.Alloc(0x20, mtReadable | mtExecutable, 0x1900), 0ull);
	EXPECT_EQ(manager.count(), 5ul);
}

TEST(MemoryManager, Remove)
{
	MemoryManager manager(NULL);

	manager.Add(7, 3, mtReadable, NULL);
	manager.Add(1, 5, mtReadable, NULL);
	ASSERT_NO_THROW(manager.Remove(4, 4));
	ASSERT_EQ(manager.count(), 2ul);
	EXPECT_EQ(manager.item(0)->address(), 1ull);
	EXPECT_EQ(manager.item(0)->end_address(), 4ull);
	EXPECT_EQ(manager.item(1)->address(), 8ull);
	EXPECT_EQ(manager.item(1)->end_address(), 10ull);

	ASSERT_NO_THROW(manager.Remove(8, 2));
	ASSERT_EQ(manager.count(), 1ul);
}

TEST(MemoryManager, Pack)
{
	MemoryManager manager(NULL);

	manager.Add(0x1000, 0x1000, mtReadable | mtExecutable, NULL);
	manager.Add(0x2000, 0x100, mtReadable | mtExecutable, NULL);
	manager.Add(0x2100, 0x100, mtReadable, NULL);
	manager.Pack();
	ASSERT_EQ(manager.count(), 2ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 0x1000ull);
	EXPECT_EQ(region->end_address(), 0x2100ull);
	EXPECT_EQ((int)region->type(), (mtReadable | mtExecutable));
	region = manager.item(1);
	EXPECT_EQ(region->address(), 0x2100ull);
	EXPECT_EQ(region->end_address(), 0x2200ull);
	EXPECT_EQ((int)region->type(), mtReadable);
}

TEST(MemoryManager, SimpleRemove)
{
	MemoryManager manager(NULL);

	manager.Add(1, 4, mtReadable, NULL);
	manager.Add(7, 3, mtReadable, NULL);
	manager.Remove(4, 4);

	ASSERT_EQ(manager.count(), 2ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 4ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 8ull);
	EXPECT_EQ(region->end_address(), 10ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(30, 30);

	ASSERT_EQ(manager.count(), 2ul);
	region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 4ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 8ull);
	EXPECT_EQ(region->end_address(), 10ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(9, 30);

	ASSERT_EQ(manager.count(), 2ul);
	region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 4ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 8ull);
	EXPECT_EQ(region->end_address(), 9ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(3, 1);

	ASSERT_EQ(manager.count(), 2ul);
	region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 3ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 8ull);
	EXPECT_EQ(region->end_address(), 9ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(0, 15);

	ASSERT_EQ(manager.count(), 0ul);
}

TEST(MemoryManager, ExtendedRemove)
{
	MemoryManager manager(NULL);

	manager.Add(1, 4, mtReadable, NULL);
	manager.Add(7, 3, mtReadable, NULL);
	manager.Add(15, 5, mtReadable, NULL);
	manager.Remove(2, 1);

	ASSERT_EQ(manager.count(), 4ul);
	MemoryRegion *region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 2ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 3ull);
	EXPECT_EQ(region->end_address(), 5ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(2);
	EXPECT_EQ(region->address(), 7ull);
	EXPECT_EQ(region->end_address(), 10ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(3);
	EXPECT_EQ(region->address(), 15ull);
	EXPECT_EQ(region->end_address(), 20ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(8, 1);
	ASSERT_EQ(manager.count(), 5ul);
	region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 2ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 3ull);
	EXPECT_EQ(region->end_address(), 5ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(2);
	EXPECT_EQ(region->address(), 7ull);
	EXPECT_EQ(region->end_address(), 8ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(3);
	EXPECT_EQ(region->address(), 9ull);
	EXPECT_EQ(region->end_address(), 10ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(4);
	EXPECT_EQ(region->address(), 15ull);
	EXPECT_EQ(region->end_address(), 20ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(16, 1);
	ASSERT_EQ(manager.count(), 6ul);
	region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 2ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 3ull);
	EXPECT_EQ(region->end_address(), 5ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(2);
	EXPECT_EQ(region->address(), 7ull);
	EXPECT_EQ(region->end_address(), 8ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(3);
	EXPECT_EQ(region->address(), 9ull);
	EXPECT_EQ(region->end_address(), 10ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(4);
	EXPECT_EQ(region->address(), 15ull);
	EXPECT_EQ(region->end_address(), 16ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(5);
	EXPECT_EQ(region->address(), 17ull);
	EXPECT_EQ(region->end_address(), 20ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(6, 5);
	ASSERT_EQ(manager.count(), 4ul);
	region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 2ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 3ull);
	EXPECT_EQ(region->end_address(), 5ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(2);
	EXPECT_EQ(region->address(), 15ull);
	EXPECT_EQ(region->end_address(), 16ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(3);
	EXPECT_EQ(region->address(), 17ull);
	EXPECT_EQ(region->end_address(), 20ull);
	EXPECT_EQ((int)region->type(), mtReadable);

	manager.Remove(15, 100);
	ASSERT_EQ(manager.count(), 2ul);
	region = manager.item(0);
	EXPECT_EQ(region->address(), 1ull);
	EXPECT_EQ(region->end_address(), 2ull);
	EXPECT_EQ((int)region->type(), mtReadable);
	region = manager.item(1);
	EXPECT_EQ(region->address(), 3ull);
	EXPECT_EQ(region->end_address(), 5ull);
	EXPECT_EQ((int)region->type(), mtReadable);
}