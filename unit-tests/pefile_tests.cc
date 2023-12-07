#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/core.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/pefile.h"
#include "../core/packer.h"
#include "../third-party/lzma/LzmaDecode.h"

#ifdef DEMO
 #include "../core/win_runtime32demo.dll.inc"
 #include "../core/win_runtime64demo.dll.inc"
#else
 #include "../core/win_runtime32.dll.inc"
 #include "../core/win_runtime64.dll.inc"
#endif

#ifdef VMP_GNU
 #include <stdlib.h>
#endif

TEST(PEFileTest, OpenEXE)
{
	// Test: read header of Win32/Intel file. Read should finish successfully.
	PEFile pf(NULL);

	ASSERT_EQ(pf.Open("test-binaries/win32-app-test1-i386", foRead), osSuccess);
	// Check architectures.
	ASSERT_EQ(pf.count(), 1ul);
	PEArchitecture *arch = pf.arch_pe();
	EXPECT_EQ(arch->name().compare("i386"), 0);
	EXPECT_EQ(arch->owner()->format_name().compare("PE"), 0);
	EXPECT_EQ(arch->cpu_address_size(), osDWord);
	EXPECT_EQ(arch->image_base(), 0x400000ull);
	EXPECT_EQ(arch->entry_point(), 0x401000ull);
	// Check segments.
	ISectionList *segment_list = arch->segment_list();
	ASSERT_EQ(segment_list->count(), 3ul);
	EXPECT_EQ(segment_list->item(0)->name().compare(".text"), 0);
	EXPECT_EQ(segment_list->item(1)->name().compare(".rdata"), 0);
	EXPECT_EQ(segment_list->item(2)->name().compare(".data"), 0);
	EXPECT_EQ((int)segment_list->item(0)->memory_type(), (mtReadable | mtExecutable));
	EXPECT_EQ((int)segment_list->item(1)->memory_type(), mtReadable);
	EXPECT_EQ((int)segment_list->item(2)->memory_type(), (mtReadable | mtWritable));
	// Check imports.
	IImportList *imp = arch->import_list();
	ASSERT_EQ(imp->count(), 3ul);
	EXPECT_EQ(imp->item(0)->name().compare("user32.dll"), 0);
	ASSERT_EQ(imp->item(0)->count(), 6ul);
	EXPECT_EQ(imp->item(0)->item(0)->name().compare("SetFocus"), 0);
	EXPECT_EQ(imp->item(0)->item(0)->address(), 0x402028ull);
	EXPECT_EQ(imp->item(0)->item(1)->name().compare("MessageBoxA"), 0);
	EXPECT_EQ(imp->item(0)->item(1)->address(), 0x40202cull);
	EXPECT_EQ(imp->item(0)->item(2)->name().compare("GetDlgItemTextA"), 0);
	EXPECT_EQ(imp->item(0)->item(2)->address(), 0x402030ull);
	EXPECT_EQ(imp->item(0)->item(3)->name().compare("GetDlgItem"), 0);
	EXPECT_EQ(imp->item(0)->item(3)->address(), 0x402034ull);
	EXPECT_EQ(imp->item(0)->item(4)->name().compare("EndDialog"), 0);
	EXPECT_EQ(imp->item(0)->item(4)->address(), 0x402038ull);
	EXPECT_EQ(imp->item(0)->item(5)->name().compare("DialogBoxIndirectParamA"), 0);
	EXPECT_EQ(imp->item(0)->item(5)->address(), 0x40203cull);
	EXPECT_EQ(imp->item(1)->name().compare("kernel32.dll"), 0);
	ASSERT_EQ(imp->item(1)->count(), 5ul);
	EXPECT_EQ(imp->item(1)->item(0)->name().compare("MultiByteToWideChar"), 0);
	EXPECT_EQ(imp->item(1)->item(0)->address(), 0x402010ull);
	EXPECT_EQ(imp->item(1)->item(1)->name().compare("GlobalFree"), 0);
	EXPECT_EQ(imp->item(1)->item(1)->address(), 0x402014ull);
	EXPECT_EQ(imp->item(1)->item(2)->name().compare("ExitProcess"), 0);
	EXPECT_EQ(imp->item(1)->item(2)->address(), 0x402018ull);
	EXPECT_EQ(imp->item(1)->item(3)->name().compare("GetModuleHandleA"), 0);
	EXPECT_EQ(imp->item(1)->item(3)->address(), 0x40201cull);
	EXPECT_EQ(imp->item(1)->item(4)->name().compare("GlobalAlloc"), 0);
	EXPECT_EQ(imp->item(1)->item(4)->address(), 0x402020ull);
	EXPECT_EQ(imp->item(2)->name().compare("VMProtectSDK32.dll"), 0);
	ASSERT_EQ(imp->item(2)->count(), 3ul);
	EXPECT_EQ(imp->item(2)->item(0)->name().compare("VMProtectDecryptStringA"), 0);
	EXPECT_EQ(imp->item(2)->item(0)->address(), 0x402000ull);
	EXPECT_EQ(imp->item(2)->item(1)->name().compare("VMProtectEnd"), 0);
	EXPECT_EQ(imp->item(2)->item(1)->address(), 0x402004ull);
	EXPECT_EQ(imp->item(2)->item(2)->name().compare("VMProtectBegin"), 0);
	EXPECT_EQ(imp->item(2)->item(2)->address(), 0x402008ull);
	// Check fixups.
	IFixupList *fixup_list = arch->fixup_list();
	EXPECT_EQ(fixup_list->count(), 0ul);
	// Check resources.
	IResourceList *resource_list = arch->resource_list();
	EXPECT_EQ(resource_list->count(), 0ul);
	// Check lists were cleared.
	pf.Close();
	EXPECT_EQ(pf.count(), 0ul);
}

TEST(PEFileTest, OpenDLL)
{
	PEFile pf(NULL);
	
	ASSERT_EQ(pf.Open("test-binaries/win32-dll-test1-i386", foRead), osSuccess);
	// Check architectures.
	ASSERT_EQ(pf.count(), 1ul);
	PEArchitecture *arch = pf.arch_pe();
	// Check exports.
	IExportList *exp = arch->export_list();
	EXPECT_EQ(exp->name().compare("ShimEng.dll"), 0);
	ASSERT_EQ(exp->count(), 11ul);
	EXPECT_EQ(exp->item(0)->name().compare("SE_DllLoaded"), 0);
	EXPECT_EQ(exp->item(0)->forwarded_name().compare("APPHELP.SE_DllLoaded"), 0);
	EXPECT_EQ(exp->item(0)->address(), 0x3ff31257ull);
	EXPECT_EQ(exp->item(10)->name().compare("SE_ProcessDying"), 0);
	EXPECT_EQ(exp->item(10)->forwarded_name().compare("APPHELP.SE_ProcessDying"), 0);
	EXPECT_EQ(exp->item(10)->address(), 0x3ff31358ull);
	// Check fixups.
	IFixupList *fixup_list = arch->fixup_list();
	ASSERT_EQ(fixup_list->count(), 12ul);
	EXPECT_EQ(fixup_list->item(0)->address(), 0x3ff3103aull);
	EXPECT_EQ(fixup_list->item(0)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(1)->address(), 0x3ff31042ull);
	EXPECT_EQ(fixup_list->item(1)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(2)->address(), 0x3ff3104dull);
	EXPECT_EQ(fixup_list->item(2)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(3)->address(), 0x3ff31089ull);
	EXPECT_EQ(fixup_list->item(3)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(4)->address(), 0x3ff310a2ull);
	EXPECT_EQ(fixup_list->item(4)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(5)->address(), 0x3FF310AEull);
	EXPECT_EQ(fixup_list->item(5)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(6)->address(), 0x3FF310B6ull);
	EXPECT_EQ(fixup_list->item(6)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(7)->address(), 0x3FF310BEull);
	EXPECT_EQ(fixup_list->item(7)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(8)->address(), 0x3FF310CAull);
	EXPECT_EQ(fixup_list->item(8)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(9)->address(), 0x3FF310E0ull);
	EXPECT_EQ(fixup_list->item(9)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(10)->address(), 0x3FF310E8ull);
	EXPECT_EQ(fixup_list->item(10)->type(), ftHighLow);
	EXPECT_EQ(fixup_list->item(11)->address(), 0x3ff310f0ull);
	EXPECT_EQ(fixup_list->item(11)->type(), ftHighLow);
	// Check resources.
	IResourceList *resource_list = arch->resource_list();
	ASSERT_EQ(resource_list->count(), 1ul);
	EXPECT_EQ(resource_list->item(0)->type(), rtVersionInfo);
	ASSERT_EQ(resource_list->item(0)->count(), 1ul);
	EXPECT_EQ(resource_list->item(0)->item(0)->name(), "1");
	EXPECT_EQ(resource_list->item(0)->item(0)->count(), 1ul);
	EXPECT_EQ(resource_list->item(0)->item(0)->item(0)->name(), "1033");
	EXPECT_EQ(resource_list->item(0)->item(0)->item(0)->address(), 0x3ff33060ull);
	EXPECT_EQ(resource_list->item(0)->item(0)->item(0)->size(), 0x3acul);
	// Check lists were cleared.
	pf.Close();
	EXPECT_EQ(pf.count(), 0ul);
}

TEST(PEFileTest, Clone)
{
	PEFile pf(NULL);

	ASSERT_EQ(pf.Open("test-binaries/win32-app-test1-i386", foRead), osSuccess);
	PEFile *f = pf.Clone(pf.file_name().c_str());
	EXPECT_EQ(pf.count(), f->count());
	PEArchitecture &src = *pf.arch_pe();
	PEArchitecture &dst = *f->arch_pe();
	EXPECT_EQ(src.owner()->format_name().compare(dst.owner()->format_name()), 0);
	EXPECT_EQ(src.name().compare(dst.name()), 0);
	EXPECT_EQ(src.cpu_address_size(), dst.cpu_address_size());
	EXPECT_EQ(src.entry_point(), dst.entry_point());
	EXPECT_EQ(src.command_list()->count(), dst.command_list()->count());
	EXPECT_EQ(src.segment_list()->count(), dst.segment_list()->count());
	EXPECT_EQ(src.import_list()->count(), dst.import_list()->count());
	EXPECT_EQ(src.fixup_list()->count(), dst.fixup_list()->count());
	EXPECT_EQ(src.export_list()->count(), dst.export_list()->count());
	EXPECT_EQ(src.resource_list()->count(), dst.resource_list()->count());
	delete f;
}

TEST(PEFileTest, Runtime_x32)
{
	PEFile file(NULL);

	ASSERT_TRUE(file.OpenResource(win_runtime32_dll_file, sizeof(win_runtime32_dll_file), true));
	ASSERT_EQ(file.count(), 1ul);
	PEArchitecture *arch = file.arch_pe();
	Buffer buffer(&win_runtime32_dll_code[0]);
	arch->ReadFromBuffer(buffer);
	EXPECT_GT(arch->export_list()->count(), 0ul);
	EXPECT_GT(arch->function_list()->count(), 0ul);
}

TEST(PEFileTest, Runtime_x64)
{
	PEFile file(NULL);

	ASSERT_TRUE(file.OpenResource(win_runtime64_dll_file, sizeof(win_runtime64_dll_file), true));
	ASSERT_EQ(file.count(), 1ul);
	PEArchitecture *arch = file.arch_pe();
	Buffer buffer(&win_runtime64_dll_code[0]);
	arch->ReadFromBuffer(buffer);
	EXPECT_GT(arch->export_list()->count(), 0ul);
	EXPECT_GT(arch->function_list()->count(), 0ul);
}

TEST(PEFileTest, Compile_x32)
{
	PEFile pf(NULL);
	size_t i;
	
	ASSERT_EQ(pf.Open("test-binaries/win32-app-delphi-i386", foRead), osSuccess);
	PEArchitecture *arch = pf.arch_pe();
	for (i = 0; i < arch->map_function_list()->count(); i++) {
		MapFunction *map_function = arch->map_function_list()->item(i);
		if (map_function->type() == otString || map_function->type() == otAPIMarker || map_function->type() == otMarker)
			arch->function_list()->AddByAddress(map_function->address(), ctVirtualization, 0, true, NULL);
	}
	ASSERT_EQ(arch->function_list()->count(), 4ul);
	std::string file_name = pf.file_name()+".exe";
	PEFile *f = pf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	delete f;
	PEFile pf2(NULL);
	ASSERT_EQ(pf2.Open(file_name.c_str(), foRead | foHeaderOnly), osSuccess);
	ASSERT_EQ(pf2.count(), pf.count());
	PEArchitecture &src = *pf.arch_pe();
	PEArchitecture &dst = *pf2.arch_pe();
	for (i = 0; i < src.fixup_list()->count(); i++) {
		IFixup *src_fixup = src.fixup_list()->item(i);
		IFixup *dst_fixup = src.fixup_list()->GetFixupByAddress(src_fixup->address());
		EXPECT_EQ(src_fixup->address(), dst_fixup->address());
		EXPECT_EQ(src_fixup->type(), dst_fixup->type());
		EXPECT_EQ(src_fixup->size(), dst_fixup->size());
	}
	if (options.flags & cpResourceProtection) {
		EXPECT_EQ(dst.resource_list()->count(), 4ul);
	} else {
		EXPECT_EQ(src.resource_list()->count(), dst.resource_list()->count());
		for (i = 0; i < src.resource_list()->count(); i++) {
			PEResource *src_resource = src.resource_list()->item(i);
			PEResource *dst_resource = dst.resource_list()->item(i);
			EXPECT_EQ(src_resource->count(), dst_resource->count());
			EXPECT_EQ(src_resource->address(), dst_resource->address());
			EXPECT_EQ(src_resource->size(), dst_resource->size());
			EXPECT_EQ(src_resource->type(), dst_resource->type());
			EXPECT_EQ(src_resource->name(), dst_resource->name());
		}
	}
	// check AV buffer
	ASSERT_EQ(dst.segment_list()->count(), 13ul);
	ASSERT_TRUE(dst.AddressSeek(dst.segment_list()->item(10)->address()));
	uint32_t sum = 0;
	for (i = 0; i < 64; i++) {
		sum += dst.ReadDWord();
	}
	EXPECT_EQ(sum, 0xB7896EB5);
	// delete file from disk
	pf2.Close();
	remove(file_name.c_str());
}

TEST(PEFileTest, Compile_DLL)
{
	PEFile pf(NULL);

	ASSERT_EQ(pf.Open("test-binaries/win32-dll-test1-i386", foRead), osSuccess);
	std::string file_name = pf.file_name()+".dll";
	PEFile *f = pf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpPack;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	delete f;
	PEFile pf2(NULL);
	ASSERT_EQ(pf2.Open(file_name.c_str(), foRead | foHeaderOnly), osSuccess);
	ASSERT_EQ(pf2.count(), pf.count());
	PEArchitecture &src = *pf.arch_pe();
	PEArchitecture &dst = *pf2.arch_pe();
	size_t i;
	for (i = 0; i < src.fixup_list()->count(); i++) {
		IFixup *src_fixup = src.fixup_list()->item(i);
		IFixup *dst_fixup = src.fixup_list()->GetFixupByAddress(src_fixup->address());
		EXPECT_EQ(src_fixup->address(), dst_fixup->address());
		EXPECT_EQ(src_fixup->type(), dst_fixup->type());
		EXPECT_EQ(src_fixup->size(), dst_fixup->size());
	}
	ASSERT_EQ(src.resource_list()->count(), dst.resource_list()->count());
	for (i = 0; i < src.resource_list()->count(); i++) {
		PEResource *src_resource = src.resource_list()->item(i);
		PEResource *dst_resource = dst.resource_list()->item(i);
		EXPECT_EQ(src_resource->count(), dst_resource->count());
		EXPECT_EQ(src_resource->address(), dst_resource->address());
		EXPECT_EQ(src_resource->size(), dst_resource->size());
		EXPECT_EQ(src_resource->type(), dst_resource->type());
		EXPECT_EQ(src_resource->name(), dst_resource->name());
	}
	ASSERT_EQ(src.export_list()->count(), dst.export_list()->count());
	for (i = 0; i < src.export_list()->count(); i++) {
		PEExport *src_export = src.export_list()->item(i);
		PEExport *dst_export = dst.export_list()->item(i);
		EXPECT_EQ(src_export->name(), dst_export->name());
		EXPECT_EQ(src_export->ordinal(), dst_export->ordinal());
		if (src_export->forwarded_name().empty()) {
			EXPECT_EQ(src_export->address(), dst_export->address());
		} else {
			EXPECT_EQ(src_export->forwarded_name(), dst_export->forwarded_name());
		}
	}
	// check AV buffer
	ASSERT_EQ(dst.segment_list()->count(), 7ul);
	ASSERT_TRUE(dst.AddressSeek(dst.segment_list()->item(4)->address()));
	uint32_t sum = 0;
	for (i = 0; i < 64; i++) {
		sum += dst.ReadDWord();
	}
	EXPECT_EQ(sum, 0xB7896EB5);
	// delete file from disk
	pf2.Close();
	remove(file_name.c_str());
}

TEST(PEFileTest, Compile_x64)
{
	PEFile pf(NULL);

	ASSERT_EQ(pf.Open("test-binaries/win64-app-msvc-amd64", foRead), osSuccess);
	PEArchitecture *arch = pf.arch_pe();
	size_t i;
	for (i = 0; i < arch->map_function_list()->count(); i++) {
		MapFunction *map_function = arch->map_function_list()->item(i);
		if (map_function->type() == otString)
			arch->function_list()->AddByAddress(map_function->address(), ctVirtualization, 0, true, NULL);
	}
	ASSERT_EQ(arch->function_list()->count(), 2ul);
	std::string file_name = pf.file_name()+".exe";
	PEFile *f = pf.Clone(file_name.c_str());
	CompileOptions options;
	options.flags = cpMaximumProtection;
	options.section_name = ".vmp";
	ASSERT_TRUE(f->Compile(options));
	delete f;
	PEFile pf2(NULL);
	ASSERT_EQ(pf2.Open(file_name.c_str(), foRead | foHeaderOnly), osSuccess);
	ASSERT_EQ(pf2.count(), pf.count());
	PEArchitecture &src = *pf.arch_pe();
	PEArchitecture &dst = *pf2.arch_pe();
	for (i = 0; i < src.fixup_list()->count(); i++) {
		IFixup *src_fixup = src.fixup_list()->item(i);
		IFixup *dst_fixup = src.fixup_list()->GetFixupByAddress(src_fixup->address());
		EXPECT_EQ(src_fixup->address(), dst_fixup->address());
		EXPECT_EQ(src_fixup->type(), dst_fixup->type());
		EXPECT_EQ(src_fixup->size(), dst_fixup->size());
	}
	if (options.flags & cpResourceProtection) {
		EXPECT_EQ(dst.resource_list()->count(), 1ul);
	} else {
		ASSERT_EQ(src.resource_list()->count(), dst.resource_list()->count());
		for (i = 0; i < src.resource_list()->count(); i++) {
			PEResource *src_resource = src.resource_list()->item(i);
			PEResource *dst_resource = dst.resource_list()->item(i);
			EXPECT_EQ(src_resource->count(), dst_resource->count());
			EXPECT_EQ(src_resource->address(), dst_resource->address());
			EXPECT_EQ(src_resource->size(), dst_resource->size());
			EXPECT_EQ(src_resource->type(), dst_resource->type());
			EXPECT_EQ(src_resource->name(), dst_resource->name());
		}
	}
	// check AV buffer
	ASSERT_EQ(dst.segment_list()->count(), 9ul);
	ASSERT_TRUE(dst.AddressSeek(dst.segment_list()->item(6)->address()));
	uint32_t sum = 0;
	for (i = 0; i < 64; i++) {
		sum += dst.ReadDWord();
	}
	EXPECT_EQ(sum, 0xB7896EB5);
	// delete file from disk
	pf2.Close();
	remove(file_name.c_str());
}

TEST(PEFileTest, Pack)
{
	PEFile pf(NULL);

	ASSERT_EQ(pf.Open("test-binaries/win32-app-delphi-i386", foRead), osSuccess);
	PEArchitecture *arch = pf.arch_pe();
	PESegment *section = arch->segment_list()->item(0);
	Data data, props;
	Packer packer;
	size_t data_size = static_cast<size_t>(section->physical_size());
	ASSERT_TRUE(packer.WriteProps(&props));
	ASSERT_TRUE(arch->AddressSeek(section->address()));
	ASSERT_TRUE(packer.Code(arch, data_size, &data));
	Byte *dst = new Byte[data_size];

	CLzmaDecoderState state;
	EXPECT_EQ(LzmaDecodeProperties(&state.Properties, props.data(), (unsigned)props.size()), LZMA_RESULT_OK);
	state.Probs = (CProb *)new Byte[LzmaGetNumProbs(&state.Properties) * sizeof(CProb)];
	SizeT src_processed_size;
	SizeT dst_processed_size;
	EXPECT_EQ(LzmaDecode(&state, data.data(), -1, &src_processed_size, dst, -1, &dst_processed_size), LZMA_RESULT_OK);
	delete [] state.Probs;
	ASSERT_EQ(dst_processed_size, data_size);

	arch->AddressSeek(section->address());
	for (size_t i = 0; i < data_size; i++) {
		EXPECT_EQ(dst[i], arch->ReadByte());
	}
	delete [] dst;
}

TEST(PEFileTest, CalcCheckSum)
{
	uint32_t sum;
	ASSERT_TRUE(os::FileGetCheckSum("test-binaries/win32-app-delphi-i386", &sum));
	EXPECT_EQ(sum, 0x000e4490ul);
	ASSERT_TRUE(os::FileGetCheckSum("test-binaries/win64-app-msvc-amd64", &sum));
	EXPECT_EQ(sum, 0x0000b8c9ul);
}

#ifndef DEMO
#ifndef __APPLE__
static bool execFile(const std::string &fileName, DWORD &exitCode)
{
	exitCode = 0xFFFFFFF;
#ifdef VMP_GNU
	int ret = system(("wine " + fileName).c_str());
	if (ret != -1)
	{
		exitCode = DWORD(ret);
		return true;
	}
	return false;
#else
	PROCESS_INFORMATION processInformation = { };
	STARTUPINFOW startupInfo = { };
	startupInfo.cb = sizeof(startupInfo);
	os::unicode_string unicodeName = os::FromUTF8(fileName);

	BOOL result = CreateProcessW(NULL, const_cast<wchar_t *>(unicodeName.c_str()),
		NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
		NULL, NULL, &startupInfo, &processInformation);

	if (!result)
		return false;

	WaitForSingleObject(processInformation.hProcess, 10000);
	result = GetExitCodeProcess(processInformation.hProcess, &exitCode);
	if (exitCode == STILL_ACTIVE)
		TerminateProcess(processInformation.hProcess, 0xFFFFFFFF);
	CloseHandle(processInformation.hProcess);
	CloseHandle(processInformation.hThread);

	return (result && (exitCode != STILL_ACTIVE));
#endif
}

/* seh.cpp:
#include <windows.h>
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

__declspec(noinline) void try2()
{
	printf("try 2\n");
	PEXCEPTION_POINTERS info = NULL;
	__try {
		if (*reinterpret_cast<char *>(0xFACE) == 0)
			return;
	}
	__except (info = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
		printf("throw at %p\n", info->ExceptionRecord->ExceptionAddress);
	}
	printf("end 2\n\n");
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
	try2();

    return 0;
}
*/

TEST(PEFileTest, SEH_x32)
{
	PEFile pf(NULL);
	ASSERT_EQ(pf.Open("test-binaries/seh-x86", foRead), osSuccess);
	PEArchitecture *arch = pf.arch_pe();
	arch->function_list()->AddByAddress(0x00401130, ctVirtualization, 0, true, NULL); // main
	arch->function_list()->AddByAddress(0x00401000, ctUltra, 0, true, NULL); // try1
	arch->function_list()->AddByAddress(0x00401070, ctMutation, 0, true, NULL); // try2
	std::string file_name = pf.file_name() + ".exe";
	PEFile *f = pf.Clone(file_name.c_str());
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
#ifndef VMP_GNU
TEST(PEFileTest, SEH_x64)
{
	PEFile pf(NULL);
	ASSERT_EQ(pf.Open("test-binaries/seh-x64", foRead), osSuccess);
	PEArchitecture *arch = pf.arch_pe();
	arch->function_list()->AddByAddress(0x00000001400010A0, ctVirtualization, 0, true, NULL); // main
	arch->function_list()->AddByAddress(0x0000000140001000, ctUltra, 0, true, NULL); // try1
	arch->function_list()->AddByAddress(0x0000000140001040, ctMutation, 0, true, NULL); // try2
	std::string file_name = pf.file_name() + ".exe";
	PEFile *f = pf.Clone(file_name.c_str());
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
