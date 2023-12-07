#ifndef CORE_H
#define CORE_H

#include "../runtime/common.h"

enum ProjectOption {
	cpDebugMode             = 0x00000002,
	cpCryptValues           = 0x00000008,
	cpIncludeWatermark      = 0x00000020,
	cpRunnerCRC             = 0x00000040,
	cpEncryptRegs           = 0x00000080,
	cpStripFixups           = 0x00008000,
	cpPack                  = 0x00000100,
	cpImportProtection      = 0x00000200,
	cpCheckDebugger			= 0x00000400,
	cpCheckVirtualMachine	= 0x00000800,
	cpMemoryProtection      = 0x00001000,
	cpResourceProtection    = 0x00010000,
	cpCheckKernelDebugger	= 0x00020000,
	cpStripDebugInfo		= 0x00040000,

	cpLoaderCRC				= 0x10000000,
#ifndef DEMO
	cpUnregisteredVersion   = 0x40000000,
#endif
	cpEncryptBytecode       = 0x80000000,
	cpVirtualFiles			= 0x08000000,
	cpInternalMemoryProtection = 0x04000000,
	cpLoader				= 0x02000000,
	cpMaximumProtection     = cpCryptValues | cpRunnerCRC | cpEncryptRegs | cpPack | cpImportProtection | cpMemoryProtection | cpResourceProtection | cpStripDebugInfo,
	cpUserOptionsMask       = 0x00FFFFFF
};

std::string VectorToBase64(const std::vector<uint8_t> &src);

static const VMP_CHAR *default_message[MESSAGE_COUNT] = 
{
	MESSAGE_DEBUGGER_FOUND_STR,
	MESSAGE_VIRTUAL_MACHINE_FOUND_STR,
	MESSAGE_FILE_CORRUPTED_STR,
	MESSAGE_SERIAL_NUMBER_REQUIRED_STR,
	MESSAGE_HWID_MISMATCHED_STR
};

struct LicenseDate {
	uint16_t Year;
	uint8_t Month;
	uint8_t Day;
	LicenseDate(uint32_t value = 0)
	{
		Day = value & 0xff;
		Month = (value >> 8) & 0xff;	
		Year = (value >> 16) & 0xffff;	
	};
	LicenseDate(uint16_t year, uint8_t month, uint8_t day)
		: Year(year), Month(month), Day(day) {};
	uint32_t value() const { return (Year << 16) | (Month << 8) | Day; }
};

class Core;
class BigNumber;
class ProjectTemplate;
class ProjectTemplateManager;

class RSA
{
public:
	RSA();
	RSA(const std::vector<uint8_t> &public_exp, const std::vector<uint8_t> &private_exp, const std::vector<uint8_t> &modulus);
	~RSA();
	bool Encrypt(Data &data);
	bool Decrypt(Data &data);
	bool CreateKeyPair(size_t key_length);
	std::vector<uint8_t> public_exp() const;
	std::vector<uint8_t> private_exp() const;
	std::vector<uint8_t> modulus() const;
private:
	BigNumber *private_exp_;
	BigNumber *public_exp_;
	BigNumber *modulus_;

	// no copy ctr or assignment op
	RSA(const RSA &);
	RSA &operator =(const RSA &);
};

#ifdef ULTIMATE
class LicensingManager;

struct LicenseInfo {
	uint32_t Flags;
	std::string CustomerName;
	std::string CustomerEmail;
	LicenseDate ExpireDate;
	std::string HWID;
	uint8_t RunningTimeLimit;
	LicenseDate MaxBuildDate;
	std::string UserData;
	LicenseInfo() : Flags(0), RunningTimeLimit(0) {}
};

class License: public IObject
{
public:
	explicit License(LicensingManager *owner, LicenseDate date, const std::string &customer_name, const std::string &customer_email, const std::string &order_ref, 
		const std::string &comments, const std::string &serial_number, bool blocked);
	~License();
	std::string customer_name() const { return customer_name_; }
	std::string customer_email() const { return customer_email_; }
	std::string order_ref() const { return order_ref_; }
	std::string comments() const { return comments_; }
	std::string serial_number() const { return serial_number_; }
	bool blocked() const { return blocked_; }
	void GetHash(uint8_t hash[20]);
	LicenseDate date() const { return date_; }
	void set_customer_name(const std::string &value);
	void set_customer_email(const std::string &value);
	void set_order_ref(const std::string &value);
	void set_date(LicenseDate value);
	void set_comments(const std::string &value);
	void set_blocked( bool value);
	LicenseInfo *info();
	void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
private:
	LicensingManager *owner_;
	LicenseDate date_;
	std::string customer_name_;
	std::string customer_email_;
	std::string order_ref_;
	std::string comments_;
	std::string serial_number_;
	bool blocked_;
	LicenseInfo *info_;

	// no copy ctr or assignment op
	License(const License &);
	License &operator =(const License &);
};

enum Algorithm {
	alNone,
	alRSA
};

enum SerialNumberFlags {
	HAS_USER_NAME		= 0x0001,
	HAS_EMAIL			= 0x0002,
	HAS_EXP_DATE		= 0x0004,
	HAS_MAX_BUILD_DATE	= 0x0008,
	HAS_TIME_LIMIT		= 0x0010,
	HAS_HARDWARE_ID		= 0x0020,
	HAS_USER_DATA		= 0x0040,
	SN_FLAGS_PADDING	= 0xFFFF
};

class LicensingManager : public ObjectList<License>
{
public:
	explicit LicensingManager(Core *owner = NULL);
	virtual bool GetLicenseData(Data &data) const;
	virtual void clear();
	bool Open(const std::string &file_name);
	bool Save();
	bool SaveAs(const std::string &file_name);
	bool empty() const { return algorithm_ == alNone; }
	uint64_t product_code() const;
	bool Init(size_t key_len);
	Algorithm algorithm() const { return algorithm_; }
	uint16_t bits() const { return bits_; }
	std::vector<uint8_t> public_exp() const { return public_exp_; }
	std::vector<uint8_t> private_exp() const { return private_exp_; }
	std::vector<uint8_t> modulus() const { return modulus_; }
	std::vector<uint8_t> hash() const;
	std::string activation_server() const { return activation_server_; }
	void set_activation_server(const std::string &activation_server) { activation_server_ = activation_server; }
	void set_build_date(uint32_t build_date) { build_date_ = build_date; }
	std::string GenerateSerialNumber(const LicenseInfo &license_info);
	bool DecryptSerialNumber(const std::string &serial_number, LicenseInfo &license_info);
	License *Add(LicenseDate date, const std::string &customer_name, const std::string &customer_email, const std::string &order_ref, 
		const std::string &comments, const std::string &serial_number, bool blocked);
	License *GetLicenseBySerialNumber(const std::string &serial_number);
	bool CompareParameters(const LicensingManager &manager) const;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	virtual void AddObject(License *license);
	virtual void RemoveObject(License *license);
private:
	void changed();
	Core *owner_;
	std::string file_name_;
	Algorithm algorithm_;
	uint16_t bits_;
	std::vector<uint8_t> public_exp_;
	std::vector<uint8_t> private_exp_;
	std::vector<uint8_t> modulus_;
	std::vector<uint8_t> product_code_;
	std::string activation_server_;
	uint32_t build_date_;
};

class FileManager;
class IFunction;
class FileStream;

class FileFolder : public ObjectList<FileFolder>
{
public:
	explicit FileFolder(FileFolder *owner, const std::string &name);
	explicit FileFolder(FileFolder *owner, const FileFolder &src);
	virtual ~FileFolder();
	FileFolder *Clone(FileFolder *owner) const;
	FileFolder *Add(const std::string &name);
	std::string name() const { return name_; }
	FileFolder *owner() const { return owner_; }
	void set_name(const std::string &name);
	void set_owner(FileFolder *owner);
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	std::string id() const;
	FileFolder *GetFolderById(const std::string &id) const;
	void WriteEntry(IFunction &data);
	void WriteName(IFunction &data, uint64_t image_base, uint32_t key);
	using IObject::CompareWith;
private:
	void changed();
	FileFolder *owner_;
	std::string name_;
	size_t entry_offset_;
};

class FileFolderList : public FileFolder
{
public:
	explicit FileFolderList(FileManager *owner);
	explicit FileFolderList(FileManager *owner, const FileFolderList &src);
	FileFolderList *Clone(FileManager *owner) const;
	std::vector<FileFolder*> GetFolderList() const;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	FileManager *owner() const { return owner_; }
private:
	FileManager *owner_;
};

enum InternalFileAction {
	faNone,
	faLoad,
	faRegister,
	faInstall
};

class InternalFile : public IObject
{
public:
	InternalFile(FileManager *owner, const std::string &name, const std::string &file_name, InternalFileAction action, FileFolder *folder);
	~InternalFile();
	std::string name() const { return name_; }
	std::string file_name() const { return file_name_; }
	std::string absolute_file_name() const;
	void set_name(const std::string &value);
	void set_file_name(const std::string &value);
	bool Open();
	void Close();
	virtual void WriteEntry(IFunction &func);
	virtual void WriteName(IFunction &func, uint64_t image_base, uint32_t key);
	virtual void WriteData(IFunction &func, uint64_t image_base, uint32_t key);
	void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	FileManager *owner() const { return owner_; }
	InternalFileAction action() const { return action_; }
	void set_action(InternalFileAction action);
	bool is_server() const { return (action_ == faRegister || action_ == faInstall); }
	FileFolder *folder() const { return folder_; }
	void set_folder(FileFolder *folder);
	size_t id() const;
	FileStream *stream() const { return stream_; }
private:
	FileManager *owner_;
	std::string name_;
	std::string file_name_;
	InternalFileAction action_;
	FileStream *stream_;
	size_t entry_offset_;
	FileFolder *folder_;

	// no copy ctr or assignment op
	InternalFile(const InternalFile &);
	InternalFile &operator =(const InternalFile &);
};

class FileManager : public ObjectList<InternalFile>
{
public:
	FileManager(Core *owner);
	~FileManager();
	virtual void clear();
	bool need_compile() const { return need_compile_; }
	void set_need_compile(bool need_compile);
	InternalFile *Add(const std::string &name, const std::string &file_name, InternalFileAction action, FileFolder *folder);
	bool OpenFiles();
	void CloseFiles();
	void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	Core *owner() const { return owner_; }
	uint32_t GetRuntimeOptions() const;
	size_t server_count() const;
	FileFolderList *folder_list() const { return folder_list_; }
private:
	Core *owner_;
	bool need_compile_;
	FileFolderList *folder_list_;

	// no copy ctr or assignment op
	FileManager(const FileManager &);
	FileManager &operator =(const FileManager &);
};

#endif

class WatermarkManager;
class IniFile;
class SettingsFile;

class Watermark : public IObject
{
public:
	Watermark(WatermarkManager *owner);
	Watermark(WatermarkManager *owner, const std::string &name, const std::string &value, size_t use_count, bool enabled);
	~Watermark();
	size_t id() const { return id_; }
	std::string name() const { return name_; }
	bool enabled() const { return enabled_; }
	std::string value() const { return value_; }
	size_t use_count() const { return use_count_; }
	void set_name(const std::string &name);
	void set_value(const std::string &value);
	void set_enabled(bool value);
	void Compile();
	bool SearchByte(uint8_t value);
	std::vector<uint8_t> dump() const { return dump_; }
	void inc_use_count();
	void ReadFromIni(IniFile &file, size_t id);
	void ReadFromNode(TiXmlElement *node);
	void SaveToNode(TiXmlElement *node);
	void SaveToFile(SettingsFile &file);
	void DeleteFromFile(SettingsFile &file);
	void InitSearch() { pos_.clear(); }
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	static bool AreSimilar(const std::string &v1, const std::string &v2);
	static bool SymbolsMatch(char v1, char v2);
private:
	WatermarkManager *owner_;
	size_t id_;
	std::string name_;
	std::string value_;
	size_t use_count_;
	bool enabled_;
	std::vector<uint8_t> dump_;
	std::vector<uint8_t> mask_;
	std::vector<size_t> pos_;
};

class WatermarkManager : public ObjectList<Watermark>
{
public:
	WatermarkManager(Core *owner);
	Watermark *Add(const std::string name, const std::string value, size_t use_count = 0, bool enabled = true);
	Watermark *GetWatermarkByName(const std::string &name);
	void InitSearch() const;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	void ReadFromFile(SettingsFile &file);
	virtual void RemoveObject(Watermark *watermark);
	void ReadFromIni(const std::string &file_name);
	void SaveToFile(SettingsFile &file);
	std::string CreateValue() const;
	Watermark *GetWatermarkByValue(const std::string &value) const;
	bool IsUniqueWatermark(const std::string &value) const;
private:
	Core *owner_;
};

enum VMProtectProductId
{
	VPI_NOT_SPECIFIED,		//0 legacy
	VPI_LITE_WIN_PERSONAL,	//1 VMProtect Lite for Windows (Personal License)
	VPI_LITE_WIN_COMPANY,	//2 VMProtect Lite for Windows (Company License)
	VPI_PROF_WIN_PERSONAL,	//3 VMProtect Professional for Windows (Personal License)
	VPI_PROF_WIN_COMPANY,	//4 VMProtect Professional for Windows (Company License)
	VPI_ULTM_WIN_PERSONAL,	//5 VMProtect Ultimate for Windows (Personal License)
	VPI_ULTM_WIN_COMPANY,	//6 VMProtect Ultimate for Windows (Company License)
	VPI_LITE_OSX_PERSONAL,	//7 VMProtect Lite for Mac OS X (Personal License)
	VPI_LITE_OSX_COMPANY,	//8 VMProtect Lite for Mac OS X (Company License)
	VPI_PROF_OSX_PERSONAL,	//9 VMProtect Professional for Mac OS X (Personal License)
	VPI_PROF_OSX_COMPANY,	//10 VMProtect Professional for Mac OS X (Company License)
	VPI_ULTM_OSX_PERSONAL,	//11 VMProtect Ultimate for Mac OS X (Personal License)
	VPI_ULTM_OSX_COMPANY,	//12 VMProtect Ultimate for Mac OS X (Company License)
	VPI_WLM_PERSONAL,		//13 VMProtect Web License Manager (Personal License)
	VPI_WLM_COMPANY,		//14 VMProtect Web License Manager (Company License)
	VPI_YEAR_PESONAL,		//15 Yearly Subscription Plan (Personal License)
	VPI_YEAR_COMPANY,		//16 Yearly Subscription Plan (Company License)
	VPI_SENS_WIN_PERSONAL,	//17 VMProtect SE for Windows (Personal License)
	VPI_SENS_WIN_COMPANY,	//18 VMProtect SE for Windows (Company License)
	VPI_LITE_LIN_PERSONAL,	//19 VMProtect Lite for Linux (Personal License)
	VPI_LITE_LIN_COMPANY,	//20 VMProtect Lite for Linux (Company License)
	VPI_PROF_LIN_PERSONAL,	//21 VMProtect Professional for Linux (Personal License)
	VPI_PROF_LIN_COMPANY,	//22 VMProtect Professional for Linux (Company License)
	VPI_ULTM_LIN_PERSONAL,	//23 VMProtect Ultimate for Linux (Personal License)
	VPI_ULTM_LIN_COMPANY,	//24 VMProtect Ultimate for Linux (Company License)
#ifdef __unix__
	VPI_LITE_PERSONAL = VPI_LITE_LIN_PERSONAL,
	VPI_LITE_COMPANY = VPI_LITE_LIN_COMPANY,
	VPI_PROF_PERSONAL = VPI_PROF_LIN_PERSONAL,
	VPI_PROF_COMPANY = VPI_PROF_LIN_COMPANY,
	VPI_ULTM_PERSONAL = VPI_ULTM_LIN_PERSONAL,
	VPI_ULTM_COMPANY = VPI_ULTM_LIN_COMPANY,
#elif defined(__APPLE__)
	VPI_LITE_PERSONAL = VPI_LITE_OSX_PERSONAL,
	VPI_LITE_COMPANY = VPI_LITE_OSX_COMPANY,
	VPI_PROF_PERSONAL = VPI_PROF_OSX_PERSONAL,
	VPI_PROF_COMPANY = VPI_PROF_OSX_COMPANY,
	VPI_ULTM_PERSONAL = VPI_ULTM_OSX_PERSONAL,
	VPI_ULTM_COMPANY = VPI_ULTM_OSX_COMPANY,
#else
	VPI_LITE_PERSONAL = VPI_LITE_WIN_PERSONAL,
	VPI_LITE_COMPANY = VPI_LITE_WIN_COMPANY,
	VPI_PROF_PERSONAL = VPI_PROF_WIN_PERSONAL,
	VPI_PROF_COMPANY = VPI_PROF_WIN_COMPANY,
	VPI_ULTM_PERSONAL = VPI_ULTM_WIN_PERSONAL,
	VPI_ULTM_COMPANY = VPI_ULTM_WIN_COMPANY,
#endif
};

#if defined(ULTIMATE)
#define EDITION "Ultimate"
#elif defined(LITE)
#define EDITION "Lite"
#else
#define EDITION "Professional"
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define STR_HELPERW(x) L ## #x
#define STRW(x) STR_HELPERW(x)
#define STR_HELPERWQ(x) L ## x
#define STRWQ(x) STR_HELPERWQ(x)

class ProjectTemplate : public IObject
{
public:
	ProjectTemplate(ProjectTemplateManager *owner, const std::string &name, bool is_default = false);
	ProjectTemplate &operator =(const ProjectTemplate &);
	~ProjectTemplate();

	void Reset();
	void ReadFromCore(const Core &core);
	void ReadFromNode(TiXmlElement *node);
	void SaveToNode(TiXmlElement *node) const;

	bool is_default() const { return is_default_; }
	void set_is_default(bool is_default) { is_default_ = is_default; }
	std::string name() const { return name_; }
	std::string display_name() const;
	uint32_t options() const { return options_; }
	std::string vm_section_name() const { return vm_section_name_; }
	std::string message(size_t idx) const;
	void set_name(const std::string &name);
	static std::string default_name() { return "(default)"; }

	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	bool operator ==(const ProjectTemplate &other) const;
	bool operator !=(const ProjectTemplate &other) const { return !operator==(other); }
private:
	void Init();
	ProjectTemplateManager *owner_;
	bool is_default_;
	uint32_t options_;
	std::string name_, vm_section_name_;
	std::string messages_[MESSAGE_COUNT];

	// no copy ctr or assignment op
	ProjectTemplate(const ProjectTemplate &);
};

class ProjectTemplateManager : public ObjectList<ProjectTemplate>
{
public:
	ProjectTemplateManager(Core *owner);
	void ReadFromFile(SettingsFile &file);
	void SaveToFile(SettingsFile &file) const;
	void Add(const std::string &name, const Core &core);
	void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	void RemoveObject(ProjectTemplate *pt);
private:
	Core *owner_;

	// no copy ctr or assignment op
	ProjectTemplateManager(const ProjectTemplateManager &);
	ProjectTemplateManager &operator =(const ProjectTemplateManager &);
};

class Script;
class ILog;
class IFile;
class IArchitecture;

class Core : public IObject
{
public:
	explicit Core(ILog *log = NULL);
	virtual ~Core();
#ifdef ULTIMATE
	bool Open(const std::string &file_name, const std::string &user_project_file_name = "", const std::string &user_licensing_params_file_name = "");
#else
	bool Open(const std::string &file_name, const std::string &user_project_file_name = "");
#endif
	bool Save();
	bool SaveAs(const std::string &file_name);
	void Close();
	bool Compile();

	uint32_t options() const { return options_; }
	std::string vm_section_name() const { return vm_section_name_; }
	std::string watermark_name() const { return watermark_name_; }
	IFile *input_file() const { return input_file_; }
	IFile *output_file() const { return output_file_; }
	ILog *log() const  { return log_; }
	std::string input_file_name() const { return input_file_name_; }
	std::string output_file_name() const { return output_file_name_; }
	void set_options(uint32_t options);
	void include_option(ProjectOption option);
	void exclude_option(ProjectOption option);
	void set_vm_section_name(const std::string &vm_section_name);
	void set_watermark_name(const std::string &watermark_name);
	void set_output_file_name(const std::string &output_file_name);
	std::string message(size_t type) const { return messages_[type]; }
	void set_message(size_t type, const std::string &message);
#ifdef ULTIMATE
	std::string hwid() const { return hwid_; }
	void set_hwid(const std::string &hwid);
	LicensingManager *licensing_manager() const { return licensing_manager_; }
	FileManager *file_manager() const { return file_manager_; }
	std::string license_data_file_name() const { return license_data_file_name_; }
	void set_license_data_file_name(const std::string &license_data_file_name);
	std::string activation_server() const { return licensing_manager_->activation_server(); }
	void set_activation_server(const std::string &activation_server);
	std::string default_license_data_file_name() const;
#endif
	std::string project_path() const;
	void Notify(MessageType type, IObject *sender, const std::string &message = "");
	std::string absolute_output_file_name() const;
	std::string project_file_name() const { return project_file_name_; }
	WatermarkManager *watermark_manager() const { return watermark_manager_; }
	ProjectTemplateManager *template_manager() const { return template_manager_; }
	Script *script() const { return script_; }
	static const char *copyright() { return "Copyright 2003-2021 VMProtect Software"; }
	static const char *edition() { return "VMProtect " EDITION; }
	static const char *version();
	static const char *build();
	static bool check_license_edition(const VMProtectSerialNumberData &lic);
	IArchitecture *input_architecture() const;
	IArchitecture *output_architecture() const { return output_architecture_; }
	void LoadFromTemplate(const ProjectTemplate &pt);
	void SaveToTemplate(ProjectTemplate &pt);
private:
	HANDLE BeginCompileTransaction();
	void EndCompileTransaction(HANDLE locked_file, bool commit);
	bool LoadFromXML(const char *project_file_name);
	bool LoadFromIni(const char *project_file_name);
	void LoadDefaultFunctions();
	std::string default_output_file_name() const;

	std::string project_file_name_;
	bool modified_;
	IFile *input_file_;
	std::string input_file_name_;
	uint32_t options_;
	uint32_t vm_options_;
	std::string vm_section_name_;
	ProjectTemplateManager *template_manager_;
	std::string output_file_name_;
	std::string watermark_name_;
	std::string messages_[MESSAGE_COUNT];
	IFile *output_file_;
	ILog *log_;
	Watermark *watermark_;
	WatermarkManager *watermark_manager_;
	Script *script_;
	IArchitecture *output_architecture_;
#ifdef ULTIMATE
	std::string hwid_;
	std::string license_data_file_name_;
	LicensingManager *licensing_manager_;
	FileManager *file_manager_;
#endif

	// no copy ctr or assignment op
	Core(const Core &);
	Core &operator =(const Core &);
};

#endif
