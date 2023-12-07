#include "objects.h"
#include "inifile.h"
#include "osutils.h"
#include "lang.h"

LangStringList language;

/**
 * Language
 */

Language::Language(LanguageManager *owner, std::string id, const std::string &file_name)
	: IObject(), owner_(owner), id_(id), file_name_(file_name)
{
	name_ = os::GetLocaleName(id_.c_str());
}

Language::~Language()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
 * LanguageManager
 */

LanguageManager::LanguageManager()
	: ObjectList<Language>()
{
	std::string path = os::GetExecutablePath();
	std::vector<std::string> lang_files = os::FindFiles(os::CombinePaths(path.c_str(), "langs").c_str(), "*.lng");
	for (size_t i = 0; i < lang_files.size(); i++) {
		Add(lang_files[i]);
	}
	if (!GetLanguageById(default_language()))
		InsertObject(0, new Language(this, default_language(), ""));
}

void LanguageManager::Add(const std::string &file_name)
{
	std::string id = os::ChangeFileExt(os::ExtractFileName(file_name.c_str()).c_str(), "");

	Language *lang = new Language(this, id, file_name);
	AddObject(lang);
}

void LanguageManager::set_language(const std::string &id)
{
	Language *lang = GetLanguageById(id);
	if (!lang)
		return;

	if (lang->file_name().empty())
		language.use_defaults();
	else
		language.ReadFromFile(lang->file_name().c_str());
}

Language *LanguageManager::GetLanguageById(const std::string &id) const
{
	for (size_t i = 0; i < count(); i++) {
		Language *lang = item(i);
		if (lang->id() == id)
			return lang;
	}
	return NULL;
}

/**
 * LangStringList
 */

std::string replace_escape_chars(const char *str)
{
	std::string res;
	while (*str) {
		if (*str == '\\' && *(str + 1) == 'n') {
			res += '\n';
			str++;
		} else {
			res += *str;
		}
		str++;
	}
	return res;
}

LangStringList::LangStringList()
{
	#include "lang_def.inc"
	use_defaults();
}

void LangStringList::use_defaults()
{
	for (size_t j = 0; j < lsCNT; j++) {
		values_[j] = default_values_[j];
	}
}

void LangStringList::ReadFromFile(const char *file_name)
{
	IniFile ini_file(file_name);
	ReadFromIni(ini_file);
}

void LangStringList::ReadFromIni(IIniFile &file)
{
#include "lang_info.inc" //key_info.id is unnecessary but informative
	for (size_t j = 0; j < _countof(key_info); j++) {
		values_[j] = replace_escape_chars(file.ReadString("Main", key_info[j].name, default_values_[j].c_str()).c_str());
	}
}

std::string LangStringList::operator[](LangString id) const
{
	assert(id != lsCNT);
	if (id == lsCNT) 
		return std::string();
	return values_[id];
}