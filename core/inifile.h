#ifndef INIFILE_H
#define INIFILE_H

long StrToIntDef(const char *str, long default_value);
uint64_t StrToInt64Def(const char *str, uint64_t default_value);

class IIniFile : public IObject
{
public:
	virtual std::string ReadString(const char *section, const char *name, const char *default_value = NULL) const = 0; 
	virtual void WriteString(const char *section, const char *name, const char *value) = 0;
	virtual int ReadInt(const char *section, const char *name, int default_value = 0) const = 0;
	virtual uint64_t ReadInt64(const char *section, const char *name, uint64_t default_value = 0) const = 0;
	virtual void WriteInt(const char *section, const char *name, int value) = 0;
	virtual bool ReadBool(const char *section, const char *name, bool default_value = false) const = 0;
	virtual void WriteBool(const char *section, const char *name, bool value) = 0;
	virtual bool DeleteKey(const char *section, const char *name) = 0;
};

class BaseIniFile : public IIniFile
{
public:
	virtual int ReadInt(const char *section, const char *name, int default_value = 0) const;
	virtual uint64_t ReadInt64(const char *section, const char *name, uint64_t default_value = 0) const;
	virtual void WriteInt(const char *section, const char *name, int value);
	virtual bool ReadBool(const char *section, const char *name, bool default_value = false) const;
	virtual void WriteBool(const char *section, const char *name, bool value);
};

class IniFile : public BaseIniFile
{
public:
	IniFile(const char *file_name);
	virtual std::string ReadString(const char *section, const char *name, const char *default_value = NULL) const;
	virtual void WriteString(const char *section, const char *name, const char *value);
	virtual bool DeleteKey(const char *section, const char *name);
	std::vector<std::string> ReadSection(const char *section, const char *name_mask = "") const;
	void EraseSection(const char *section);
	std::string file_name() const { return file_name_; }
private:
	std::string file_name_;
};

class LanguageManager;

// временный костыль для проверки многопроцессорной сборки
#ifdef VMP_GNU
#define MUTEX_FILE "/tmp/VMProtect.dat.mutex"
#endif

struct GlobalLocker
{
#ifdef VMP_GNU
	int mutex_;
	static int depth;
#else
	HANDLE mutex_;
#endif

	GlobalLocker()
	{
#ifdef VMP_GNU
		// многопоточной защиты здесь нет, только многопроцессная и от рекурсии
		++depth;
		if (depth == 1)  
		{
			mutex_ = open(MUTEX_FILE, O_CREAT | O_EXLOCK, 0666);
		}
#else
		static HANDLE mutex = CreateMutexA(NULL, FALSE, "Global\\VMProtect.dat");
		mutex_ = mutex;
		WaitForSingleObject(mutex_, 10000);
#endif
	}

	~GlobalLocker()
	{
#ifdef VMP_GNU
		if (depth == 1)
		{
			if(mutex_ >= 0) 
				close(mutex_);
			unlink(MUTEX_FILE);
		}
		--depth;
#else
		ReleaseMutex(mutex_);
#endif
	}
};

class SettingsFile : IObject
{
public:
	explicit SettingsFile();
	~SettingsFile();
	std::vector<std::string> project_list() const;
	void set_project_list(const std::vector<std::string> &project_list);
	std::string language() const { return language_; }
	void set_language(const std::string &language);
	TiXmlElement *watermarks_node() const { return watermarks_node_; }
	TiXmlElement *root_node() const { return settings_node_; }
	size_t inc_watermark_id();
	TiXmlElement *watermark_node(size_t id, bool can_create = false);
	LanguageManager *language_manager() const { return language_manager_; }
	void Save();
	bool watermarks_node_created() const { return watermarks_node_created_; } 
	bool auto_save_project() const { return auto_save_project_; };
	void set_auto_save_project(bool auto_save_project);
private:
	void Open();
	std::string file_name_;
	TiXmlDocument *document_;
	TiXmlElement *root_node_;
	TiXmlElement *watermarks_node_;
	TiXmlElement *settings_node_;
	uint64_t last_write_time_;
	std::string language_;
	LanguageManager *language_manager_;
	bool watermarks_node_created_;
	bool auto_save_project_;

	// no copy ctr or assignment op
	SettingsFile(const SettingsFile &);
	SettingsFile &operator =(const SettingsFile &);
};

SettingsFile &settings_file();

#endif