#include "objects.h"
#include "common.h"
#include "core.h"
#include "crypto.h"
#include "string_manager.h"

/**
 * exported functions
 */

#ifdef VMP_GNU
EXPORT_API const void * WINAPI ExportedDecryptString(const void *str) __asm__ ("ExportedDecryptString");
EXPORT_API bool WINAPI ExportedFreeString(const void *str) __asm__ ("ExportedFreeString");
#endif

const void * WINAPI ExportedDecryptString(const void *str)
{
	StringManager *string_manager = Core::Instance()->string_manager();
	return string_manager ? string_manager->DecryptString(str) : str;
}

bool WINAPI ExportedFreeString(const void *str)
{
	StringManager *string_manager = Core::Instance()->string_manager();
	return string_manager ? string_manager->FreeString(str) : false;
}

/**
 * StringManager
 */

StringManager::StringManager(const uint8_t *data, HMODULE instance, const uint8_t *key)
	: data_(data), instance_(instance)
{
	CriticalSection::Init(critical_section_);
	key_ = *(reinterpret_cast<const uint32_t *>(key));
	const STRING_DIRECTORY *directory_enc = reinterpret_cast<const STRING_DIRECTORY *>(data_);
	STRING_DIRECTORY directory = DecryptDirectory(directory_enc);
	size_ = directory.NumberOfEntries;
	strings_ = new VirtualString*[size_];
	memset(strings_, 0, sizeof(VirtualString *) * size_);
}

StringManager::~StringManager()
{
	for (size_t i = 0; i < size_; i++) {
		delete strings_[i];
	}
	delete [] strings_;
	CriticalSection::Free(critical_section_);
}

STRING_DIRECTORY StringManager::DecryptDirectory(const STRING_DIRECTORY *directory_enc) const
{
	STRING_DIRECTORY res;
	res.NumberOfEntries = directory_enc->NumberOfEntries ^ key_;
	return res;
}

STRING_ENTRY StringManager::DecryptEntry(const STRING_ENTRY *entry_enc) const
{
	STRING_ENTRY res;
	res.Id = entry_enc->Id ^ key_;
	res.OffsetToData = entry_enc->OffsetToData ^ key_;
	res.Size = entry_enc->Size ^ key_;
	return res;
}

size_t StringManager::IndexById(uint32_t id) const
{
	const STRING_ENTRY *entry_enc = reinterpret_cast<const STRING_ENTRY *>(data_ + sizeof(STRING_DIRECTORY));

	int min = 0;
	int max = (int)size_ - 1;
	while (min <= max) {
		int i = (min + max) / 2;
		STRING_ENTRY entry = DecryptEntry(entry_enc + i);
		if (entry.Id == id) {
			return i;
		}
		if (entry.Id > id) {
			max = i - 1;
		} else {
			min = i + 1;
		}
	}
	return NOT_ID;
}

const void *StringManager::DecryptString(const void *str)
{
	size_t id = reinterpret_cast<size_t>(str) - reinterpret_cast<size_t>(instance_);
	if ((id >> 31) == 0) {
		size_t i = IndexById(static_cast<uint32_t>(id));
		if (i != NOT_ID) {
			CriticalSection cs(critical_section_);
			VirtualString *string = strings_[i];
			if (!string) {
				const STRING_ENTRY *entry_enc = reinterpret_cast<const STRING_ENTRY *>(data_ + sizeof(STRING_DIRECTORY));
				string = new VirtualString(entry_enc + i, instance_, key_);
				strings_[i] = string;
			}
			return string->AcquirePointer();
		}
	}
	return str;
}

size_t StringManager::IndexByAddress(const void *address) const
{
	for (size_t i = 0; i < size_; i++) {
		VirtualString *string = strings_[i];
		if (string && string->address() == address)
			return i;
	}
	return NOT_ID;
}

bool StringManager::FreeString(const void *str)
{
	CriticalSection cs(critical_section_);

	size_t i = IndexByAddress(str);
	if (i == NOT_ID)
		return false;

	VirtualString *string = strings_[i];
	if (string->Release()) {
		delete string;
		strings_[i] = NULL;
	}
	return true;
}

/**
 * VirtualString
 */

VirtualString::VirtualString(const STRING_ENTRY *entry_enc, HMODULE instance, uint32_t key)
	: use_count_(0)
{
	STRING_ENTRY entry;
	entry.Id = entry_enc->Id ^ key;
	entry.OffsetToData = entry_enc->OffsetToData ^ key;
	entry.Size = entry_enc->Size ^ key;

	size_ = entry.Size;
	address_ = new uint8_t[size_];
	uint8_t *source = reinterpret_cast<uint8_t *>(instance) + entry.OffsetToData;
	for (size_t i = 0; i < size_; i++) {
		address_[i] = static_cast<uint8_t>(source[i] ^ (_rotl32(key, static_cast<int>(i)) + i));
	}
}

VirtualString::~VirtualString()
{
	Clear();
}

void VirtualString::Clear()
{
	if (address_) {
		volatile uint8_t *ptrSecureZeroing = address_;
		while (size_--) *ptrSecureZeroing++ = 0;
		delete [] address_;
		address_ = NULL;
	}
}

uint8_t *VirtualString::AcquirePointer()
{
	use_count_++;
	return address_;
}

bool VirtualString::Release()
{
	use_count_--;
	if (use_count_)
		return false;

	Clear();
	return true;
}