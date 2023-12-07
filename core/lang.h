#ifndef LANG_H
#define LANG_H

class LanguageManager;
class IIniFile;

class Language : public IObject
{
public:
	explicit Language(LanguageManager *owner, std::string id, const std::string &file_name);
	~Language();
	std::string id() const { return id_; }
	std::string file_name() const { return file_name_; }
	std::string name() const { return name_; }
private:
	LanguageManager *owner_;
	std::string id_;
	std::string file_name_;
	std::string name_;
};

class LanguageManager : public ObjectList<Language>
{
public:
	explicit LanguageManager();
	void set_language(const std::string &id);
	Language *GetLanguageById(const std::string &id) const;
	static std::string default_language() { return "en"; }
private:
	void Add(const std::string &file_name);
};

#include "lang_enum.inc"

class LangStringList
{
public:
	LangStringList();
	void ReadFromFile(const char *file_name);
	void use_defaults();
	std::string operator[](LangString index) const;
private:
	void ReadFromIni(IIniFile &file);
	std::string values_[lsCNT],	default_values_[lsCNT];
};

extern LangStringList language;

#endif