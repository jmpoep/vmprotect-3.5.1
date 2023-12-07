#include "common.h"
#include "objects.h"
#include "utils.h"
#include "core.h"
#include "crypto.h"
#include "licensing_manager.h"
#include "hwid.h"
#include "loader.h"

#if defined(__unix__)
#include <sys/time.h>
#include <curl/curl.h>
#endif

/**
 * exported functions
 */

#ifdef VMP_GNU
EXPORT_API int WINAPI ExportedSetSerialNumber(const char *serial) __asm__ ("ExportedSetSerialNumber");
EXPORT_API int WINAPI ExportedGetSerialNumberState() __asm__ ("ExportedGetSerialNumberState");
EXPORT_API bool WINAPI ExportedGetSerialNumberData(VMProtectSerialNumberData *data, int size) __asm__ ("ExportedGetSerialNumberData");
EXPORT_API int WINAPI ExportedActivateLicense(const char *code, char *serial, int size) __asm__ ("ExportedActivateLicense");
EXPORT_API int WINAPI ExportedDeactivateLicense(const char *serial) __asm__ ("ExportedDeactivateLicense");
EXPORT_API int WINAPI ExportedGetOfflineActivationString(const char *code, char *buf, int size) __asm__ ("ExportedGetOfflineActivationString");
EXPORT_API int WINAPI ExportedGetOfflineDeactivationString(const char *serial, char *buf, int size) __asm__ ("ExportedGetOfflineDeactivationString");
EXPORT_API void WINAPI ExportedDecryptBuffer(uint8_t *buffer) __asm__ ("ExportedDecryptBuffer");
#elif defined(USE_WININET)
 #include <wininet.h>
#else
 #ifndef WIN_DRIVER
  #include <winhttp.h>
  #include <xstring>
 #endif
#endif

int WINAPI ExportedSetSerialNumber(const char *serial)
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	return licensing_manager ? licensing_manager->SetSerialNumber(serial) : SERIAL_STATE_FLAG_CORRUPTED;
}

int WINAPI ExportedGetSerialNumberState()
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	return licensing_manager ? licensing_manager->GetSerialNumberState() : SERIAL_STATE_FLAG_CORRUPTED;
}

bool WINAPI ExportedGetSerialNumberData(VMProtectSerialNumberData *data, int size)
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	return licensing_manager ? licensing_manager->GetSerialNumberData(data, size) : false;
}

int WINAPI ExportedActivateLicense(const char *code, char *serial, int size)
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	return licensing_manager ? licensing_manager->ActivateLicense(code, serial, size) : ACTIVATION_NOT_AVAILABLE;
}

int WINAPI ExportedDeactivateLicense(const char *serial)
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	return licensing_manager ? licensing_manager->DeactivateLicense(serial) : ACTIVATION_NOT_AVAILABLE;
}

int WINAPI ExportedGetOfflineActivationString(const char *code, char *buf, int size)
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	return licensing_manager ? licensing_manager->GetOfflineActivationString(code, buf, size) : ACTIVATION_NOT_AVAILABLE;
}

int WINAPI ExportedGetOfflineDeactivationString(const char *serial, char *buf, int size)
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	return licensing_manager ? licensing_manager->GetOfflineDeactivationString(serial, buf, size) : ACTIVATION_NOT_AVAILABLE;
}

void WINAPI ExportedDecryptBuffer(uint8_t *buffer)
{
	LicensingManager *licensing_manager = Core::Instance()->licensing_manager();
	if (licensing_manager)
		licensing_manager->DecryptBuffer(buffer);
}

/**
 * LicensingManager
 */

#ifdef __APPLE__

#include "CFGregorianDateCreate.hpp"

uint32_t GetTickCount()
{
	const int64_t one_million = 1000 * 1000;
	mach_timebase_info_data_t timebase_info;
	mach_timebase_info(&timebase_info);

	// mach_absolute_time() returns billionth of seconds,
	// so divide by one million to get milliseconds
	return static_cast<uint32_t>((mach_absolute_time() * timebase_info.numer) / (one_million * timebase_info.denom));
}
#elif defined(WIN_DRIVER)
uint32_t GetTickCount()
{
	LARGE_INTEGER tick_count;
    KeQueryTickCount(&tick_count);
    return static_cast<uint32_t>(tick_count.QuadPart * KeQueryTimeIncrement() / 10000);
}
#endif
#ifdef __unix__
unsigned long GetTickCount()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif
LicensingManager::LicensingManager(uint8_t *data, uint32_t size, uint8_t *key)
	: start_(0), serial_(NULL)
{
	CriticalSection::Init(critical_section_);
	session_key_ = 0 - static_cast<uint32_t>(loader_data->session_key());
	license_data_ = new CryptoContainer(data, size, key);
	start_tick_count_ = GetTickCount();
	save_state(SERIAL_STATE_FLAG_INVALID);
}

LicensingManager::~LicensingManager()
{
	delete license_data_;
	delete serial_;
	CriticalSection::Free(critical_section_);
}

int LicensingManager::save_state(int new_state)
{
	if (new_state & (SERIAL_STATE_FLAG_CORRUPTED | SERIAL_STATE_FLAG_INVALID | SERIAL_STATE_FLAG_BLACKLISTED | SERIAL_STATE_FLAG_BAD_HWID)) {
		product_code_ = 0;
		if (serial_) {
			delete serial_;
			serial_ = NULL;
		}
	} else if (new_state & (SERIAL_STATE_FLAG_DATE_EXPIRED | SERIAL_STATE_FLAG_RUNNING_TIME_OVER | SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED)) {
		if (state_ == SERIAL_STATE_FLAG_INVALID)
			product_code_ = 0;
	}
	state_ = new_state;
	return state_;
}

bool LicensingManager::CheckLicenseDataCRC() const
{
	size_t crc_pos = license_data_->GetDWord(FIELD_CRC_OFFSET * sizeof(uint32_t));
	size_t size = crc_pos + 16;
	if (size != license_data_->size())
		return false; // bad key size

	// CRC check
	SHA1 hash;
	hash.Input(license_data_->data(), crc_pos);
	const uint8_t *p = hash.Result();
	for (size_t i = crc_pos; i < size; i++) {
		if (license_data_->GetByte(i) != p[i - crc_pos])
			return false;
	}

	return true;
}

int LicensingManager::SetSerialNumber(const char *serial)
{
	CriticalSection	cs(critical_section_);

	save_state(SERIAL_STATE_FLAG_INVALID);

	if (!serial) 
		return SERIAL_STATE_FLAG_INVALID; // the key is empty
		
	size_t len = 0;
	while (serial[len]) {
		len++;
	}
	if (!len)
		return SERIAL_STATE_FLAG_INVALID; // the key is empty

	// decode serial number from base64
	uint8_t *binary_serial = new uint8_t[len];
	if (!Base64Decode(serial, len, binary_serial, len) || len < 16) {
		delete [] binary_serial;
		return SERIAL_STATE_FLAG_INVALID;
	}

	// check license data integrity
	if (!CheckLicenseDataCRC()) {
		delete [] binary_serial;
		return save_state(SERIAL_STATE_FLAG_CORRUPTED);
	}

	// check serial by black list
	size_t black_list_size = license_data_->GetDWord(FIELD_BLACKLIST_SIZE * sizeof(uint32_t));
	if (black_list_size) {
		size_t black_list_offset = license_data_->GetDWord(FIELD_BLACKLIST_OFFSET * sizeof(uint32_t));

		SHA1 hash;
		hash.Input(binary_serial, len);
		const uint32_t *p = reinterpret_cast<const uint32_t *>(hash.Result());
		int min = 0;
		int max = (int)black_list_size / 20 - 1;
		while (min <= max) {
			int i = (min + max) / 2;
			bool blocked = true;
			for (size_t j = 0; j < 20 / sizeof(uint32_t); j++) {
				uint32_t dw = license_data_->GetDWord(black_list_offset + i * 20 + j * sizeof(uint32_t));
				if (dw == p[j])
					continue;

				if (__builtin_bswap32(dw) > __builtin_bswap32(p[j])) {
					max = i - 1;
				} else {
					min = i + 1;
				}
				blocked = false;
				break;
			}
			if (blocked) {
				delete[] binary_serial;
				return save_state(SERIAL_STATE_FLAG_BLACKLISTED);
			}
		}
	}

	// decode serial number
	BigNumber x(binary_serial, len);
	delete [] binary_serial;

	serial_ = x.modpow(*license_data_, 
		license_data_->GetDWord(FIELD_PUBLIC_EXP_OFFSET * sizeof(uint32_t)), license_data_->GetDWord(FIELD_PUBLIC_EXP_SIZE * sizeof(uint32_t)),
		license_data_->GetDWord(FIELD_MODULUS_OFFSET * sizeof(uint32_t)), license_data_->GetDWord(FIELD_MODULUS_SIZE * sizeof(uint32_t)));

	if (!serial_)
		return SERIAL_STATE_FLAG_INVALID;

	if (serial_->GetByte(0) != 0 || serial_->GetByte(1) != 2)
		return SERIAL_STATE_FLAG_INVALID;

	size_t pos;
	for (pos = 2; pos < serial_->size(); pos++) {
		if (!serial_->GetByte(pos)) {
			pos++;
			break;
		}
	}
	if (pos == serial_->size())
		return SERIAL_STATE_FLAG_INVALID;

	start_ = pos;
	return ParseSerial(NULL);
}

uint32_t LicensingManager::GetCurrentDate()
{
	uint32_t cur_date;
#ifdef VMP_GNU
	time_t rawtime;
	time(&rawtime);
	struct tm local_tm;
	tm *timeinfo = localtime_r(&rawtime, &local_tm);
	cur_date = ((timeinfo->tm_year + 1900) << 16) + (static_cast<uint8_t>(timeinfo->tm_mon + 1) << 8) + static_cast<uint8_t>(timeinfo->tm_mday);
#elif defined(WIN_DRIVER)
	LARGE_INTEGER sys_time;
	LARGE_INTEGER local_time;
	TIME_FIELDS time_fields;

	KeQuerySystemTime(&sys_time);
	ExSystemTimeToLocalTime(&sys_time, &local_time);
	RtlTimeToTimeFields(&local_time, &time_fields);
	cur_date = (time_fields.Year << 16) + (static_cast<uint8_t>(time_fields.Month) << 8) + static_cast<uint8_t>(time_fields.Day);
#else
	typedef struct _KSYSTEM_TIME
	{
		ULONG LowPart;
		LONG High1Time;
		LONG High2Time;
	} KSYSTEM_TIME, *PKSYSTEM_TIME;

	typedef struct _KUSER_SHARED_DATA
	{
		ULONG TickCountLowDeprecated;
		ULONG TickCountMultiplier;
		KSYSTEM_TIME InterruptTime;
		KSYSTEM_TIME SystemTime;
		KSYSTEM_TIME TimeZoneBias;
		//...
	} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

	PKUSER_SHARED_DATA user_shared_data = reinterpret_cast<PKUSER_SHARED_DATA>(0x7FFE0000);
	LARGE_INTEGER sys_time, time_zone_bias, local_time;
	while (true) {
		sys_time.HighPart = user_shared_data->SystemTime.High1Time;
		sys_time.LowPart = user_shared_data->SystemTime.LowPart;
		if (sys_time.HighPart == user_shared_data->SystemTime.High2Time) 
			break;
	}
	while (true) {
		time_zone_bias.HighPart = user_shared_data->TimeZoneBias.High1Time;
		time_zone_bias.LowPart = user_shared_data->TimeZoneBias.LowPart;
		if (time_zone_bias.HighPart == user_shared_data->TimeZoneBias.High2Time)
			break;
	}
	local_time.QuadPart = sys_time.QuadPart - time_zone_bias.QuadPart;

	__int64 total_days_since_1601 = local_time.QuadPart / 864000000000ull;
	uint32_t number_of_400s = static_cast<uint32_t>(total_days_since_1601 / 146097);
	total_days_since_1601 -= number_of_400s * 146097;
	uint32_t number_of_100s = static_cast<uint32_t>((total_days_since_1601 * 100 + 75) / 3652425);
	total_days_since_1601 -= number_of_100s * 36524;
	uint32_t number_of_4s = static_cast<uint32_t>(total_days_since_1601 / 1461);
	total_days_since_1601 -= number_of_4s * 1461;
	uint16_t year = static_cast<uint16_t>((total_days_since_1601 * 100 + 75) / 36525);
	total_days_since_1601 -= 365 * year;
	year = static_cast<uint16_t>((number_of_400s * 400) + (number_of_100s * 100) + (number_of_4s * 4) + year + 1601);
	uint16_t day = static_cast<uint16_t>(total_days_since_1601 + 1);
	int days_in_month[] = {31, ((year % 400 == 0) || (year % 100 != 0) && (year % 4 == 0)) ? 29 : 28,
		31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	uint8_t month = 1;
	for (size_t i = 0; i < _countof(days_in_month); i++) {
		if (day > days_in_month[i]) {
			++month;
			day -= days_in_month[i];
		} else
			break;
	}
	assert(month <= 12);
	cur_date = (year << 16) + (static_cast<uint8_t>(month) << 8) + static_cast<uint8_t>(day);
#endif

	return std::max(cur_date, loader_data->server_date());
}

int LicensingManager::ParseSerial(VMProtectSerialNumberData *data)
{
	if (!serial_)
		return SERIAL_STATE_FLAG_INVALID;

	int new_state = state_ & (SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED | SERIAL_STATE_FLAG_DATE_EXPIRED | SERIAL_STATE_FLAG_RUNNING_TIME_OVER);
	size_t pos = start_;
	while (pos < serial_->size()) {
		uint8_t b = serial_->GetByte(pos++);
		uint8_t s;
		switch (b) {
		case SERIAL_CHUNK_VERSION:
			if (serial_->GetByte(pos) != 1)
				return save_state(SERIAL_STATE_FLAG_INVALID);
			pos += 1;
			break;
		case SERIAL_CHUNK_EXP_DATE:
			uint32_t exp_date;
			exp_date = serial_->GetDWord(pos);
			if ((new_state & SERIAL_STATE_FLAG_DATE_EXPIRED) == 0) {
				if (license_data_->GetDWord(FIELD_BUILD_DATE * sizeof(uint32_t)) > exp_date || GetCurrentDate() > exp_date)
					new_state |= SERIAL_STATE_FLAG_DATE_EXPIRED;
			}
			if (data) {
				data->dtExpire.wYear = exp_date >> 16;
				data->dtExpire.bMonth = static_cast<uint8_t>(exp_date >> 8);
				data->dtExpire.bDay = static_cast<uint8_t>(exp_date);
			}
			pos += 4;
			break;
		case SERIAL_CHUNK_RUNNING_TIME_LIMIT:
			s = serial_->GetByte(pos);
			if ((new_state & SERIAL_STATE_FLAG_RUNNING_TIME_OVER) == 0) {
				uint32_t tick_count = GetTickCount();
				size_t cur_time = (tick_count - start_tick_count_) / 1000 / 60;
				if (cur_time > s)
					new_state |= SERIAL_STATE_FLAG_RUNNING_TIME_OVER;
			}
			if (data)
				data->bRunningTime = s;
			pos += 1;
			break;
		case SERIAL_CHUNK_PRODUCT_CODE:
			if (state_ == SERIAL_STATE_FLAG_INVALID)
				product_code_ = serial_->GetQWord(pos);
			pos += 8;
			break;
		case SERIAL_CHUNK_MAX_BUILD:
			uint32_t max_build_date;
			max_build_date = serial_->GetDWord(pos);
			if ((new_state & SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED) == 0) {
				if (license_data_->GetDWord(FIELD_BUILD_DATE * sizeof(uint32_t)) > max_build_date)
					new_state |= SERIAL_STATE_FLAG_MAX_BUILD_EXPIRED;
			}
			if (data) {
				data->dtMaxBuild.wYear = max_build_date >> 16;
				data->dtMaxBuild.bMonth = static_cast<uint8_t>(max_build_date >> 8);
				data->dtMaxBuild.bDay = static_cast<uint8_t>(max_build_date);
			}
			pos += 4;
			break;
		case SERIAL_CHUNK_USER_NAME:
			s = serial_->GetByte(pos++);
			if (data)
				serial_->UTF8ToUnicode(pos, s, data->wUserName, _countof(data->wUserName));
			pos += s;
			break;
		case SERIAL_CHUNK_EMAIL:
			s = serial_->GetByte(pos++);
			if (data)
				serial_->UTF8ToUnicode(pos, s, data->wEMail, _countof(data->wEMail));
			pos += s;
			break;
		case SERIAL_CHUNK_HWID:
			s = serial_->GetByte(pos++);
			if (state_ == SERIAL_STATE_FLAG_INVALID) {
				HardwareID *hardware_id = Core::Instance()->hardware_id();
				if (!hardware_id->IsCorrect(*serial_, pos, s)) 
					return save_state(SERIAL_STATE_FLAG_BAD_HWID);
			}
			pos += s;
			break;
		case SERIAL_CHUNK_USER_DATA:
			s = serial_->GetByte(pos++);
			if (data) {
				data->nUserDataLength = static_cast<uint8_t>(s);
				for (size_t i = 0; i < s; i++) {
					data->bUserData[i] = serial_->GetByte(pos + i);
				}
			}
			pos += s;
			break;
		case SERIAL_CHUNK_END:
			if (pos + 4 > serial_->size())
				return save_state(SERIAL_STATE_FLAG_INVALID);

			if (state_ == SERIAL_STATE_FLAG_INVALID) {
				// calc hash without last chunk
				SHA1 hash;
				hash.Input(*serial_, start_, pos - start_ - 1);

				// check CRC
				const uint8_t *p = hash.Result();
				for (size_t i = 0; i < 4; i++) {
					if (serial_->GetByte(pos + i) != p[3 - i])
						return save_state(SERIAL_STATE_FLAG_INVALID);
				}
			}

			return save_state(new_state);
		}
	}

	// SERIAL_CHUNK_END not found
	return save_state(SERIAL_STATE_FLAG_INVALID);
}

int LicensingManager::GetSerialNumberState()
{
	CriticalSection	cs(critical_section_);

	if (state_ & (SERIAL_STATE_FLAG_CORRUPTED | SERIAL_STATE_FLAG_INVALID | SERIAL_STATE_FLAG_BLACKLISTED | SERIAL_STATE_FLAG_BAD_HWID))
		return state_; // no reasons to continue

	return ParseSerial(NULL);
}

bool LicensingManager::GetSerialNumberData(VMProtectSerialNumberData *data, int size)
{
	if (!data || size != sizeof(VMProtectSerialNumberData)) 
		return false; // bad input

	CriticalSection	cs(critical_section_);

	if (state_ == SERIAL_STATE_FLAG_CORRUPTED)
		return false;

	// clean memory
	uint8_t *p = reinterpret_cast<uint8_t *>(data);
	for (int i = 0; i < size; i++) {
		p[i] = 0;
	}
	
	data->nState = (state_ & (SERIAL_STATE_FLAG_INVALID | SERIAL_STATE_FLAG_BLACKLISTED | SERIAL_STATE_FLAG_BAD_HWID)) ? state_ : ParseSerial(data);
	return true;
}

int LicensingManager::ActivateLicense(const char *code, char *serial, int size) const
{
#ifdef WIN_DRIVER
	return ACTIVATION_NOT_AVAILABLE;
#else
	if (!CheckLicenseDataCRC())
		return ACTIVATION_CORRUPTED;

	ActivationRequest request;
	int res = request.Process(*license_data_, code, false);
	if (res == ACTIVATION_OK) {
		int need = static_cast<int>(strlen(request.serial()));
		if (need > size - 1) 
			return ACTIVATION_SMALL_BUFFER;
		strncpy_s(serial, size, request.serial(), need);
	}
	return res;
#endif
}

int LicensingManager::DeactivateLicense(const char *serial) const
{
#ifdef WIN_DRIVER
	return ACTIVATION_NOT_AVAILABLE;
#else
	if (!CheckLicenseDataCRC())
		return ACTIVATION_CORRUPTED;

	DeactivationRequest request;
	return request.Process(*license_data_, serial, false);
#endif
}

int LicensingManager::GetOfflineActivationString(const char *code, char *buf, int size) const
{
#ifdef WIN_DRIVER
	return ACTIVATION_NOT_AVAILABLE;
#else
	if (!buf || size <= 0) 
		return ACTIVATION_SMALL_BUFFER;

	if (!CheckLicenseDataCRC())
		return ACTIVATION_CORRUPTED;

	ActivationRequest request;
	int res = request.Process(*license_data_, code, true);
	if (res == ACTIVATION_OK) {
		int need = static_cast<int>(strlen(request.url()));
		if (need > size - 1) 
			return ACTIVATION_SMALL_BUFFER;
		strncpy_s(buf, size, request.url(), need);
	}
	return res;
#endif
}

int LicensingManager::GetOfflineDeactivationString(const char *serial, char *buf, int size) const
{
#ifdef WIN_DRIVER
	return ACTIVATION_NOT_AVAILABLE;
#else
	if (!buf || size <= 0) 
		return ACTIVATION_SMALL_BUFFER;

	if (!CheckLicenseDataCRC())
		return ACTIVATION_CORRUPTED;

	DeactivationRequest request;
	int res = request.Process(*license_data_, serial, true);
	if (res == ACTIVATION_OK) {
		int need = static_cast<int>(strlen(request.url()));
		if (need > size - 1) 
			return ACTIVATION_SMALL_BUFFER;
		strncpy_s(buf, size, request.url(), need);
	}
	return res;
#endif
}

void LicensingManager::DecryptBuffer(uint8_t *buffer)
{
	uint32_t key0 = static_cast<uint32_t>(product_code_);
	uint32_t key1 = static_cast<uint32_t>(product_code_ >> 32) + session_key_;
	uint32_t *p = reinterpret_cast<uint32_t*>(buffer);

	p[0] = _rotl32((p[0] + session_key_) ^ key0,  7) + key1;
	p[1] = _rotl32((p[1] + session_key_) ^ key0, 11) + key1;
	p[2] = _rotl32((p[2] + session_key_) ^ key0, 17) + key1;
	p[3] = _rotl32((p[3] + session_key_) ^ key0, 23) + key1;

	if (p[0] + p[1] + p[2] + p[3] != session_key_ * 4) {
		const VMP_CHAR *message;
#ifdef VMP_GNU
		message = VMProtectDecryptStringA(MESSAGE_SERIAL_NUMBER_REQUIRED_STR);
#else
		message = VMProtectDecryptStringW(MESSAGE_SERIAL_NUMBER_REQUIRED_STR);
#endif
		if (message[0]) 
			ShowMessage(message);

#if defined(VMP_GNU)
		exit(0xDEADC0DE);
#elif defined(WIN_DRIVER)
		DbgBreakPointWithStatus(0xDEADC0DE);
#else
		TerminateProcess(GetCurrentProcess(), 0xDEADC0DE);
#endif
	}
}

#ifndef WIN_DRIVER

/**
 * BaseRequest
 */

BaseRequest::BaseRequest()
	: response_(NULL)
{
	url_[0] = 0;
}

BaseRequest::~BaseRequest()
{
	delete [] response_;
}

bool BaseRequest::BuildUrl(const CryptoContainer &license_data)
{
	size_t url_size = license_data.GetDWord(FIELD_ACTIVATION_URL_SIZE * sizeof(uint32_t));
	if (!url_size)
		return false;

	size_t url_offset = license_data.GetDWord(FIELD_ACTIVATION_URL_OFFSET * sizeof(uint32_t));
	for (size_t i = 0; i < url_size; i++) {
		url_[i] = license_data.GetByte(url_offset + i);
	}
	if (url_[url_size] != '/')
		url_[url_size++] = '/';
	url_[url_size] = 0;
	return true;
}

#ifdef __unix__
static size_t curl_write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = size * nmemb;
  
  std::string *dest = (std::string *)stream;
  *dest += std::string((char *)ptr, written);
  return written;
}
static size_t curl_header(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = size * nmemb;

	if(strncmp((char*)ptr, "Date: ", 6) == 0)
	{
		*((time_t *)stream) = curl_getdate((char *)ptr + 6, NULL);
	}
	return written;
}
#endif

bool BaseRequest::Send()
{
	if (response_) {
		delete [] response_;
		response_ = NULL;
	}

#ifdef __APPLE__
	CFStringRef str_ref = CFStringCreateWithCString(NULL, url_, kCFStringEncodingMacRoman);
	CFURLRef url_ref = CFURLCreateWithString(kCFAllocatorDefault, str_ref, NULL);
	CFHTTPMessageRef req_ref = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("GET"), url_ref, kCFHTTPVersion1_1);
	CFReadStreamRef stream_ref = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, req_ref);

	CFReadStreamOpen(stream_ref);
	int current;
	int size = 8096;
	response_ = new char[size];
	CFIndex read_size;
	for (current = 0; current < size; current += read_size) {
		read_size = CFReadStreamRead(stream_ref, reinterpret_cast<uint8_t *>(response_ + current), size - current);
		if (read_size < 0)
			break; // error
		if (!read_size) 
			break; // end of data
	}
	if (current < size)
		response_[current] = 0;
	bool res = current > 0 && current < size;

	CFHTTPMessageRef resp = (CFHTTPMessageRef)CFReadStreamCopyProperty(stream_ref, kCFStreamPropertyHTTPResponseHeader);
	if(resp)
	{
		CFStringRef dateHeaderRef = CFHTTPMessageCopyHeaderFieldValue(resp, CFSTR("Date"));
		if (dateHeaderRef != NULL)
		{
			CFGregorianDate gdate;
			CFIndex count = _CFGregorianDateCreateWithString(kCFAllocatorDefault, dateHeaderRef, &gdate, NULL);
			if (count != 0)
				loader_data->set_server_date((gdate.year << 16) + (static_cast<uint8_t>(gdate.month) << 8) + static_cast<uint8_t>(gdate.day));
			CFRelease(dateHeaderRef);
		}
		CFRelease(resp);
	}
	CFReadStreamClose(stream_ref);

	CFRelease(stream_ref);
	CFRelease(req_ref);
	CFRelease(url_ref);
	CFRelease(str_ref);

	return res;
#elif defined(__unix__)
	CURL *curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url_);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_header);
		std::string dest;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dest);
		time_t file_time = (time_t)-1;
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &file_time);
		CURLcode c = curl_easy_perform(curl);
		if (c == CURLE_OK) {
			response_ = strdup(dest.c_str());
			if (file_time != (time_t)-1) {
				struct tm local_tm;
				tm *t = localtime_r(&file_time, &local_tm);
				if (t)
					loader_data->set_server_date(((1900 + t->tm_year) << 16) + (static_cast<uint8_t>(t->tm_mon + 1) << 8) + static_cast<uint8_t>(t->tm_mday));
			}
		}
		curl_easy_cleanup(curl);
		return (c == CURLE_OK);
	}
	return false;
#else
	HMODULE dll = LoadLibraryA(VMProtectDecryptStringA("winhttp.dll"));
	if (!dll) 
		return false;

	typedef HINTERNET (WINAPI *HTTP_OPEN)(LPCWSTR lpszAgent, DWORD dwAccessType, LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags);
	typedef BOOL (WINAPI *HTTP_CLOSE_HANDLE)(HINTERNET hInternet);
	typedef BOOL (WINAPI *HTTP_READ_DATA)(HINTERNET hFile, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead);
	typedef BOOL (WINAPI *HTTP_CRACK_URL)(LPCWSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTS lpUrlComponents);
	typedef HINTERNET (WINAPI *HTTP_CONNECT)(HINTERNET hInternet, LPCWSTR lpszServerName, INTERNET_PORT nServerPort, DWORD dwReserved);
	typedef BOOL (WINAPI *HTTP_SETCREDENTIALS)(HINTERNET hRequest, DWORD AuthTargets, DWORD AuthScheme, LPCWSTR pszUserName, LPCWSTR pszPassword, LPVOID pAuthParams);
	typedef HINTERNET (WINAPI *HTTP_OPEN_REQUEST)(HINTERNET hConnect, LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion, LPCWSTR lpszReferer, LPCWSTR *lplpszAcceptTypes, DWORD dwFlags);
	typedef BOOL (WINAPI *HTTP_SEND_REQUEST)(HINTERNET hRequest, LPCWSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength, DWORD dwTotalLength, DWORD_PTR dwContext);
	typedef BOOL (WINAPI *HTTP_RECEIVE_RESPONSE)(HINTERNET hRequest, LPVOID lpReserved);
	typedef BOOL (WINAPI *HTTP_SET_OPTION)(HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength);
	typedef BOOL (WINAPI *HTTP_GET_IE_PROXY_CONFIG)(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *pProxyConfig);
	typedef BOOL(WINAPI *HTTP_QUERY_HEADERS)(HINTERNET hRequest, DWORD dwInfoLevel, LPCWSTR pwszName, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex);

	HTTP_OPEN http_open = reinterpret_cast<HTTP_OPEN>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpOpen")));
	HTTP_READ_DATA http_read_data = reinterpret_cast<HTTP_READ_DATA>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpReadData")));
	HTTP_CLOSE_HANDLE http_close_handle = reinterpret_cast<HTTP_CLOSE_HANDLE>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpCloseHandle")));
	HTTP_CRACK_URL http_crack_url = reinterpret_cast<HTTP_CRACK_URL>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpCrackUrl")));
	HTTP_CONNECT http_connect = reinterpret_cast<HTTP_CONNECT>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpConnect")));
	HTTP_SETCREDENTIALS http_setcredentials = reinterpret_cast<HTTP_SETCREDENTIALS>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpSetCredentials")));
	HTTP_OPEN_REQUEST http_open_request = reinterpret_cast<HTTP_OPEN_REQUEST>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpOpenRequest")));
	HTTP_SEND_REQUEST http_send_request = reinterpret_cast<HTTP_SEND_REQUEST>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpSendRequest")));
	HTTP_RECEIVE_RESPONSE http_receive_response = reinterpret_cast<HTTP_RECEIVE_RESPONSE>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpReceiveResponse")));
	HTTP_SET_OPTION http_set_option = reinterpret_cast<HTTP_SET_OPTION>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpSetOption")));
	HTTP_GET_IE_PROXY_CONFIG http_get_ie_proxy_config  = reinterpret_cast<HTTP_GET_IE_PROXY_CONFIG>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpGetIEProxyConfigForCurrentUser")));
	HTTP_QUERY_HEADERS http_query_headers = reinterpret_cast<HTTP_QUERY_HEADERS>(InternalGetProcAddress(dll, VMProtectDecryptStringA("WinHttpQueryHeaders")));

	bool res = false;
	if (http_open
		&& http_read_data
		&& http_close_handle
		&& http_crack_url
		&& http_connect
		&& http_setcredentials
		&& http_open_request
		&& http_send_request
		&& http_receive_response
		&& http_set_option
		&& http_query_headers) {
			WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_proxy_config = WINHTTP_CURRENT_USER_IE_PROXY_CONFIG();
			if (http_get_ie_proxy_config)
				http_get_ie_proxy_config(&ie_proxy_config);
			// We are not using WPAD directly, it is buggy (OLE actively used and may hung).
			DWORD dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY; // default: settings from global registry, 8.1+ deprecated. 
#ifdef _WIN64
			PEB64 *peb = reinterpret_cast<PEB64 *>(__readgsqword(0x60));
#else
			PEB32 *peb = reinterpret_cast<PEB32 *>(__readfsdword(0x30));
#endif
			uint16_t os_build_number = peb->OSBuildNumber;
			if (os_build_number >= WINDOWS_8_1)
				dwAccessType = 4; // WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY - 8.1+: smart from user IE config and/or global registry
			
			if (ie_proxy_config.lpszProxy)
				dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY; // IE manual proxy setup
			HINTERNET inet = http_open(VMProtectDecryptStringW(L"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)"), dwAccessType, ie_proxy_config.lpszProxy, ie_proxy_config.lpszProxyBypass, 0);
			if (inet) {
				URL_COMPONENTS components = URL_COMPONENTS();

				components.dwStructSize = sizeof(components);
#define INTERNET_MAX_HOST_NAME_LENGTH   256
#define INTERNET_MAX_USER_NAME_LENGTH   128
#define INTERNET_MAX_PASSWORD_LENGTH    128
#define INTERNET_MAX_PATH_LENGTH        2048
				wchar_t url_host[INTERNET_MAX_HOST_NAME_LENGTH];
				wchar_t url_path[INTERNET_MAX_PATH_LENGTH];
				wchar_t url_user[INTERNET_MAX_USER_NAME_LENGTH];
				wchar_t url_password[INTERNET_MAX_PASSWORD_LENGTH];
				components.lpszHostName = url_host;
				components.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
				components.lpszUserName = url_user;
				components.dwUserNameLength = INTERNET_MAX_USER_NAME_LENGTH;
				components.lpszPassword = url_password;
				components.dwPasswordLength = INTERNET_MAX_PASSWORD_LENGTH;
				components.lpszUrlPath = url_path;
				components.dwUrlPathLength = INTERNET_MAX_PATH_LENGTH;
				http_crack_url(std::wstring(url_, url_ + strlen(url_)).c_str(), 0, 0, &components);

				HINTERNET h = 0;
				HINTERNET connect = http_connect(inet, components.lpszHostName, components.nPort, 0);
				if (connect) {
					h = http_open_request(connect, L"GET", components.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_BYPASS_PROXY_CACHE | (
						(components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0
						));
					if (h) {
						//TODO: internet_setcredentials if need
						if (components.nScheme == INTERNET_SCHEME_HTTPS) {
							DWORD data = SECURITY_FLAG_IGNORE_UNKNOWN_CA 
								| SECURITY_FLAG_IGNORE_CERT_DATE_INVALID 
								| SECURITY_FLAG_IGNORE_CERT_CN_INVALID 
								/*| SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE*/;
							http_set_option(h, WINHTTP_OPTION_SECURITY_FLAGS, &data, sizeof(data));
						}
						if (!http_send_request(h, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL) || !http_receive_response( h, NULL)) {
							http_close_handle(h);
							h = 0;
						}
					}
				}

				if (h) {
					DWORD current;
					DWORD size = 8096;
					response_ = new char[size];
					DWORD read_size;
					for (current = 0; current < size; current += read_size) {
						if (http_read_data(h, response_ + current, size - current, &read_size) != TRUE) 
							break; // error
						if (!read_size) 
							break; // end of data
					}
					if (current < size)
						response_[current] = 0;
					res = current > 0 && current < size;
					SYSTEMTIME dtBuf;
					DWORD dtBufLength = sizeof(dtBuf);
					if(http_query_headers(h, WINHTTP_QUERY_DATE | WINHTTP_QUERY_FLAG_SYSTEMTIME, WINHTTP_HEADER_NAME_BY_INDEX, &dtBuf, &dtBufLength, WINHTTP_NO_HEADER_INDEX)) {
						FILETIME ft;
						SystemTimeToFileTime(&dtBuf, &ft);
						FileTimeToSystemTime(&ft, &dtBuf);
						loader_data->set_server_date((dtBuf.wYear << 16) + (static_cast<uint8_t>(dtBuf.wMonth) << 8) + static_cast<uint8_t>(dtBuf.wDay));
					}
					http_close_handle(h);
				}
				if (connect)
					http_close_handle(connect);
				http_close_handle(inet);
			}

			if (ie_proxy_config.lpszAutoConfigUrl)
				GlobalFree(ie_proxy_config.lpszAutoConfigUrl);
			if (ie_proxy_config.lpszProxy)
				GlobalFree(ie_proxy_config.lpszProxy);
			if (ie_proxy_config.lpszProxyBypass)
				GlobalFree(ie_proxy_config.lpszProxyBypass);
	}
	FreeLibrary(dll);
	return res;
#endif
}

void BaseRequest::AppendUrlParam(const char *param, const char *value)
{
	AppendUrl(param, false);
	AppendUrl(value, true);
}

void BaseRequest::AppendUrl(const char *str, bool escape)
{
	size_t pos = 0;
	while (url_[pos]) {
		pos++;
	}

	size_t size = _countof(url_);
	if (escape) {
		while (*str && pos < size - 1 - 3)
		{
			uint8_t c = *str; 
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
				url_[pos++] = c;
			} else if (c == ' ') {
				url_[pos++] = '+';
			} else {
				const char *hex = "0123456789abcdef";
				url_[pos++] = '%';
				url_[pos++] = hex[c >> 4];
				url_[pos++] = hex[c & 0x0f];
			}
			str++;
        }
	} else {
		while (*str && pos < size) {
			url_[pos++] = *str;
			str++;
		}
	}
	url_[pos] = 0;
}

void BaseRequest::EncodeUrl()
{
	char buf[2048];
	strcpy_s(buf, url_);
	size_t url_size = sizeof(url_);
	Base64Encode(reinterpret_cast<const uint8_t *>(buf), strlen(buf), url_, url_size);
	url_[url_size] = 0;
}

/**
 * ActivationRequest
 */

ActivationRequest::ActivationRequest()
	: BaseRequest(), serial_(NULL)
{

}

bool ActivationRequest::VerifyCode(const char *code) const 
{
	if (!code || !code[0]) 
		return false;

	static const char *alphabet = "0123456789abcdefghijklmnopqrstuvwxyz-";
	for (const char *p = code; *p; p++)	{
		if (!strchr(alphabet, tolower(*p)))
			return false;
		if (p - code > 32)
			return false; // too long
	}
	return true;
}

bool ActivationRequest::BuildUrl(const CryptoContainer &license_data, const char *code, bool offline)
{
	if (!offline) {
		if (!BaseRequest::BuildUrl(license_data))
			return false;
	}

	// hwid -> base64
	char str_hwid[100];
	{
		uint8_t hwid[16 * sizeof(uint32_t)]; // HardwareID::MAX_BLOCKS
		
		size_t hwid_size = Core::Instance()->hardware_id()->Copy(hwid, sizeof(hwid));
		size_t dest_len = sizeof(str_hwid);
		Base64Encode(hwid, hwid_size, str_hwid, dest_len);
		str_hwid[dest_len] = 0;
	}

	// hash -> base64
	char str_hash[64];
	{
		SHA1 sha;
		sha.Input(license_data, license_data.GetDWord(FIELD_MODULUS_OFFSET * sizeof(uint32_t)), license_data.GetDWord(FIELD_MODULUS_SIZE * sizeof(uint32_t)));
		size_t dest_len = sizeof(str_hash);
		Base64Encode(sha.Result(), 20, str_hash, dest_len);
		str_hash[dest_len] = 0;
	}

	// build url
	if (offline) {
		char str[] = {'t', 'y', 'p', 'e', '=', 'a', 'c', 't', 'i', 'v', 'a', 't', 'i', 'o', 'n', '&', 'c', 'o', 'd', 'e', '=', 0};
		AppendUrlParam(str, code);
	} else {
		char str[] = {'a', 'c', 't', 'i', 'v', 'a', 't', 'i', 'o', 'n', '.', 'p', 'h', 'p', '?', 'c', 'o', 'd', 'e', '=', 0};
		AppendUrlParam(str, code);
	}
	{
		char str[] = {'&', 'h', 'w', 'i', 'd', '=', 0};
		AppendUrlParam(str, str_hwid);
	}
	{
		char str[] = {'&', 'h', 'a', 's', 'h', '=', 0};
		AppendUrlParam(str, str_hash);
	}

	if (offline)
		EncodeUrl();

	return true;
}

int ActivationRequest::Process(const CryptoContainer &license_data, const char *code, bool offline)
{
	if (!VerifyCode(code))
		return ACTIVATION_BAD_CODE;

	if (!BuildUrl(license_data, code, offline))
		return ACTIVATION_NOT_AVAILABLE;

	if (offline)
		return ACTIVATION_OK;

	if (!Send())
		return ACTIVATION_NO_CONNECTION;

	const char *res = response();
	if (!res || !res[0])
		return ACTIVATION_BAD_REPLY;

	// possible answers: OK, BAD, BANNED, USED, EXPIRED
	// if OK - see the serial number below

	if (res[0] == 'B' && res[1] == 'A' && res[2] == 'D' && res[3] == 0)
		return ACTIVATION_BAD_CODE;

	if (res[0] == 'B' && res[1] == 'A' && res[2] == 'N' && res[3] == 'N' && res[4] == 'E' && res[5] == 'D' && res[6] == 0)
		return ACTIVATION_BANNED;

	if (res[0] == 'U' && res[1] == 'S' && res[2] == 'E' && res[3] == 'D' && res[4] == 0)
		return ACTIVATION_ALREADY_USED;

	if (res[0] == 'E' && res[1] == 'X' && res[2] == 'P' && res[3] == 'I' && res[4] == 'R' && res[5] == 'E' && res[6] == 'D' && res[7] == 0)
		return ACTIVATION_EXPIRED;

	const char *endl = strchr(res, '\n');
	if (!endl)
		return ACTIVATION_BAD_REPLY;

	if (endl - res != 2)
		return ACTIVATION_BAD_REPLY;

	if (res[0] != 'O' || res[1] != 'K')
		return ACTIVATION_BAD_REPLY;
	
	size_t len = strlen(res + 3);
	if (len < 64)
		return ACTIVATION_BAD_REPLY;

	serial_ = res + 3;
	return ACTIVATION_OK;
}

/**
 * DeactivationRequest
 */

bool DeactivationRequest::VerifySerial(const char *serial) const
{
	if (!serial || !serial[0]) 
		return false;

	return true;
}

 bool DeactivationRequest::BuildUrl(const CryptoContainer &license_data, const char *serial, bool offline)
{
	if (!offline) {
		if (!BaseRequest::BuildUrl(license_data))
			return false;
	}

	size_t code_len = strlen(serial);
	size_t len = code_len;
	uint8_t *p = new uint8_t[len];
	if (!Base64Decode(serial, code_len, p, len)) {
		delete [] p;
		return false;
	}

	// get binary hash
	char str_hash[64];
	{
		SHA1 sha;
		sha.Input(p, len);
		size_t dst_len = sizeof(str_hash);
		Base64Encode(sha.Result(), 20, str_hash, dst_len);
		str_hash[dst_len] = 0;
	}
	delete [] p;

	if (offline) {
		char str[] = { 't', 'y', 'p', 'e', '=', 'd', 'e', 'a', 'c', 't', 'i', 'v', 'a', 't', 'i', 'o', 'n', '&', 'h', 'a', 's', 'h', '=', 0 };
		AppendUrlParam(str, str_hash);
	}
	else {
		char str[] = { 'd', 'e', 'a', 'c', 't', 'i', 'v', 'a', 't', 'i', 'o', 'n', '.', 'p', 'h', 'p', '?', 'h', 'a', 's', 'h', '=', 0 };
		AppendUrlParam(str, str_hash);
	}

	if (offline)
		EncodeUrl();

	return true;
}

int DeactivationRequest::Process(const CryptoContainer &license_data, const char *serial, bool offline)
{
	if (!VerifySerial(serial))
		return ACTIVATION_BAD_CODE;

	if (!BuildUrl(license_data, serial, offline)) 
		return ACTIVATION_NOT_AVAILABLE;

	if (offline)
		return ACTIVATION_OK;

	if (!Send())
		return ACTIVATION_NO_CONNECTION;

	const char *res = response();
	if (!res || !res[0])
		return ACTIVATION_BAD_REPLY;

	if (res[0] == 'O' && res[1] == 'K' && res[2] == 0)
		return ACTIVATION_OK;

	if (res[0] == 'E' && res[1] == 'R' && res[2] == 'R' && res[3] == 'O' && res[4] == 'R' && res[5] == 0)
		return ACTIVATION_CORRUPTED;

	if (res[0] == 'U' && res[1] == 'N' && res[2] == 'K' && res[3] == 'N' && res[4] == 'O' && res[5] == 'W' && res[6] == 'N' && res[7] == 0)
		return ACTIVATION_SERIAL_UNKNOWN;

	return ACTIVATION_BAD_REPLY;
};

#endif