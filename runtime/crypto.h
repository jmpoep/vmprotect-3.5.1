#ifndef CRYPTO_H
#define CRYPTO_H

#include "common.h"

uint32_t rand32();
uint64_t rand64();

#ifdef VMP_GNU

inline uint8_t _rotl8(uint8_t value, int shift)
{
	__asm__ __volatile__ ("rolb %%cl, %0"
							: "=r"(value)
							: "0"(value), "c"(shift)
							);
	return value;
}

inline uint16_t _rotl16(uint16_t value, int shift)
{
	__asm__ __volatile__ ("rolw %%cl, %0"
							: "=r"(value)
							: "0"(value), "c"(shift)
							);
	return value;
}

inline uint32_t _rotl32(uint32_t value, int shift)
{
	__asm__ __volatile__ ("roll %%cl, %0"
							: "=r"(value)
							: "0"(value), "c"(shift)
							);
	return value;
}

inline uint64_t _rotl64(uint64_t value, int shift)
{
	return (value << shift) | (value >> (sizeof(value) * 8 - shift));
}

inline uint8_t _rotr8(uint8_t value, int shift)
{
	__asm__ __volatile__ ("rorb %%cl, %0"
							: "=r"(value)
							: "0"(value), "c"(shift)
							);
	return value;
}

inline uint16_t _rotr16(uint16_t value, int shift)
{
	__asm__ __volatile__ ("rorw %%cl, %0"
							: "=r"(value)
							: "0"(value), "c"(shift)
							);
	return value;
}

inline uint32_t _rotr32(uint32_t value, int shift)
{
	__asm__ __volatile__ ("rorl %%cl, %0"
							: "=r"(value)
							: "0"(value), "c"(shift)
							);
	return value;
}

inline uint64_t _rotr64(uint64_t value, int shift)
{
	return (value >> shift) | (value << (sizeof(value) * 8 - shift));
}

inline uint64_t __rdtsc()
{
	uint32_t hi, lo;
	__asm__ __volatile__ ("rdtsc" 
							: "=a"(lo), "=d"(hi)
							);
	return static_cast<uint64_t>(lo) | static_cast<uint64_t>(hi) << 32;
}


inline void __cpuid(int regs[4], uint32_t value)
{
	__asm__ __volatile__ ("cpuid" 
							: "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
							: "a"(value)
							);
}

inline void __movsb(void *d, const void *s, size_t n) {
	asm volatile ("rep movsb"
		: "=D" (d),
		"=S" (s),
		"=c" (n)
		: "0" (d),
		"1" (s),
		"2" (n)
		: "memory");
}

#ifdef __APPLE__
inline uint16_t __builtin_bswap16(uint16_t value)
{
	__asm__ __volatile__ ("rorw $8, %0"
							: "+r"(value)
							);
	return value;
}
#endif

#else
#define _rotl32 _lrotl
#define _rotr32 _lrotr
#define __builtin_bswap16 _byteswap_ushort
#define __builtin_bswap32 _byteswap_ulong
#define __builtin_bswap64 _byteswap_uint64
inline unsigned long __builtin_ctz(unsigned int x) { unsigned long r; _BitScanForward(&r, x); return r; }

#if defined(WIN_DRIVER) && (WDK_NTDDI_VERSION <= NTDDI_WIN7)
#ifndef WIN64
#ifdef __cplusplus
extern "C" {
#endif
VOID
__movsb(
	__out_ecount_full(Count) PUCHAR Destination,
	__in_ecount(Count) UCHAR const *Source,
	__in SIZE_T Count
);
#ifdef __cplusplus
}
#endif
#endif
#endif

#endif 

inline uint64_t ByteToInt64(uint8_t value)
{
	return static_cast<int64_t>(static_cast<int8_t>(value));
}

inline uint64_t WordToInt64(uint16_t value)
{
	return static_cast<int64_t>(static_cast<int16_t>(value));
}

inline uint64_t DWordToInt64(uint32_t value)
{
	return static_cast<int64_t>(static_cast<int32_t>(value));
}

struct RC5Key {
	uint8_t Value[8];
#ifndef RUNTIME
	uint32_t P;
	uint32_t Q;
#endif
	RC5Key() {} //-V730 мусор тоже годится
#ifdef RUNTIME
	RC5Key(const uint8_t *key)
	{
		memcpy(Value, key, sizeof(Value));
	}
#endif
	void Create();
};

class CipherRC5
{
public:
	CipherRC5(const RC5Key &key);
	void Encrypt(uint8_t *buff, size_t count) const;
	void Decrypt(const uint8_t *in, uint8_t *out, size_t count) const;
	void Encrypt(const uint32_t *in, uint32_t *out) const;
	void Decrypt(const uint32_t *in, uint32_t *out) const;
private:
	enum {
		w = 32, // u32 size in bits
		r = 15, // number of rounds
		b = 8, // number of bytes in key
		c = 8 * b / w, // 16 - number u32s in key = ceil(8*b/w)
		t = 2 * (r + 1), // 34 - size of table S = 2*(r+1) u32s
	};
	uint32_t S[t]; // expanded key table
#ifdef RUNTIME
	enum {
		P = FACE_RC5_P, 
		Q = FACE_RC5_Q
	};
#else
	uint32_t P;
	uint32_t Q;
#endif
};

class CryptoContainer;

typedef unsigned short BignumInt;
typedef unsigned long BignumDblInt;
typedef BignumInt *Bignum;

class BigNumber
{
public:
	BigNumber();
	BigNumber(const BigNumber &src);
	BigNumber(const uint8_t *data, size_t size, bool inverse_order = false);
	~BigNumber();
	BigNumber modpow(const BigNumber &exp, const BigNumber &mod) const;
	CryptoContainer *modpow(const CryptoContainer &source, size_t exp_offset, size_t exp_size, size_t mod_offset, size_t mod_size) const;
	size_t size() const;
	bool operator < (const BigNumber &b) const;
	uint8_t operator [] (size_t index) const;
	BignumInt data(size_t index) const { return bignum_get_word(data_ + index); }
private:
	
	// no assignment op
	BigNumber &operator =(const BigNumber &);

	BigNumber(Bignum data, const BignumInt *salt);
	BignumInt bignum_get_word(Bignum b) const;
	void bignum_set_word(Bignum b, BignumInt value) const;
	void init(size_t length);
	void internal_mul(BignumInt *a, BignumInt *b, BignumInt *c, int len) const;
	void internal_add_shifted(BignumInt *number, unsigned n, int shift) const;
	void internal_mod(BignumInt *a, int alen, BignumInt *m, int mlen, BignumInt *quot, int qshift) const;
	uint8_t bignum_byte(Bignum bn, size_t i) const;
	int bignum_cmp(const BigNumber &b) const;
	enum {
		BIGNUM_INT_MASK = 0xFFFFU,
		BIGNUM_TOP_BIT = 0x8000U,
		BIGNUM_INT_BITS = 16,
		BIGNUM_INT_BYTES = (BIGNUM_INT_BITS / 8)
	};

	Bignum data_;
#ifdef RUNTIME
	BignumInt salt_[20 / BIGNUM_INT_BYTES];
#endif
};

enum {
	ATL_BASE64_FLAG_NONE = 0,
	ATL_BASE64_FLAG_NOPAD, 
	ATL_BASE64_FLAG_NOCRLF
};
bool Base64Encode(const uint8_t *src, size_t src_len, char *dst, size_t &dst_len);
bool Base64Decode(const char *src, size_t src_len, uint8_t *dst, size_t &dst_len);
size_t Base64EncodeGetRequiredLength(size_t src_len);

class CryptoContainer
{
public:
	CryptoContainer(uint8_t *data, size_t size, const RC5Key &key);
	CryptoContainer(size_t size, const RC5Key &key);
	CryptoContainer(const BigNumber &bn);
	~CryptoContainer();
	const uint8_t *data() const { return reinterpret_cast<const uint8_t *>(data_); }
	size_t size() const { return size_; }
	uint32_t GetDWord(size_t pos) const;
	uint16_t GetWord(size_t pos) const;
	uint8_t GetByte(size_t pos) const;
	uint64_t GetQWord(size_t pos) const;
	bool SetDWord(size_t pos, uint32_t value) const;
	bool SetWord(size_t pos, uint16_t value) const;
	bool SetByte(size_t pos, uint8_t value) const;
	void UTF8ToUnicode(size_t offset, size_t len, VMP_WCHAR *dest, size_t dest_size) const;
private:
	#define RC5_BLOCK_SIZE 8
	bool EncryptValue(size_t pos, uint8_t *value, size_t value_size) const;
	bool DecryptValue(size_t pos, uint8_t *value, size_t value_size) const;
	bool is_own_data_;
	uint32_t *data_;
	size_t size_;
	CipherRC5 *cipher_;

	// no copy ctr or assignment op
	CryptoContainer(const CryptoContainer &);
	CryptoContainer &operator =(const CryptoContainer &);
};

class SHA1
{
public:
	SHA1();
	void Reset();
	void Input(const uint8_t *data, size_t size);
	void Input(const CryptoContainer &data, size_t offset, size_t size);
	const uint8_t *Result();
	size_t ResultSize() const { return sizeof(digest_); }
	bool operator ==(SHA1 &other) { return memcmp(Result(), other.Result(), ResultSize()) == 0; }
private:
	void ProcessMessageBlock();
	void PadMessage();
	bool computed_;
	size_t message_block_index_;
	uint8_t message_block_[64];
	uint32_t length_high_;
	uint32_t length_low_;
	uint32_t hash_[5];
	uint32_t digest_[5];
};

static uint32_t crc32_table[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

#ifdef RUNTIME
#ifdef VMP_GNU
EXPORT_API uint32_t WINAPI CalcCRC(const void * key, size_t len) __asm__ ("CalcCRC");
#else
EXPORT_API uint32_t WINAPI CalcCRC(const void * key, size_t len);
#endif
#else
uint32_t CalcCRC(const void * key, size_t len);
#endif

class CRCValueCryptor
{
public:
#ifdef RUNTIME
	FORCE_INLINE CRCValueCryptor() : key_(FACE_CRC_INFO_SALT) {}
	FORCE_INLINE uint32_t Decrypt(uint32_t value)
	{
		uint32_t res = value ^ key_;
		key_ = _rotl32(key_, 7) ^ res;
		return res;
	}
#else
	CRCValueCryptor(uint32_t key) : key_(key) {}
	uint32_t Encrypt(uint32_t value)
	{
		uint32_t old_key = key_;
		key_ = _rotl32(key_, 7) ^ value;
		return value ^ old_key;
	}
#endif
private:
	uint32_t key_;
};

#endif