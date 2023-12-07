/**
 * Stream implementation.
 */

#ifndef STREAMS_H
#define STREAMS_H

class AbstractStream
{
public:
	virtual ~AbstractStream() {}
	virtual size_t Read(void *buffer, size_t size) = 0;
	virtual size_t Write(const void *buffer, size_t size) = 0;
	virtual	uint64_t Seek(int64_t offset, SeekOrigin origin) = 0;
	virtual size_t CopyFrom(AbstractStream &source, size_t count);
	uint64_t Tell();
	uint64_t Size();
	virtual uint64_t Resize(uint64_t new_size) = 0;
	virtual void Flush() {}
};

class FileStream : public AbstractStream
{
public:
	explicit FileStream(size_t CACHE_ALLOC_SIZE = 0x10000);
	virtual ~FileStream();
	virtual bool Open(const char *filename, int mode);
	virtual void Close();
	virtual size_t Read(void *buffer, size_t size);
	virtual size_t Write(const void *buffer, size_t size);
	virtual	uint64_t Seek(int64_t offset, SeekOrigin origin);
	virtual uint64_t Resize(uint64_t new_size);
	virtual void Flush() { FlushCache(); }
	bool ReadLine(std::string &line);
	std::string ReadAll();
protected:
	uint8_t *Cache();
	void FlushCache(bool need_seek = false);

	HANDLE h_;
	enum CacheMode {
		cmNone,
		cmRead,
		cmWrite
	};
	CacheMode cache_mode_;
	const size_t CACHE_ALLOC_SIZE_;
	std::auto_ptr<uint8_t> cache_;
	size_t cache_pos_;
	size_t cache_size_;
	uint64_t cache_offset_;
};

class MemoryStream : public AbstractStream
{
public:
	explicit MemoryStream(size_t buf_size_increment = 1024);
	virtual size_t Read(void *buffer, size_t size);
	virtual size_t Write(const void *buffer, size_t size);
	virtual	uint64_t Seek(int64_t offset, SeekOrigin origin);
	virtual uint64_t Resize(uint64_t new_size);
	std::vector<uint8_t> data() const { return buf_; }
private:
	std::vector <uint8_t> buf_;
	size_t pos_;
};

class MemoryStreamEnc : public MemoryStream
{
public:
	explicit MemoryStreamEnc(const void *buffer, size_t size, uint32_t key);
	virtual size_t Read(void *buffer, size_t size);
	virtual size_t Write(const void *buffer, size_t size);
private:
	uint32_t key_;
};

class ModuleStream : public AbstractStream
{
public:
	ModuleStream();
	~ModuleStream();
	bool Open(uint32_t process_id, HMODULE module);
	void Close();
	virtual size_t Read(void *buffer, size_t size);
	virtual size_t Write(const void *buffer, size_t size);
	virtual	uint64_t Seek(int64_t offset, SeekOrigin origin);
	virtual uint64_t Resize(uint64_t /*new_size*/) { return size_; }
private:
	size_t pos_;
	size_t size_;
	void *base_address_;
	HANDLE process_;
};

class Buffer
{
public:
	Buffer(const uint8_t *memory);
	uint8_t ReadByte();
	uint16_t ReadWord();
	uint32_t ReadDWord();
	uint64_t ReadQWord();
private:
	void ReadBuff(void *buff, size_t size);
	const uint8_t *memory_;
	size_t position_;
};

#endif
