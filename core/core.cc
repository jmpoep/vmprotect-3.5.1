#include "../runtime/crypto.h"

#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "processors.h"
#include "inifile.h"
#include "script.h"
#include "pefile.h"
#include "macfile.h"
#include "elffile.h"
#include "lang.h"
#include "core.h"

#ifdef __APPLE__
#include <Security/SecKey.h>
#include <Security/SecItem.h>
#include <Security/SecImportExport.h>
#endif // __APPLE__

#ifdef __unix__
namespace OpenSSL {
 #include <openssl/rsa.h>
}
#endif //__unix__

void Base64ToVector(const char *src, size_t src_len, std::vector<uint8_t> &dst)
{
	if (!src || !src_len) {
		dst.clear();
		return;
	}
	size_t dst_len = src_len;
	dst.resize(dst_len, 0);
	Base64Decode(src, src_len, &dst[0], dst_len);
	if (dst_len != dst.size())
		dst.resize(dst_len);
}

std::string VectorToBase64(const std::vector<uint8_t> &src)
{
	std::string dst;

	if (!src.empty()) {
		size_t dst_len = Base64EncodeGetRequiredLength(src.size());
		dst.resize(dst_len);
		Base64Encode(&src[0], src.size(), &dst[0], dst_len);
		if (dst_len != dst.size())
			dst.resize(dst_len);
	}
	return dst;
}

/**
 * Watermark
 */

Watermark::Watermark(WatermarkManager *owner)
	: IObject(), owner_(owner), id_(-1), use_count_(0), enabled_(false)
{

}

Watermark::Watermark(WatermarkManager *owner, const std::string &name, const std::string &value, size_t use_count, bool enabled)
	: IObject(), owner_(owner), id_(-1), name_(name), value_(value), use_count_(use_count), enabled_(enabled)
{
	Compile();
}

Watermark::~Watermark()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void Watermark::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

void Watermark::set_name(const std::string &name)
{
	if (name_ != name) {
		name_ = name;
		Notify(mtChanged, this);
	}
}

void Watermark::set_value(const std::string &value)
{
	if (value_ != value) {
		value_ = value;
		Notify(mtChanged, this);
	}
}

void Watermark::set_enabled(bool value)
{
	if (enabled_ != value) {
		enabled_ = value;
		Notify(mtChanged, this);
	}
}

void Watermark::inc_use_count()
{
	use_count_++;
	Notify(mtChanged, this);
}

void Watermark::ReadFromIni(IniFile &file, size_t id)
{
	id_ = id;
	name_ = file.ReadString("Watermarks", string_format("Name%d", id).c_str());
	value_ = file.ReadString("Watermarks", string_format("Value%d", id).c_str());
	use_count_ = file.ReadInt("Watermarks", string_format("UseCount%d", id).c_str());
	enabled_ = file.ReadBool("Watermarks", string_format("Enabled%d", id).c_str(), true);
	Compile();
}

void Watermark::ReadFromNode(TiXmlElement *node)
{
	unsigned int u;

	u = 0;
	node->QueryUnsignedAttribute("Id", &u);
	id_ = u;
	node->QueryStringAttribute("Name", &name_);
	u = 0;
	node->QueryUnsignedAttribute("UseCount", &u);
	use_count_ = u;
	enabled_ = true;
	node->QueryBoolAttribute("Enabled", &enabled_);
	if (const char *str = node->GetText())
		value_ = std::string(str);
	Compile();
}

void Watermark::SaveToNode(TiXmlElement *node)
{
	if (!node)
		return;

	node->Clear();
	node->SetAttribute("Id", (int)id_);
	node->SetAttribute("Name", name_);
	node->SetAttribute("UseCount", (int)use_count_);
	if (enabled_)
		node->RemoveAttribute("Enabled");
	else 
		node->SetAttribute("Enabled", enabled_);
	node->LinkEndChild(new TiXmlText(value_));
}

void Watermark::SaveToFile(SettingsFile &file)
{
	GlobalLocker locker;
	if (id_ == NOT_ID)
		id_ = file.inc_watermark_id();

	TiXmlElement *node = file.watermark_node(id_, true);
	if (node) {
		SaveToNode(node);
		file.Save();
	}
}

void Watermark::DeleteFromFile(SettingsFile &file)
{
	TiXmlElement *node = file.watermark_node(id_);
	if (node && node->Parent()->RemoveChild(node))
		file.Save();
}

void Watermark::Compile()
{
	dump_.clear();
	mask_.clear();

	if (value_.size() == 0)
		return;

	for (size_t i = 0; i < value_.size(); i++) {
		size_t p = i / 2;
		if (p >= dump_.size()) {
			dump_.push_back(0);
			mask_.push_back(0);
		}

		uint8_t m = 0xff;
		uint8_t b;
		char c = value_[i];
		if ((c >= '0') && (c <= '9')) {
			b = c - '0';
		} else if ((c >= 'A') && (c <= 'F')) {
			b = c - 'A' + 0x0a;
		} else if ((c >= 'a') && (c <= 'f')) {
			b = c - 'a' + 0x0a;
		} else {
			m = 0;
			b = rand();
		}

		if ((i & 1) == 0) {
			dump_[p] = (dump_[p] & 0x0f) | (b << 4);
			mask_[p] = (mask_[p] & 0x0f) | (m << 4);
		} else {
			dump_[p] = (dump_[p] & 0xf0) | (b & 0x0f);
			mask_[p] = (mask_[p] & 0xf0) | (m & 0x0f);
		}
	}
}

bool Watermark::SearchByte(uint8_t value)
{
	if (dump_.size() == 0)
		return false;

	bool res = false;
	for (int i = (int)(pos_.size() - 1); i >= -1; i--)	{
		size_t p = (i == -1) ? 0 : pos_[i];
		if ((dump_[p] & mask_[p]) == (value & mask_[p])) {
			p++;
			if (p == dump_.size()) {
				res = true;
				if (i > -1)
					pos_.erase(pos_.begin() + i);
			} else if (i == -1) {
				pos_.push_back(p);
			} else {
				pos_[i] = p;
			}
		} else if (i > -1) {
			pos_.erase(pos_.begin() + i);
		}
	}

	return res;
}

bool Watermark::AreSimilar(const std::string &v1, const std::string &v2)
{
	// v1 is part of v2 or is vice versa
	int v1_in_v2_pos = 0, v1_in_v2_pos_max = int(v2.length()) - int(v1.length());
	int v2_in_v1_pos = 0, v2_in_v1_pos_max = int(v1.length()) - int(v2.length());

	while(v1_in_v2_pos <= v1_in_v2_pos_max)
	{
		size_t v1p = 0;
		for(; v1p < v1.length(); v1p++)
		{
			if(!SymbolsMatch(v1[v1p], v2[v1_in_v2_pos + v1p]))
				break;
		}
		if(v1p == v1.length())
			return true;
		v1_in_v2_pos++;
	}
	if(v1_in_v2_pos_max == v2_in_v1_pos_max)
		return false;
	while(v2_in_v1_pos <= v2_in_v1_pos_max)
	{
		size_t v2p = 0;
		for(; v2p < v2.length(); v2p++)
		{
			if(!SymbolsMatch(v2[v2p], v1[v2_in_v1_pos + v2p]))
				break;
		}
		if(v2p == v2.length())
			return true;
		v2_in_v1_pos++;
	}
	return false;
}

bool Watermark::SymbolsMatch(char v1, char v2)
{
	if(v1 == '?' || v2 == '?')
		return true;
	assert('a' > 'A');
	if(v1 >= 'a')
		v1 = v1 - 'a' + 'A';
	if(v2 >= 'a')
		v2 = v2 - 'a' + 'A';
	return v1 == v2;
}

/**
 * WatermarkManager
 */

WatermarkManager::WatermarkManager(Core *owner)
	: ObjectList<Watermark>(), owner_(owner)
{

}

Watermark *WatermarkManager::Add(const std::string name, const std::string value, size_t use_count, bool enabled)
{
	Watermark *watermark = new Watermark(this, name, value, use_count, enabled);
	AddObject(watermark);
	Notify(mtAdded, watermark);
	return watermark;
}

Watermark *WatermarkManager::GetWatermarkByName(const std::string &name)
{
	for (size_t i = 0; i < count(); i++) {
		Watermark *watermark = item(i);
		if (watermark->name() == name)
			return watermark;
	}
	return NULL;
}

void WatermarkManager::InitSearch() const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->InitSearch();
	}
}

void WatermarkManager::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

void WatermarkManager::RemoveObject(Watermark *watermark)
{
	Notify(mtDeleted, watermark);
	ObjectList<Watermark>::RemoveObject(watermark);
}

void WatermarkManager::ReadFromIni(const std::string &file_name)
{
	IniFile file(file_name.c_str());
	std::vector<std::string> key_list = file.ReadSection("Watermarks", "Name");
	for (size_t i = 0; i < key_list.size(); i++) {
		std::string str = key_list[i];
		size_t id = StrToIntDef(&str[4], -1);
		if (id == NOT_ID)
			continue;

		Watermark *watermark = new Watermark(this);
		watermark->ReadFromIni(file, id);
		AddObject(watermark);
	}
}

void WatermarkManager::ReadFromFile(SettingsFile &file)
{
	TiXmlElement *watermarks_node = file.watermarks_node();
	if (!watermarks_node)
		return;

	TiXmlElement *node = watermarks_node->FirstChildElement("Watermark");
	while (node) {
		Watermark *watermark = new Watermark(this);
		watermark->ReadFromNode(node);
		AddObject(watermark);
		node = node->NextSiblingElement(node->Value());
	}
}

void WatermarkManager::SaveToFile(SettingsFile &file)
{
	TiXmlElement *watermarks_node = file.watermarks_node();
	if (!watermarks_node)
		return;

	watermarks_node->Clear();
	size_t id = 0;
	for (size_t i = 0; i < count(); i++) {
		TiXmlElement *node = new TiXmlElement("Watermark");
		watermarks_node->LinkEndChild(node);

		Watermark *watermark = item(i);
		watermark->SaveToNode(node);
		if (id < watermark->id())
			id = watermark->id();
	}
	watermarks_node->SetAttribute("Id", (int)(id + 1));
}

Watermark *WatermarkManager::GetWatermarkByValue(const std::string &value) const
{
	for (size_t i = 0; i < count(); i++) {
		Watermark *watermark = item(i);
		if (watermark->value() == value)
			return watermark;
	}
	return NULL;
}

bool WatermarkManager::IsUniqueWatermark(const std::string &value) const
{
	for (size_t i = 0; i < count(); i++) {
		Watermark *watermark = item(i);
		if (Watermark::AreSimilar(watermark->value(), value))
			return false;
	}
	return true;
}

std::string WatermarkManager::CreateValue() const
{
	std::string res;
	res.reserve(2 * (20 + 0xFF));
	do {
		res.clear();
		size_t c = 20 + rand() % 0x100;
		for (size_t i = 0; i < 2 * c; i++) {
			if (rand() & 1)
				res += '?';
			else
				res += string_format("%x", rand() % 0x10);
		}
	} while (!IsUniqueWatermark(res));
	return res;
}

/**
 * Core
 */

Core::Core(ILog *log /*=NULL*/)
	: IObject(), log_(log), input_file_(NULL), output_file_(NULL), watermark_(NULL), output_architecture_(NULL),
	options_(0), vm_options_(0)
{
#ifdef ULTIMATE
	licensing_manager_ = new LicensingManager(this);
	file_manager_ = new FileManager(this);
#endif
	watermark_manager_ = new WatermarkManager(this);
	template_manager_ = new ProjectTemplateManager(this);
	script_ = new Script(this);
#ifdef __APPLE__
	watermark_manager_->ReadFromFile(settings_file());
#else
	if (settings_file().watermarks_node_created()) {
		// convert old settings file into new format
		std::string ini_file_name = os::CombinePaths(os::GetSysAppDataDirectory().c_str(), "PolyTech/VMProtect/VMProtect.ini");
		if (os::FileExists(ini_file_name.c_str())) {
			watermark_manager_->ReadFromIni(ini_file_name);
			watermark_manager_->SaveToFile(settings_file());
			settings_file().Save();
		}
	} else {
		watermark_manager_->ReadFromFile(settings_file());
	}
#endif
	template_manager_->ReadFromFile(settings_file());
}

Core::~Core() 
{
	Close();

	delete script_;
	delete watermark_manager_;
	delete template_manager_;
#ifdef ULTIMATE
	delete file_manager_;
	delete licensing_manager_;
#endif
}

static std::string GetProjectFileName(std::string &exe_file_name)
{
	TiXmlDocument doc;
	std::string project_file_name = exe_file_name;

	if (doc.LoadFile(project_file_name.c_str())) {
		TiXmlElement *root_node = doc.FirstChildElement("Document");
		if (root_node) {
			exe_file_name.clear();
			TiXmlElement *protection_node = root_node->FirstChildElement("Protection");
			if (protection_node)
				protection_node->QueryStringAttribute("InputFileName", &exe_file_name);
			if (!exe_file_name.empty())
				exe_file_name = os::CombinePaths(os::ExtractFilePath(project_file_name.c_str()).c_str(), exe_file_name.c_str());
			return project_file_name;
		}
	}

	IniFile ini_file(exe_file_name.c_str());
	if (ini_file.ReadSection("ProjectInfo").size()) {
		exe_file_name = ini_file.ReadString("ProjectInfo", "InputFileName");
		if (!exe_file_name.empty())
			exe_file_name = os::CombinePaths(os::ExtractFilePath(project_file_name.c_str()).c_str(), exe_file_name.c_str());
		return project_file_name;
	}

	std::string file_name_new = exe_file_name + ".vmp";
	std::string file_name_old = os::ChangeFileExt(exe_file_name.c_str(), ".vmp");

	if (os::FileExists(file_name_new.c_str()) || !os::FileExists(file_name_old.c_str()))
		return file_name_new;

	return file_name_old;
}

std::string Core::default_output_file_name() const
{
	std::string res = input_file_name_;
	std::string ext = os::ExtractFileExt(res.c_str());
	if (ext.empty())
		return res + "_vmp";

	return os::ChangeFileExt(res.c_str(), ".vmp") + ext;
}

#ifdef ULTIMATE
std::string Core::default_license_data_file_name() const
{
	return os::ExtractFileName(project_file_name_.c_str());
}
#endif

#ifdef ULTIMATE
bool Core::Open(const std::string &file_name, const std::string &user_project_file_name, const std::string &user_licensing_params_file_name)
#else
bool Core::Open(const std::string &file_name, const std::string &user_project_file_name)
#endif
{
	std::string exe_file_name;
	size_t i;

	Close();

	exe_file_name = file_name;
	project_file_name_ = (user_project_file_name.empty()) ? GetProjectFileName(exe_file_name) : user_project_file_name;

#ifdef __APPLE__
	std::string new_file_name = os::GetMainExeFileName(exe_file_name.c_str());
	if (!new_file_name.empty())
		exe_file_name = new_file_name;
#endif

	if (!exe_file_name.empty()) {
		if (log_)
			log_->StartProgress(string_format("%s %s...", language[lsLoading].c_str(), os::ExtractFileName(exe_file_name.c_str()).c_str()), 1);

		std::auto_ptr<IFile> file[] = { std::auto_ptr<IFile>(new PEFile(log_)), std::auto_ptr<IFile>(new MacFile(log_)), 
			std::auto_ptr<IFile>(new ELFFile(log_))
		};
		std::string open_error;
		for (i = 0; i < _countof(file); i++) {
			open_error.clear();
			OpenStatus status = file[i]->Open(exe_file_name.c_str(), foRead | foCopyToTemp, &open_error);
			if (status == osSuccess) {
				IArchitecture *arch = file[i]->item(0);
				ISection *code_segment = NULL;
				for (size_t j = 0; j < arch->segment_list()->count(); j++) {
					ISection *segment = arch->segment_list()->item(j);
					if (segment->physical_size() && (segment->memory_type() & mtExecutable)) {
						code_segment = segment;
						break;
					}
				}
				if (!code_segment) {
					Notify(mtError, NULL, string_format(language[lsFileHasNoCodeSegment].c_str(), exe_file_name.c_str())); 
					return false;
				}

				delete input_file_;
				input_file_ = file[i].release();
				break;
			} else {
				switch (status) {
				case osOpenError:
					Notify(mtError, NULL, string_format(language[lsOpenFileError].c_str(), exe_file_name.c_str(), file[i]->format_name().c_str())); 
					break;
				case osInvalidFormat:
					Notify(mtError, NULL, string_format(language[lsFileHasIncorrectFormat].c_str(), exe_file_name.c_str(), file[i]->format_name().c_str()) + (!open_error.empty() ? ": " + open_error : ""));
					break;
				case osUnsupportedCPU:
					Notify(mtError, NULL, string_format(language[lsFileHasUnsupportedProcessor].c_str(), exe_file_name.c_str(), file[i]->item(0)->name().c_str())); 
					break;
				case osUnsupportedSubsystem:
					Notify(mtError, NULL, string_format(language[lsFileHasUnsupportedSubsystem].c_str(), exe_file_name.c_str())); 
					break;
				}
				if (status != osUnknownFormat)
					return false;
			}
		}

		if (!input_file_) {
			Notify(mtError, NULL, string_format(language[lsFileHasUnknownFormat].c_str(), exe_file_name.c_str())); 
			return false;
		}

		if (input_file_->visible_count() == 0)
			return false;
	}

#ifndef ULTIMATE
	if (!input_file_)
		return false;
#endif

	// set default values
	ProjectTemplate *default_template = template_manager_->item(0);
	options_ = default_template->options();
	vm_options_ = 0;
	vm_section_name_ = default_template->vm_section_name();
	for (i = 0; i < _countof(messages_); i++) {
		messages_[i] = default_template->message(i);
	}

	if (!LoadFromXML(project_file_name_.c_str()) && !LoadFromIni(project_file_name_.c_str())) {
		if (input_file_) {
			if (log_)
				log_->StartProgress(string_format("%s %s...", language[lsLoading].c_str(), os::ExtractFileName(project_file_name_.c_str()).c_str()), 0);
			LoadDefaultFunctions();
			if (log_)
				log_->EndProgress();
		}
	}

	if (input_file_name_.empty())
		input_file_name_ = os::SubtractPath(project_path().c_str(), exe_file_name.c_str());

	if (output_file_name_.empty())
		output_file_name_ = default_output_file_name();

#ifdef ULTIMATE
	if (!user_licensing_params_file_name.empty())
		license_data_file_name_ = user_licensing_params_file_name;
	if (license_data_file_name_.empty())
		license_data_file_name_ = default_license_data_file_name();

	licensing_manager_->Open(os::CombinePaths(project_path().c_str(), license_data_file_name_.c_str()));
#endif

	return true;
}

bool Core::LoadFromXML(const char *project_file_name)
{
	TiXmlDocument doc;
	TiXmlElement *root_node, *script_node, *protection_node, *procedures_node, *procedure_node, *objects_node, *object_node,
		*messages_node, *message_node, *folders_node, *folder_node, *ext_command_node;
	size_t i, j;
	CompilationType compilation_type;
	uint64_t func_address, address;
	bool need_compile;
	std::string func_name, arch_name;
	IFunction *function;
	IArchitecture *arch;
	uint32_t compilation_options;
    unsigned int u, version;
	unsigned int func_index;

	if (!doc.LoadFile(project_file_name))
		return false;

	root_node = doc.FirstChildElement("Document");
	if (!root_node)
		return false;

	version = 1;
	root_node->QueryUnsignedAttribute("Version", &version);

	if (input_file_) {
		protection_node = root_node ? root_node->FirstChildElement("Protection") : NULL;
		if (log_) {
			i = 0;
			if (protection_node) {
				procedures_node = protection_node->FirstChildElement("Procedures");
				if (procedures_node) {
					procedure_node = procedures_node->FirstChildElement("Procedure");
					while (procedure_node) {
						i++;
						procedure_node = procedure_node->NextSiblingElement("Procedure");
					}
				}
			}
			log_->StartProgress(string_format("%s %s...", language[lsLoading].c_str(), os::ExtractFileName(project_file_name).c_str()), i);
		}

		if (protection_node) {
			protection_node->QueryStringAttribute("InputFileName", &input_file_name_);
			u = options_;
			protection_node->QueryUnsignedAttribute("Options", &u);
			options_ = u;
			u = 0;
			protection_node->QueryUnsignedAttribute("VMOptions", &u);
			vm_options_ = u;
			if (version < 2) {
				bool check_kernel_debugger = false;
				protection_node->QueryBoolAttribute("CheckKernelDebugger", &check_kernel_debugger);
				if (check_kernel_debugger)
					options_ |= cpCheckKernelDebugger;
			}
			protection_node->QueryStringAttribute("VMCodeSectionName", &vm_section_name_);
			protection_node->QueryStringAttribute("OutputFileName", &output_file_name_);
			protection_node->QueryStringAttribute("WaterMarkName", &watermark_name_);
#ifdef ULTIMATE
			protection_node->QueryStringAttribute("HWID", &hwid_);
			protection_node->QueryStringAttribute("LicenseDataFileName", &license_data_file_name_);
#endif

			messages_node = protection_node->FirstChildElement("Messages");
			if (messages_node) {
				message_node = messages_node->FirstChildElement("Message");
				while (message_node) {
					u = 0;
					message_node->QueryUnsignedAttribute("Id", &u);
					if (u < _countof(messages_)) {
						if (const char *text = message_node->GetText())
							messages_[u] = text;
						else
							messages_[u].clear();
					}
					message_node = message_node->NextSiblingElement(message_node->Value());
				}
			}

			if (version < 2) {
				options_ |= cpStripDebugInfo;
			}

			std::vector<Folder *> folder_list;
			Folder *parent_folder;
			folders_node = protection_node->FirstChildElement("Folders");
			if (folders_node) {
				folder_node = folders_node->FirstChildElement("Folder");
				while (folder_node) {
					u = -1;
					std::string name;
					folder_node->QueryUnsignedAttribute("Parent", &u);
					folder_node->QueryStringAttribute("Name", &name);
					parent_folder = (u < folder_list.size()) ? folder_list[u] : input_file_->folder_list(); 
					folder_list.push_back(parent_folder->Add(name));
					folder_node = folder_node->NextSiblingElement(folder_node->Value());
				}
			}

			procedures_node = protection_node->FirstChildElement("Procedures");
			if (procedures_node) {
				std::vector<uint64_t> address_list;
				procedure_node = procedures_node->FirstChildElement("Procedure");
				while (procedure_node) {
					arch_name.clear();
					procedure_node->QueryStringAttribute("Architecture", &arch_name);
					u = ctVirtualization;
					procedure_node->QueryUnsignedAttribute("CompilationType", &u);
					if (u > 2)
						u = 0;
					compilation_type = static_cast<CompilationType>(u);
					u = 0;
					procedure_node->QueryUnsignedAttribute("Options", &u);
					compilation_options = u;
					need_compile = true;
					procedure_node->QueryBoolAttribute("IncludedInCompilation", &need_compile);
					u = -1;
					procedure_node->QueryUnsignedAttribute("Folder", &u);
					parent_folder = (u < folder_list.size()) ? folder_list[u] : NULL;
					func_name.clear();
					procedure_node->QueryStringAttribute("MapAddress", &func_name);
					func_address = 0;
					func_index = (unsigned int)-1;
					if (func_name.empty()) {
						std::string str;
						procedure_node->QueryStringAttribute("Address", &str);
						func_address = StrToInt64Def(str.c_str(), 0);
					}
					else {
						procedure_node->QueryUnsignedAttribute("Index", &func_index);
					}

					for (i = 0; i < input_file_->count(); i++) {
						arch = input_file_->item(i);
						if (!arch->visible())
							continue;

						struct AddressInfo {
							IArchitecture *arch;
							uint64_t address;
						};

						std::vector<AddressInfo> address_info_list;
						AddressInfo address_info;

						address_info.arch = arch;
						if (func_name.empty()) {
							address_info.address = func_address;
							address_info_list.push_back(address_info);
						} else {
							address_list = arch->map_function_list()->GetAddressListByName(func_name, true);
							for (j = 0; j < address_list.size(); j++) {
								address_info.address = address_list[j];
								address_info_list.push_back(address_info);
							}
						}

						if (!func_name.empty()) {
							if (func_index != (unsigned int)-1) {
								if (func_index < address_info_list.size()) {
									address_info = address_info_list[func_index];
									address_info_list.clear();
									address_info_list.push_back(address_info);
								}
								else {
									address_info_list.clear();
								}
							}
							if (address_list.empty()) {
								function = arch->function_list()->AddUnknown(func_name, compilation_type, compilation_options, need_compile, parent_folder);
								function->set_tag(func_index);
							}
						}

						for (j = 0; j < address_info_list.size(); j++) {
							address_info = address_info_list[j];
							address = address_info.address;
							arch = address_info.arch;
							function = arch->function_list()->AddByAddress(address_info.address, compilation_type, compilation_options, need_compile, parent_folder);
							if (function) {
								u = 0;
								procedure_node->QueryUnsignedAttribute("BreakOffset", &u);
								if (u)
									function->set_break_address(address + u);

								ext_command_node = procedure_node->FirstChildElement("ExtOffset");
								while (ext_command_node) {
									if (const char *str = ext_command_node->GetText())
										function->ext_command_list()->Add(address + strtoul(str, 0, 10));
									ext_command_node = ext_command_node->NextSiblingElement("ExtOffset");
								}
							}
						}
					}

					procedure_node = procedure_node->NextSiblingElement(procedure_node->Value());
					if (log_)
						log_->StepProgress(1ull, true);
				}
			}

   			objects_node = protection_node->FirstChildElement("Objects");
			if (objects_node) {
				std::string name;
				std::string type;
				object_node = objects_node->FirstChildElement("Object");
				while (object_node) {
					arch_name.clear();
					object_node->QueryStringAttribute("Architecture", &arch_name);
					type.clear();
					object_node->QueryStringAttribute("Type", &type);
					name.clear();
					object_node->QueryStringAttribute("Name", &name);
				
					for (i = 0; i < input_file_->count(); i++) {
						arch = input_file_->item(i);
						if (!arch->visible())
							continue;

						if (!arch_name.empty() && arch->name() != arch_name)
							continue;

						if (type == "Segment") {
							if (ISection *segment = arch->segment_list()->GetSectionByName(name)) {
								bool excluded_from_packing = false;
								object_node->QueryBoolAttribute("ExcludedFromPacking", &excluded_from_packing);
								bool excluded_from_memory_protection = false;
								object_node->QueryBoolAttribute("ExcludedFromMemoryProtection", &excluded_from_memory_protection);
								segment->set_excluded_from_packing(excluded_from_packing);
								segment->set_excluded_from_memory_protection(excluded_from_memory_protection);
							}
						} else if (type == "Resource" && arch->resource_list()) {
							if (IResource *resource = arch->resource_list()->GetResourceById(name)) {
								bool excluded_from_packing = false;
								object_node->QueryBoolAttribute("ExcludedFromPacking", &excluded_from_packing);
								resource->set_excluded_from_packing(excluded_from_packing);
							}
						}
						else if (type == "Import") {
							if (IImport *import = arch->import_list()->GetImportByName(name)) {
								bool excluded_from_import_protection = false;
								object_node->QueryBoolAttribute("ExcludedFromImportProtection", &excluded_from_import_protection);
								import->set_excluded_from_import_protection(excluded_from_import_protection);
							}
						}
					}

					object_node = object_node->NextSiblingElement(object_node->Value());
				}
			}
		}

		LoadDefaultFunctions();

		if (log_)
			log_->EndProgress();
	}

#ifdef ULTIMATE
	TiXmlElement *files_node = root_node ? root_node->FirstChildElement("DLLBox") : NULL;
	if (files_node) {
		std::vector<FileFolder *> folder_list;
		FileFolder *parent_folder;
		folders_node = files_node->FirstChildElement("Folders");
		if (folders_node) {
			folder_node = folders_node->FirstChildElement("Folder");
			while (folder_node) {
				u = -1;
				std::string name;
				folder_node->QueryUnsignedAttribute("Parent", &u);
				folder_node->QueryStringAttribute("Name", &name);
				parent_folder = (u < folder_list.size()) ? folder_list[u] : file_manager_->folder_list(); 
				folder_list.push_back(parent_folder->Add(name));
				folder_node = folder_node->NextSiblingElement(folder_node->Value());
			}
		}

		need_compile = true;
		files_node->QueryBoolAttribute("IncludedInCompilation", &need_compile);
		file_manager_->set_need_compile(need_compile);

		TiXmlElement *file_node = files_node->FirstChildElement("DLL");
		while (file_node) {
			std::string name;
			std::string file_name;
				
			file_node->QueryStringAttribute("Name", &name);
			file_node->QueryStringAttribute("FileName", &file_name);
			u = -1;
			file_node->QueryUnsignedAttribute("Folder", &u);
			parent_folder = (u < folder_list.size()) ? folder_list[u] : NULL;
			InternalFileAction action = faNone;
			if (version < 2) {
				bool load_at_start = false;
				file_node->QueryBoolAttribute("LoadAtStart", &load_at_start);
				if (load_at_start)
					action = faLoad;
			} else {
				u = 0;
				file_node->QueryUnsignedAttribute("Options", &u);
				if (u & 1)
					action = faLoad;
				else if (u & 2)
					action = faRegister;
				else if (u & 4)
					action = faInstall;
			}
			file_manager_->Add(name, file_name, action, parent_folder);

			file_node = file_node->NextSiblingElement(file_node->Value());
		}
	}
#endif

	script_node = root_node ? root_node->FirstChildElement("Script") : NULL;
	if (script_node) {
		need_compile = true;
		script_node->QueryBoolAttribute("IncludedInCompilation", &need_compile);
		script_->set_need_compile(need_compile);

		const char *str = script_node->GetText();
		if (str)
			script_->set_text(std::string(str));
	}

	return true;
}

bool Core::LoadFromIni(const char *project_file_name)
{
	size_t i, c, j, k, u, version;
	std::vector<Folder *> folder_list;
	Folder *parent_folder;
	bool need_compile;
	uint64_t address;
	CompilationType compilation_type;
	uint32_t compilation_options;
	std::string map_address;
	IFunction *function;
	IArchitecture *arch;

	IniFile doc(project_file_name);
	if (doc.ReadSection("ProjectInfo").empty())
		return false;

	if (input_file_) {
		if (log_) {
			i = doc.ReadInt("ProjectInfo", "Count");
			log_->StartProgress(string_format("%s %s...", language[lsLoading].c_str(), os::ExtractFileName(project_file_name).c_str()), i);
		}

		version = doc.ReadInt("ProjectInfo", "Version", 1);
		input_file_name_ = doc.ReadString("ProjectInfo", "InputFileName");
		options_ = doc.ReadInt("ProjectInfo", "Options", options_);
		bool check_kernel_debugger = doc.ReadBool("ProjectInfo", "CheckKernelModeDebugger", false);
		if (check_kernel_debugger)
			options_ |= cpCheckKernelDebugger;
		vm_section_name_ = doc.ReadString("ProjectInfo", "VMCodeSectionName", vm_section_name_.c_str());
		output_file_name_ = doc.ReadString("ProjectInfo", "OutputFileName");
		watermark_name_ = doc.ReadString("ProjectInfo", "WaterMarkName");

		if (version < 2) {
			options_ |= cpStripDebugInfo;
		}

		c = doc.ReadInt("Folders", "Count");
		for (i = 0; i < c; i++) {
			u = doc.ReadInt("Folders", string_format("ParentFolder%d", i).c_str(), -1);
			parent_folder = (u < folder_list.size()) ? folder_list[u] : input_file_->folder_list(); 
			folder_list.push_back(parent_folder->Add(doc.ReadString("Folders", string_format("FolderName%d", i).c_str())));
		}

		c = doc.ReadInt("ProjectInfo", "Count");
		std::vector<uint64_t >address_list;
		for (i = 0; i < c; i++) {
			u = doc.ReadInt("Procedures", string_format("CompilationType%d", i).c_str(), ctVirtualization);
			if (u > 2)
				u = 0;
			compilation_type = static_cast<CompilationType>(u);
			compilation_options = doc.ReadInt("Procedures", string_format("Options%d", i).c_str());
			need_compile = doc.ReadBool("Procedures", string_format("NeedCompile%d", i).c_str());
			u = doc.ReadInt("Procedures", string_format("ParentFolder%d", i).c_str(), -1);
			parent_folder = (u < folder_list.size()) ? folder_list[u] : NULL;
			map_address = doc.ReadString("Procedures", string_format("MapAddress%d", i).c_str());

			for (j = 0; j < input_file_->count(); j++) {
				arch = input_file_->item(j);
				if (!arch->visible())
					continue;

				if (map_address.empty()) {
					address_list.clear();
					address_list.push_back(doc.ReadInt64("Procedures", string_format("Address%d", i).c_str()));
				} else {
					address_list = arch->map_function_list()->GetAddressListByName(map_address, true);
					if (address_list.empty()) {
						function = arch->function_list()->AddUnknown(map_address, compilation_type, compilation_options, need_compile, parent_folder);
						function->set_tag(-1);
					}
				}

				for (k = 0; k < address_list.size(); k++) {
					address = address_list[k];
					function = arch->function_list()->AddByAddress(address, compilation_type, compilation_options, need_compile, parent_folder);
					if (function) {
						u = doc.ReadInt("Procedures", string_format("BreakAddress%d", i).c_str());
						if (u)
							function->set_break_address(address + u);

						size_t e = doc.ReadInt("Procedures", string_format("ExtAddressCount%d", i).c_str());
						for (size_t k = 0; k < e; k++) {
							u = doc.ReadInt(string_format("ExtAddress%d", i).c_str(), string_format("Address%d", k).c_str());
							function->ext_command_list()->Add(address + u);
						}
					}
				}
			}
			if (log_)
				log_->StepProgress(1ull, true);
		}

		LoadDefaultFunctions();

		if (log_)
			log_->EndProgress();
	}

#ifdef ULTIMATE
	c = doc.ReadInt("DLLs", "Count");
	for (i = 0; i < c; i++) {
		file_manager_->Add(doc.ReadString("DLLs", string_format("DLLName%d", i).c_str()), 
							doc.ReadString("DLLs", string_format("DLLFileName%d", i).c_str()),
							faNone, NULL);
	}
#endif

	std::string script_file_name = os::ChangeFileExt(doc.file_name().c_str(), ".vms");
	if (os::FileExists(script_file_name.c_str()))
		script_->LoadFromFile(script_file_name);

	return true;
}

void Core::LoadDefaultFunctions()
{
	if (!input_file_)
		return;

	// add new markers and strings into project
	Folder *parent_folder = NULL;
	for (size_t i = 0; i < input_file_->count(); i++) {
		IArchitecture *arch = input_file_->item(i);
		if (!arch->visible())
			continue;

		for (size_t j = 0; j < arch->map_function_list()->count(); j++) {
			IFunctionList *function_list = arch->function_list();

			MapFunction *map_function = arch->map_function_list()->item(j);
			if ((map_function->type() == otMarker || map_function->type() == otAPIMarker || map_function->type() == otString) 
				&& function_list->GetFunctionByAddress(map_function->address()) == NULL) {
				if (!parent_folder) {
					parent_folder = input_file_->folder_list()->Add("New markers and strings");
					parent_folder->set_read_only(true);
				}
				function_list->AddByAddress(map_function->address(), ctVirtualization, 0, true, parent_folder);
			}
		}
	}
}

void Core::LoadFromTemplate(const ProjectTemplate &pt)
{
	set_options(pt.options());
	set_vm_section_name(pt.vm_section_name());
	for (size_t i = 0; i < _countof(messages_); i++) {
		set_message(i, pt.message(i));
	}
}

bool Core::SaveAs(const std::string &file_name)
{
	std::string old_project_file_name = project_file_name_;
	std::string old_input_file_name = input_file_name_;
	std::string old_output_file_name = output_file_name_;
#ifdef ULTIMATE
	std::string old_license_data_file_name = license_data_file_name_;
#endif
	std::string new_project_path = os::ExtractFilePath(file_name.c_str());

	input_file_name_ = os::SubtractPath(new_project_path.c_str(), os::CombinePaths(project_path().c_str(), input_file_name().c_str()).c_str());
	output_file_name_ = os::SubtractPath(new_project_path.c_str(), os::CombinePaths(project_path().c_str(), output_file_name().c_str()).c_str());
#ifdef ULTIMATE
	license_data_file_name_ = os::SubtractPath(new_project_path.c_str(), os::CombinePaths(project_path().c_str(), license_data_file_name().c_str()).c_str());
#endif
	project_file_name_ = file_name;
	if (!Save()) {
		input_file_name_ = old_input_file_name;
		output_file_name_ = old_output_file_name;
		project_file_name_ = old_project_file_name;
#ifdef ULTIMATE
		license_data_file_name_ = old_license_data_file_name;
#endif
		return false;
	}

	if (output_file_name_ != old_output_file_name 
#ifdef ULTIMATE
		|| license_data_file_name_ != old_license_data_file_name
#endif		
		)
		Notify(mtChanged, this);
	return true;
}

bool Core::Save()
{
	if (input_file_) {
		size_t i, j, k;
		TiXmlDocument doc;
		unsigned int old_version;

		if (!doc.LoadFile(project_file_name_.c_str()))
			doc.LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", ""));

		TiXmlElement *root_node = doc.FirstChildElement("Document");
		if (!root_node) {
			root_node = new TiXmlElement("Document");
			doc.LinkEndChild(root_node);
		}
		old_version = 1;
		root_node->QueryUnsignedAttribute("Version", &old_version);

		root_node->SetAttribute("Version", 2);

		TiXmlElement *protection_node = root_node->FirstChildElement("Protection");
		if (!protection_node) {
			protection_node = new TiXmlElement("Protection");
			root_node->LinkEndChild(protection_node);
		}
		protection_node->SetAttribute("InputFileName", input_file_name_);
		protection_node->SetAttribute("Options", options_);
		protection_node->SetAttribute("VMCodeSectionName", vm_section_name_);

#ifdef ULTIMATE
		if (!hwid_.empty())
			protection_node->SetAttribute("HWID", hwid_);
		else
			protection_node->RemoveAttribute("HWID");

		if (license_data_file_name_ != default_license_data_file_name()) {
			protection_node->SetAttribute("LicenseDataFileName", license_data_file_name_);
		} else {
			protection_node->RemoveAttribute("LicenseDataFileName");
		}
#endif

		if (output_file_name_ != default_output_file_name()) {
			protection_node->SetAttribute("OutputFileName", output_file_name_);
		} else {
			protection_node->RemoveAttribute("OutputFileName");
		}

		if (!watermark_name_.empty()) {
			protection_node->SetAttribute("WaterMarkName", watermark_name_);
		} else {
			protection_node->RemoveAttribute("WaterMarkName");
		}

		TiXmlElement *messages_node = protection_node->FirstChildElement("Messages");
		if (!messages_node) {
			messages_node = new TiXmlElement("Messages");
			protection_node->LinkEndChild(messages_node);
		} else {
			messages_node->Clear();
		}

		for (i = 0; i < _countof(messages_); i++) {
			std::string message = messages_[i];
			if (message != 
#ifdef VMP_GNU
				default_message[i]
#else
				os::ToUTF8(default_message[i])
#endif
				) {
				TiXmlElement *message_node = new TiXmlElement("Message");
				messages_node->LinkEndChild(message_node);
				message_node->SetAttribute("Id", (int)i);
				message_node->LinkEndChild(new TiXmlText(message));
			}
		}

		TiXmlElement *folders_node = protection_node->FirstChildElement("Folders");
		if (!folders_node) {
			folders_node = new TiXmlElement("Folders");
			protection_node->LinkEndChild(folders_node);
		} else {
			folders_node->Clear();
		}
		std::vector<Folder*> folder_list = input_file_->folder_list()->GetFolderList(true);
		for (i = 0; i < folder_list.size(); i++) {
			Folder *folder = folder_list[i];

			TiXmlElement *folder_node = new TiXmlElement("Folder");
			folders_node->LinkEndChild(folder_node);
			folder_node->SetAttribute("Name", folder->name());
			std::vector<Folder*>::const_iterator it = std::find(folder_list.begin(), folder_list.end(), folder->owner());
			if (it != folder_list.end())
				folder_node->SetAttribute("Parent", (int)(it - folder_list.begin()));
		}

		TiXmlElement *procedures_node = protection_node->FirstChildElement("Procedures");
		if (!procedures_node) {
			procedures_node = new TiXmlElement("Procedures");
			protection_node->LinkEndChild(procedures_node);
		} else {
			procedures_node->Clear();
		}
		std::map<Data, std::map<IArchitecture*, std::vector<IFunction*> > > function_map;
		std::map<IFunction*, size_t> function_index;
		size_t arch_count = input_file_->visible_count();
		bool show_arch_name = input_file_->function_list()->show_arch_name();
		for (k = 0; k < input_file_->count(); k++) {
			IArchitecture *arch = input_file_->item(k);
			if (!arch->visible())
				continue;

			for (i = 0; i < arch->function_list()->count(); i++) {
				IFunction *function = arch->function_list()->item(i);
				if (function->name().empty())
					continue;

				Data hash = function->hash();
				std::map<Data, std::map<IArchitecture*, std::vector<IFunction*> > >::iterator it = function_map.find(hash);
				if (it == function_map.end()) {
					function_map[hash][arch].push_back(function);
				} else {
					it->second[arch].push_back(function);
				}
			}
		}

		for (std::map<Data, std::map<IArchitecture*, std::vector<IFunction*> > >::iterator it = function_map.begin(); it != function_map.end(); it++) {
			for (std::map<IArchitecture*, std::vector<IFunction*> >::iterator arch_it = it->second.begin(); arch_it != it->second.end(); arch_it++) {
				IArchitecture *arch = arch_it->first;
				std::vector<uint64_t> address_list;
				for (i = 0; i < arch_it->second.size(); i++) {
					IFunction *function = arch_it->second[i];
					if (i == 0)
						address_list = arch->map_function_list()->GetAddressListByName(function->name(), true);
					if (function->type() == otUnknown) {
						function_index[function] = (function->tag() == 0xff) ? -1 : function->tag();
					} else {
						if (address_list.size() == arch_it->second.size()) {
							function_index[function] = -1;
						} else {
							std::vector<uint64_t>::iterator address_it = std::find(address_list.begin(), address_list.end(), function->address());
							if (address_it != address_list.end())
								function_index[function] = address_it - address_list.begin();
						}
					}
				}
			}
		}

		std::string arch_name;
		size_t first_arch_index = -1;
		for (k = 0; k < input_file_->count(); k++) {
			IArchitecture *arch = input_file_->item(k);
			if (!arch->visible())
				continue;

			if (first_arch_index == NOT_ID)
				first_arch_index = k;

			for (i = 0; i < arch->function_list()->count(); i++) {
				IFunction *function = arch->function_list()->item(i);

				size_t map_index = -1;
				arch_name = arch->name();
				if (!function->name().empty()) {
					std::map<IFunction*, size_t>::iterator index_it = function_index.find(function);
					if (index_it == function_index.end())
						continue;

					map_index = index_it->second;
					std::map<Data, std::map<IArchitecture*, std::vector<IFunction*> > >::iterator it = function_map.find(function->hash());
					if (it != function_map.end()) {
						if (it->second.empty() || it->second[arch].empty())
							continue;

						if (show_arch_name) {
							if (it->second.size() == arch_count && k == first_arch_index) {
								bool is_equal = true;
								std::vector<size_t> source_index_list;
								for (std::map<IArchitecture*, std::vector<IFunction*> >::iterator arch_it = it->second.begin(); arch_it != it->second.end(); arch_it++) {
									std::vector<size_t> index_list;
									for (j = 0; j < it->second[arch].size(); j++) {
										index_it = function_index.find(function);
										if (index_it != function_index.end()) {
											index_list.push_back(index_it->second);
											if (index_it->second == NOT_ID)
												break;
										}
									}
									if (source_index_list.empty())
										source_index_list = index_list;
									else {
										if (source_index_list.size() == index_list.size()) {
											for (j = 0; j < source_index_list.size(); j++) {
												if (std::find(index_list.begin(), index_list.end(), source_index_list[j]) == index_list.end()) {
													is_equal = false;
													break;
												}
											}
										} else 
											is_equal = false;
										if (!is_equal)
											break;
									}
								}

								if (is_equal) {
									it->second.clear();
									arch_name.clear();
								}
							}
						}

						if (map_index == NOT_ID && !it->second.empty())
							it->second[arch].clear();
					}
				}

				TiXmlElement *procedure_node = new TiXmlElement("Procedure");
				procedures_node->LinkEndChild(procedure_node);

				if (show_arch_name && !arch_name.empty())
					procedure_node->SetAttribute("Architecture", arch_name);

				if (function->name().empty()) {
					procedure_node->SetAttribute("Address", string_format("%llu", function->address()));
				} else {
					procedure_node->SetAttribute("MapAddress", function->name());
					if (map_index != NOT_ID)
						procedure_node->SetAttribute("Index", (int)map_index);
				}

				if (!function->need_compile())
					procedure_node->SetAttribute("IncludedInCompilation", function->need_compile());

				procedure_node->SetAttribute("Options", function->compilation_options());

				std::vector<Folder*>::const_iterator it = std::find(folder_list.begin(), folder_list.end(), function->folder());
				if (it != folder_list.end())
					procedure_node->SetAttribute("Folder", (int)(it - folder_list.begin()));

				if (function->compilation_type() != ctVirtualization)
					procedure_node->SetAttribute("CompilationType", function->compilation_type());

				if (function->break_address())
					procedure_node->SetAttribute("BreakOffset", static_cast<int>(function->break_address() - function->address()));

				for (j = 0; j < function->ext_command_list()->count(); j++) {
					TiXmlElement *ext_command_node = new TiXmlElement("ExtOffset");
					procedure_node->LinkEndChild(ext_command_node);
					ext_command_node->LinkEndChild(new TiXmlText(string_format("%d", static_cast<int>(function->ext_command_list()->item(j)->address() - function->address()))));
				}
			}
		}

		TiXmlElement *objects_node = protection_node->FirstChildElement("Objects");
		if (!objects_node) {
			objects_node = new TiXmlElement("Objects");
			protection_node->LinkEndChild(objects_node);
		} else {
			objects_node->Clear();
		}

		std::map<Data, size_t> segment_map;
		std::map<Data, size_t> resource_map;
		std::map<Data, size_t> import_map;
		if (show_arch_name) {
			for (k = 0; k < input_file_->count(); k++) {
				IArchitecture *arch = input_file_->item(k);
				if (!arch->visible())
					continue;

				for (i = 0; i < arch->segment_list()->count(); i++) {
					ISection *segment = arch->segment_list()->item(i);
					if (!segment->excluded_from_packing() && !segment->excluded_from_memory_protection())
						continue;

					Data hash = segment->hash();
					std::map<Data, size_t>::iterator it = segment_map.find(hash);
					if (it == segment_map.end())
						segment_map[hash] = 1;
					else
						it->second++;
				}

				if (arch->resource_list()) {
					std::vector<IResource*> resource_list = arch->resource_list()->GetResourceList();
					for (i = 0; i < resource_list.size(); i++) {
						IResource *resource = resource_list[i];
						if (!resource->excluded_from_packing())
							continue;

						Data hash = resource->hash();
						std::map<Data, size_t>::iterator it = resource_map.find(hash);
						if (it == resource_map.end())
							resource_map[hash] = 1;
						else
							it->second++;
					}
				}

				for (i = 0; i < arch->import_list()->count(); i++) {
					IImport *import = arch->import_list()->item(i);
					if (!import->excluded_from_import_protection())
						continue;

					Data hash = import->hash();
					std::map<Data, size_t>::iterator it = import_map.find(hash);
					if (it == import_map.end())
						import_map[hash] = 1;
					else
						it->second++;
				}
			}
		}

		for (k = 0; k < input_file_->count(); k++) {
			IArchitecture *arch = input_file_->item(k);
			if (!arch->visible())
				continue;

			for (i = 0; i < arch->segment_list()->count(); i++) {
				ISection *segment = arch->segment_list()->item(i);
				if (!segment->excluded_from_packing() && !segment->excluded_from_memory_protection())
					continue;

				if (show_arch_name) {
					arch_name = arch->name();
					std::map<Data, size_t>::iterator it = segment_map.find(segment->hash());
					if (it != segment_map.end()) {
						if (it->second == 0)
							continue;
						if (it->second == arch_count) {
							it->second = 0;
							arch_name.clear();
						}
					}
				}

				TiXmlElement *object_node = new TiXmlElement("Object");
				objects_node->LinkEndChild(object_node);

				if (show_arch_name && !arch_name.empty())
					object_node->SetAttribute("Architecture", arch_name);

				object_node->SetAttribute("Type", "Segment");
				object_node->SetAttribute("Name", segment->name());
				if (segment->excluded_from_packing())
					object_node->SetAttribute("ExcludedFromPacking", segment->excluded_from_packing());
				if (segment->excluded_from_memory_protection())
					object_node->SetAttribute("ExcludedFromMemoryProtection", segment->excluded_from_memory_protection());
			}

			if (arch->resource_list()) {
				std::vector<IResource*> resource_list = arch->resource_list()->GetResourceList();
				for (i = 0; i < resource_list.size(); i++) {
					IResource *resource = resource_list[i];
					if (!resource->excluded_from_packing())
						continue;

					if (show_arch_name) {
						arch_name = arch->name();
						std::map<Data, size_t>::iterator it = resource_map.find(resource->hash());
						if (it != resource_map.end()) {
							if (it->second == 0)
								continue;
							if (it->second == arch_count) {
								it->second = 0;
								arch_name.clear();
							}
						}
					}

					TiXmlElement *object_node = new TiXmlElement("Object");
					objects_node->LinkEndChild(object_node);

					if (show_arch_name && !arch_name.empty())
						object_node->SetAttribute("Architecture", arch_name);

					object_node->SetAttribute("Type", "Resource");
					object_node->SetAttribute("Name", resource->id());
					object_node->SetAttribute("ExcludedFromPacking", resource->excluded_from_packing());
				}
			}

			for (i = 0; i < arch->import_list()->count(); i++) {
				IImport *import = arch->import_list()->item(i);
				if (!import->excluded_from_import_protection())
					continue;

				if (show_arch_name) {
					arch_name = arch->name();
					std::map<Data, size_t>::iterator it = import_map.find(import->hash());
					if (it != import_map.end()) {
						if (it->second == 0)
							continue;
						if (it->second == arch_count) {
							it->second = 0;
							arch_name.clear();
						}
					}
				}

				TiXmlElement *object_node = new TiXmlElement("Object");
				objects_node->LinkEndChild(object_node);

				if (show_arch_name && !arch_name.empty())
					object_node->SetAttribute("Architecture", arch_name);

				object_node->SetAttribute("Type", "Import");
				object_node->SetAttribute("Name", import->name());
				if (import->excluded_from_import_protection())
					object_node->SetAttribute("ExcludedFromImportProtection", import->excluded_from_import_protection());
			}
		}

#ifdef ULTIMATE
		{
			TiXmlElement *files_node = root_node->FirstChildElement("DLLBox");
			if (!files_node) {
				files_node = new TiXmlElement("DLLBox");
				root_node->LinkEndChild(files_node);
			} else {
				files_node->Clear();
			}

			folders_node = new TiXmlElement("Folders");
			files_node->LinkEndChild(folders_node);
			std::vector<FileFolder*> folder_list = file_manager_->folder_list()->GetFolderList();
			for (i = 0; i < folder_list.size(); i++) {
				FileFolder *folder = folder_list[i];

				TiXmlElement *folder_node = new TiXmlElement("Folder");
				folders_node->LinkEndChild(folder_node);
				folder_node->SetAttribute("Name", folder->name());
				std::vector<FileFolder*>::const_iterator it = std::find(folder_list.begin(), folder_list.end(), folder->owner());
				if (it != folder_list.end())
					folder_node->SetAttribute("Parent", (int)(it - folder_list.begin()));
			}

			if (!file_manager_->need_compile())
				files_node->SetAttribute("IncludedInCompilation", file_manager_->need_compile());
			else
				files_node->RemoveAttribute("IncludedInCompilation");

			for (i = 0; i < file_manager_->count(); i++) {
				InternalFile *file = file_manager_->item(i);
				TiXmlElement *file_node = new TiXmlElement("DLL");
				files_node->LinkEndChild(file_node);
				file_node->SetAttribute("Name", file->name());
				file_node->SetAttribute("FileName", file->file_name());
				int options;
				switch (file->action()) {
				case faLoad:
					options = 1;
					break;
				case faRegister:
					options = 2;
					break;
				case faInstall:
					options = 4;
					break;
				default:
					options = 0;
					break;
				}
				if (options)
					file_node->SetAttribute("Options", options);
				std::vector<FileFolder*>::const_iterator it = std::find(folder_list.begin(), folder_list.end(), file->folder());
				if (it != folder_list.end())
					file_node->SetAttribute("Folder", (int)(it - folder_list.begin()));
			}
		}
#endif

		TiXmlElement *script_node = root_node->FirstChildElement("Script");
		if (!script_node) {
			script_node = new TiXmlElement("Script");
			root_node->LinkEndChild(script_node);
		} else {
			script_node->Clear();
		}
		if (!script_->need_compile())
			script_node->SetAttribute("IncludedInCompilation", script_->need_compile());
		else
			script_node->RemoveAttribute("IncludedInCompilation");

		if (!script_->text().empty())
		{
			std::string st = script_->text();
			st.erase(std::remove(st.begin(), st.end(), '\r'), st.end());
			TiXmlText *stn = new TiXmlText(st);
			stn->SetCDATA(true); //readability improved
			script_node->LinkEndChild(stn);
		}

		if (!doc.SaveFile(project_file_name_.c_str()))
			return false;
	}

#ifdef ULTIMATE
	licensing_manager_->Save();
#endif

	return true;
}

void Core::Close()
{
	input_file_name_.clear();
	output_file_name_.clear();
	watermark_name_.clear();
	script_->clear();
	script_->set_need_compile(true);
#ifdef ULTIMATE
	hwid_.clear();
	licensing_manager_->clear();
	file_manager_->clear();
	license_data_file_name_.clear();
#endif
	if (input_file_) {
		delete input_file_;
		input_file_ = NULL;
	}
}

std::string Core::absolute_output_file_name() const
{
	return os::CombinePaths(project_path().c_str(), os::ExpandEnvironmentVariables(output_file_name_.c_str()).c_str());
}

bool Core::Compile()
{
	uint32_t rand_seed = os::GetTickCount();
#ifdef CHECKED
	std::cout << "------------------- Core::Compile " << __LINE__ << " -------------------" << std::endl;
	std::cout << "rand_seed: " << rand_seed << std::endl;
	std::cout << "---------------------------------------------------------" << std::endl;
#endif

	rand_seed = 0;
	OutputDebugStringA(string_format("rand_seed:%d\n", rand_seed).c_str());

	srand(rand_seed);
	
	output_file_ = NULL;
	output_architecture_ = NULL;
	if (!script_->Compile())
		return false;

	Watermark *watermark = NULL;
	if (!watermark_name_.empty()) {
		watermark = watermark_manager_->GetWatermarkByName(watermark_name_);
		if (!watermark) {
			Notify(mtError, watermark_manager_, string_format(language[lsWatermarkNotFound].c_str(), watermark_name_.c_str()));
			return false;
		}
		if (!watermark->enabled()) {
			Notify(mtError, watermark_manager_, string_format(language[lsWatermarkIsDisabled].c_str(), watermark_name_.c_str()));
			return false;
		}
	}

	if (!input_file_)
		return true;

	CompileOptions options;

	options.flags = (options_ & cpUserOptionsMask);
	options.flags &= ~input_file_->disable_options();
	if ((options.flags & cpCheckDebugger) == 0)
		options.flags &= ~cpCheckKernelDebugger;
#ifndef DEMO
	if (VMProtectGetSerialNumberState() == SERIAL_STATE_SUCCESS) {
		options.flags |= cpEncryptBytecode;
		if ((options.flags & cpMemoryProtection) == 0)
			options.flags |= cpLoaderCRC;
	} else 
		options.flags |= cpUnregisteredVersion;
#endif

	options.section_name = vm_section_name_;
	options.vm_flags = vm_options_;
	options.vm_count = 
#ifdef DEMO
		true
#else
		((options.flags & cpUnregisteredVersion) != 0 || (options.vm_flags & 2) != 0)
#endif	
		? 1 : 10;
	for (size_t i = 0; i < _countof(options.messages); i++) {
		options.messages[i] = messages_[i];
	}

#ifdef DEMO
	if (true)
#else
	if (options.flags & cpUnregisteredVersion)
#endif
		options.messages[MESSAGE_HWID_MISMATCHED] = 
#ifdef VMP_GNU
				VMProtectDecryptStringA(MESSAGE_UNREGISTERED_VERSION_STR);
#else
				os::ToUTF8(VMProtectDecryptStringW(MESSAGE_UNREGISTERED_VERSION_STR));
#endif

	options.watermark = watermark;
	options.script = script_;
	options.architecture = &output_architecture_;
#ifdef ULTIMATE
	options.hwid = hwid_;
	options.licensing_manager = licensing_manager_;
	if (file_manager_->need_compile() && file_manager_->count())
		options.file_manager = file_manager_;
#endif

	if (log_)
		log_->StartProgress(string_format("%s...", language[lsCompiling].c_str()), 1);

	HANDLE locked_file = BeginCompileTransaction();
	output_file_->set_log(log_);

	bool res;
	try {
		res = output_file_->Compile(options);
	} catch(std::runtime_error &) {
		EndCompileTransaction(locked_file, false);
		throw;
	}

	if (log_) {
		log_->EndProgress();
		log_->set_arch_name("");
	}
	uint32_t output_file_size = static_cast<uint32_t>(output_file_->size()); 
	EndCompileTransaction(locked_file, res);
	if (res) {
		Notify(mtInformation, NULL, string_format(
			language[lsOutputFileSize].c_str(), 
			output_file_size, 
			static_cast<int>(100.0 * output_file_size / input_file_->size())
			));

		if (options.script)
			options.script->DoAfterCompilation();
	}
	return res;
}

HANDLE Core::BeginCompileTransaction()
{
	HANDLE res = INVALID_HANDLE_VALUE;
	std::string output_file_name = absolute_output_file_name();
	if (os::FileExists(output_file_name.c_str())) {
		res = os::FileCreate(output_file_name.c_str(), fmOpenReadWrite | fmShareDenyWrite);
		if (res == INVALID_HANDLE_VALUE)
			throw std::runtime_error(string_format(language[lsOpenFileError].c_str(), output_file_name.c_str()));

		output_file_name = os::GetTempFilePathNameFor(output_file_name.c_str());
		if (output_file_name.empty())
			throw std::runtime_error(string_format(language[lsCreateFileError].c_str(), "'temporary'"));
	}
	output_file_ = input_file_->Clone(output_file_name.c_str());
	return res;
}

void Core::EndCompileTransaction(HANDLE locked_file, bool commit)
{
	std::string output_file = output_file_->file_name();
	delete output_file_;
	output_file_ = NULL;

	if (locked_file == INVALID_HANDLE_VALUE) {
		//direct compile to new file, no tmp file created
		if (!commit)
			os::FileDelete(output_file.c_str()); //remove new garbage
	} else {
		//compiled to tmp file
		os::FileClose(locked_file);
		if (commit) {
			std::string final_file = absolute_output_file_name();
			os::FileDelete(final_file.c_str(), true);
			if (!os::FileMove(output_file.c_str(), final_file.c_str())) {
				os::FileDelete(output_file.c_str());
				throw std::runtime_error(string_format(language[lsCreateFileError].c_str(), final_file.c_str()));
			}
		} else {
			os::FileDelete(output_file.c_str());
		}
	}
}

void Core::set_options(uint32_t options) 
{ 
	if (options_ != options) {
		options_ = options;
		Notify(mtChanged, this);
	}
}

void Core::include_option(ProjectOption option) 
{ 
	set_options(options_ | option);
}

void Core::exclude_option(ProjectOption option) 
{ 
	set_options(options_ & ~option);
}

void Core::set_vm_section_name(const std::string &vm_section_name)
{ 
	if (vm_section_name_ != vm_section_name) {
		vm_section_name_ = vm_section_name;
		Notify(mtChanged, this);
	}
}

void Core::set_watermark_name(const std::string &watermark_name)
{ 
	if (watermark_name_ != watermark_name) {
		watermark_name_ = watermark_name;
		Notify(mtChanged, this);
	}
}

void Core::set_output_file_name(const std::string &output_file_name)
{
	if (output_file_name_ != output_file_name) {
		output_file_name_ = output_file_name.empty() ? default_output_file_name() : output_file_name;
		Notify(mtChanged, this);
	}
}

void Core::set_message(size_t type, const std::string &message)
{
	if (messages_[type] != message) {
		messages_[type] = message;
		Notify(mtChanged, this);
	}
}

#ifdef ULTIMATE
void Core::set_hwid(const std::string &hwid)
{
	if (hwid_ != hwid) {
		hwid_ = hwid;
		Notify(mtChanged, this);
	}
}

void Core::set_license_data_file_name(const std::string &license_data_file_name)
{
	if (license_data_file_name_ != license_data_file_name) {
		license_data_file_name_ = license_data_file_name.empty() ? default_license_data_file_name() : license_data_file_name;
		Notify(mtChanged, this);
		licensing_manager_->Open(os::CombinePaths(project_path().c_str(), license_data_file_name_.c_str()));
	}
}

void Core::set_activation_server(const std::string &activation_server)
{
	if (licensing_manager_->activation_server() != activation_server) {
		licensing_manager_->set_activation_server(activation_server);
		Notify(mtChanged, this);
	}
}
#endif

std::string Core::project_path() const 
{ 
	return os::ExtractFilePath(project_file_name_.c_str()); 
}

void Core::Notify(MessageType type, IObject *sender, const std::string &message)
{
	if (sender) {
		Watermark *watermark = dynamic_cast<Watermark *>(sender);
		if (watermark) {
			switch (type) {
			case mtAdded:
			case mtChanged:
				watermark->SaveToFile(settings_file());
				break;
			case mtDeleted:
				watermark->DeleteFromFile(settings_file());
				break;
			default:
				// do nothing
				break;
			}
		}
		ProjectTemplate *pt = dynamic_cast<ProjectTemplate *>(sender);
		if (pt) {
			switch (type) {
			case mtAdded:
			case mtChanged:
			case mtDeleted:
				template_manager_->SaveToFile(settings_file());
				break;
			default:
				// do nothing
				break;
			}
		}
	}
	if (log_)
		log_->Notify(type, sender, message);
}

IArchitecture * Core::input_architecture() const
{
	return (input_file_ && output_architecture_) ? input_file_->GetArchitectureByType(output_architecture_->type()) : NULL;
}

#include "version.h"
const char * Core::version()
{
	return STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_PATCH);
}

const char * Core::build()
{
	return STR(VER_BUILD);
}

bool Core::check_license_edition(const VMProtectSerialNumberData &lic)
{
	if (lic.nState != SERIAL_STATE_SUCCESS)
		return false;

	uint8_t productId = (lic.nUserDataLength > 0) ? lic.bUserData[0] : VPI_NOT_SPECIFIED;
	VMProtectProductId allowed[] = {
#if defined(ULTIMATE)
		VPI_NOT_SPECIFIED, VPI_ULTM_PERSONAL, VPI_ULTM_COMPANY
#elif defined(LITE)
		VPI_NOT_SPECIFIED, VPI_ULTM_PERSONAL, VPI_ULTM_COMPANY, VPI_PROF_PERSONAL, VPI_PROF_COMPANY, VPI_LITE_PERSONAL, VPI_LITE_COMPANY
#else
		VPI_NOT_SPECIFIED, VPI_ULTM_PERSONAL, VPI_ULTM_COMPANY, VPI_PROF_PERSONAL, VPI_PROF_COMPANY
#endif
	};

	for (size_t i = 0; i < _countof(allowed); i++) {
		if (productId == allowed[i])
			return true;
	}
	return false;
}

#ifdef ULTIMATE

/**
 * License
 */

License::License(LicensingManager *owner, LicenseDate date, const std::string &customer_name, const std::string &customer_email, const std::string &order_ref, 
	const std::string &comments, const std::string &serial_number, bool blocked)
	: owner_(owner), date_(date), customer_name_(customer_name), customer_email_(customer_email), order_ref_(order_ref), 
	comments_(comments), serial_number_(serial_number), blocked_(blocked), info_(NULL)
{

}

License::~License()
{
	delete info_;
	if (owner_)
		owner_->RemoveObject(this);
}

void License::GetHash(uint8_t hash[20])
{
	std::vector<uint8_t> binary_serial;
	Base64ToVector(serial_number_.c_str(), serial_number_.size(), binary_serial);

	SHA1 sha;
	sha.Input(&binary_serial[0], binary_serial.size());
	const uint8_t *src = sha.Result();
	memcpy(hash, src, 20);
}

void License::set_customer_name(const std::string &value)
{
	if (customer_name_ != value) {
		customer_name_ = value;
		Notify(mtChanged, this);
	}
}

void License::set_customer_email(const std::string &value) 
{ 
	if (customer_email_ != value) {
		customer_email_ = value;
		Notify(mtChanged, this);
	}
}

void License::set_order_ref(const std::string &value) 
{ 
	if (order_ref_ != value) {
		order_ref_ = value;
		Notify(mtChanged, this);
	}
}

void License::set_date(LicenseDate value) 
{ 
	if (date_.value() != value.value()) {
		date_ = value; 
		Notify(mtChanged, this);
	}
}

void License::set_comments(const std::string &value) 
{ 
	if (comments_ != value) {
		comments_ = value; 
		Notify(mtChanged, this);
	}
}

void License::set_blocked(bool value) 
{ 
	if (blocked_ != value) {
		blocked_ = value; 
		Notify(mtChanged, this);
	}
}

void License::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

LicenseInfo *License::info()
{
	if (!info_ && owner_) {
		LicenseInfo *tmp_info = new LicenseInfo();
		if (owner_->DecryptSerialNumber(serial_number_, *tmp_info))
			info_ = tmp_info;
		else
			delete tmp_info;
	}

	return info_;
}

/**
 * LicensingManager
 */

LicensingManager::LicensingManager(Core *owner)
	: ObjectList<License>(), owner_(owner), algorithm_(alNone), bits_(0), build_date_(0)
{

}

void LicensingManager::clear()
{
	ObjectList<License>::clear();
	file_name_.clear();
	algorithm_ = alNone;
	bits_ = 0;
	product_code_.clear();
	public_exp_.clear();
	private_exp_.clear();
	modulus_.clear();
	activation_server_.clear();
}

void LicensingManager::changed()
{
	if (owner_)
		owner_->Notify(mtChanged, this);
}

bool LicensingManager::Init(size_t key_len)
{
	RSA rsa;
	if (!rsa.CreateKeyPair(key_len))
		return false;

	std::string file_name = file_name_;
	clear();
	file_name_ = file_name;
	algorithm_ = alRSA;
	bits_ = (uint16_t)key_len;
	public_exp_ = rsa.public_exp();
	private_exp_ = rsa.private_exp();
	modulus_ = rsa.modulus();
	srand(os::GetTickCount());
	for (size_t i = 0; i < 8; i++) {
		product_code_.push_back(rand());
	}
	changed();

	return true;
}

bool LicensingManager::GetLicenseData(Data &data) const
{
	if (algorithm_ == alNone)
		return false;

	size_t i;
	std::vector<std::vector<uint8_t> > black_list;

	for (i = 0; i < count(); i++) {
		License *license = item(i);
		if (!license->blocked())
			continue;

		uint8_t hash[20];
		license->GetHash(hash);
		black_list.push_back(std::vector<uint8_t>(hash, hash + sizeof(hash)));
	}
	std::sort(black_list.begin(), black_list.end());

	uint32_t build_date = build_date_;
	if (!build_date) {
		SYSTEM_TIME time;
		os::GetLocalTime(&time);
		build_date = (time.year << 16) + (time.month << 8) + time.day;
	}

	for (i = 0; i < FIELD_COUNT; i++) {
		data.PushDWord(0);
	}

	data.WriteDWord(FIELD_BUILD_DATE * sizeof(uint32_t), build_date);

	data.WriteDWord(FIELD_PUBLIC_EXP_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(data.size()));
	data.WriteDWord(FIELD_PUBLIC_EXP_SIZE * sizeof(uint32_t), static_cast<uint32_t>(public_exp_.size()));
	data.PushBuff(public_exp_.data(), public_exp_.size());

	data.WriteDWord(FIELD_MODULUS_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(data.size()));
	data.WriteDWord(FIELD_MODULUS_SIZE * sizeof(uint32_t), static_cast<uint32_t>(modulus_.size()));
	data.PushBuff(modulus_.data(), modulus_.size());

	data.WriteDWord(FIELD_BLACKLIST_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(data.size()));
	data.WriteDWord(FIELD_BLACKLIST_SIZE * sizeof(uint32_t), static_cast<uint32_t>(black_list.size() * 20));
	if (!black_list.empty()) {
		for (i = 0; i < black_list.size(); i++) {
			data.PushBuff(black_list[i].data(), black_list[i].size());
		}
	}

	data.WriteDWord(FIELD_ACTIVATION_URL_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(data.size()));
	data.WriteDWord(FIELD_ACTIVATION_URL_SIZE * sizeof(uint32_t), static_cast<uint32_t>(activation_server_.size()));
	if (!activation_server_.empty())
		data.PushBuff(activation_server_.data(), activation_server_.size());

	data.resize(AlignValue(data.size(), 8));
	size_t crc_pos = data.size();
	data.WriteDWord(FIELD_CRC_OFFSET * sizeof(uint32_t), static_cast<uint32_t>(crc_pos));

	// calc CRC
	SHA1 sha1;
	sha1.Input(data.data(), crc_pos);
	data.PushBuff(sha1.Result(), 16);
	return true;
}

bool LicensingManager::Open(const std::string &file_name)
{
	clear();

	file_name_ = file_name;

	TiXmlDocument doc;
	if (!doc.LoadFile(file_name.c_str()))
		return false;

	unsigned int u;
	TiXmlElement *root_node = doc.FirstChildElement("Document");
	TiXmlElement *license_manager_node = root_node ? root_node->FirstChildElement("LicenseManager") : NULL;
	if (license_manager_node) {
		std::string str;
		license_manager_node->QueryStringAttribute("Algorithm", &str);
		if (str.compare("RSA") == 0) {
			algorithm_ = alRSA;
			str.clear();
			license_manager_node->QueryStringAttribute("ProductCode", &str);
			Base64ToVector(str.c_str(), str.size(), product_code_);
			u = 0;
			license_manager_node->QueryUnsignedAttribute("Bits", &u);
			bits_ = u;
			str.clear();
			license_manager_node->QueryStringAttribute("PublicExp", &str);
			Base64ToVector(str.c_str(), str.size(), public_exp_);
			str.clear();
			license_manager_node->QueryStringAttribute("PrivateExp", &str);
			Base64ToVector(str.c_str(), str.size(), private_exp_);
			str.clear();
			license_manager_node->QueryStringAttribute("Modulus", &str);
			Base64ToVector(str.c_str(), str.size(), modulus_);
			license_manager_node->QueryStringAttribute("ActivationServer", &activation_server_);

			if ((bits_ & 0xf) || bits_ < 1024 || bits_ > 16384 || public_exp_.empty() || private_exp_.empty() || modulus_.empty())
				algorithm_ = alNone;
		}

		if (algorithm_ != alNone) {
			TiXmlElement *license_node = license_manager_node->FirstChildElement("License");
			while (license_node) {
				std::string date_str;
				std::string customer_name;
				std::string customer_email;
				std::string order_ref;
				std::string serial_number;
				std::string comments;
				bool blocked = false;
				
				license_node->QueryStringAttribute("Date", &date_str);
				license_node->QueryStringAttribute("CustomerName", &customer_name);
				license_node->QueryStringAttribute("CustomerEmail", &customer_email);
				license_node->QueryStringAttribute("OrderRef", &order_ref);
				license_node->QueryStringAttribute("SerialNumber", &serial_number);
				license_node->QueryBoolAttribute("Blocked", &blocked);
				TiXmlElement *comments_node = license_node->FirstChildElement("Comments");
				const char *str = comments_node ? comments_node->GetText() : license_node->GetText();
				if (str)
					comments = std::string(str);

				Add(LicenseDate(atoi(date_str.substr(0, 4).c_str()), atoi(date_str.substr(5, 2).c_str()), atoi(date_str.substr(8, 2).c_str())),
					customer_name, customer_email, order_ref, comments, serial_number, blocked);
				license_node = license_node->NextSiblingElement("License");
			}
		}
	}

	changed();
	return true;
}

bool LicensingManager::Save()
{
	if (file_name_.empty())
		return false;

	TiXmlDocument doc;
	if (!doc.LoadFile(file_name_.c_str()))
		doc.LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", ""));

	TiXmlElement *root_node = doc.FirstChildElement("Document");
	if (!root_node) {
		root_node = new TiXmlElement("Document");
		doc.LinkEndChild(root_node);
	}

	TiXmlElement *license_manager_node = root_node->FirstChildElement("LicenseManager");
	if (!license_manager_node) {
		license_manager_node = new TiXmlElement("LicenseManager");
		root_node->LinkEndChild(license_manager_node);
	} else {
		license_manager_node->Clear();
	}

	if (!product_code_.empty())
		license_manager_node->SetAttribute("ProductCode", VectorToBase64(product_code_));
	if (!activation_server_.empty())
		license_manager_node->SetAttribute("ActivationServer", activation_server_);
	if (algorithm_ != alNone) {
		license_manager_node->SetAttribute("Algorithm", "RSA");
		license_manager_node->SetAttribute("Bits", bits_);
		license_manager_node->SetAttribute("PublicExp", VectorToBase64(public_exp_));
		license_manager_node->SetAttribute("PrivateExp", VectorToBase64(private_exp_));
		license_manager_node->SetAttribute("Modulus", VectorToBase64(modulus_));
		for (size_t i = 0; i < count(); i++) {
			License *license = item(i);

			TiXmlElement *license_node = new TiXmlElement("License");
			license_manager_node->LinkEndChild(license_node);
			license_node->SetAttribute("Date", string_format("%.4d-%.2d-%.2d", license->date().Year, license->date().Month, license->date().Day));
			if (!license->customer_name().empty())
				license_node->SetAttribute("CustomerName", license->customer_name());
			if (!license->customer_email().empty())
				license_node->SetAttribute("CustomerEmail", license->customer_email());
			if (!license->order_ref().empty())
				license_node->SetAttribute("OrderRef", license->order_ref());
			license_node->SetAttribute("SerialNumber", license->serial_number());
			if (license->blocked())
				license_node->SetAttribute("Blocked", license->blocked());
			if (!license->comments().empty())
				license_node->LinkEndChild(new TiXmlText(license->comments()));
		}
	}

	if (!doc.SaveFile(file_name_.c_str()))
		return false;

	return true;
}

bool LicensingManager::SaveAs(const std::string &file_name)
{
	std::string old_file_name = file_name_;
	file_name_ = file_name;
	if (!Save()) {
		file_name_ = old_file_name;
		return false;
	}
	if (old_file_name != file_name_)
		changed();
	return true;
}

License *LicensingManager::Add(LicenseDate date, const std::string &customer_name, const std::string &customer_email, const std::string &order_ref, 
	const std::string &comments, const std::string &serial_number, bool blocked)
{
	License *license = new License(this, date, customer_name, customer_email, order_ref, comments, serial_number, blocked);
	AddObject(license);
	return license;
}

uint64_t LicensingManager::product_code() const
{
	uint64_t res = 0;
	if (!product_code_.empty())
		memcpy(&res, &product_code_[0], std::min(product_code_.size(), sizeof(res)));
	return res;
}

std::vector<uint8_t> LicensingManager::hash() const
{
	std::vector<uint8_t> res;
	if (!modulus_.empty()) {
		SHA1 sha;
		sha.Input(modulus_.data(), modulus_.size());
		res.insert(res.end(), sha.Result(), sha.Result() + sha.ResultSize());
	}
	return res;
}

enum SerialNumberChunks {
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

std::string LicensingManager::GenerateSerialNumber(const LicenseInfo &info)
{
	if (algorithm_ == alNone)
		throw std::runtime_error(language[lsLicensingParametersNotInitialized]);

	Data data;

	data.PushByte(SERIAL_CHUNK_VERSION);
	data.PushByte(1);

	if (info.Flags & HAS_USER_NAME)	{
		data.PushByte(SERIAL_CHUNK_USER_NAME);
		if (info.CustomerName.size() > 255)
			throw std::runtime_error(language[lsCustomerNameTooLong]);
		data.PushByte((uint8_t)info.CustomerName.size());
		data.PushBuff(info.CustomerName.c_str(), info.CustomerName.size());
	}

	if (info.Flags & HAS_EMAIL)	{
		data.PushByte(SERIAL_CHUNK_EMAIL);
		if (info.CustomerEmail.size() > 255)
			throw std::runtime_error(language[lsEmailTooLong]);
		data.PushByte((uint8_t)info.CustomerEmail.size());
		data.PushBuff(info.CustomerEmail.c_str(), info.CustomerEmail.size());
	}

	if (info.Flags & HAS_HARDWARE_ID) {
		data.PushByte(SERIAL_CHUNK_HWID);
		std::vector<uint8_t> hwid;
		Base64ToVector(info.HWID.c_str(), info.HWID.size(), hwid);
		if (!hwid.size() || hwid.size() > 255 || hwid.size() % 4 != 0)
			throw std::runtime_error(language[lsInvalidHWID]);
		data.PushByte((uint8_t)hwid.size());
		data.PushBuff(&hwid[0], hwid.size());
	}

	if (info.Flags & HAS_EXP_DATE) {
		data.PushByte(SERIAL_CHUNK_EXP_DATE);
		data.PushDWord(info.ExpireDate.value());
	}

	if (info.Flags & HAS_TIME_LIMIT) {
		data.PushByte(SERIAL_CHUNK_RUNNING_TIME_LIMIT);
		data.PushByte(info.RunningTimeLimit);
	}

	if (product_code_.size() != 8)
		throw std::runtime_error(language[lsInvalidProductCode]);
	data.PushByte(SERIAL_CHUNK_PRODUCT_CODE);
	data.PushBuff(&product_code_[0], product_code_.size());

	if (info.Flags & HAS_USER_DATA)	{
		data.PushByte(SERIAL_CHUNK_USER_DATA);
		if (info.UserData.size() > 255)
			throw std::runtime_error(language[lsUserDataTooLong]);
		data.PushByte((uint8_t)info.UserData.size());
		data.PushBuff(info.UserData.c_str(), info.UserData.size());
	}

	if (info.Flags & HAS_MAX_BUILD_DATE) {
		data.PushByte(SERIAL_CHUNK_MAX_BUILD);
		data.PushDWord(info.MaxBuildDate.value());
	}

	// compute hash
	{
		SHA1 sha;
		sha.Input(data.data(), data.size());
		data.PushByte(SERIAL_CHUNK_END);
		const uint8_t *p = sha.Result();
		for (size_t i = 0; i < 4; i++) {
			data.PushByte(p[3 - i]);
		}
	}

	// add padding
	size_t min_padding = 8 + 3;
	size_t max_padding = min_padding + 16;
	size_t max_bytes = bits_ / 8;
	if (data.size() + min_padding > max_bytes)
		throw std::runtime_error(language[lsSerialNumberTooLong]);

	srand(os::GetTickCount());
	size_t padding_bytes = min_padding + rand() % (max_padding - min_padding);

	data.InsertBuff(0, data.data(), padding_bytes);
	data[0] = 0;
	data[1] = 2;
	data[padding_bytes - 1] = 0;
	for (size_t i = 2; i < padding_bytes - 1; i++) {
		uint8_t b = 0;
		while (!b) {
			b = rand();
		}
		data[i] = b;
	}
	while (data.size() < max_bytes) {
		data.PushByte(rand());
	}

	{
		RSA rsa(public_exp_, private_exp_, modulus_);
		if (!rsa.Encrypt(data))
			throw std::runtime_error(language[lsSerialNumberTooLong]);
	}

	size_t len = Base64EncodeGetRequiredLength(data.size());
	char *buffer = new char[len];
	Base64Encode(data.data(), data.size(), buffer, len);
	std::string res = std::string(buffer, len);
	delete [] buffer;

	return res;
}

bool LicensingManager::DecryptSerialNumber(const std::string &serial_number, LicenseInfo &license_info)
{
	if (serial_number.empty())
		return false;

	size_t len = serial_number.size();
	uint8_t *buffer = new uint8_t[len];
	Base64Decode(serial_number.c_str(), serial_number.size(), buffer, len);
	Data data;
	data.PushBuff(buffer, len);
	delete [] buffer;

	{
		RSA rsa(public_exp_, private_exp_, modulus_);
		if (!rsa.Decrypt(data))
			return false;
	}

	if (data.size() < (8 + 3 + 1 + 4) || data[0] != 0 || data[1] != 2)
		return false;

	size_t i;
	for (i = 2; i < data.size() && data[i] != 0; i++) {
	}

	i++;
	size_t pos = i;
	while (pos < data.size()) {
		uint8_t b = data[pos++];
		switch (b) {
		case SERIAL_CHUNK_VERSION:
			b = data[pos++];
			if (b < 1 || b > 2)
				return false;
			break;
		case SERIAL_CHUNK_USER_NAME:
			b = data[pos++];
			license_info.CustomerName = std::string(reinterpret_cast<char *>(&data[pos]), b);
			license_info.Flags |= HAS_USER_NAME;
			pos += b;
			break;
		case SERIAL_CHUNK_EMAIL:
			b = data[pos++];
			license_info.CustomerEmail = std::string(reinterpret_cast<char *>(&data[pos]), b);
			license_info.Flags |= HAS_EMAIL;
			pos += b;
			break;
		case SERIAL_CHUNK_HWID:
			b = data[pos++];
			{
				size_t len = Base64EncodeGetRequiredLength(b);
				char *buffer = new char[len];
				Base64Encode(&data[pos], b, buffer, len);
				license_info.HWID = std::string(buffer, len);
				delete [] buffer;
			}
			license_info.Flags |= HAS_HARDWARE_ID;
			pos += b;
			break;
		case SERIAL_CHUNK_EXP_DATE:
			license_info.ExpireDate = LicenseDate(data.ReadDWord(pos));
			license_info.Flags |= HAS_EXP_DATE;
			pos += 4;
			break;
		case SERIAL_CHUNK_RUNNING_TIME_LIMIT:
			license_info.RunningTimeLimit = data[pos];
			license_info.Flags |= HAS_TIME_LIMIT;
			pos++;
			break;
		case SERIAL_CHUNK_PRODUCT_CODE:
			pos += 8;
			break;
		case SERIAL_CHUNK_USER_DATA:
			b = data[pos++];
			license_info.UserData = std::string(reinterpret_cast<char *>(&data[pos]), b);
			license_info.Flags |= HAS_USER_DATA;
			pos += b;
			break;
		case SERIAL_CHUNK_MAX_BUILD:
			license_info.MaxBuildDate = LicenseDate(data.ReadDWord(pos));
			license_info.Flags |= HAS_MAX_BUILD_DATE;
			pos += 4;
			break;
		case SERIAL_CHUNK_END:
			if (pos + 4 > data.size())
				return false;
			{
				SHA1 sha;
				sha.Input(&data[i], pos - i - 1);
				const uint8_t *p = sha.Result();
				for (size_t j = 0; j < 4; j++) {
					if (data[pos + j] != p[3 - j])
						return false;
				}
			}
			return true;
		}
	}

	return false;
}

License *LicensingManager::GetLicenseBySerialNumber(const std::string &serial_number)
{
	std::vector<uint8_t> binary_serial;
	Base64ToVector(serial_number.c_str(), serial_number.size(), binary_serial);

	SHA1 sha;
	sha.Input(&binary_serial[0], binary_serial.size());
	const uint8_t *src = sha.Result();
	uint8_t hash[20];
	memcpy(hash, src, sizeof(hash));

	for (size_t i = 0; i < count(); i++) {
		License *license = item(i);
		uint8_t license_hash[20];
		license->GetHash(license_hash);
		if (memcmp(hash, license_hash, sizeof(hash)) == 0)
			return license;
	}

	return NULL;
}

bool LicensingManager::CompareParameters(const LicensingManager &manager) const
{
	return (algorithm_ == alRSA 
			&& algorithm_ == manager.algorithm_ 
			&& bits_ == manager.bits_
			&& public_exp_ == manager.public_exp_
			&& private_exp_ == manager.private_exp_
			&& modulus_ == manager.modulus_
			&& product_code_ == manager.product_code_);
}

void LicensingManager::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

void LicensingManager::AddObject(License *license)
{
	ObjectList<License>::AddObject(license);
	Notify(mtAdded, license);
}

void LicensingManager::RemoveObject(License *license)
{
	Notify(mtDeleted, license);
	ObjectList<License>::RemoveObject(license);
}

/**
 * FileFolder
 */

FileFolder::FileFolder(FileFolder *owner, const std::string &name)
	: ObjectList<FileFolder>(), owner_(owner), name_(name), entry_offset_(0)
{

}

FileFolder::~FileFolder()
{
	clear();
	if (owner_)
		owner_->RemoveObject(this);
	Notify(mtDeleted, this);
}

FileFolder::FileFolder(FileFolder *owner, const FileFolder &src)
	: ObjectList<FileFolder>(src), owner_(owner), entry_offset_(0)
{
	name_ = src.name_;

	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

FileFolder *FileFolder::Clone(FileFolder *owner) const
{
	FileFolder *folder = new FileFolder(owner, *this);
	return folder;
}

FileFolder *FileFolder::Add(const std::string &name)
{
	FileFolder *folder = new FileFolder(this, name);
	AddObject(folder);
	Notify(mtAdded, folder);
	return folder;
}

void FileFolder::changed()
{
	Notify(mtChanged, this);
}

std::string FileFolder::id() const
{
	std::string res;
	const FileFolder *folder = this;
	while (folder->owner_) {
		res = res + string_format("\\%d", folder->owner_->IndexOf(folder));
		folder = folder->owner_;
	}
	return res;
}

void FileFolder::set_name(const std::string &name)
{
	if (name_ != name) {
		name_ = name;
		changed();
	}
}

void FileFolder::set_owner(FileFolder *owner)
{
	if (owner == owner_)
		return;
	if (owner_)
		owner_->RemoveObject(this);
	owner_ = owner;
	if (owner_)
		owner_->AddObject(this);
	changed();
}

void FileFolder::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

FileFolder *FileFolder::GetFolderById(const std::string &id) const
{
	if (this->id() == id)
		return (FileFolder *)this;
	for (size_t i = 0; i < count(); i++) {
		FileFolder *res = item(i)->GetFolderById(id);
		if (res)
			return res;
	}
	return NULL;
}

void FileFolder::WriteEntry(IFunction &func)
{
	entry_offset_ = func.count();
	func.AddCommand(osDWord, 0);
	func.AddCommand(osDWord, 0);
	func.AddCommand(osDWord, 0);
	func.AddCommand(osDWord, 0);
}

void FileFolder::WriteName(IFunction &func, uint64_t image_base, uint32_t key)
{
	std::string full_name = name_;
	FileFolder *folder = owner_;
	while (folder && folder->owner()) {
		full_name = folder->name() + '\\' + full_name;
		folder = folder->owner();
	}
	os::unicode_string unicode_name = os::FromUTF8(full_name);
	const os::unicode_char *p = unicode_name.c_str();
	Data str;
	for (size_t i = 0; i < unicode_name.size() + 1; i++) {
		str.PushWord(static_cast<uint16_t>(p[i] ^ (_rotl32(key, static_cast<int>(i)) + i)));
	}
	ICommand *command = func.AddCommand(str);
	command->include_option(roCreateNewBlock);

	CommandLink *link = func.item(entry_offset_)->AddLink(0, ltOffset, command);
	link->set_sub_value(image_base);
}

/**
 * FileFolderList
 */

FileFolderList::FileFolderList(FileManager *owner)
	: FileFolder(NULL, ""), owner_(owner)
{

}

FileFolderList::FileFolderList(FileManager *owner, const FileFolderList & /*src*/)
	: FileFolder(NULL, ""), owner_(owner)
{

}

FileFolderList *FileFolderList::Clone(FileManager *owner) const
{
	FileFolderList *list = new FileFolderList(owner, *this);
	return list;
}

std::vector<FileFolder*> FileFolderList::GetFolderList() const
{
	std::vector<FileFolder*> res;
	FileFolder *folder;
	size_t i, j;

	for (i = 0; i < count(); i++) {
		res.push_back(item(i));
	}
	for (i = 0; i < res.size(); i++) {
		folder = res[i];
		for (j = 0; j < folder->count(); j++) {
			res.push_back(folder->item(j));
		}
	}
	return res;
}

void FileFolderList::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_) {
		if (type == mtDeleted) {
			for (size_t i = owner_->count(); i > 0; i--) {
				InternalFile *file = owner_->item(i - 1);
				if (file->folder() == sender)
					delete file;
			}
		}
		owner_->Notify(type, sender, message);
	}
}

/**
 * InternalFile
 */

InternalFile::InternalFile(FileManager *owner, const std::string &name, const std::string &file_name, InternalFileAction action, FileFolder *folder)
	: owner_(owner), name_(name), file_name_(file_name), action_(action), stream_(NULL), entry_offset_(0), folder_(folder)
{

}

InternalFile::~InternalFile()
{
	Close();
	if (owner_)
		owner_->RemoveObject(this);
	Notify(mtDeleted, this);
}

void InternalFile::set_name(const std::string &value)
{
	if (name_ != value) {
		name_ = value;
		Notify(mtChanged, this);
	}
}

void InternalFile::set_file_name(const std::string &value)
{
	if (file_name_ != value) {
		file_name_ = value;
		Notify(mtChanged, this);
	}
}

void InternalFile::set_action(InternalFileAction action)
{ 
	if (action_ != action) {
		action_ = action;
		Notify(mtChanged, this);
	}
}

void InternalFile::set_folder(FileFolder *folder)
{
	if (folder_ != folder) {
		folder_ = folder;
		Notify(mtChanged, this);
	}
}

size_t InternalFile::id() const 
{ 
	return owner_->IndexOf(this); 
}

void InternalFile::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

std::string InternalFile::absolute_file_name() const
{
	std::string project_path = owner_ ? owner_->owner()->project_path() : std::string();
	return os::CombinePaths(project_path.c_str(), os::ExpandEnvironmentVariables(file_name_.c_str()).c_str());
}

bool InternalFile::Open()
{
	Close();
	std::string file_name = absolute_file_name();
	FileStream *stream = new FileStream();
	if (stream->Open(file_name.c_str(), fmOpenRead | fmShareDenyWrite)) {
		stream_ = stream;
	} else {
		delete stream;
		Notify(mtError, this, string_format(language[lsOpenFileError].c_str(), file_name.c_str()));
	}
	return stream_ != NULL;
}

void InternalFile::Close()
{
	if (stream_) {
		delete stream_;
		stream_ = NULL;
	}
}

void InternalFile::WriteEntry(IFunction &func)
{
	entry_offset_ = func.count();
	func.AddCommand(osDWord, 0);
	func.AddCommand(osDWord, 0);
	func.AddCommand(osDWord, stream_->Size());
	uint32_t value;
	switch (action_) {
	case faLoad:
		value = FILE_LOAD;
		break;
	case faRegister:
		value = FILE_REGISTER;
		break;
	case faInstall:
		value = FILE_INSTALL;
		break;
	default:
		value = 0;
		break;
	}
	func.AddCommand(osDWord, value);
}

void InternalFile::WriteName(IFunction &func, uint64_t image_base, uint32_t key)
{
	std::string full_name = name_;
	FileFolder *folder = folder_;
	while (folder && folder->owner()) {
		full_name = folder->name() + '\\' + full_name;
		folder = folder->owner();
	}
	os::unicode_string unicode_name = os::FromUTF8(full_name);
	const os::unicode_char *p = unicode_name.c_str();
	Data str;
	for (size_t i = 0; i < unicode_name.size() + 1; i++) {
		str.PushWord(static_cast<uint16_t>(p[i] ^ (_rotl32(key, static_cast<int>(i)) + i)));
	}
	ICommand *command = func.AddCommand(str);
	command->include_option(roCreateNewBlock);

	CommandLink *link = func.item(entry_offset_)->AddLink(0, ltOffset, command);
	link->set_sub_value(image_base);
}

void InternalFile::WriteData(IFunction &func, uint64_t image_base, uint32_t key)
{
	std::vector<uint8_t> buf;
	buf.resize(static_cast<size_t>(stream_->Size()));
	stream_->Read(&buf[0], buf.size());

	Data d;
	for (size_t i = 0; i < buf.size(); i++) {
		d.PushByte(buf[i] ^ static_cast<uint8_t>(_rotl32(key, static_cast<int>(i)) + i));
	}

	ICommand *command = func.AddCommand(d);
	command->include_option(roCreateNewBlock);

	CommandLink *link = func.item(entry_offset_ + 1)->AddLink(0, ltOffset, command);
	link->set_sub_value(image_base);
}

/**
 * FileManager
 */

FileManager::FileManager(Core *owner)
	: ObjectList<InternalFile>(), owner_(owner), need_compile_(true)
{
	folder_list_ = new FileFolderList(this);
}

FileManager::~FileManager()
{
	delete folder_list_;
}

void FileManager::clear()
{
	ObjectList<InternalFile>::clear();
	folder_list_->clear();
}

InternalFile *FileManager::Add(const std::string &name, const std::string &file_name, InternalFileAction action, FileFolder *folder)
{
	InternalFile *file = new InternalFile(this, name, file_name, action, folder);
	AddObject(file);
	Notify(mtAdded, file);
	return file;
}

bool FileManager::OpenFiles()
{
	bool res = true;
	for (size_t i = 0; i < count(); i++) {
		InternalFile *file = item(i);
		if (!file->Open()) {
			res = false;
			break;
		}
	}
	if (!res)
		CloseFiles();
	return res;
}

void FileManager::CloseFiles() 
{
	for (size_t i = 0; i < count(); i++) {
		InternalFile *file = item(i);
		file->Close();
	}
}

void FileManager::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

void FileManager::set_need_compile(bool need_compile)
{ 
	if (need_compile_ != need_compile) {
		need_compile_ = need_compile;
		Notify(mtChanged, this);
	}
}

uint32_t FileManager::GetRuntimeOptions() const
{
	if (!count())
		return roNone;

	uint32_t res = roBundler;
	if (server_count())
		res |= roRegistry;
	return res;
}

size_t FileManager::server_count() const
{
	size_t res = 0;
	for (size_t i = 0; i < count(); i++) {
		if (item(i)->is_server())
			res++;
	}
	return res;
}

#endif

/**
 * RSA
 */

RSA::RSA()
{
	private_exp_ = new BigNumber();
	public_exp_ = new BigNumber();
	modulus_ = new BigNumber();
}

RSA::RSA(const std::vector<uint8_t> &public_exp, const std::vector<uint8_t> &private_exp, const std::vector<uint8_t> &modulus)
{
	private_exp_ = new BigNumber(private_exp.data(), private_exp.size());
	public_exp_ = new BigNumber(public_exp.data(), public_exp.size());
	modulus_ = new BigNumber(modulus.data(), modulus.size());
}

RSA::~RSA()
{
	delete private_exp_;
	delete public_exp_;
	delete modulus_;
}

bool RSA::Encrypt(Data &data)
{
	if (!private_exp_->size() || !modulus_->size())
		return false;

	BigNumber x(data.data(), data.size());
	if (*modulus_ < x)
		return false;

	BigNumber y = x.modpow(*private_exp_, *modulus_);
	size_t len = y.size();
	data.resize(len);
	for (size_t i = 0; i < len; i++) {
		data[i] = y[len - 1 - i];
	}

	return true;
}

bool RSA::Decrypt(Data &data)
{
	if (!public_exp_->size() || !modulus_->size())
		return false;

	BigNumber x(data.data(), data.size());
	if (*modulus_ < x)
		return false;

	BigNumber y = x.modpow(*public_exp_, *modulus_);
	size_t len = y.size();
	data.resize(len);
	for (size_t i = 0; i < len; i++) {
		data[i] = y[len - 1 - i];
	}

	return true;
}

static std::vector<uint8_t> bignum_to_vector(const BigNumber &value)
{
	std::vector<uint8_t> res;
	size_t len = value.size();
	res.resize(len);
	for (size_t i = 0; i < len; i++) {
		res[i] = value[len - 1 - i];
	}
	return res;
}

std::vector<uint8_t> RSA::public_exp() const
{
	return bignum_to_vector(*public_exp_);
}

std::vector<uint8_t> RSA::private_exp() const
{
	return bignum_to_vector(*private_exp_);
}

std::vector<uint8_t> RSA::modulus() const
{
	return bignum_to_vector(*modulus_);
}

#ifdef __APPLE__
static const uint8_t *BerForward(const uint8_t *curPtr, size_t *len)
{
	if(*curPtr++ != 0x02) return NULL; // only INTEGER accepted
	*len = *curPtr++;
	if(*len > 0x80)
	{
		int sizeOctets = *len & 0x7F;
		*len = 0;
		while(sizeOctets--)
		{
			*len = *len * 0x100 + *curPtr++;
		}
	}
	return curPtr;
}
#endif

bool RSA::CreateKeyPair(size_t key_length)
{
	bool res = false;
#ifdef __unix__
	OpenSSL::BIGNUM *bne = OpenSSL::BN_new();
	if (OpenSSL::BN_set_word(bne, RSA_F4) == 1)
	{
		OpenSSL::RSA *r = OpenSSL::RSA_new();
		if (OpenSSL::RSA_generate_key_ex(r, key_length, bne, NULL) == 1)
		{
			delete private_exp_;
			delete public_exp_;
			delete modulus_;
			bool bInv = false;
			uint8_t *data = new uint8_t[key_length / 8 + 1];
			OpenSSL::BN_bn2bin(r->n, data);
			modulus_ = new BigNumber(data, BN_num_bytes(r->n), bInv);
			OpenSSL::BN_bn2bin(r->e, data);
			public_exp_ = new BigNumber(data, BN_num_bytes(r->e), bInv);
			OpenSSL::BN_bn2bin(r->d, data);
			private_exp_ = new BigNumber(data, BN_num_bytes(r->d), bInv);
			delete data;
			res = true;
		}
		OpenSSL::RSA_free(r);
	}
	OpenSSL::BN_free(bne);
#elif defined(__APPLE__)
	CFMutableDictionaryRef parameters = CFDictionaryCreateMutable(
		NULL,
		0,
		&kCFTypeDictionaryKeyCallBacks,
		&kCFTypeDictionaryValueCallBacks);

	CFDictionarySetValue(parameters,
		kSecAttrKeyType,
		kSecAttrKeyTypeRSA);

	int rawnum = key_length;
	CFNumberRef num = CFNumberCreate(
		NULL,
		kCFNumberIntType, &rawnum);

	CFDictionarySetValue(
		parameters,
		kSecAttrKeySizeInBits,
		num);

	// Keychain Access tool should display our items as VMProtect XXX, no <key> (if it was not deleted)
	std::ostringstream stringStream;
	stringStream << "VMProtect " << rand();
	CFStringRef name = CFStringCreateWithCString(NULL, stringStream.str().c_str(), kCFStringEncodingUTF8);
	CFDictionarySetValue(
		parameters,
		kSecAttrLabel,
		name);

	SecKeyRef publickey, privatekey;
	//http://stackoverflow.com/questions/11206327/secitemexport-fails-when-exporting-private-key
	OSStatus status = SecKeyGeneratePair(parameters, &publickey, &privatekey);
#ifdef CHECKED
	std::cout << "SecKeyGeneratePair(...) = " << status << std::endl;
#endif
	if (status == errSecSuccess)
	{
		int bytes_in_key = key_length / 8;

		CFDataRef priv;
		status = SecItemExport(privatekey, kSecFormatUnknown, 0, NULL, &priv); //PKCS#1
		if(status == errSecSuccess)
		{
#ifdef CHECKED
			std::cout << "SecItemExport(...) = " << status << std::endl;
#endif
			const uint8_t *privBytes = CFDataGetBytePtr(priv);
			size_t length = CFDataGetLength(priv);

			if(privBytes[0] == 0x30 && // signature
				privBytes[4] == 0x02 && privBytes[5] == 0x01 && privBytes[6] == 0x00) // version = 0
			{
#ifdef CHECKED
				std::cout << "signature OK" << std::endl;
#endif
				size_t modulus_len, public_exp_len, private_exp_len;
				const uint8_t *modulus_bytes = BerForward(privBytes + 7, &modulus_len);

				if(modulus_bytes)
				{
#ifdef CHECKED
					std::cout << "modulus found" << std::endl;
#endif
					const uint8_t *public_exp_bytes = BerForward(modulus_bytes + modulus_len, &public_exp_len);
					if(public_exp_bytes)
					{
#ifdef CHECKED
						std::cout << "public found" << std::endl;
#endif
						const uint8_t *private_exp_bytes = BerForward(public_exp_bytes + public_exp_len, &private_exp_len);
						if(private_exp_bytes)
						{
#ifdef CHECKED
							std::cout << "private found" << std::endl;
#endif
							delete private_exp_;
							delete public_exp_;
							delete modulus_;
							bool bInv = false;
							modulus_ = new BigNumber(modulus_bytes, modulus_len, bInv);
							public_exp_ = new BigNumber(public_exp_bytes, public_exp_len, bInv);
							private_exp_ = new BigNumber(private_exp_bytes, private_exp_len, bInv);

							res = true;
						}
					}
				}

				CFRelease(priv);
			}
		}
		// do not waste system: delete key pair
		CFMutableDictionaryRef attrDict = CFDictionaryCreateMutable(NULL, 4, &kCFTypeDictionaryKeyCallBacks, NULL);
		CFDictionaryAddValue(attrDict, kSecClass, kSecClassKey);
		CFDictionaryAddValue(attrDict, kSecAttrKeyType, kSecAttrKeyTypeRSA);
		CFDictionaryAddValue(attrDict, kSecAttrLabel, name);
		CFDictionaryAddValue(attrDict, kSecMatchLimit, kSecMatchLimitAll);
		SecItemDelete(attrDict);
		CFRelease(attrDict);
		CFRelease(publickey);
		CFRelease(privatekey);
	}

	CFRelease(num);
	CFRelease(name);
	CFRelease(parameters);
#else
	struct {LPCWSTR name; DWORD type;} providers[] =  // structure of providers names and types. we'll try them one by one until they end or we'll find one that works
	{
		{MS_STRONG_PROV, PROV_RSA_FULL},
		{NULL, PROV_RSA_FULL},
		{NULL, PROV_RSA_SCHANNEL},
		{NULL, PROV_RSA_AES},
	};


	for (size_t i = 0; i < _countof(providers) && !res; i++) {
		HCRYPTPROV prov;
		if (CryptAcquireContext(&prov, NULL, providers[i].name, providers[i].type, CRYPT_VERIFYCONTEXT)) {
			HCRYPTKEY key;
			if (CryptGenKey(prov, CALG_RSA_KEYX, MAKELONG(CRYPT_EXPORTABLE, key_length), &key)) {
				DWORD len = (DWORD)(key_length * 2);
				uint8_t *data = new uint8_t[len];
				memset(data, 0, len);
				if (CryptExportKey(key, NULL, PRIVATEKEYBLOB, 0, data, &len)) {
					BLOBHEADER *header = reinterpret_cast<BLOBHEADER *>(data);
					if ((header->aiKeyAlg & ALG_TYPE_RSA) == ALG_TYPE_RSA) {
						RSAPUBKEY *rsa_key = reinterpret_cast<RSAPUBKEY *>(data + sizeof(BLOBHEADER));
						if (rsa_key->magic == 0x32415352 && rsa_key->bitlen == key_length) {
							// RSA2

							delete private_exp_;
							delete public_exp_;
							delete modulus_;

							int bytes_in_key = rsa_key->bitlen / 8;
							public_exp_ = new BigNumber(reinterpret_cast<uint8_t *>(&rsa_key->pubexp), 4, true);
							private_exp_ = new BigNumber(data + len - bytes_in_key, bytes_in_key, true);
							modulus_ = new BigNumber(data + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY), bytes_in_key, true);

							res = true;
						}
					}
				}
				delete [] data;
			}
		}
	}
#endif

	return res;
}

/**
 * ProjectTemplateManager
 */

ProjectTemplateManager::ProjectTemplateManager(Core *owner) : ObjectList<ProjectTemplate>(), owner_(owner)
{
	ProjectTemplate *pt = new ProjectTemplate(this, ProjectTemplate::default_name(), true);
	AddObject(pt);
}

void ProjectTemplateManager::ReadFromFile(SettingsFile &file)
{
	TiXmlElement *node = file.root_node();
	TiXmlElement *templates_node = node->FirstChildElement("Templates");
	if (templates_node) {
		TiXmlElement *template_node = templates_node->FirstChildElement("Template");
		while (template_node) {
			std::string name;
			template_node->QueryStringAttribute("Name", &name);
			ProjectTemplate *pt;
			if (name == ProjectTemplate::default_name())
				pt = item(0);
			else {
				pt = new ProjectTemplate(this, name);
				AddObject(pt);
			}
			pt->ReadFromNode(template_node);
			template_node = template_node->NextSiblingElement();
		}
	}
}

void ProjectTemplateManager::SaveToFile(SettingsFile &file) const
{
	TiXmlElement *node = file.root_node();
	TiXmlElement *templates_node = node->FirstChildElement("Templates");
	if (!templates_node) {
		templates_node = new TiXmlElement("Templates");
		node->LinkEndChild(templates_node);
	} else {
		templates_node->Clear();
	}

	for (size_t i = 0; i < count(); i++) {
		TiXmlElement *template_node = new TiXmlElement("Template");
		templates_node->LinkEndChild(template_node);
		template_node->SetAttribute("Name", item(i)->name());
		item(i)->SaveToNode(template_node);
	}
	file.Save();
}

void ProjectTemplateManager::Add(const std::string &name, const Core &core)
{
	ProjectTemplate *pt = NULL;
	if (name == item(0)->display_name())
		pt = item(0);
	else for (size_t i = 0; i < count(); i++) {
		if (item(i)->name() == name) {
			pt = item(i);
			break;
		}
	}
	if (!pt) {
		pt = new ProjectTemplate(this, name);
		AddObject(pt);
		Notify(mtAdded, pt);
	}
	pt->ReadFromCore(core);
}

void ProjectTemplateManager::Notify(MessageType type, IObject *sender, const std::string &message) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

void ProjectTemplateManager::RemoveObject(ProjectTemplate *pt)
{
	ObjectList<ProjectTemplate>::RemoveObject(pt);
	Notify(mtDeleted, pt);
}

/**
 * ProjectTemplate
 */

ProjectTemplate::ProjectTemplate(ProjectTemplateManager *owner, const std::string &name, bool is_default) 
	: IObject(), owner_(owner), name_(name), options_(0), is_default_(is_default)
{
	Init();
}

ProjectTemplate::~ProjectTemplate()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void ProjectTemplate::ReadFromNode(TiXmlElement *node)
{
	unsigned u = options_;
	node->QueryUnsignedAttribute("Options", &u);
	options_ = u;
	bool check_kernel_debugger = false;
	node->QueryBoolAttribute("CheckKernelDebugger", &check_kernel_debugger);
	if (check_kernel_debugger)
		options_ |= cpCheckKernelDebugger;
	node->QueryStringAttribute("VMCodeSectionName", &vm_section_name_);

	TiXmlElement *messages_node = node->FirstChildElement("Messages");
	if (messages_node) {
		TiXmlElement *message_node = messages_node->FirstChildElement("Message");
		while (message_node) {
			u = 0;
			message_node->QueryUnsignedAttribute("Id", &u);
			if (u < _countof(messages_)) {
				if (const char *text = message_node->GetText())
					messages_[u] = text;
				else
					messages_[u].clear();
			}
			message_node = message_node->NextSiblingElement(message_node->Value());
		}
	}
}

void ProjectTemplate::ReadFromCore(const Core &core)
{
	options_ = core.options();
	vm_section_name_ = core.vm_section_name();
	for (size_t i = 0; i < _countof(messages_); i++) {
		messages_[i] = core.message(i);
	}

	Notify(mtChanged, this);
}

void ProjectTemplate::SaveToNode(TiXmlElement *node) const
{
	node->SetAttribute("Options", options_);
	node->SetAttribute("VMCodeSectionName", vm_section_name_);

	TiXmlElement *messages_node = node->FirstChildElement("Messages");
	if (!messages_node) {
		messages_node = new TiXmlElement("Messages");
		node->LinkEndChild(messages_node);
	} else {
		messages_node->Clear();
	}

	for (size_t i = 0; i < _countof(messages_); i++) {
		const std::string &message = messages_[i];
		if (message != 
#ifdef VMP_GNU
			default_message[i]
#else
			os::ToUTF8(default_message[i])
#endif
			) {
				TiXmlElement *message_node = new TiXmlElement("Message");
				messages_node->LinkEndChild(message_node);
				message_node->SetAttribute("Id", (int)i);
				message_node->LinkEndChild(new TiXmlText(message));
		}
	}
}

void ProjectTemplate::Init()
{
	options_ = cpMaximumProtection;

	// create random section name
	vm_section_name_ = ".";
	size_t i;
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	srand(os::GetTickCount());
	for (i = 0; i < 3; i++) {
		vm_section_name_ += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	for (i = 0; i < _countof(messages_); i++) {
#ifdef VMP_GNU
		messages_[i] = default_message[i];
#else
		messages_[i] = os::ToUTF8(default_message[i]);
#endif
	}
}

void ProjectTemplate::Reset()
{
	Init();
	Notify(mtChanged, this);
}

bool ProjectTemplate::operator==(const ProjectTemplate &other) const
{
	if (options_ != other.options_)
		return false;
	if (vm_section_name_ != other.vm_section_name_)
		return false;
	for (size_t i = 0; i < _countof(messages_); i++)
	{
		if(messages_[i] != other.messages_[i])
			return false;
	}
	return true;
}

std::string ProjectTemplate::display_name() const
{
	return is_default_ ? "(" + language[lsDefault] + ")" : name_;
}

std::string ProjectTemplate::message(size_t idx) const
{
   if (idx >= _countof(messages_))
       throw std::runtime_error("subscript out of range");
   return messages_[idx];
}

void ProjectTemplate::Notify(MessageType type, IObject *sender, const std::string &message /*= ""*/) const
{
	if (owner_)
		owner_->Notify(type, sender, message);
}

void ProjectTemplate::set_name(const std::string &name)
{
	if (name != name_) {
		name_ = name;
		Notify(mtChanged, this);
	}
}