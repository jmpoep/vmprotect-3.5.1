#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/core.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/elffile.h"
#include "../core/lin_runtime32.so.inc"
#include "../core/lin_runtime64.so.inc"

#ifdef ELFFileTest_TestOpen1
TEST(ELFFileTest, TestOpen1)
{
	ELFFile mf(NULL);
	ISectionList *segl;
	ELFSectionList *secl;
	//ELFSymbolList *syml;
	IImportList *impl;

	ASSERT_EQ(osSuccess, mf.Open("test-binaries/Project1-linux-x64", true));
	// Check architectures.
	EXPECT_EQ(1ul, mf.count());
	ELFArchitecture &arch = *mf.item(0);
	EXPECT_EQ("amd64", arch.name());
	EXPECT_EQ("ELF", arch.owner()->format_name());
	EXPECT_EQ(0x4007F0ull, arch.entry_point());
	// Check segments were read.
	segl = arch.segment_list();
	struct seg { const char *name; uint32_t mt; } segs[] = {
		 {".interp", mtReadable} // IDA не показывает
		,{".init", mtReadable | mtExecutable}
		,{".plt", mtReadable | mtExecutable}
		,{".text", mtReadable | mtExecutable}
		,{".fini", mtReadable | mtExecutable}
		,{".rodata", mtReadable}
		,{".eh_frame_hdr", mtReadable}
		,{".eh_frame", mtReadable}
		//,{".init_array", mtReadable | mtWritable} - IDA
		//,{".fini_array", mtReadable | mtWritable} - IDA
		,{".jcr", mtReadable | mtWritable}
		,{".got", mtReadable | mtWritable}
		,{".got.plt", mtReadable | mtWritable}
		,{".data", mtReadable | mtWritable}
		,{".bss", mtReadable | mtWritable}
		//,{"extern"} - IDA
	};
	ASSERT_EQ(_countof(segs), segl->count());
	for(size_t i = 0; i < _countof(segs); i++)
	{
		EXPECT_EQ(segs[i].name, segl->item(i)->name()) << "i=" << i;
		EXPECT_EQ(segs[i].mt, segl->item(i)->memory_type()) << "i=" << i;
	}
	// Check sections were read.
	secl = arch.section_list();
	ASSERT_EQ(0ul, secl->count());
	//EXPECT_EQ("sn", secl->item(0)->name());
	// Check symbols.
	/*syml = arch.symbol_list();
	ASSERT_EQ(syml->count(), 129ul);
	EXPECT_EQ(syml->item(0)->name()
		.compare("/Users/macuser/work2/ios-test2/ios-test2/"), 0);*/
	// Check imports.
	impl = arch.import_list();
	ASSERT_EQ(3ul, impl->count());
	//Why [0].is_sdk == false?
	EXPECT_EQ("libVMProtectSDK64.so", impl->item(0)->name());
	EXPECT_EQ(0ul, impl->item(0)->count());
	EXPECT_EQ("libc.so.6", impl->item(1)->name());
	EXPECT_EQ(0ul, impl->item(1)->count());
	EXPECT_EQ("", impl->item(2)->name());
	const char *fnames[] = {
		 "__gmon_start__"
		,"_Jv_RegisterClasses"
		,"puts"
		,"__libc_start_main"
		,"VMProtectDecryptStringA"
		,"fgets"
		,"_ITM_deregisterTMCloneTable"
		,"_ITM_registerTMCloneTable"
		,"atoi"
		,"__stack_chk_fail"
		,"VMProtectBegin"
		,"VMProtectEnd"
		,"__gmon_start__"
		,"_Jv_RegisterClasses"
		,"puts@@GLIBC_2.2.5"
		,"__libc_start_main@@GLIBC_2.2.5"
		,"VMProtectDecryptStringA"
		,"fgets@@GLIBC_2.2.5"
		,"_ITM_deregisterTMCloneTable"
		,"_ITM_registerTMCloneTable"
		,"atoi@@GLIBC_2.2.5"
		,"__stack_chk_fail@@GLIBC_2.4"
		,"VMProtectBegin"
		,"VMProtectEnd"};
	EXPECT_EQ(_countof(fnames), impl->item(2)->count());
	for(size_t i = 0; i < _countof(fnames); i++)
	{
		EXPECT_EQ(fnames[i], impl->item(2)->item(i)->name()) << "i=" << i;
	}
	mf.Close();
	EXPECT_EQ(mf.count(), 0ul);
}
#endif

TEST(ELFFileTest, TestOpen2)
{
	ELFFile mf(NULL);
	ISectionList *segl;
	ELFSectionList *secl;
	//ELFSymbolList *syml;
	IImportList *impl;
	IExportList *expl;

	ASSERT_EQ(mf.Open("test-binaries/Project1-linux-x86", true), osSuccess);
	// Check architectures.
	ASSERT_EQ(mf.count(), 1ul);
	ELFArchitecture &arch = *mf.item(0);
	EXPECT_EQ(arch.name().compare("i386"), 0);
	EXPECT_EQ(arch.owner()->format_name().compare("ELF"), 0);
	EXPECT_EQ(arch.entry_point(), 0x80485E0ULL);
	// Check segments were read.
	segl = arch.segment_list();
	/*ASSERT_EQ(segl->count(), 4ul);
	EXPECT_EQ(segl->item(0)->name().compare("__PAGEZERO"), 0);
	EXPECT_EQ(segl->item(0)->memory_type(), mtNone);
	EXPECT_EQ(segl->item(1)->name().compare("__TEXT"), 0);
	EXPECT_EQ((int)segl->item(1)->memory_type(), (mtReadable | mtExecutable));
	EXPECT_EQ(segl->item(2)->name().compare("__DATA"), 0);
	EXPECT_EQ((int)segl->item(2)->memory_type(), (mtReadable | mtWritable));
	EXPECT_EQ(segl->item(3)->name().compare("__LINKEDIT"), 0);
	EXPECT_EQ(segl->item(3)->memory_type(), mtReadable);*/
	// Check sections were read.
	secl = arch.section_list();
	/*ASSERT_EQ(secl->count(), 10ul);
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
		.compare("/Users/mac/Library/Developer/Xcode/DerivedData/hello-flxnqzrlabvwqrdcesuuqdaabhfa/Build/Intermediates/hello.build/Release/hello.build/Objects-normal/x86_64/main.o"), 0);*/
	// Check imports.
	impl = arch.import_list();
	/*ASSERT_EQ(impl->count(), 3ul);
	EXPECT_EQ(impl->item(0)->name().compare("/usr/lib/libstdc++.6.dylib"), 0);
	EXPECT_EQ(impl->item(1)->name().compare("/usr/lib/libgcc_s.1.dylib"), 0);
	EXPECT_EQ(impl->item(2)->name().compare("/usr/lib/libSystem.B.dylib"), 0);*/
	// Check exports.
	expl = arch.export_list();
	/*ASSERT_EQ(expl->count(), 5ul);
	EXPECT_EQ(expl->item(0)->name().compare("_NXArgc"), 0);
	EXPECT_EQ(expl->item(1)->name().compare("_NXArgv"), 0);
	EXPECT_EQ(expl->item(2)->name().compare("___progname"), 0);
	EXPECT_EQ(expl->item(3)->name().compare("_environ"), 0);
	EXPECT_EQ(expl->item(4)->name().compare("start"), 0);
	EXPECT_EQ(expl->item(4)->address(), 0x100000c04ULL);*/
	mf.Close();
	EXPECT_EQ(mf.count(), 0ul);
}

/*TEST(ELFFileTest, OpenSharedLib)
{
	ELFFile mf(NULL);

	EXPECT_EQ(mf.Open("test-binaries/macos-dll-test1-i386", foRead), osSuccess);
	// Check architectures.
	ASSERT_EQ(mf.count(), 1ul);
	ELFArchitecture &arch = *mf.item(0);
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
}*/

TEST(ELFFileTest, Clone)
{
	ELFFile mf(NULL);

	ASSERT_EQ(mf.Open("test-binaries/Project1-linux-x64", foRead), osSuccess);
	ELFFile *f = mf.Clone(mf.file_name().c_str());
	ASSERT_EQ(mf.count(), f->count());
	ELFArchitecture &src = *mf.item(0);
	ELFArchitecture &dst = *f->item(0);
	EXPECT_EQ(src.owner()->format_name().compare(dst.owner()->format_name()), 0);
	EXPECT_EQ(src.name().compare(dst.name()), 0);
	EXPECT_EQ(src.cpu_address_size(), dst.cpu_address_size());
	EXPECT_EQ(src.entry_point(), dst.entry_point());
	EXPECT_EQ(src.command_list()->count(), dst.command_list()->count());
	EXPECT_EQ(src.command_list()->count(), dst.command_list()->count());
	EXPECT_EQ(src.segment_list()->count(), dst.segment_list()->count());
	EXPECT_EQ(src.section_list()->count(), dst.section_list()->count());
	//EXPECT_EQ(src.symbol_list()->count(), dst.symbol_list()->count());
	EXPECT_EQ(src.import_list()->count(), dst.import_list()->count());
	EXPECT_EQ(src.fixup_list()->count(), dst.fixup_list()->count());
	EXPECT_EQ(src.export_list()->count(), dst.export_list()->count());
	//std::string symbol_name = src.indirect_symbol_list()->item(1)->symbol()->name();
	//av std::string segment_name = src.section_list()->item(0)->parent()->name();
	mf.Close();
	//EXPECT_EQ(dst.indirect_symbol_list()->item(1)->symbol()->name().compare(symbol_name), 0);
	//av EXPECT_EQ(dst.section_list()->item(0)->parent()->name().compare(segment_name), 0);
	delete f;
}

TEST(ELFFileTest, Runtime_x32)
{
	ELFFile file(NULL);

	ASSERT_TRUE(file.OpenResource(lin_runtime32_so_file, sizeof(lin_runtime32_so_file), true));
	ASSERT_EQ(file.count(), 1ul);
	ELFArchitecture *arch = file.item(0);
	Buffer buffer(&lin_runtime32_so_code[0]);
	arch->ReadFromBuffer(buffer);
	EXPECT_EQ(arch->export_list()->count(), 21ul);
	EXPECT_GT(arch->function_list()->count(), 0ul);
}

TEST(ELFFileTest, Runtime_x64)
{
	ELFFile file(NULL);

	ASSERT_TRUE(file.OpenResource(lin_runtime64_so_file, sizeof(lin_runtime64_so_file), true));
	ASSERT_EQ(file.count(), 1ul);
	ELFArchitecture *arch = file.item(0);
	Buffer buffer(&lin_runtime64_so_code[0]);
	arch->ReadFromBuffer(buffer);
	EXPECT_EQ(arch->export_list()->count(), 21ul);
	EXPECT_GT(arch->function_list()->count(), 0ul);
}

#ifdef __unix__
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
//  clang++ -std=c++11 -target x86_64-linux-gnu -o exc-linux-x64 exc.cpp -L../bin -lVMProtectSDK64 -I ../sdk
//  clang++ -std=c++11 -target i386-linux-gnu -o exc-linux-x86 exc.cpp -L../bin -lVMProtectSDK32 -I ../sdk 
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

TEST(ELFFileTest, EXC_x32)
{
	ELFFile pf(NULL);
	ASSERT_EQ(pf.Open("test-binaries/exc-linux-x86", foRead), osSuccess);
	ELFArchitecture *arch = pf.item(0);
	arch->function_list()->AddByAddress(0x08048940, ctVirtualization, 0, true, NULL); // main
	arch->function_list()->AddByAddress(0x08048890, ctUltra, 0, true, NULL); // try1
	std::string file_name = pf.file_name() + "_vmp";
	ELFFile *f = pf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	delete f;
	DWORD ret;
	ASSERT_TRUE(execFile(file_name, ret));
	ASSERT_EQ(ret, 0u);
}

TEST(ELFFileTest, EXC_x64)
{
	ELFFile pf(NULL);
	ASSERT_EQ(pf.Open("test-binaries/exc-linux-x64", foRead), osSuccess);
	ELFArchitecture *arch = pf.item(0);
	arch->function_list()->AddByAddress(0x0000000000400B80, ctVirtualization, 0, true, NULL); // main
	arch->function_list()->AddByAddress(0x0000000000400AE0, ctUltra, 0, true, NULL); // try1
	std::string file_name = pf.file_name() + "_vmp";
	ELFFile *f = pf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	delete f;
	DWORD ret;
	ASSERT_TRUE(execFile(file_name, ret));
	ASSERT_EQ(ret, 0u);
}
#endif // DEMO
#endif // __unix__