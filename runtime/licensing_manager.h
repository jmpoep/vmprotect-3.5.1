#ifndef LICENSING_MANAGER_H
#define LICENSING_MANAGER_H

class CryptoContainer;

class LicensingManager
{
public:
	LicensingManager(uint8_t *data, uint32_t size, uint8_t *key);
	~LicensingManager();
	int SetSerialNumber(const char *serial);
	int GetSerialNumberState();
	bool GetSerialNumberData(VMProtectSerialNumberData *data, int size);
	int ActivateLicense(const char *code, char *serial, int size) const;
	int DeactivateLicense(const char *serial) const;
	int GetOfflineActivationString(const char *code, char *buf, int size) const;
	int GetOfflineDeactivationString(const char *serial, char *buf, int size) const;
	void DecryptBuffer(uint8_t *buffer);
	static uint32_t GetCurrentDate();
private:
	enum ChunkType
	{
		SERIAL_CHUNK_VERSION				= 0x01,	//	1 byte of data - version
		SERIAL_CHUNK_USER_NAME				= 0x02,	//	1 + N bytes - length + N bytes of customer's name (without enging \0).
		SERIAL_CHUNK_EMAIL					= 0x03,	//	1 + N bytes - length + N bytes of customer's email (without ending \0).
		SERIAL_CHUNK_HWID					= 0x04,	//	1 + N bytes - length + N bytes of hardware id (N % 4 == 0)
		SERIAL_CHUNK_EXP_DATE				= 0x05,	//	4 bytes - (year << 16) + (month << 8) + (day)
		SERIAL_CHUNK_RUNNING_TIME_LIMIT		= 0x06,	//	1 byte - number of minutes
		SERIAL_CHUNK_PRODUCT_CODE			= 0x07,	//	8 bytes - used for decrypting some parts of exe-file
		SERIAL_CHUNK_USER_DATA				= 0x08,	//	1 + N bytes - length + N bytes of user data
		SERIAL_CHUNK_MAX_BUILD				= 0x09,	//	4 bytes - (year << 16) + (month << 8) + (day)
		SERIAL_CHUNK_END					= 0xFF	//	4 bytes - checksum: the first four bytes of sha-1 hash from the data before that chunk
	};

	int save_state(int state);
	int ParseSerial(VMProtectSerialNumberData *data);
	bool CheckLicenseDataCRC() const;
	bool SendRequest(char *url, char *response, size_t size) const;

	CryptoContainer *license_data_;
	int state_;
	uint32_t start_tick_count_;
	size_t start_;
	CryptoContainer *serial_;
	uint64_t product_code_;
	uint32_t session_key_;
	CRITICAL_SECTION critical_section_;

	// no copy ctr or assignment op
	LicensingManager(const LicensingManager &);
	LicensingManager &operator =(const LicensingManager &);
};

#ifndef WIN_DRIVER
class BaseRequest
{
public:
	BaseRequest();
	virtual ~BaseRequest();
	bool Send();
	const char *response() const { return response_; }
	const char *url() const { return url_; }
protected:
	bool BuildUrl(const CryptoContainer &license_data);
	void EncodeUrl();
	void AppendUrlParam(const char *param, const char *value);
private:
	void AppendUrl(const char *str, bool escape);
	char url_[2048];
	char *response_;

	// no copy ctr or assignment op
	BaseRequest(const BaseRequest &);
	BaseRequest &operator =(const BaseRequest &);
};

class ActivationRequest : public BaseRequest
{
public:
	ActivationRequest();
	int Process(const CryptoContainer &license_data, const char *code, bool offline);
	const char *serial() const { return serial_; }
private:
	bool VerifyCode(const char *code) const;
	bool BuildUrl(const CryptoContainer &license_data, const char *code, bool offline);

	const char *serial_;
};

class DeactivationRequest : public BaseRequest
{
public:
	int Process(const CryptoContainer &license_data, const char *serial, bool offline);
private:
	bool VerifySerial(const char *serial) const;
	bool BuildUrl(const CryptoContainer &license_data, const char *serial, bool offline);
};
#endif

#endif