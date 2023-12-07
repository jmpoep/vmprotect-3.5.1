#include "common.h"
#include "crypto.h"

uint32_t rand32()
{
	uint32_t v1 = rand();
	uint32_t v2 = rand();
	return (v1 << 16) | v2;
}

uint64_t rand64()
{
	uint64_t v1 = rand32();
	uint64_t v2 = rand32();
	return (v1 << 32) | v2;
}

/**
 * CipherRC5
 */

CipherRC5::CipherRC5(const RC5Key &key)
{
#ifndef RUNTIME
	P = key.P;
	Q = key.Q;
#endif

	uint32_t i, j, k, u = w / 8, A, B, L[c];

	for (i = b - 1, L[c - 1] = 0; i != (uint32_t)-1; i--) //-V621
		L[i / u] = (L[i / u] << 8) + key.Value[i];

	for (S[0] = P, i = 1; i < t; i++)
		S[i] = S[i - 1] + Q;
	for (A = B = i = j = k = 0; k < 3 * t; k++, i = (i + 1) % t, j = (j + 1) % c) /* 3*t > 3*c */
	{
		A = S[i] = _rotl32(S[i] + (A + B), 3);
		B = L[j] = _rotl32(L[j] + (A + B), (A + B));
	}
}

void CipherRC5::Encrypt(const uint32_t *in, uint32_t *out) const
{
	uint32_t i, A = in[0] + S[0], B = in[1] + S[1];
	for (i = 1; i <= r; i++) 
	{
		A = _rotl32(A ^ B, B) + S[2 * i];
		B = _rotl32(B ^ A, A) + S[2 * i + 1];
	}
	out[0] = A;
	out[1] = B;
}

void CipherRC5::Decrypt(const uint32_t *in, uint32_t *out) const
{
	uint32_t i, B = in[1], A = in[0];
	for (i = r; i > 0; i--)	{
		B = _rotr32(B - S[2 * i + 1], A) ^ A;
		A = _rotr32(A - S[2 * i], B) ^ B;
	}
	out[1] = B - S[1];
	out[0] = A - S[0];
}

void CipherRC5::Encrypt(uint8_t *buff, size_t count) const
{
	for (size_t i = 0; i < count; i += 8) {
		Encrypt(reinterpret_cast<uint32_t *>(buff + i), reinterpret_cast<uint32_t *>(buff + i));
	}
}

void CipherRC5::Decrypt(const uint8_t *in, uint8_t *out, size_t count) const
{
	for (size_t i = 0; i < count; i += 8) {
		Decrypt(reinterpret_cast<const uint32_t *>(in + i), reinterpret_cast<uint32_t *>(out + i));
	}
}

/**
 * RC5Key
 */

void RC5Key::Create()
{
#ifdef RUNTIME
	uint64_t key = __rdtsc();
	key ^= ~key << 32;
	uint8_t *p = reinterpret_cast<uint8_t *>(&key);
	for (size_t i = 0; i < _countof(Value); i++) {
		Value[i] = p[i];
	}
#else
	for (size_t i = 0; i < _countof(Value); i++) {
		Value[i] = rand();
	}
	P = rand32();
	Q = rand32();
#endif
}

/**
 * BigNumber
 */

BigNumber::BigNumber()
{
	init(0);
}

BigNumber::BigNumber(const uint8_t *data, size_t size, bool inverse_order)
{
	size_t w, i;

	w = (size + BIGNUM_INT_BYTES - 1) / BIGNUM_INT_BYTES; /* bytes->words */
	init(w);

	if (inverse_order) {
		if (size)
			memcpy(&data_[1], data, size);
	} else {
		for (i = size; i--;) {
			uint8_t byte = *data++;
			bignum_set_word(data_ + 1 + i / BIGNUM_INT_BYTES, bignum_get_word(data_ + 1 + i / BIGNUM_INT_BYTES) | byte << (8*i % BIGNUM_INT_BITS));
		}
	}

	while (bignum_get_word(data_ + 0) > 1 && bignum_get_word(data_ + bignum_get_word(data_ + 0)) == 0)
		bignum_set_word(data_ + 0, bignum_get_word(data_ + 0) - 1);
}

BigNumber::BigNumber(const BigNumber &src)
{
	size_t size = src.data(0);
	init(size);
	for (size_t i = 1; i <= size; i++) {
		bignum_set_word(data_ + i, src.data(i));
	}
}

BigNumber::BigNumber(Bignum data, const BignumInt *salt)
{
	data_ = data;
#ifdef RUNTIME
	for (size_t i = 0; i < _countof(salt_); i++) {
		salt_[i] = salt[i];
	}
#endif
}

BigNumber::~BigNumber()
{
	delete [] data_;
}

bool BigNumber::operator < (const BigNumber &b) const
{
	return (bignum_cmp(b) < 0);
}

size_t BigNumber::size() const
{ 
	return data(0) * BIGNUM_INT_BYTES;
}

uint8_t BigNumber::operator [] (size_t index) const
{
	return bignum_byte(data_, index);
}

#define MUL_WORD(w1, w2) ((BignumDblInt)w1 * w2)
#define DIVMOD_WORD(q, r, hi, lo, w) { \
	BignumDblInt n = (((BignumDblInt)hi) << BIGNUM_INT_BITS) | lo; \
		q = n / w; \
		r = n % w; \
}

BignumInt BigNumber::bignum_get_word(Bignum b) const
{
#ifdef RUNTIME
	size_t addr = reinterpret_cast<size_t>(b);
	size_t offset = (addr + (addr >> 7)) % _countof(salt_);
	size_t salt = salt_[offset] + 0x73 + (addr >> 4);
	return static_cast<BignumInt>((*b ^ salt) + addr);
#else
	return *b;
#endif
}

void BigNumber::bignum_set_word(Bignum b, BignumInt value) const
{
#ifdef RUNTIME
	size_t addr = reinterpret_cast<size_t>(b);
	size_t offset = (addr + (addr >> 7)) % _countof(salt_);
	size_t salt = salt_[offset] + 0x73 + (addr >> 4);
	*b = static_cast<BignumInt>((value - addr) ^ salt);
#else
	*b = value;
#endif
}

void BigNumber::init(size_t length)
{
	data_ = new BignumInt[length + 1];

#ifdef RUNTIME
	{
		uint64_t rand = __rdtsc();
		SHA1 hash;
		hash.Input(reinterpret_cast<const uint8_t *>(&rand), sizeof(rand));
		hash.Input(reinterpret_cast<const uint8_t *>(&data_), sizeof(data_));

		const BignumInt *p = reinterpret_cast<const BignumInt *>(hash.Result());
		for (size_t i = 0; i < _countof(salt_); i++) {
			salt_[i] = p[i];
		}
	}
#endif

	bignum_set_word(data_, (BignumInt)length);
	for (size_t i = 1; i <= length; i++) 
		bignum_set_word(data_ + i, 0);
}

void BigNumber::internal_mul(BignumInt *a, BignumInt *b, BignumInt *c, int len) const
{
	int i, j;
	BignumDblInt t;

	for (j = 0; j < 2 * len; j++)
		bignum_set_word(c + j, 0);

	for (i = len - 1; i >= 0; i--) {
		t = 0;
		for (j = len - 1; j >= 0; j--) {
			t += MUL_WORD(bignum_get_word(a + i), (BignumDblInt) bignum_get_word(b + j));
			t += (BignumDblInt) bignum_get_word(c + i + j + 1);
			bignum_set_word(c + i + j + 1, (BignumInt) t);
			t = t >> BIGNUM_INT_BITS;
		}
		bignum_set_word(c + i, (BignumInt) t);
	}
}

void BigNumber::internal_add_shifted(BignumInt *number, unsigned n, int shift) const
{
	int word = 1 + (shift / BIGNUM_INT_BITS);
	int bshift = shift % BIGNUM_INT_BITS;
	BignumDblInt addend;

	addend = (BignumDblInt)n << bshift;

	while (addend) {
		addend += bignum_get_word(number + word);
		bignum_set_word(number + word, (BignumInt) addend & BIGNUM_INT_MASK);
		addend >>= BIGNUM_INT_BITS;
		word++;
	}
}

void BigNumber::internal_mod(BignumInt *a, int alen,
		BignumInt *m, int mlen,
		BignumInt *quot, int qshift) const
{
	BignumInt m0, m1;
	unsigned int h;
	int i, k;

	m0 = bignum_get_word(m + 0);
	m1 = (mlen > 1) ? bignum_get_word(m + 1) : 0;

	for (i = 0; i <= alen - mlen; i++) {
		BignumDblInt t;
		unsigned int q, r, c, ai1;

		if (i == 0) {
			h = 0;
		} else {
			h = bignum_get_word(a + i - 1);
			bignum_set_word(a + i - 1, 0);
		}

		if (i == alen - 1)
			ai1 = 0;
		else
			ai1 = bignum_get_word(a + i + 1);

		/* Find q = h:a[i] / m0 */
		DIVMOD_WORD(q, r, h, bignum_get_word(a + i), m0);

		/* Refine our estimate of q by looking at h:a[i]:a[i+1] / m0:m1 */
		t = MUL_WORD(m1, q);
		if (t > ((BignumDblInt) r << BIGNUM_INT_BITS) + ai1) {
			q--;
			t -= m1;
			r = (r + m0) & BIGNUM_INT_MASK;     /* overflow? */
			if (r >= (BignumDblInt) m0 &&
					t > ((BignumDblInt) r << BIGNUM_INT_BITS) + ai1) q--;
		}

		/* Subtract q * m from a[i...] */
		c = 0;
		for (k = mlen - 1; k >= 0; k--) {
			t = MUL_WORD(q, bignum_get_word(m + k));
			t += c;
			c = t >> BIGNUM_INT_BITS;
			if ((BignumInt) t > bignum_get_word(a + i + k))
				c++;
			bignum_set_word(a + i + k, bignum_get_word(a + i + k) - (BignumInt) t);
		}

		/* Add back m in case of borrow */
		if (c != h) {
			t = 0;
			for (k = mlen - 1; k >= 0; k--) {
				t += bignum_get_word(m + k);
				t += bignum_get_word(a + i + k);
				bignum_set_word(a + i + k, (BignumInt) t);
				t = t >> BIGNUM_INT_BITS;
			}
			q--;
		}
		if (quot)
			internal_add_shifted(quot, q, qshift + BIGNUM_INT_BITS * (alen - mlen - i));
	}
}

BigNumber BigNumber::modpow(const BigNumber &exp, const BigNumber &mod) const
{
	BignumInt *a, *b, *n, *m;
	int mshift;
	int i,j,mlen;
	Bignum result;

	/* Allocate m of size mlen, copy mod to m */
	/* We use big endian internally */
	mlen = mod.data(0);
#ifdef WIN_DRIVER
	NT_ASSERT(mlen > 0);
#else
	assert(mlen > 0);
#endif
	if(mlen <= 0) return BigNumber();

	m = new BignumInt[mlen];
	for (j = 0; j < mlen; j++)
		bignum_set_word(m + j, mod.data(mlen - j));

	/* Shift m left to make msb bit set */
	for (mshift = 0; mshift < BIGNUM_INT_BITS-1; mshift++)
		if ((bignum_get_word(m + 0) << mshift) & BIGNUM_TOP_BIT)
			break;
	if (mshift) {
		for (i = 0; i < mlen - 1; i++)
			m[i] = (bignum_get_word(m + i) << mshift) | (bignum_get_word(m + i + 1) >> (BIGNUM_INT_BITS - mshift));
		bignum_set_word(m + mlen - 1, bignum_get_word(m + mlen - 1) << mshift);
	}

	/* Allocate n of size mlen, copy base to n */
	int blen = data(0);
	n = new BignumInt[mlen];
	i = mlen - blen;
	for (j = 0; j < i; j++)
		bignum_set_word(n + j, 0);
	for (j = 0; j < blen; j++)
		bignum_set_word(n + i + j, data(blen - j));

	/* Allocate a and b of size 2*mlen. Set a = 1 */
	a = new BignumInt[2 * mlen];
	b = new BignumInt[2 * mlen];
	for (i = 0; i < 2 * mlen; i++)
		bignum_set_word(a + i, 0);
	bignum_set_word(a + 2 * mlen - 1, 1);

	/* Skip leading zero bits of exp. */
	i = 0;
	j = BIGNUM_INT_BITS-1;
	int elen = exp.data(0);
	while (i < elen && (exp.data(elen - i) & (1 << j)) == 0) {
		j--;
		if (j < 0) {
			i++;
			j = BIGNUM_INT_BITS-1;
		}
	}

	/* Main computation */
	while (i < elen) {
		while (j >= 0) {
			internal_mul(a + mlen, a + mlen, b, mlen);
			internal_mod(b, mlen * 2, m, mlen, NULL, 0);
			if ((exp.data(elen - i) & (1 << j)) != 0) {
				internal_mul(b + mlen, n, a, mlen);
				internal_mod(a, mlen * 2, m, mlen, NULL, 0);
			} else {
				BignumInt *t;
				t = a;
				a = b;
				b = t;
			}
			j--;
		}
		i++;
		j = BIGNUM_INT_BITS-1;
	}

	/* Fixup result in case the modulus was shifted */
	if (mshift) {
		for (i = mlen - 1; i < 2 * mlen - 1; i++)
			a[i] = (bignum_get_word(a + i) << mshift) | (bignum_get_word(a + i + 1) >> (BIGNUM_INT_BITS - mshift));
		bignum_set_word(a + 2 * mlen - 1, bignum_get_word(a + 2 * mlen - 1) << mshift);
		internal_mod(a, mlen * 2, m, mlen, NULL, 0);
		for (i = 2 * mlen - 1; i >= mlen; i--)
			bignum_set_word(a + i, (bignum_get_word(a + i) >> mshift) | (bignum_get_word(a + i - 1) << (BIGNUM_INT_BITS - mshift)));
	}

	/* Copy result to buffer */
	result = new BignumInt[mlen + 1];
	bignum_set_word(result, (BignumInt)mlen);
	for (i = 0; i < mlen; i++)
		bignum_set_word(result + mlen - i, bignum_get_word(a + i + mlen));
	while (bignum_get_word(result + 0) > 1 && bignum_get_word(result + bignum_get_word(result + 0)) == 0)
		bignum_set_word(result + 0, bignum_get_word(result + 0) - 1);

	/* Free temporary arrays */
	delete [] a;
	delete [] b;
	delete [] m;
	delete [] n;

	return BigNumber(result, 
#ifdef RUNTIME
		salt_
#else
		NULL
#endif
		);
}

CryptoContainer *BigNumber::modpow(const CryptoContainer &source, size_t exp_offset, size_t exp_size, size_t mod_offset, size_t mod_size) const
{
	BignumInt *a, *b, *n, *m, *e, *mem, *next;
	int mshift;
	int i, j, mlen, elen, blen;
	Bignum result;
	size_t k;
	size_t arrays[5];

	mlen = (int)(mod_size + BIGNUM_INT_BYTES - 1) / BIGNUM_INT_BYTES;
	elen = (int)(exp_size + BIGNUM_INT_BYTES - 1) / BIGNUM_INT_BYTES;

#ifdef WIN_DRIVER
	NT_ASSERT(mlen > 0);
#else
	assert(mlen > 0);
#endif
	if (mlen <= 0 || data(0) > mlen) return NULL;

	for (k = 0; k < _countof(arrays); k++) {
		arrays[k] = k;
	}
	for (k = 0; k < _countof(arrays); k++) {
		uint64_t rand = __rdtsc();
		size_t r = static_cast<size_t>(rand ^ (rand >> 32)) % _countof(arrays);
		size_t t = arrays[k];
		arrays[k] = arrays[r];
		arrays[r] = t;
	}

	// Allocate memory for temporary arrays
	mem = new BignumInt[mlen + mlen + mlen * 2 + mlen * 2 + elen];
	next = mem;
	for (k = 0; k < _countof(arrays); k++) {
		switch (arrays[k]) {
		case 0:
			/* Allocate m of size mlen, copy mod to m */
			m = next;
			next = m + mlen;

			for (j = 0; j < mlen; j++) {
				BignumInt value = 0;
				for (i = 0; i < BIGNUM_INT_BYTES; i++) {
					value = (value << 8) | source.GetByte(mod_offset++);
				}
				bignum_set_word(m + j, value);
			}

			/* Shift m left to make msb bit set */
			for (mshift = 0; mshift < BIGNUM_INT_BITS - 1; mshift++)
				if ((bignum_get_word(m + 0) << mshift) & BIGNUM_TOP_BIT)
					break;
			if (mshift) {
				for (i = 0; i < mlen - 1; i++)
					m[i] = (bignum_get_word(m + i) << mshift) | (bignum_get_word(m + i + 1) >> (BIGNUM_INT_BITS - mshift));
				bignum_set_word(m + mlen - 1, bignum_get_word(m + mlen - 1) << mshift);
			}
			break;

		case 1:
			/* Allocate e of size elen, copy exp to e */
			e = next;
			next = next + elen;

			for (j = 0; j < elen; j++) {
				BignumInt value = 0;
				for (i = 0; i < BIGNUM_INT_BYTES; i++) {
					value = (value << 8) | source.GetByte(exp_offset++);
				}
				bignum_set_word(e + j, value);
			}
			break;

		case 2:
			/* Allocate n of size mlen, copy base to n */
			n = next;
			next = next + mlen;

			blen = data(0);
			i = mlen - blen;
			for (j = 0; j < i; j++)
				bignum_set_word(n + j, 0);
			for (j = 0; j < blen; j++)
				bignum_set_word(n + i + j, data(blen - j));
			break;

		case 3:
			/* Allocate a of size 2*mlen. Set a = 1 */
			a = next;
			next = next + 2 * mlen;

			for (i = 0; i < 2 * mlen; i++)
				bignum_set_word(a + i, 0);
			bignum_set_word(a + 2 * mlen - 1, 1);
			break;

		case 4:
			/* Allocate b of size 2*mlen */
			b = next;
			next = next + 2 * mlen;
			break;
		}
	}

	/* Skip leading zero bits of exp. */
	i = 0;
	j = BIGNUM_INT_BITS - 1;
	while (i < elen && (bignum_get_word(e + i) & (1 << j)) == 0) {
		j--;
		if (j < 0) {
			i++;
			j = BIGNUM_INT_BITS - 1;
		}
	}

	/* Main computation */
	while (i < elen) {
		while (j >= 0) {
			internal_mul(a + mlen, a + mlen, b, mlen);
			internal_mod(b, mlen * 2, m, mlen, NULL, 0);
			if ((bignum_get_word(e + i) & (1 << j)) != 0) {
				internal_mul(b + mlen, n, a, mlen);
				internal_mod(a, mlen * 2, m, mlen, NULL, 0);
			}
			else {
				BignumInt *t;
				t = a;
				a = b;
				b = t;
			}
			j--;
		}
		i++;
		j = BIGNUM_INT_BITS - 1;
	}

	/* Fixup result in case the modulus was shifted */
	if (mshift) {
		for (i = mlen - 1; i < 2 * mlen - 1; i++)
			a[i] = (bignum_get_word(a + i) << mshift) | (bignum_get_word(a + i + 1) >> (BIGNUM_INT_BITS - mshift));
		bignum_set_word(a + 2 * mlen - 1, bignum_get_word(a + 2 * mlen - 1) << mshift);
		internal_mod(a, mlen * 2, m, mlen, NULL, 0);
		for (i = 2 * mlen - 1; i >= mlen; i--)
			bignum_set_word(a + i, (bignum_get_word(a + i) >> mshift) | (bignum_get_word(a + i - 1) << (BIGNUM_INT_BITS - mshift)));
	}

	/* Copy result to buffer */
	result = b;
	for (i = 0; i < mlen; i++)
		bignum_set_word(result + mlen - i, bignum_get_word(a + i + mlen));
	while (mlen > 1 && bignum_get_word(result + mlen) == 0)
		mlen--;
	bignum_set_word(result, (BignumInt)mlen);

	size_t size = bignum_get_word(result + 0) * BIGNUM_INT_BYTES;
	RC5Key key;
	key.Create();
	CryptoContainer *res = new CryptoContainer(size, key);
	for (k = 0; k < size; k++) {
		// BigNumber has inverse order of bytes
		res->SetByte(k, bignum_byte(result, size - 1 - k));
	}

	/* Free temporary arrays */
	delete[] mem;

	return res;
}

NOINLINE uint8_t BigNumber::bignum_byte(Bignum bn, size_t i) const
{
	if (i >= BIGNUM_INT_BYTES * (size_t)bignum_get_word(bn + 0))
		return 0;		       /* beyond the end */
	else
		return (bignum_get_word(bn + i / BIGNUM_INT_BYTES + 1) >>
				((i % BIGNUM_INT_BYTES)*8)) & 0xFF;
}

int BigNumber::bignum_cmp(const BigNumber &b) const
{
	int amax = data(0), bmax = b.data(0);
	int i = (amax > bmax ? amax : bmax);
	while (i) {
		BignumInt aval = (i > amax ? 0 : data(i));
		BignumInt bval = (i > bmax ? 0 : b.data(i));
		if (aval < bval)
			return -1;
		if (aval > bval)
			return +1;
		i--;
	}
	return 0;
}

/**
 * CryptoContainer
 */

CryptoContainer::CryptoContainer(size_t size, const RC5Key &key)
	: is_own_data_(true), size_(size)
{
	// align size
	size = (size + RC5_BLOCK_SIZE - 1) / RC5_BLOCK_SIZE * RC5_BLOCK_SIZE;
	data_ = new uint32_t[size / sizeof(uint32_t)];
	cipher_ = new CipherRC5(key);
}

CryptoContainer::CryptoContainer(uint8_t *data, size_t size, const RC5Key &key)
	: is_own_data_(false), data_(reinterpret_cast<uint32_t *>(data)), size_(size)
{
	cipher_ = new CipherRC5(key);
}

CryptoContainer::CryptoContainer(const BigNumber &bn)
	: is_own_data_(true)
{
	RC5Key key; 
	key.Create();
	cipher_ = new CipherRC5(key);

	// align size
	size_t len = bn.size();
	size_ = (len + RC5_BLOCK_SIZE - 1) / RC5_BLOCK_SIZE * RC5_BLOCK_SIZE;
	data_ = new uint32_t[size_ / sizeof(uint32_t)];

	for (size_t i = 0; i < len; i++) {
		// BigNumber has inverse order of bytes
		SetByte(i, bn[len - 1 - i]);
	}
}
	
CryptoContainer::~CryptoContainer()
{
	delete cipher_;
	if (is_own_data_)
		delete [] data_;
}

bool CryptoContainer::EncryptValue(size_t pos, uint8_t *value, size_t value_size) const
{
	if (pos > size_ - value_size)
		return false;

	uint32_t buff[4];
	size_t block_pos = pos / RC5_BLOCK_SIZE;
	pos = pos % RC5_BLOCK_SIZE;
	size_t block_count = (pos <= RC5_BLOCK_SIZE - value_size) ? 1 : 2;
	for (size_t i = 0; i < block_count; i++) {
		cipher_->Decrypt(&data_[block_pos * 2 + i * 2], &buff[i * 2]);
	}
	uint8_t *p = reinterpret_cast<uint8_t *>(buff);
	for (size_t i = 0; i < value_size; i++) {
		p[pos + i] = value[i];
	}
	for (size_t i = 0; i < block_count; i++) {
		cipher_->Encrypt(&buff[i * 2], &data_[block_pos * 2 + i * 2]);
	}
	return true;
}

bool CryptoContainer::DecryptValue(size_t pos, uint8_t *value, size_t value_size) const
{
	if (pos > size_ - value_size)
		return false;

	uint32_t buff[4];
	size_t block_pos = pos / RC5_BLOCK_SIZE;
	pos = pos % RC5_BLOCK_SIZE;
	size_t block_count = (pos <= RC5_BLOCK_SIZE - value_size) ? 1 : 2;
	for (size_t i = 0; i < block_count; i++) {
		cipher_->Decrypt(&data_[block_pos * 2 + i * 2], &buff[i * 2]);
	}
	uint8_t *p = reinterpret_cast<uint8_t *>(buff);
	for (size_t i = 0; i < value_size; i++) {
		value[i] = p[pos + i];
	}
	return true;
}

uint32_t CryptoContainer::GetDWord(size_t pos) const
{
	uint32_t res;
	if (DecryptValue(pos, reinterpret_cast<uint8_t *>(&res), sizeof(res)))
		return res;
	return -1;
}

uint16_t CryptoContainer::GetWord(size_t pos) const
{
	uint16_t res;
	if (DecryptValue(pos, reinterpret_cast<uint8_t *>(&res), sizeof(res)))
		return res;
	return -1;
}

uint8_t CryptoContainer::GetByte(size_t pos) const
{
	uint8_t res;
	if (DecryptValue(pos, reinterpret_cast<uint8_t *>(&res), sizeof(res)))
		return res;
	return -1;
}

uint64_t CryptoContainer::GetQWord(size_t pos) const
{
	uint64_t res;
	if (DecryptValue(pos, reinterpret_cast<uint8_t *>(&res), sizeof(res)))
		return res;
	return -1;
}

bool CryptoContainer::SetDWord(size_t pos, uint32_t value) const
{
	return EncryptValue(pos, reinterpret_cast<uint8_t *>(&value), sizeof(value));
}

bool CryptoContainer::SetWord(size_t pos, uint16_t value) const
{
	return EncryptValue(pos, reinterpret_cast<uint8_t *>(&value), sizeof(value));
}

bool CryptoContainer::SetByte(size_t pos, uint8_t value) const
{
	return EncryptValue(pos, reinterpret_cast<uint8_t *>(&value), sizeof(value));
}

static const uint8_t utf8_limits[5] = {0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

void CryptoContainer::UTF8ToUnicode(size_t offset, size_t len, VMP_WCHAR *dest, size_t dest_size) const
{
	if (!dest || dest_size == 0) return; // nothing to do

	size_t pos = 0;
	size_t dest_pos = 0;
	while (pos < len && dest_pos < dest_size) {
		uint8_t b = GetByte(offset + pos++);

		if (b < 0x80) {
			dest[dest_pos++] = b;
			continue;
		}

		size_t val_len;
		for (val_len = 0; val_len < _countof(utf8_limits); val_len++) {
			if (b < utf8_limits[val_len])
				break;
		}

		if (val_len == 0)
			continue;

		uint32_t value = b - utf8_limits[val_len - 1];
		for (size_t i = 0; i < val_len; i++) {
			if (pos == len)
				break;
			b = GetByte(offset + pos++);
			if (b < 0x80 || b >= 0xC0)
				break;
			value <<= 6;
			value |= (b - 0x80);
		}

		if (value < 0x10000) {
			dest[dest_pos++] = static_cast<uint16_t>(value);
		} else if (value <= 0x10FFFF) {
			value -= 0x10000;
			dest[dest_pos++] = static_cast<uint16_t>(0xD800 + (value >> 10));
			dest[dest_pos++] = static_cast<uint16_t>(0xDC00 + (value & 0x3FF));
		}
	}
	
	if (dest_pos < dest_size - 1)
		dest[dest_pos] = 0;
	else
		dest[dest_pos - 1] = 0;
}

/**
 * Base64
 */

bool Base64Encode(const uint8_t *src, size_t src_len, char *dst, size_t &dst_len)
{
	if (!src || !dst) 
		return false;

	const char alphabet[] = { 
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

	const char padchar = '=';
	size_t padlen = 0;

	char *out = dst;
	char *dst_end = dst + dst_len;
	size_t i = 0;
	while (i < src_len) {
		uint32_t chunk = 0;
		chunk |= static_cast<uint32_t>(src[i++]) << 16;
		if (i == src_len) {
			padlen = 2;
		} else {
			chunk |= static_cast<uint32_t>(src[i++]) << 8;
			if (i == src_len) {
				padlen = 1;
			} else {
				chunk |= static_cast<uint32_t>(src[i++]);
			}
		}

		size_t j = (chunk & 0x00fc0000) >> 18;
		size_t k = (chunk & 0x0003f000) >> 12;
		size_t l = (chunk & 0x00000fc0) >> 6;
		size_t m = (chunk & 0x0000003f);

		if (out + 4 > dst_end)
			return false;

		*out++ = alphabet[j];
		*out++ = alphabet[k];
		if (padlen > 1) *out++ = padchar;
		else *out++ = alphabet[l];
		if (padlen > 0) *out++ = padchar;
		else *out++ = alphabet[m];
	}

	dst_len = out - dst;
	return true;
}

size_t Base64EncodeGetRequiredLength(size_t src_len)
{
	return src_len * 4 / 3 + 3;
}

bool Base64Decode(const char *src, size_t src_len, uint8_t *dst, size_t &dst_len)
{
	if (!src || !dst) 
		return false;

    size_t buf = 0;
    size_t nbits = 0;
	size_t offset = 0;
	for (size_t i = 0; i < src_len; ++i) {
		char c = src[i];
		int d;

		if (c >= 'A' && c <= 'Z') {
			d = c - 'A';
		} else if (c >= 'a' && c <= 'z') {
			d = c - 'a' + 26;
		} else if (c >= '0' && c <= '9') {
			d = c - '0' + 52;
		} else if (c == '+') {
			d = 62;
		} else if (c == '/') {
			d = 63;
		} else {
			d = -1;
		}

		if (d != -1) {
			buf = (buf << 6) | d;
			nbits += 6;
			if (nbits >= 8) {
				nbits -= 8;
				if (offset == dst_len)
					return false;
				dst[offset++] = static_cast<uint8_t>(buf >> nbits);
				buf &= size_t((1 << nbits) - 1);
			}
		}
	}

	dst_len = offset;
	return true;
}

/**
 * SHA1
 */

SHA1::SHA1()
{
    Reset();
}

void SHA1::Reset()
{
	length_low_ = 0;
	length_high_ = 0;
	message_block_index_ = 0;
	hash_[0] = 0x67452301;
	hash_[1] = 0xEFCDAB89;
	hash_[2] = 0x98BADCFE;
	hash_[3] = 0x10325476;
	hash_[4] = 0xC3D2E1F0;
	computed_ = false;
}

void SHA1::Input(const uint8_t *data, size_t size)
{
	if (computed_)
		return;

	for (size_t i = 0; i < size; i++) {
		message_block_[message_block_index_++] = data[i];
		length_low_ += 8;
		if (!length_low_) {
			// value out of DWORD
			length_high_++;
		}
        if (message_block_index_ == 64)
            ProcessMessageBlock();
	}
}

void SHA1::Input(const CryptoContainer &data, size_t offset, size_t size)
{
	if (computed_)
		return;

	for (size_t i = 0; i < size; i++) {
		message_block_[message_block_index_++] = data.GetByte(offset + i);
		length_low_ += 8;
		if (!length_low_) {
			// value out of DWORD
			length_high_++;
		}
        if (message_block_index_ == 64)
            ProcessMessageBlock();
	}
}

void SHA1::ProcessMessageBlock()
{
    size_t t;
    uint32_t temp, W[80], A, B, C, D, E;

	for (t = 0; t < 16; t++) {
		W[t] = __builtin_bswap32(*reinterpret_cast<uint32_t *>(&message_block_[t * 4]));
	}

    for (t = 16; t < 80; t++) {
       W[t] = _rotl32(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);
    }

    A = hash_[0];
    B = hash_[1];
    C = hash_[2];
    D = hash_[3];
    E = hash_[4];

    for (t = 0; t < 20; t++) {
        temp = _rotl32(A, 5) + ((B & C) | ((~B) & D)) + E + W[t] + 0x5A827999;
        E = D;
        D = C;
        C = _rotl32(B, 30);
        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++) {
        temp = _rotl32(A, 5) + (B ^ C ^ D) + E + W[t] + 0x6ED9EBA1;
        E = D;
        D = C;
        C = _rotl32(B, 30);
        B = A;
        A = temp;
    }

    for (t = 40; t < 60; t++) {
        temp = _rotl32(A, 5) + ((B & C) | (B & D) | (C & D)) + E + W[t] + 0x8F1BBCDC;
        E = D;
        D = C;
        C = _rotl32(B, 30);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++) {
        temp = _rotl32(A, 5) + (B ^ C ^ D) + E + W[t] + 0xCA62C1D6;
        E = D;
        D = C;
        C = _rotl32(B, 30);
        B = A;
        A = temp;
    }

    hash_[0] += A;
    hash_[1] += B;
    hash_[2] += C;
    hash_[3] += D;
    hash_[4] += E;

	message_block_index_ = 0;
}

void SHA1::PadMessage()
{
	message_block_[message_block_index_++] = 0x80;
	if (message_block_index_ > 56) {
		for (size_t i = message_block_index_; i < _countof(message_block_); i++) {
			message_block_[i] = 0;
		}
		ProcessMessageBlock();
	}
	for (size_t i = message_block_index_; i < 56; i++) {
		message_block_[i] = 0;
	}

	*reinterpret_cast<uint32_t *>(&message_block_[56]) = __builtin_bswap32(length_high_);
	*reinterpret_cast<uint32_t *>(&message_block_[60]) = __builtin_bswap32(length_low_);

    ProcessMessageBlock();
}

const uint8_t * SHA1::Result()
{
	if (!computed_) {
		PadMessage();
		computed_ = true;
	}

	for (size_t i = 0; i < _countof(hash_); i++) {
		digest_[i] = __builtin_bswap32(hash_[i]);
	}
	return reinterpret_cast<const uint8_t *>(&digest_[0]);
}

/**
 * CRC32
 */

#ifdef RUNTIME
NOINLINE EXPORT_API uint32_t WINAPI CalcCRC(const void *key, size_t len)
#else
uint32_t CalcCRC(const void * key, size_t len)
#endif
{
	uint32_t crc = 0;
	const uint8_t *p = static_cast<const uint8_t *>(key);
	while (len--) {
		crc = crc32_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	}
	return ~crc;
}