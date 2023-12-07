/**
 * Stream implementation.
 */

#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"

/**
 * AbstractStream
 */

size_t AbstractStream::CopyFrom(AbstractStream &source, size_t count)
{
	size_t copied_size, n, nc, total;
#define BUF_SIZE 4096 /* Let's use page size */
	uint8_t buf[BUF_SIZE];

	total = 0; /* Total copied */
	for (n = 0; n < count; n += BUF_SIZE) {
		nc = BUF_SIZE; /* Number of bytes to copy */
		if (count - n < BUF_SIZE)
			nc = count - n;
		copied_size = source.Read(buf, nc);
		if (copied_size)
			Write(buf, copied_size);

		total += copied_size;
		if (nc != copied_size)
			break;
	}
	Flush();
	return total;
}

uint64_t AbstractStream::Tell()
{
	return Seek(0, soCurrent);
}

uint64_t AbstractStream::Size()
{
	uint64_t pos = Tell();
	uint64_t size = Seek(0, soEnd);
	/* Restore previous position */
	Seek(pos, soBeginning);
	return size;
}

/*static __inline bool IsCrLf(char c)
{
	return c == '\r' || c == '\n';
}*/

/**
 * MemoryStream
 */

MemoryStream::MemoryStream(size_t /*buf_size_increment*/)
	: pos_(0)/*, buf_size_inc_(buf_size_increment)*/
{
}

size_t MemoryStream::Read(void *buffer, size_t size)
{
	if (pos_ + size > buf_.size())
		size = buf_.size() - pos_;
	if (size) {
		memcpy(buffer, &buf_[pos_], size);
		pos_ += size;
	}
	return size;
}

uint64_t MemoryStream::Resize(uint64_t new_size)
{
	buf_.resize(static_cast<size_t>(new_size));
	pos_ = buf_.size();
	return new_size;
}

size_t MemoryStream::Write(const void *buffer, size_t size)
{
	if (size) {
		if (pos_ + size > buf_.size())
			buf_.resize(pos_ + size);

		memcpy(&buf_[pos_], buffer, size);
		pos_ += size;
	}
	return size;
}

uint64_t MemoryStream::Seek(int64_t offset, SeekOrigin origin)
{
	intptr_t req_pos;

	req_pos = static_cast<intptr_t>(offset);
	switch (origin) {
	case soBeginning:
		break;
	case soCurrent:
		req_pos += pos_;
		break;
	case soEnd:
		req_pos += buf_.size();
		break;
	default:
		return -1;
	}
	pos_ = req_pos;
	return pos_;
}

/**
 * MemoryStreamEnc
 */

MemoryStreamEnc::MemoryStreamEnc(const void *buffer, size_t size, uint32_t key)
	: MemoryStream(), key_(key)
{
	MemoryStream::Write(buffer, size);
}

size_t MemoryStreamEnc::Read(void *buffer, size_t size)
{
	uint64_t pos = Seek(0, soCurrent);
	size_t res = MemoryStream::Read(buffer, size);
	for (size_t i = 0; i < res; i++) { 
		int p = static_cast<int>(pos + i);
		reinterpret_cast<uint8_t *>(buffer)[i] ^= static_cast<uint8_t>(_rotl32(key_, p) + p); 
	}
	return res;
}

size_t MemoryStreamEnc::Write(const void *buffer, size_t size)
{
	size_t res;
	if (size) {
		uint64_t pos = Seek(0, soCurrent);
		uint8_t *enc_buffer = new uint8_t[size];
		for (size_t i = 0; i < size; i++) {
			int p = static_cast<int>(pos + i);
			enc_buffer[i] = reinterpret_cast<const uint8_t *>(buffer)[i] ^ static_cast<uint8_t>(_rotl32(key_, p) + p);
		}
		res = MemoryStream::Write(enc_buffer, size);
		delete [] enc_buffer;
	} else {
		res = MemoryStream::Write(buffer, size);
	}
	return res;
}

/**
 * ModuleStream
 */

ModuleStream::ModuleStream()
	: AbstractStream(), pos_(0), size_(0), base_address_(0), process_(0)
{

}

ModuleStream::~ModuleStream()
{
	Close();
}

bool ModuleStream::Open(uint32_t process_id, HMODULE module)
{
	Close();

	process_ = os::ProcessOpen(process_id);
	if (!process_)
		return false;

	MODULE_INFO module_info;
	if (!os::GetModuleInformation(process_, module, &module_info, sizeof(module_info)))
		return false;

	base_address_ = module_info.address;
	size_ = module_info.size;
	return true;
}

void ModuleStream::Close()
{
	pos_ = 0;
	size_ = 0;
	base_address_ = NULL;
	if (process_) {
		os::ProcessClose(process_);
		process_ = 0;
	}
}

size_t ModuleStream::Read(void *buffer, size_t size)
{
	size_t res = os::ProcessRead(process_, reinterpret_cast<uint8_t*>(base_address_) + pos_, buffer, size);
	if (res != (size_t)-1)
		pos_ += res;
	return res;
}

size_t ModuleStream::Write(const void *buffer, size_t size)
{
	size_t res = os::ProcessWrite(process_, reinterpret_cast<uint8_t*>(base_address_) + pos_, buffer, size);
	if (res != (size_t)-1)
		pos_ += res;
	return res;
}

uint64_t ModuleStream::Seek(int64_t offset, SeekOrigin origin)
{
	intptr_t req_pos;

	req_pos = static_cast<intptr_t>(offset);
	switch (origin) {
	case soBeginning:
		break;
	case soCurrent:
		req_pos += pos_;
		break;
	case soEnd:
		req_pos += size_;
		break;
	default:
		return -1;
	}
	pos_ = req_pos;
	return pos_;
}

/**
 * FileStream
 */

FileStream::FileStream(size_t CACHE_ALLOC_SIZE /*= 0x10000*/)
	:  h_(INVALID_HANDLE_VALUE), cache_mode_(cmNone), CACHE_ALLOC_SIZE_(CACHE_ALLOC_SIZE), cache_pos_(0), cache_size_(0), cache_offset_(0)
{

}

FileStream::~FileStream()
{ 
	Close();
}

bool FileStream::Open(const char *filename, int mode)
{
	Close();

	h_ = os::FileCreate(filename, mode);
	return (h_ != INVALID_HANDLE_VALUE);
}

void FileStream::Close()
{
	FlushCache();

	if (h_ != INVALID_HANDLE_VALUE) {
		os::FileClose(h_);
		h_ = INVALID_HANDLE_VALUE;
	}
}

uint8_t * FileStream::Cache()
{
	uint8_t *ret = cache_.get();
	if (ret == NULL)
	{
		ret = new uint8_t[CACHE_ALLOC_SIZE_];
		cache_.reset(ret);
	}
	return ret;
}

void FileStream::FlushCache(bool need_seek)
{
	switch (cache_mode_) {
	case cmRead:
		if (need_seek && os::FileSeek(h_, cache_offset_ + cache_pos_, soBeginning) == (uint64_t)-1)
			throw std::runtime_error("Runtime Error at Flush");
		break;
	case cmWrite:
		if (cache_pos_ && os::FileWrite(h_, Cache(), cache_pos_) != cache_pos_)
			throw std::runtime_error("Runtime Error at Flush");
		break;
	}
	cache_mode_ = cmNone;
	cache_size_ = 0;
	cache_pos_ = 0;
	cache_offset_ = 0;
}

size_t FileStream::Read(void *buffer, size_t size)
{
	size_t add_size = 0;
	if (cache_mode_ == cmRead) {
		size_t cache_size = cache_size_ - cache_pos_;
		if (size <= cache_size) {
			memcpy(buffer, Cache() + cache_pos_, size);
			cache_pos_ += size;
			return size;
		}
		if (cache_size) {
			memcpy(buffer, Cache() + cache_pos_, cache_size);
			cache_pos_ += cache_size;
			size -= cache_size;
			add_size = cache_size;
			buffer = static_cast<uint8_t*>(buffer) + cache_size;
		}
	}
	FlushCache();

	if (size < CACHE_ALLOC_SIZE_) {
		cache_offset_ = os::FileSeek(h_, 0, soCurrent);
		size_t cache_size = os::FileRead(h_, Cache(), CACHE_ALLOC_SIZE_);
		// check error
		if (cache_size == (size_t)-1)
			return cache_size;
		if (size > cache_size)
			size = cache_size;
		cache_mode_ = cmRead;
		cache_size_ = cache_size;
		memcpy(buffer, Cache(), size);
		cache_pos_ = size;
		return size + add_size;
	}

	size_t res = os::FileRead(h_, buffer, size);
	// check error
	if (res == (size_t)-1)
		return res;
	return res + add_size;
}

size_t FileStream::Write(const void *buffer, size_t size)
{
	size_t add_size = 0;
	if (cache_mode_ == cmWrite) {
		size_t cache_size = cache_size_ - cache_pos_;
		if (size <= cache_size) {
			memcpy(Cache() + cache_pos_, buffer, size);
			cache_pos_ += size;
			return size;
		}
		if (cache_size) {
			memcpy(Cache() + cache_pos_, buffer, cache_size);
			cache_pos_ += cache_size;
			size -= cache_size;
			add_size = cache_size;
			buffer = static_cast<const uint8_t*>(buffer) + cache_size;
		}
	}
	FlushCache(true);

	if (size < CACHE_ALLOC_SIZE_) {
		cache_offset_ = os::FileSeek(h_, 0, soCurrent);
		cache_mode_ = cmWrite;
		cache_size_ = CACHE_ALLOC_SIZE_;
		memcpy(Cache(), buffer, size);
		cache_pos_ = size;
		return size + add_size;
	}

	size_t res = os::FileWrite(h_, buffer, size);
	// check error
	if (res == (size_t)-1)
		return res;
	return res + add_size;
}

uint64_t FileStream::Seek(int64_t offset, SeekOrigin origin)
{
	if (cache_mode_ != cmNone) {
		switch (origin) {
		case soBeginning:
			{
				uint64_t pos = static_cast<uint64_t>(offset);
				if (pos == cache_offset_ + cache_pos_)
					return cache_offset_ + cache_pos_;
				if (cache_mode_ == cmRead && pos >= cache_offset_ && pos <= cache_offset_ + cache_size_) {
					cache_pos_ = static_cast<size_t>(pos - cache_offset_);
					return cache_offset_ + cache_pos_;
				}
			}
			break;
		case soCurrent:
			if (cache_mode_ == cmRead) {
				if (offset + (int64_t)cache_pos_ >= 0 && offset + cache_pos_ <= cache_size_) {
					cache_pos_ = static_cast<size_t>(offset + cache_pos_);
					return cache_offset_ + cache_pos_;
				}
				offset -= cache_size_ - cache_pos_;
			}
			break;
		}
		FlushCache();
	}

	return os::FileSeek(h_, offset, origin);
}

uint64_t FileStream::Resize(uint64_t new_size)
{
	uint64_t res = Seek(new_size, soBeginning);
	FlushCache(true);
	os::FileSetEnd(h_);
	return res;
}

bool FileStream::ReadLine(std::string &out_line)
{
	bool res = true;
	out_line.clear();
	for (;;) {
		char c;
		if (Read(&c, sizeof(c)) == 0) {
			res = !out_line.empty();
			break;
		}

		if (c == '\n') {
			break;
		} else if (c == '\r') {
			if (Read(&c, sizeof(c)) && c != '\n')
				Seek(-1, soCurrent);
			break;
		} else {
			out_line.push_back(c);
		}
	}
	return res;
}

std::string FileStream::ReadAll()
{
	std::string res;
	size_t sz = static_cast<size_t>(Size());
	if (sz) {
		res.resize(sz);
		Read(&res[0], sz);
	}
	return res;
}

/**
 * Buffer
 */

Buffer::Buffer(const uint8_t *memory)
	: memory_(memory), position_(0)
{

}

uint8_t Buffer::ReadByte()
{
	uint8_t res;
	ReadBuff(&res, sizeof(res));
	return res;
}

uint16_t Buffer::ReadWord()
{
	uint16_t res;
	ReadBuff(&res, sizeof(res));
	return res;
}

uint32_t Buffer::ReadDWord()
{
	uint32_t res;
	ReadBuff(&res, sizeof(res));
	return res;
}

uint64_t Buffer::ReadQWord()
{
	uint64_t res;
	ReadBuff(&res, sizeof(res));
	return res;
}

void Buffer::ReadBuff(void *buff, size_t size)
{
	memcpy(buff, &memory_[position_], size);
	position_ += size;
}