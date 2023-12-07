#include "../runtime/common.h"
#include "../runtime/objects.h"
#include "../runtime/crypto.h"
#include "../runtime/licensing_manager.h"
#include "../runtime/string_manager.h"
#include "../runtime/hwid.h"
#include "../runtime/hook_manager.h"
#include "../runtime/core.h"
#ifndef VMP_GNU
#include "../runtime/registry_manager.h"
#include "../runtime/file_manager.h"
#endif

/**
 * LicensingManagerTests
 */

void Base64ToVector(const char *src, std::vector<uint8_t> &dst)
{
	size_t src_len = strlen(src);
	uint8_t *data = new uint8_t[src_len];
	size_t size = src_len;
	Base64Decode(src, src_len, data, size);
	dst.insert(dst.end(), &data[0], &data[size]);
	delete [] data;
}

class Data
{
public:
	void PushByte(uint8_t b){ m_vData.push_back(b);	}
	void PushDWord(uint32_t d) { PushBuff(&d, sizeof(d)); }
	void PushQWord(uint64_t q) { PushBuff(&q, sizeof(q)); }
	void PushWord(uint16_t w) { PushBuff(&w, sizeof(w)); }
	void PushBuff(const void *pBuff, size_t nCount)
	{
		m_vData.insert(m_vData.end(), reinterpret_cast<const uint8_t *>(pBuff), reinterpret_cast<const uint8_t *>(pBuff) + nCount);
	}
	void InsertByte(size_t position, uint8_t value)
	{
		m_vData.insert(m_vData.begin() + position, value);
	}

	uint32_t ReadDWord(size_t nPosition) const
	{
		return *reinterpret_cast<const uint32_t *>(&m_vData[nPosition]);
	}

	void WriteDWord(size_t nPosition, uint32_t dwValue)
	{
		*reinterpret_cast<uint32_t *>(&m_vData[nPosition]) = dwValue;
	}

	size_t size() const { return m_vData.size(); }
	void clear() { m_vData.clear(); }
	bool empty() const { return m_vData.empty(); }
	void resize(size_t size) { m_vData.resize(size); }
	void resize(size_t size, uint8_t value) { m_vData.resize(size, value); }
	uint8_t *data() const { return const_cast<uint8_t *>(m_vData.data()); }
	uint8_t &operator[](size_t pos) { return m_vData[pos]; }
private:
	std::vector<uint8_t> m_vData;
};

TEST(LicensingManagerTests, ParseSerial)
{
	uint8_t key[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	std::vector<uint8_t> public_exp;
	std::vector<uint8_t> private_exp;
	std::vector<uint8_t> modulus;
	Data data, data_crypted;

	Base64ToVector("AAEAAQ==", public_exp);
	Base64ToVector("qA/5YwcsKJ63gVXf438aCxF0p45Mmew/CXj9TBg0i+biY7nJQqOFP4BhqiBY+4zrqh2iWrse3pZStEm7jqFh488xP2p7KMQHB+EPWx4ZYA6OFVl1SLG3gK7C0pqDmqQ289NUi5u56N+gNV+YA/mZ/dXBCbXv/HDgSuPEfkygz+CG20uRWoKbF+JKF6dHLbQHnn7l9nu1ht7+6NeL32UV8sEu8E508qF4cEni4RPu4cvnp98t8FF5e10JFpCE5wjkg47GQ+vUB5z+Z2AzuQdW8EHPvWwn+TNvcIh2tHEPWT39IeZ2X2KbfLCqdW4T2wwHHesiNwhqOwpNnxzHmDyaAQ==", private_exp);
	Base64ToVector("rnK4CvdHf4d3DTtS7B7Fyu5kzT16BTWlecKTZcjw5Ji9BTLfhjgIxSyY6uqD2wNxd+qTzNY4PHvAf8lwxsLE9ir7sDhrDgzLp5PR5EW5nuZgHC+QKpXoaXH9Q73i6KcfDOUSHquQmRNtDHqyublT9/yv1pLuA+mCC+/gWZhgdAGZbwA0Ek61mGH1YFOUY+qLGyiJwGSMBXDWgEAm36wUHyTjZhN2003JuJwtpT8IWKiR5sy+vNQdtj+QEeyADbTFuYv5RHXUkUVvc0RUXecFLAQuE/1i1AJDAd5s4oDKUizZ10nCsHawSnHn4pgdQFzvhxnNTEVcjhcWMW0+/Zvfhw==", modulus);

	for (size_t i = 0; i < FIELD_COUNT; i++) {
		data.PushDWord(0);
	}

	data.WriteDWord(FIELD_PUBLIC_EXP_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(data.size()));
	data.WriteDWord(FIELD_PUBLIC_EXP_SIZE * sizeof(uint32_t), static_cast<uint32_t>(public_exp.size()));
	data.PushBuff(public_exp.data(), public_exp.size());

	data.WriteDWord(FIELD_MODULUS_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(data.size()));
	data.WriteDWord(FIELD_MODULUS_SIZE * sizeof(uint32_t), static_cast<uint32_t>(modulus.size()));
	data.PushBuff(modulus.data(), modulus.size());

	while (data.size() % 8) {
		data.PushByte(0);
	}
	size_t crc_pos = data.size();
	data.WriteDWord(FIELD_CRC_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(crc_pos));

	data_crypted.resize(crc_pos + 16);
	CryptoContainer cont(data_crypted.data(), data_crypted.size(), key);
	for (size_t i = 0; i < crc_pos; i++) {
		cont.SetByte(i, data[i]);
	}

	SHA1 hash;
	hash.Input(data_crypted.data(), crc_pos);
	const uint8_t *cp = hash.Result();
	for (size_t i = crc_pos; i < data_crypted.size(); i++) {
		cont.SetByte(i, cp[i - crc_pos]);
	}

	static GlobalData fake_loader_data;
	loader_data = &fake_loader_data;

	LicensingManager licensing_manager(data_crypted.data(), (uint32_t)data_crypted.size(), key);
	ASSERT_TRUE(licensing_manager.SetSerialNumber("FgeTRyB93RJeABJN83qKo7mTEvXHGHwYXmZ6pGY5o1X09bc8fmmebq1mryX5yHUWb6F+cauzsLxUyG+uTQBT9zC31EJPQqG9YgaRbOPPS9A7u1HXZA8x491ew0lDKJISy8TYNsjTJ2vB903jW+/A/UVc2+cgDCF3eqztkvYBEas6Y40Hj9A2ylJVYsYWq41RCZyDcXZtlbnuk8W2CDBAk+PGZeUPjEq/oasnTCeKdkO4NY2QVrFzvC/msT+7FWAaBNwe5OQYZLCH875qFVw0H2B9xcnVuIAHAYD00H0Hzk9sf/jAGx4l5CS8FQ4tshZfEllF6CcwVQuRdGz07gQWAQ==") == 0);
	//ASSERT_TRUE(licensing_manager.SetSerialNumber("ZQQJD0w10nAco1VOACoCV5OSPFTUFHdYCzCKoERSdPpSCpCbL1qPkHTH5H+97W6k9amdncxcaOqXIhpe8ABCbWawJAk84S/IJAPcr1ki9faWOYfaLB2wxb3BRa2knvK++5Dr9EfrXhmk6VeL4MaJLTtFRSQSJVY6ZgFZnACHyEQYGRapk7vOVFlbQpKBaZ+s2eyipTeYpaF4gV15wWwx4uhBmCUMjJfMgMyx5jN6wRO/OzCG2db0keAHZ0136byI8QTqqgwV0lLjgSNC2+Idmp+uO5G7Q2YJetOQQjTwo3QAnEpzK2TSsgATfX50g17OXOb2sBfNMdPKdupKLtLlRg==") == 0);
}

#ifndef VMP_GNU
#ifndef WIN_DRIVER
TEST(LicensingManagerTests, VmpLocalTime)
{
	SYSTEMTIME local_time;
	uint32_t t1, t2, t3;
	t1 = LicensingManager::GetCurrentDate();
	GetLocalTime(&local_time);
	t2 = (local_time.wYear << 16) + (static_cast<uint8_t>(local_time.wMonth) << 8) + static_cast<uint8_t>(local_time.wDay);
	t3 = LicensingManager::GetCurrentDate();
	ASSERT_TRUE((t1 == t2) || (t3 = t2));
}
#endif
#endif

/**
 * StringManagerTests
 */

class StringNode
{
	std::vector<uint8_t>	m_vString;

public:
	StringNode(const uint8_t *pBuff, size_t nCount, DWORD dwKey)
	{
		for (size_t i = 0; i < nCount; i++)
		{
			uint8_t x = static_cast<uint8_t>(_rotl32(dwKey, static_cast<int>(i)) + i);
			m_vString.push_back(pBuff[i] ^ x);
		}
	}

	void WriteHeader(uint32_t id, Data *s, uint32_t offsetToData) 
	{
		STRING_ENTRY sDataEntry = STRING_ENTRY();
		sDataEntry.Size = static_cast<DWORD>(m_vString.size());
		sDataEntry.Id = id;
		sDataEntry.OffsetToData = offsetToData;
		s->PushBuff(reinterpret_cast<const uint8_t *>(&sDataEntry), sizeof(sDataEntry));
	}

	void WriteData(Data *s) 
	{
		s->PushBuff(&m_vString[0], m_vString.size());
	}

	size_t size() const { return m_vString.size(); }
};

class StringInput
{
	std::vector<uint8_t>	m_vData;
	std::vector<StringNode>	m_vStrings;
	uint32_t m_dwKey;
	RC5Key m_bKey;

	void Compile()
	{
		Data s;
		s.PushDWord((uint32_t)m_vStrings.size());
		uint32_t offsetToData = (uint32_t)(s.size() + m_vStrings.size() * sizeof(STRING_ENTRY));
		for (size_t i = 0; i < m_vStrings.size(); i++)
		{
			m_vStrings[i].WriteHeader((uint32_t)i, &s, offsetToData);
			offsetToData += (uint32_t)m_vStrings[i].size();
		}
		size_t nTreeSize = s.size();
		for (size_t i = 0; i < nTreeSize; i += sizeof(uint32_t)) {
			s.WriteDWord(i, s.ReadDWord(i) ^ m_dwKey);
		}

		for (size_t i = 0; i < m_vStrings.size(); i++)
			m_vStrings[i].WriteData(&s);

		m_vData.insert(m_vData.end(), s.data(), s.data() + s.size());
	}

	void PushBuff(const uint8_t *pBuff, size_t nCount)
	{
		StringNode s(pBuff, nCount, m_dwKey);
		m_vStrings.push_back(s);

		m_vData.clear();
	}

public:
	StringInput(const uint8_t *pKey) 
		: m_dwKey(0)
	{
		memset(&m_bKey, 0, sizeof(m_bKey));
		if (pKey)
		{
			memcpy(&m_bKey.Value[0], pKey, sizeof(m_bKey.Value));
			memcpy(&m_dwKey, pKey, sizeof(m_dwKey));
		}
	}

	void Add(const wchar_t *lpString)
	{
		PushBuff((uint8_t *)lpString, (wcslen(lpString) + 1)*sizeof(wchar_t));
	}

	void Add(const char *lpString)
	{
		PushBuff((uint8_t *)lpString, strlen(lpString) + 1);
	}

	StringNode &GetItem(size_t nIndex) { return m_vStrings[nIndex]; }

	operator const uint8_t *()
	{
		if (m_vData.empty()) Compile();
		return &m_vData[0];
	}

	size_t size() 
	{ 
		return m_vData.size();
	}
};

TEST(StringManagerTests, InitDone)
{
	uint8_t key[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	uint8_t data[] = {0, 0, 0, 0};

	ASSERT_NO_THROW(StringManager(data, 0, key));
}

TEST(StringManagerTests, BuildAGoodTree)
{
	StringInput string_input(NULL);
	wchar_t s1[] = L"String1";
	char s2[] = "String2";
	string_input.Add(s1);
	string_input.Add(s2);

	EXPECT_EQ(sizeof(s1), string_input.GetItem(0).size());
	EXPECT_EQ(sizeof(s2), string_input.GetItem(1).size());
}

TEST(StringManagerTests, DecryptStrings)
{
	uint8_t key[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	StringInput string_input(key);
	wchar_t s1[] = L"String1";
	char s2[] = "String2";
	string_input.Add(s1);
	string_input.Add(s2);
	const uint8_t *p = string_input;

	StringManager string_manager(p, (HMODULE)p, key);

	const void *p3 = string_manager.DecryptString(p + 2);
	EXPECT_EQ((const void *)(p + 2), p3);

	const void *p2 = string_manager.DecryptString(p + 1);
	EXPECT_STREQ((char *)p2, s2);

	const void *p1 = string_manager.DecryptString(p + 0); // 1st decrypt
	EXPECT_STREQ((wchar_t *)p1, s1);

	EXPECT_NE(p1, p2) << "different strings cannot live at the same address";
	EXPECT_EQ(p1, string_manager.DecryptString(p + 0)) << "should not allocate the same string twice"; // 2nd decrypt

	EXPECT_TRUE(string_manager.FreeString(p1)); // rollback 1st decrypt, string alive
	EXPECT_STREQ((wchar_t *)p1, s1);
	EXPECT_TRUE(string_manager.FreeString(p1)); // rollback 2nd decrypt, string dead
	EXPECT_STRNE((wchar_t *)p1, s1); // maybe dangerous (don't know about conditions), delete this line if crashed here
	EXPECT_FALSE(string_manager.FreeString(p1)); // there were no 3rd decrypt to rollback

	EXPECT_FALSE(string_manager.FreeString(reinterpret_cast<const void *>(0x12)));
	EXPECT_FALSE(string_manager.FreeString(NULL));
	EXPECT_FALSE(string_manager.FreeString(reinterpret_cast<const void *>("hello, world!")));
}

/**
 * HardwareIDTests
 */

TEST(HardwareIDTests, GetCurrentHWID)
{
	char buff[1024];
	HardwareID hardware_id;
	ASSERT_TRUE(hardware_id.GetCurrent(buff, sizeof(buff)) != 0);
}

TEST(HardwareIDTests, Compare)
{
	uint32_t cur[100], test[100];
	HardwareID hardware_id;
	size_t size = 0;
	ASSERT_NO_THROW(size = hardware_id.Copy(reinterpret_cast<uint8_t *>(cur), sizeof(cur)) / sizeof(uint32_t));
	ASSERT_GT(size, 1u);

	// change one id
	for (size_t i = 0; i < size; i++) {
		test[i] = cur[i];
		switch (test[i] & 3) {
			case 3:	
				test[i] ^= 4;
				break;
		}
	}
	EXPECT_TRUE(hardware_id.IsCorrect(reinterpret_cast<uint8_t *>(test), size * sizeof(uint32_t)));

	// change two ids
	for (size_t i = 0; i < size; i++) {
		test[i] = cur[i];
		switch (test[i] & 3) {
			case 2:	
			case 3:	
				test[i] ^= 4;
				break;
		}
	}
	EXPECT_FALSE(hardware_id.IsCorrect(reinterpret_cast<uint8_t *>(test), size * sizeof(uint32_t)));

	// change three ids
	for (size_t i = 0; i < size; i++) {
		test[i] = cur[i];
		switch (test[i] & 3) {
			case 1:	
			case 2:	
			case 3:	
				test[i] ^= 4;
				break;
		}
	}
	EXPECT_FALSE(hardware_id.IsCorrect(reinterpret_cast<uint8_t *>(test), size * sizeof(uint32_t)));

	// change cpu`s id
	for (size_t i = 0; i < size; i++) {
		test[i] = cur[i];
		switch (test[i] & 3) {
			case 0:	
				test[i] ^= 4;
				break;
		}
	}
	EXPECT_FALSE(hardware_id.IsCorrect(reinterpret_cast<uint8_t *>(test), size * sizeof(uint32_t)));
}

#ifndef VMP_GNU
/**
 * RegistryManagerTests
 */
#include "../runtime/registry_manager.h"

TEST(RegistryManagerTests, UnicodeString)
{
	UnicodeString str(L"123");
	ASSERT_EQ(str.size(), 3);
	ASSERT_NO_THROW(str = str + L'4');
	ASSERT_EQ(str.size(), 4);
	ASSERT_NO_THROW(str = str + L"56");
	ASSERT_EQ(str.size(), 6);
	ASSERT_EQ(wcscmp(L"123456", str.c_str()), 0);
}

TEST(RegistryManagerTests, RegistryTree)
{
	RegistryKey root;
	RegistryKey *key;
	RegistryValue *value;

	EXPECT_TRUE(root.AddKey(L"\\registry\\machine\\classes", true, NULL) != NULL);
	EXPECT_TRUE(root.GetKey(L"\\registry\\machine") != NULL);
	EXPECT_TRUE(root.GetKey(L"\\registry\\123") == NULL);
	EXPECT_TRUE(root.AddKey(L"\\registry\\machine\\objects", false, NULL) != NULL);
	EXPECT_TRUE(root.AddKey(L"\\registry\\\\machine", true, NULL) == NULL);
	ASSERT_NO_THROW(key = root.GetKey(L"\\registry\\machine"));
	EXPECT_TRUE(key->is_real());
	EXPECT_EQ(key->keys_size(), 2);
	EXPECT_TRUE(key->key(0)->is_real());
	EXPECT_FALSE(key->key(1)->is_real());
	ASSERT_NO_THROW(key->SetValue(L"Value", REG_SZ, L"123456", 14));
	EXPECT_EQ(key->values_size(), 1);
	EXPECT_FALSE(key->GetValue(L""));
	ASSERT_NO_THROW(value = key->GetValue(L"Value"));
	EXPECT_TRUE(value != NULL);
	EXPECT_EQ(value->size(), 14);
	EXPECT_EQ(value->type(), REG_SZ);
}

/**
* FileManagerTests
*/

TEST(FileManagerTests, Path)
{
	Path path(L"1\\2\\3\\4");
	UnicodeString str = path.Combine(L"1.txt");
	ASSERT_EQ(wcscmp(L"1\\2\\3\\4\\1.txt", str.c_str()), 0);
	str = path.Combine(L"..\\1.txt");
	ASSERT_EQ(wcscmp(L"1\\2\\3\\1.txt", str.c_str()), 0);
	str = path.Combine(L"..\\5\\..\\6\\1.txt");
	ASSERT_EQ(wcscmp(L"1\\2\\3\\6\\1.txt", str.c_str()), 0);
}

#endif
