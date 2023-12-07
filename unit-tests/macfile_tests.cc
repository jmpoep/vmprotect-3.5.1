#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/core.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/macfile.h"
#include "../core/mac_runtime32.dylib.inc"
#include "../core/mac_runtime64.dylib.inc"

TEST(MacFileTest, TestOpen1)
{
	MacFile mf(NULL);

	ASSERT_EQ(mf.Open("test-binaries/ios-app-test2-arm", true), osUnsupportedCPU);
}

TEST(MacFileTest, TestOpen2)
{
	// Test: read header of iOS/i386 file. Read should finish successfully.
	MacFile mf(NULL);
	ISectionList *segl;
	MacSectionList *secl;
	MacSymbolList *syml;
	IImportList *impl;

	ASSERT_EQ(mf.Open("test-binaries/ios-app-test1-i386", true), osSuccess);
	// Check architectures.
	EXPECT_EQ(mf.count(), 1ul);
	MacArchitecture &arch = *mf.item(0);
	EXPECT_EQ(arch.name().compare("i386"), 0);
	EXPECT_EQ(arch.owner()->format_name().compare("Mach-O"), 0);
	EXPECT_EQ(arch.entry_point(), 0x2130ull);
	// Check segments were read.
	segl = arch.segment_list();
	ASSERT_EQ(segl->count(), 4ul);
	EXPECT_EQ(segl->item(0)->name().compare("__PAGEZERO"), 0);
	EXPECT_EQ(segl->item(0)->memory_type(), mtNone);
	EXPECT_EQ(segl->item(1)->name().compare("__TEXT"), 0);
	EXPECT_EQ((int)segl->item(1)->memory_type(), (mtReadable | mtExecutable));
	EXPECT_EQ(segl->item(2)->name().compare("__DATA"), 0);
	EXPECT_EQ((int)segl->item(2)->memory_type(), (mtReadable | mtWritable));
	EXPECT_EQ(segl->item(3)->name().compare("__LINKEDIT"), 0);
	EXPECT_EQ(segl->item(3)->memory_type(), mtReadable);
	// Check sections were read.
	secl = arch.section_list();
	ASSERT_EQ(secl->count(), 18ul);
	EXPECT_EQ(secl->item(0)->name().compare("__text"), 0);
	EXPECT_EQ(secl->item(1)->name().compare("__symbol_stub"), 0);
	EXPECT_EQ(secl->item(2)->name().compare("__stub_helper"), 0);
	EXPECT_EQ(secl->item(3)->name().compare("__cstring"), 0);
	EXPECT_EQ(secl->item(4)->name().compare("__unwind_info"), 0);
	EXPECT_EQ(secl->item(5)->name().compare("__eh_frame"), 0);
	EXPECT_EQ(secl->item(6)->name().compare("__program_vars"), 0);
	EXPECT_EQ(secl->item(7)->name().compare("__nl_symbol_ptr"), 0);
	EXPECT_EQ(secl->item(8)->name().compare("__la_symbol_ptr"), 0);
	EXPECT_EQ(secl->item(9)->name().compare("__objc_classlist"), 0);
	EXPECT_EQ(secl->item(10)->name().compare("__objc_protolist"), 0);
	EXPECT_EQ(secl->item(11)->name().compare("__objc_imageinfo"), 0);
	EXPECT_EQ(secl->item(12)->name().compare("__objc_const"), 0);
	EXPECT_EQ(secl->item(13)->name().compare("__objc_selrefs"), 0);
	EXPECT_EQ(secl->item(14)->name().compare("__objc_classrefs"), 0);
	EXPECT_EQ(secl->item(15)->name().compare("__objc_superrefs"), 0);
	EXPECT_EQ(secl->item(16)->name().compare("__objc_data"), 0);
	EXPECT_EQ(secl->item(17)->name().compare("__data"), 0);
	// Check symbols.
	syml = arch.symbol_list();
	ASSERT_EQ(syml->count(), 129ul);
	EXPECT_EQ(syml->item(0)->name()
		.compare("/Users/macuser/work2/ios-test2/ios-test2/"), 0);
	// Check imports.
	impl = arch.import_list();
	ASSERT_EQ(impl->count(), 6ul);
	EXPECT_EQ(impl->item(0)->name().compare("/System/Library/Frameworks/UIKit.framework/UIKit"), 0);
	EXPECT_EQ(impl->item(5)->name().compare("/usr/lib/libobjc.A.dylib"), 0);
	EXPECT_EQ(impl->item(5)->count(), 11ul);
	EXPECT_EQ(impl->item(5)->item(0)->name().compare("__objc_empty_cache"), 0);
	EXPECT_EQ(impl->item(5)->item(1)->name().compare("__objc_empty_cache"), 0);
	EXPECT_EQ(impl->item(5)->item(2)->name().compare("__objc_empty_cache"), 0);
	EXPECT_EQ(impl->item(5)->item(3)->name().compare("__objc_empty_cache"), 0);
	EXPECT_EQ(impl->item(5)->item(4)->name().compare("__objc_empty_vtable"), 0);
	EXPECT_EQ(impl->item(5)->item(5)->name().compare("__objc_empty_vtable"), 0);
	EXPECT_EQ(impl->item(5)->item(6)->name().compare("__objc_empty_vtable"), 0);
	EXPECT_EQ(impl->item(5)->item(7)->name().compare("__objc_empty_vtable"), 0);
	EXPECT_EQ(impl->item(5)->item(8)->name().compare("_objc_msgSend"), 0);
	EXPECT_EQ(impl->item(5)->item(9)->name().compare("_objc_msgSendSuper2"), 0);
	EXPECT_EQ(impl->item(5)->item(10)->name().compare("_objc_setProperty"), 0);
	mf.Close();
	EXPECT_EQ(mf.count(), 0ul);
}

TEST(MacFileTest, TestOpen3)
{
	// Test: read header of iOS/x86-64 file. Read should finish successfully.
	MacFile mf(NULL);
	ISectionList *segl;
	MacSectionList *secl;
	MacSymbolList *syml;
	IImportList *impl;
	IExportList *expl;

	ASSERT_EQ(mf.Open("test-binaries/macos-app-test1-amd64", true), osSuccess);
	// Check architectures.
	ASSERT_EQ(mf.count(), 1ul);
	MacArchitecture &arch = *mf.item(0);
	EXPECT_EQ(arch.name().compare("x86_64"), 0);
	EXPECT_EQ(arch.owner()->format_name().compare("Mach-O"), 0);
	EXPECT_EQ(arch.entry_point(), 0x0000000100000c04ULL);
	// Check segments were read.
	segl = arch.segment_list();
	ASSERT_EQ(segl->count(), 4ul);
	EXPECT_EQ(segl->item(0)->name().compare("__PAGEZERO"), 0);
	EXPECT_EQ(segl->item(0)->memory_type(), mtNone);
	EXPECT_EQ(segl->item(1)->name().compare("__TEXT"), 0);
	EXPECT_EQ((int)segl->item(1)->memory_type(), (mtReadable | mtExecutable));
	EXPECT_EQ(segl->item(2)->name().compare("__DATA"), 0);
	EXPECT_EQ((int)segl->item(2)->memory_type(), (mtReadable | mtWritable));
	EXPECT_EQ(segl->item(3)->name().compare("__LINKEDIT"), 0);
	EXPECT_EQ(segl->item(3)->memory_type(), mtReadable);
	// Check sections were read.
	secl = arch.section_list();
	ASSERT_EQ(secl->count(), 10ul);
	EXPECT_EQ(secl->item(0)->name().compare("__text"), 0);
	EXPECT_EQ(secl->item(1)->name().compare("__stubs"), 0);
	EXPECT_EQ(secl->item(2)->name().compare("__stub_helper"), 0);
	EXPECT_EQ(secl->item(3)->name().compare("__cstring"), 0);
	EXPECT_EQ(secl->item(4)->name().compare("__unwind_info"), 0);
	EXPECT_EQ(secl->item(5)->name().compare("__eh_frame"), 0);
	EXPECT_EQ(secl->item(6)->name().compare("__dyld"), 0);
	EXPECT_EQ(secl->item(7)->name().compare("__got"), 0);
	EXPECT_EQ(secl->item(8)->name().compare("__la_symbol_ptr"), 0);
	EXPECT_EQ(secl->item(9)->name().compare("__data"), 0);
	// Check symbols.
	syml = arch.symbol_list();
	ASSERT_EQ(syml->count(), 57ul);
	EXPECT_EQ(syml->item(2)->name()
		.compare("/Users/mac/Library/Developer/Xcode/DerivedData/hello-flxnqzrlabvwqrdcesuuqdaabhfa/Build/Intermediates/hello.build/Release/hello.build/Objects-normal/x86_64/main.o"), 0);
	// Check imports.
	impl = arch.import_list();
	ASSERT_EQ(impl->count(), 3ul);
	EXPECT_EQ(impl->item(0)->name().compare("/usr/lib/libstdc++.6.dylib"), 0);
	EXPECT_EQ(impl->item(1)->name().compare("/usr/lib/libgcc_s.1.dylib"), 0);
	EXPECT_EQ(impl->item(2)->name().compare("/usr/lib/libSystem.B.dylib"), 0);
	// Check exports.
	expl = arch.export_list();
	ASSERT_EQ(expl->count(), 5ul);
	EXPECT_EQ(expl->item(0)->name().compare("_NXArgc"), 0);
	EXPECT_EQ(expl->item(1)->name().compare("_NXArgv"), 0);
	EXPECT_EQ(expl->item(2)->name().compare("___progname"), 0);
	EXPECT_EQ(expl->item(3)->name().compare("_environ"), 0);
	EXPECT_EQ(expl->item(4)->name().compare("start"), 0);
	EXPECT_EQ(expl->item(4)->address(), 0x100000c04ULL);
	mf.Close();
	EXPECT_EQ(mf.count(), 0ul);
}

TEST(MacFileTest, OpenMacDLL)
{
	MacFile mf(NULL);

	EXPECT_EQ(mf.Open("test-binaries/macos-dll-test1-i386", foRead), osSuccess);
	// Check architectures.
	ASSERT_EQ(mf.count(), 1ul);
	MacArchitecture &arch = *mf.item(0);
	// Check exports.
	IExportList *exp = arch.export_list();
	EXPECT_EQ(exp->name().compare("@executable_path/hello_lib_cpp.dylib"), 0);
	ASSERT_EQ(exp->count(), 1ul);
	EXPECT_EQ(exp->item(0)->name().compare("__ZN13hello_lib_cpp10HelloWorldEPKc"), 0);
	EXPECT_EQ(exp->item(0)->forwarded_name().compare(""), 0);
	EXPECT_EQ(exp->item(0)->address(), 0x00000deaull);
	// Check fixups.
	IFixupList *fixup_list = arch.fixup_list();
	ASSERT_EQ(fixup_list->count(), 23ul);
	EXPECT_EQ(fixup_list->item(0)->address(), 0x00000e84ull);
	EXPECT_EQ(fixup_list->item(0)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(22)->address(), 0x00001034ull);
	EXPECT_EQ(fixup_list->item(22)->type(), ftHighLow);
	mf.Close();
	EXPECT_EQ(mf.count(), 0ul);
}

TEST(MacFileTest, Clone)
{
	MacFile mf(NULL);

	ASSERT_EQ(mf.Open("test-binaries/ios-app-test1-i386", foRead), osSuccess);
	MacFile *f = mf.Clone(mf.file_name().c_str());
	ASSERT_EQ(mf.count(), f->count());
	MacArchitecture &src = *mf.item(0);
	MacArchitecture &dst = *f->item(0);
	EXPECT_EQ(src.owner()->format_name().compare(dst.owner()->format_name()), 0);
	EXPECT_EQ(src.name().compare(dst.name()), 0);
	EXPECT_EQ(src.cpu_address_size(), dst.cpu_address_size());
	EXPECT_EQ(src.entry_point(), dst.entry_point());
	EXPECT_EQ(src.command_list()->count(), dst.command_list()->count());
	EXPECT_EQ(src.command_list()->count(), dst.command_list()->count());
	EXPECT_EQ(src.segment_list()->count(), dst.segment_list()->count());
	EXPECT_EQ(src.section_list()->count(), dst.section_list()->count());
	EXPECT_EQ(src.symbol_list()->count(), dst.symbol_list()->count());
	EXPECT_EQ(src.import_list()->count(), dst.import_list()->count());
	EXPECT_EQ(src.fixup_list()->count(), dst.fixup_list()->count());
	EXPECT_EQ(src.export_list()->count(), dst.export_list()->count());
	std::string symbol_name = src.indirect_symbol_list()->item(1)->symbol()->name();
	std::string segment_name = src.section_list()->item(0)->parent()->name();
	mf.Close();
	EXPECT_EQ(dst.indirect_symbol_list()->item(1)->symbol()->name().compare(symbol_name), 0);
	EXPECT_EQ(dst.section_list()->item(0)->parent()->name().compare(segment_name), 0);
	delete f;
}

TEST(MacFileTest, Runtime_x32)
{
	MacFile file(NULL);

	ASSERT_TRUE(file.OpenResource(mac_runtime32_dylib_file, sizeof(mac_runtime32_dylib_file), true));
	ASSERT_EQ(file.count(), 1ul);
	MacArchitecture *arch = file.item(0);
	Buffer buffer(&mac_runtime32_dylib_code[0]);
	arch->ReadFromBuffer(buffer);
	EXPECT_EQ(arch->export_list()->count(), 21ul);
	EXPECT_GT(arch->function_list()->count(), 0ul);
}

TEST(MacFileTest, Runtime_x64)
{
	MacFile file(NULL);

	ASSERT_TRUE(file.OpenResource(mac_runtime64_dylib_file, sizeof(mac_runtime64_dylib_file), true));
	ASSERT_EQ(file.count(), 1ul);
	MacArchitecture *arch = file.item(0);
	Buffer buffer(&mac_runtime64_dylib_code[0]);
	arch->ReadFromBuffer(buffer);
	EXPECT_EQ(arch->export_list()->count(), 21ul);
	EXPECT_GT(arch->function_list()->count(), 0ul);
}

TEST(MacFileTest, Compile_x32)
{
	MacFile mf(NULL);
	size_t i;
	MacSegment *segment;

	ASSERT_EQ(mf.Open("test-binaries/exc-osx-x86", foRead), osSuccess);
	MacArchitecture *arch = mf.item(0);
	std::string file_name = mf.file_name() + "_vmp";
	MacFile *f = mf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	MacArchitecture *dst = f->item(0);
	MacSegment *text_segment = dst->segment_list()->GetSectionByName(SEG_TEXT);
	ASSERT_TRUE(text_segment != NULL);
	if (dst->entry_point()) {
		segment = dst->segment_list()->GetSectionByAddress(dst->entry_point());
		ASSERT_EQ(segment, text_segment);
	}
	size_t pointer_size = dst->cpu_address_size() == osDWord ? 4 : 8;
	for (i = 0; i < dst->section_list()->count(); i++) {
		MacSection *section = dst->section_list()->item(i);
		if (section->type() == S_MOD_INIT_FUNC_POINTERS) {
			ASSERT_TRUE((section->address() % pointer_size) == 0);
			ASSERT_TRUE((section->size() % pointer_size) == 0);
			ASSERT_TRUE(dst->AddressSeek(section->address()));
			for (uint64_t j = 0; j < section->size(); j += pointer_size) {
				uint64_t address = 0;
				dst->Read(&address, pointer_size);
				segment = dst->segment_list()->GetSectionByAddress(address);
				ASSERT_EQ(segment, text_segment);
			}
		}
	}
	for (i = 0; i < dst->fixup_list()->count(); i++) {
		MacFixup *fixup = dst->fixup_list()->item(i);
		if (fixup->is_relocation())
			continue;
		MacSegment *segment = dst->segment_list()->GetSectionByAddress(fixup->address());
		ASSERT_TRUE(segment != NULL);
		switch (fixup->bind_type()) {
		case REBASE_TYPE_POINTER:
			ASSERT_EQ(segment->memory_type() & (mtExecutable | mtWritable), mtWritable);
			break;
		case REBASE_TYPE_TEXT_ABSOLUTE32:
		case REBASE_TYPE_TEXT_PCREL32:
			ASSERT_EQ(segment->memory_type() & (mtExecutable | mtWritable), mtExecutable);
			break;
		}
	}
	// check AV buffer
	ASSERT_EQ(dst->segment_list()->count(), 7ul);
	ASSERT_TRUE(dst->AddressSeek(dst->segment_list()->item(4)->address()));
	uint32_t sum = 0;
	for (i = 0; i < 64; i++) {
		sum += dst->ReadDWord();
	}
	EXPECT_EQ(sum, 0xB7896EB5);
	// delete file from disk
	delete f;
	remove(file_name.c_str());
}

TEST(MacFileTest, Compile_x64)
{
	MacFile mf(NULL);
	size_t i;
	MacSegment *segment;

	ASSERT_EQ(mf.Open("test-binaries/exc-osx-x64", foRead), osSuccess);
	MacArchitecture *arch = mf.item(0);
	std::string file_name = mf.file_name() + "_vmp";
	MacFile *f = mf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	MacArchitecture *dst = f->item(0);
	MacSegment *text_segment = dst->segment_list()->GetSectionByName(SEG_TEXT);
	ASSERT_TRUE(text_segment != NULL);
	if (dst->entry_point()) {
		segment = dst->segment_list()->GetSectionByAddress(dst->entry_point());
		ASSERT_EQ(segment, text_segment);
	}
	size_t pointer_size = dst->cpu_address_size() == osDWord ? 4 : 8;
	for (i = 0; i < dst->section_list()->count(); i++) {
		MacSection *section = dst->section_list()->item(i);
		if (section->type() == S_MOD_INIT_FUNC_POINTERS) {
			ASSERT_TRUE((section->address() % pointer_size) == 0);
			ASSERT_TRUE((section->size() % pointer_size) == 0);
			ASSERT_TRUE(dst->AddressSeek(section->address()));
			for (uint64_t j = 0; j < section->size(); j += pointer_size) {
				uint64_t address = 0;
				dst->Read(&address, pointer_size);
				segment = dst->segment_list()->GetSectionByAddress(address);
				ASSERT_EQ(segment, text_segment);
			}
		}
	}
	for (i = 0; i < dst->fixup_list()->count(); i++) {
		MacFixup *fixup = dst->fixup_list()->item(i);
		if (fixup->is_relocation())
			continue;
		MacSegment *segment = dst->segment_list()->GetSectionByAddress(fixup->address());
		ASSERT_TRUE(segment != NULL);
		switch (fixup->bind_type()) {
		case REBASE_TYPE_POINTER:
			ASSERT_EQ(segment->memory_type() & (mtExecutable | mtWritable), mtWritable);
			break;
		case REBASE_TYPE_TEXT_ABSOLUTE32:
		case REBASE_TYPE_TEXT_PCREL32:
			ASSERT_EQ(segment->memory_type() & (mtExecutable | mtWritable), mtExecutable);
			break;
		}
	}
	// check AV buffer
	ASSERT_EQ(dst->segment_list()->count(), 7ul);
	ASSERT_TRUE(dst->AddressSeek(dst->segment_list()->item(4)->address()));
	uint32_t sum = 0;
	for (i = 0; i < 64; i++) {
		sum += dst->ReadDWord();
	}
	EXPECT_EQ(sum, 0xB7896EB5);
	// delete file from disk
	delete f;
	remove(file_name.c_str());
}

#ifdef __APPLE__
#ifndef DEMO
static bool execFile(const std::string &fileName, DWORD &exitCode)
{
	exitCode = 0xFFFFFFF;
	int ret = system(fileName.c_str());
	if (ret != -1)
	{
		exitCode = DWORD(ret);
		return true;
	}
	return false;
}

/* 
// exc.cpp:
//  clang++ -std=c++11 -arch x86_64 -o exc-osx-x64 exc.cpp ../bin/libVMProtectSDK.dylib -I ../sdk
//  clang++ -std=c++11 -arch i386 -o exc-osx-x86 exc.cpp ../bin/libVMProtectSDK.dylib -I ../sdk 
#include <stdio.h>
#include "VMProtectSDK.h"

__declspec(noinline) void try1()
{
	printf("try 1\n");
	try {
		throw 1;
	}
	catch (int) {
		printf("catch 1\n");
		throw;
	}
	printf("end 1\n\n");
}

int main()
{
	if (VMProtectIsDebuggerPresent(true))
		printf("debugger detected\n");
	if (VMProtectIsVirtualMachinePresent())
		printf("virtual machine detected\n");

	printf("try main\n");
	try {
		try1();
	}
	catch (...) {
		printf("catch main\n");
	}
	printf("end main\n");

    return 0;
}
*/

TEST(MacFileTest, EXC_x32)
{
	MacFile pf(NULL);
	ASSERT_EQ(pf.Open("test-binaries/exc-osx-x86", foRead), osSuccess);
	MacArchitecture *arch = pf.item(0);
	arch->function_list()->AddByAddress(0x01C60, ctVirtualization, 0, true, NULL); // main
	arch->function_list()->AddByAddress(0x01BA0, ctUltra, 0, true, NULL); // try1
	std::string file_name = pf.file_name() + "_vmp";
	MacFile *f = pf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	delete f;
	DWORD ret;
	ASSERT_TRUE(execFile(file_name, ret));
	ASSERT_EQ(ret, 0u);
}

TEST(MacFileTest, EXC_x64)
{
	MacFile pf(NULL);
	ASSERT_EQ(pf.Open("test-binaries/exc-osx-x64", foRead), osSuccess);
	MacArchitecture *arch = pf.item(0);
	arch->function_list()->AddByAddress(0x0000000100000C50, ctVirtualization, 0, true, NULL); // main
	arch->function_list()->AddByAddress(0x0000000100000BB0, ctUltra, 0, true, NULL); // try1
	std::string file_name = pf.file_name() + "_vmp";
	MacFile *f = pf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	delete f;
	DWORD ret;
	ASSERT_TRUE(execFile(file_name, ret));
	ASSERT_EQ(ret, 0u);
}
#endif
#endif