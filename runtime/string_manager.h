#ifndef STRING_MANAGER_H
#define STRING_MANAGER_H

class CipherRC5;

struct STRING_DIRECTORY {
	uint32_t NumberOfEntries;
	// STRING_ENTRY Entries[];
};

struct STRING_ENTRY {
	uint32_t Id;
	uint32_t OffsetToData;
	uint32_t Size;
};

class VirtualString
{
public:
	VirtualString(const STRING_ENTRY *entry, HMODULE instance, uint32_t key);
	~VirtualString();
	uint8_t *AcquirePointer();
	bool Release();
	uint8_t *address() const { return address_; }
private:
	void Clear();
	size_t use_count_;
	uint8_t *address_;
	size_t size_;

	// no copy ctr or assignment op
	VirtualString(const VirtualString &);
	VirtualString &operator =(const VirtualString &);
};

class StringManager
{
public:
	StringManager(const uint8_t *data, HMODULE instance, const uint8_t *key);
	~StringManager();
	const void *DecryptString(const void *str);
	bool FreeString(const void *str);
private:
	STRING_DIRECTORY DecryptDirectory(const STRING_DIRECTORY *directory_enc) const;
	STRING_ENTRY DecryptEntry(const STRING_ENTRY *entry_enc) const;
	size_t IndexById(uint32_t id) const;
	size_t IndexByAddress(const void *address) const;
	const uint8_t *data_;
	HMODULE instance_;
	CRITICAL_SECTION critical_section_;
	VirtualString **strings_;
	uint32_t key_;
	size_t size_;

	// no copy ctr or assignment op
	StringManager(const StringManager &);
	StringManager &operator =(const StringManager &);
};

#endif