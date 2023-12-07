#include "../runtime/common.h"
#include "../runtime/crypto.h"

TEST(CryptoContainerTests, GetRange)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(1, vKey);
	for (size_t i = 1; i < 100; i++)
	{
		EXPECT_EQ(255, cont.GetByte(i));
		EXPECT_EQ(255, cont.GetByte(-1*i));
	}
}

TEST(CryptoContainerTests, GetSetByte)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(1, vKey);
	ASSERT_NO_THROW(cont.SetByte(0, 123));
	EXPECT_EQ(123, cont.GetByte(0));
}

TEST(CryptoContainerTests, GetSetNBytes)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	const int N = 256;
	CryptoContainer cont(N, vKey);
	for (int i = 0; i < N; i++) 
		ASSERT_NO_THROW(cont.SetByte(i, i));
	for (int i = 0; i < N; i++)
		EXPECT_EQ(i, cont.GetByte(i));
}

TEST(CryptoContainerTests, CryptedBuffer)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	// crypt buffer
	uint32_t vData[256];
	{
		CryptoContainer cont((uint8_t *)vData, sizeof(vData), vKey);
		for (size_t i = 0; i < _countof(vData); i++) cont.SetDWord(i * sizeof(uint32_t), static_cast<uint32_t>(123456 * i));
	}

	// read data
	{
		CryptoContainer cont((uint8_t *)vData, sizeof(vData), vKey);
		for (size_t i = 0; i < _countof(vData); i++)
		{
			EXPECT_EQ(static_cast<uint32_t>(123456 * i), cont.GetDWord(i * sizeof(uint32_t)));

			// check there is no uint32_t(0) and uint32(123456) inside the buffer
			EXPECT_NE((uint32_t)0, vData[i]);
			EXPECT_NE((uint32_t)123456, vData[i]);
		}
	}


}

TEST(CryptoContainerTests, GetSetWordInSingleBlock)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(8, vKey);
	ASSERT_NO_THROW(cont.SetWord(3, 12345));
	EXPECT_EQ(12345, cont.GetWord(3));
}

TEST(CryptoContainerTests, GetSetWordInTwoBlocks)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(16, vKey);
	ASSERT_NO_THROW(cont.SetWord(7, 0x1234));
	EXPECT_EQ(0x1234, cont.GetWord(7));
}

TEST(CryptoContainerTests, GetSetWordOutOfRange)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(8, vKey);
	ASSERT_NO_THROW(cont.SetByte(7, 1));
	EXPECT_EQ(65535, cont.GetWord(7));

	ASSERT_NO_THROW(cont.SetByte(7, 0));
	ASSERT_NO_THROW(cont.SetWord(7, 12345));
	EXPECT_EQ(0, cont.GetByte(7));
}

TEST(CryptoContainerTests, GetSetDWordInSingleBlock)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(8, vKey);
	ASSERT_NO_THROW(cont.SetDWord(3, 0x12345678));
	EXPECT_EQ((uint32_t)0x12345678, cont.GetDWord(3));
}

TEST(CryptoContainerTests, GetSetDWordInTwoBlocks)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(16, vKey);
	uint32_t vValues[4] = {0x12345678, 0xA0B0C0D0, 0x01020304, 0x1A2B3C4D};
	for (size_t i = 0; i < 4; i++)
	{
		ASSERT_NO_THROW(cont.SetDWord(5 + i, vValues[i]));
		EXPECT_EQ(vValues[i], cont.GetDWord(5 + i));
	}
}

TEST(CryptoContainerTests, GetSetDWordOutOfRange)
{
	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(8, vKey);
	for (size_t i = 5; i < 8; i++) ASSERT_NO_THROW(cont.SetByte(i, 1));
	for (size_t i = 5; i < 8; i++) EXPECT_EQ(0xFFFFFFFF, cont.GetDWord(i));

	for (size_t i = 5; i < 8; i++) ASSERT_NO_THROW(cont.SetByte(i, 0));
	for (size_t i = 5; i < 8; i++) ASSERT_NO_THROW(cont.SetDWord(i, 0x12345678));
	for (size_t i = 5; i < 8; i++) EXPECT_EQ(0, cont.GetByte(i));
}

TEST(CryptoContainerTests, SHA1)
{
	std::vector<uint8_t> vData;
	srand((unsigned int)__rdtsc());
	for (size_t i = 0; i < 1111; i++) vData.push_back(rand() % 255);

	SHA1	hash1, hash2;

	hash1.Input(&vData[0], vData.size());

	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(vData.size(), vKey);
	for (size_t i = 0; i < vData.size(); i++) cont.SetByte(i, vData[i]);
	hash2.Input(cont, 0, cont.size());

	const uint8_t *p1 = hash1.Result();
	const uint8_t *p2 = hash2.Result();
	for (int i = 0; i < 20; i++) ASSERT_EQ(p1[i], p2[i]);
}

TEST(CryptoContainerTests, SHA1_CRC)
{
	std::vector<uint8_t>	vData;
	srand((unsigned int)__rdtsc());
	for (size_t i = 0; i < 1111; i++) vData.push_back(rand() % 255);

	SHA1	hash1, hash2;

	hash1.Input(&vData[0], vData.size() - 4);
	memcpy(&vData[vData.size() - 4], hash1.Result(), 4);

	uint8_t vKey[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	CryptoContainer cont(vData.size(), vKey);
	for (size_t i = 0; i < vData.size(); i++) cont.SetByte(i, vData[i]);
	hash2.Input(cont, 0, cont.size() - 4);
	EXPECT_TRUE(hash1 == hash2);
}

TEST(CryptoContainerTests, GetByte)
{
	uint8_t vKey[] = {1, 2, 3, 4, 5, 6, 7, 8};
	const size_t nMax = 20;
	CryptoContainer cont(nMax, vKey);
	for (size_t i = 0; i < nMax; i++) cont.SetByte(i, static_cast<uint8_t>(i));

	for(size_t i = 0; i < nMax; i++)
	{
		EXPECT_EQ(cont.GetByte(i), i);
	}
}
