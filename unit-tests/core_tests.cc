#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "../core/objects.h"
#include "../core/osutils.h"
#include "../core/streams.h"
#include "../core/files.h"
#include "../core/processors.h"
#include "../core/core.h"

TEST(CoreTest, OpenAndCompile)
{
	Core vmp;

	ASSERT_TRUE(vmp.Open("test-binaries/win32-app-test1-i386.vmp"));
	EXPECT_EQ(vmp.vm_section_name().compare(".test"), 0);
	IFile &file = *vmp.input_file();
	ASSERT_EQ(file.count(), 1ul);
	IArchitecture &arch = *file.item(0);
	// Check folders.
	Folder &folder = *file.folder_list();
	ASSERT_EQ(folder.count(), 2ul);
	EXPECT_EQ(folder.item(0)->name().compare("Folder1") , 0);
	EXPECT_EQ(folder.item(1)->name().compare("Folder2") , 0);
	ASSERT_EQ(folder.item(1)->count(), 1ul);
	EXPECT_EQ(folder.item(1)->item(0)->name().compare("Strings"), 0);
	EXPECT_EQ(file.folder_list()->GetFolderList().size(), 3ul);
	// Check functions.
	ASSERT_EQ(arch.function_list()->count(), 4ul);
	EXPECT_EQ(arch.function_list()->item(0)->count(), 6ul);
	EXPECT_EQ(arch.function_list()->item(1)->folder()->name().compare("Strings"), 0);
	EXPECT_EQ(arch.function_list()->item(2)->folder()->name().compare("Strings"), 0);
	// Check external commands.
	ExtCommandList *ext_command_list = arch.function_list()->item(0)->ext_command_list();
	ASSERT_EQ(ext_command_list->count(), 2ul);
	EXPECT_EQ(ext_command_list->item(0)->address(), 0x00401007ull);
	EXPECT_EQ(ext_command_list->item(1)->address(), 0x00401011ull);
	ASSERT_TRUE(vmp.Compile());
}

TEST(CoreTest, CryptRSA)
{
	RSA cryptor;

	size_t key_lengths[] = { 1024ul, 1536ul, 2048ul, 2560ul, 3072ul, 3584ul, 4096ul };
	//for(size_t i = 0; i < _countof(key_lengths); i++)
	size_t i = rand() % _countof(key_lengths); 
	{
		size_t key_length = key_lengths[i];
		ASSERT_TRUE(cryptor.CreateKeyPair(key_length)) << key_length;

		const uint64_t c = 0xDEAD00F00D00AA55ull;
		Data data;
		data.PushQWord(c);
		ASSERT_TRUE(cryptor.Encrypt(data)) << key_length;
		ASSERT_NE(memcmp(data.data(), &c, sizeof(c)), 0) << key_length;
		ASSERT_TRUE(cryptor.Decrypt(data)) << key_length;
		ASSERT_EQ(memcmp(data.data(), &c, sizeof(c)), 0) << key_length;
	}
}

TEST(CoreTest, IsUniqueWatermark)
{
	WatermarkManager wm(NULL);
	
	wm.Add("name", "AbCdE?");
	ASSERT_FALSE(wm.IsUniqueWatermark("AB"));
	ASSERT_FALSE(wm.IsUniqueWatermark("b?"));
	ASSERT_FALSE(wm.IsUniqueWatermark("DEF"));
	ASSERT_FALSE(wm.IsUniqueWatermark("AbCdE?"));
	ASSERT_FALSE(wm.IsUniqueWatermark("?123?AbCdE8EF"));

	ASSERT_TRUE(wm.IsUniqueWatermark("?B12"));
	ASSERT_TRUE(wm.IsUniqueWatermark("34587?B123"));
}

#ifndef VMP_GNU
TEST(CoreTest, UTF8Validator)
{
	ASSERT_TRUE(os::ValidateUTF8(std::string()));
	ASSERT_TRUE(os::ValidateUTF8(os::ToUTF8(os::unicode_string(L"кириллица"))));
	ASSERT_FALSE(os::ValidateUTF8(std::string("кириллица 1251")));
}
TEST(CoreTest, FromACP)
{
	ASSERT_TRUE(os::FromACP(std::string("кириллица 1251")) == os::unicode_string(L"кириллица 1251"));
}
#endif