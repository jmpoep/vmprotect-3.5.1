#include "objects.h"
#include "inifile.h"
#include "osutils.h"
#include "lang.h"

SettingsFile &settings_file()
{
	static SettingsFile file_;
	return file_;
}

long StrToIntDef(const char *str, long default_value)
{
	size_t len = strlen(str);
	if (len == 0)
		return default_value;

	char *end;
	long res = strtol(str, &end, 0);
	if (end != str + len)
		return default_value;

	return res;
}

uint64_t StrToInt64Def(const char *str, uint64_t default_value)
{
	size_t len = strlen(str);
	if (len == 0)
		return default_value;

	char *end;
	uint64_t res = _strtoi64(str, &end, 0);
	if (end != str + len)
		return default_value;

	return res;
}

/**
 * BaseIniFile
 */

int BaseIniFile::ReadInt(const char *section, const char *name, int default_value) const
{
	std::string str = ReadString(section, name);
	return StrToIntDef(str.c_str(), default_value);
}

uint64_t BaseIniFile::ReadInt64(const char *section, const char *name, uint64_t default_value) const
{
	std::string str = ReadString(section, name);
	return StrToInt64Def(str.c_str(), default_value);
}

void BaseIniFile::WriteInt(const char *section, const char *name, int value)
{
	WriteString(section, name, string_format("%d", value).c_str());
}

bool BaseIniFile::ReadBool(const char *section, const char *name, bool default_value) const
{
	return ReadInt(section, name, default_value ? 1 : 0) != 0;
}

void BaseIniFile::WriteBool(const char *section, const char *name, bool value)
{
	WriteString(section, name, value ? "1" : "0");
}

/**
 * IniFile
 */

IniFile::IniFile(const char *file_name)
	: BaseIniFile(), file_name_(file_name)
{

}

std::string IniFile::ReadString(const char *section, const char *name, const char *default_value) const
{
	return os::ReadIniString(section, name, default_value, file_name_.c_str());
}

void IniFile::WriteString(const char *section, const char *name, const char *value)
{
	if (!os::WriteIniString(section, name, value, file_name_.c_str()))
		throw std::runtime_error(string_format("Unable to write to %s", file_name_.c_str()));
}

bool IniFile::DeleteKey(const char *section, const char *name)
{
	return os::WriteIniString(section, name, NULL, file_name_.c_str());
}

std::vector<std::string> IniFile::ReadSection(const char *section, const char *name_mask) const
{
	std::vector<std::string> res;
	std::string buffer = os::ReadIniString(section, NULL, NULL, file_name_.c_str());
	const char *p = buffer.c_str();
	while (*p) {
		std::string str = p;
		if (!name_mask || str.find(name_mask) == 0)
			res.push_back(str);
		p += str.size() + 1;
	}
	return res;
}

void IniFile::EraseSection(const char *section)
{
	os::WriteIniString(section, NULL, NULL, file_name_.c_str());
}

/**
 * SettingsFile
 */

#ifdef VMP_GNU
int GlobalLocker::depth;
#endif

SettingsFile::SettingsFile()
	: IObject(), document_(NULL), last_write_time_(0), watermarks_node_created_(false), auto_save_project_(false)
{
	file_name_ = os::CombineThisAppDataDirectory("VMProtect.dat");
	if (!os::FileExists(file_name_.c_str()))
		os::PathCreate(os::ExtractFilePath(file_name_.c_str()).c_str());

	language_manager_ = new LanguageManager();
	language_ = os::GetCurrentLocale();
	if (!language_manager_->GetLanguageById(language_)) {
		// remove country code
		size_t i = language_.find('_');
		if (i != std::string::npos)
			language_ = language_.substr(0, i);
	}

	document_ = new TiXmlDocument();
	Open();

	if (settings_node_) {
		settings_node_->QueryStringAttribute("Language", &language_);
		settings_node_->QueryBoolAttribute("AutoSaveProject", &auto_save_project_);
	}

	if (!language_manager_->GetLanguageById(language_))
		language_ = language_manager_->default_language();
	language_manager_->set_language(language_);
}

SettingsFile::~SettingsFile()
{
	delete document_;
	delete language_manager_;
}

std::vector<std::string> SettingsFile::project_list() const
{
	std::vector<std::string> res;
	if (settings_node_) {
		TiXmlElement *node = settings_node_->FirstChildElement("Projects");
		if (node) {
			node = node->FirstChildElement("Project");
			while (node) {
				const char *prj = node->GetText();
				if (prj) res.push_back(prj);
				node = node->NextSiblingElement(node->Value());
			}
		}
	}
	return res;
}

void SettingsFile::set_project_list(const std::vector<std::string> &project_list)
{
	GlobalLocker locker;
	Open();
	if (!settings_node_)
		return;

	TiXmlElement *projects_node = settings_node_->FirstChildElement("Projects");
	if (!projects_node) {
		projects_node = new TiXmlElement("Projects");
		settings_node_->LinkEndChild(projects_node);
	} else {
		projects_node->Clear();
	}
	for (size_t i = 0; i < project_list.size(); i++) {
		TiXmlElement *node = new TiXmlElement("Project");
		projects_node->LinkEndChild(node);
		node->LinkEndChild(new TiXmlText(project_list[i]));
	}
	Save();
}

void SettingsFile::set_language(const std::string &language)
{
	language_ = language;
	language_manager_->set_language(language_);

	GlobalLocker locker;
	Open();
	if (!settings_node_)
		return;

	settings_node_->SetAttribute("Language", language_);
	Save();
}

void SettingsFile::set_auto_save_project(bool auto_save_project)
{
	auto_save_project_ = auto_save_project;

	GlobalLocker locker;
	Open();
	if (!settings_node_)
		return;

	settings_node_->SetAttribute("AutoSaveProject", auto_save_project_);
	Save();
}

size_t SettingsFile::inc_watermark_id()
{
	GlobalLocker locker;
	Open();
	if (!watermarks_node_)
		return -1;

	unsigned int u;
	u = 0;
	watermarks_node_->QueryUnsignedAttribute("Id", &u);
	watermarks_node_->SetAttribute("Id", u + 1);
	Save();
	return u;
}

TiXmlElement *SettingsFile::watermark_node(size_t id, bool can_create)
{
	Open();
	if (!watermarks_node_)
		return NULL;

	TiXmlElement *node = watermarks_node_->FirstChildElement("Watermark");
	while (node) {
		unsigned int u;
		u = 0;
		node->QueryUnsignedAttribute("Id", &u);
		if (id == u)
			return node;
		node = node->NextSiblingElement(node->Value());
	}
	if (can_create) {
		node = new TiXmlElement("Watermark");
		watermarks_node_->LinkEndChild(node);
	}
	return node;
}

void SettingsFile::Open()
{
	GlobalLocker locker;
	uint64_t cur_write_time = os::GetLastWriteTime(file_name_.c_str());
	if (cur_write_time && last_write_time_ == cur_write_time)
		return;

	if (!document_->LoadFile(file_name_.c_str())) {
		document_->Clear();
		document_->LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", ""));
	}

	root_node_ = document_->FirstChildElement("Document");
	if (!root_node_) {
		root_node_ = new TiXmlElement("Document");
		document_->LinkEndChild(root_node_);
	}

	settings_node_ = root_node_->FirstChildElement("Settings");
	if (!settings_node_) {
		settings_node_ = new TiXmlElement("Settings");
		root_node_->LinkEndChild(settings_node_);
	}

	watermarks_node_ = root_node_->FirstChildElement("Watermarks");
	if (!watermarks_node_) {
		watermarks_node_ = new TiXmlElement("Watermarks");
		root_node_->LinkEndChild(watermarks_node_);
		watermarks_node_created_ = true;
	} else {
		watermarks_node_created_ = false;
	}

	last_write_time_ = os::GetLastWriteTime(file_name_.c_str());
}

void SettingsFile::Save()
{
	GlobalLocker locker;
	document_->SaveFile(file_name_.c_str());

	last_write_time_ = os::GetLastWriteTime(file_name_.c_str());
}