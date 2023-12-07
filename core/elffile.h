#ifndef ELFFILE_H
#define ELFFILE_H

class ELFFile;
class ELFArchitecture;
class ELFSegmentList;
class ELFDirectoryList;
class ELFImport;
class ELFImportList;
class ELFFixupList;
class ELFSymbolList;
class ELFExportList;
class ELFSymbol;
class ELFStringTable;
class ELFRelocationList;
class ELFSectionList;
class ELFVerneedList;
class ELFVerneed;
class ELFVerdefList;
class ELFVerdef;
class ELFRuntimeFunctionList;
class CommonInformationEntry;
class CommonInformationEntryList;

class ELFSegment : public BaseSection
{
public:
	explicit ELFSegment(ELFSegmentList *owner);
	explicit ELFSegment(ELFSegmentList *owner, uint64_t address, uint64_t size, uint32_t physical_offset, 
		uint32_t physical_size, uint32_t flags, uint32_t type, uint64_t alignment);
	explicit ELFSegment(ELFSegmentList *owner, const ELFSegment &src);
	virtual ELFSegment *Clone(ISectionList *owner) const;
	void ReadFromFile(ELFArchitecture &file);
	size_t WriteToFile(ELFArchitecture &file);
	virtual uint32_t type() const { return type_; }
	virtual void set_type(uint32_t type) { type_ = type; }
	virtual uint64_t address() const { return address_; }
	virtual uint64_t size() const { return size_; }
	virtual uint32_t physical_offset() const { return physical_offset_; }
	virtual uint32_t physical_size() const { return physical_size_; }
	virtual std::string name() const;
	virtual uint32_t memory_type() const;
	virtual void update_type(uint32_t mt);
	void set_size(uint64_t size) { size_ = size; }
	void set_physical_offset(uint32_t offset) { physical_offset_ = offset; }
	void set_physical_size(uint32_t size) { physical_size_ = size; }
	virtual uint32_t flags() const { return flags_; }
	virtual void Rebase(uint64_t delta_base);
	uint64_t alignment() const { return alignment_; }
	uint32_t prot() const;
private:
	uint32_t type_;
	uint64_t address_;
	uint64_t size_;
	uint32_t physical_offset_;
	uint32_t physical_size_;
	uint32_t flags_;
	uint64_t alignment_;
};

class ELFSegmentList : public BaseSectionList
{
public:
	explicit ELFSegmentList(ELFArchitecture *owner);
	explicit ELFSegmentList(ELFArchitecture *owner, const ELFSegmentList &src);
	virtual ELFSegmentList *Clone(ELFArchitecture *owner) const;
	virtual ELFSegment *GetSectionByAddress(uint64_t address) const;
	virtual ELFSegment *GetSectionByOffset(uint64_t offset) const;
	ELFSegment *GetSectionByType(uint32_t type) const;
	void ReadFromFile(ELFArchitecture &file, size_t count);
	size_t WriteToFile(ELFArchitecture &file);
	ELFSegment *item(size_t index) const;
	ELFSegment *last() const;
	ELFSegment *Add(uint64_t address, uint64_t size, uint32_t physical_offset, uint32_t physical_size, uint32_t flags,
		uint32_t type, uint64_t alignment);
private:
	ELFSegment *Add();
};

class ELFSection : public BaseSection
{
public:
	explicit ELFSection(ELFSectionList *owner);
	explicit ELFSection(ELFSectionList *owner, uint64_t address, uint32_t size, uint32_t physical_offset, uint32_t flags, uint32_t type, const std::string &name);
	explicit ELFSection(ELFSectionList *owner, const ELFSection &src);
	virtual ELFSection *Clone(ISectionList *owner) const;
	void ReadFromFile(ELFArchitecture &file);
	void WriteToFile(ELFArchitecture &file);
	void ReadName(ELFStringTable &string_table);
	void WriteName(ELFStringTable &string_table);
	virtual uint64_t address() const { return address_; }
	virtual uint64_t size() const { return size_; }
	virtual uint32_t physical_offset() const { return physical_offset_; }
	virtual uint32_t physical_size() const { return size_; }
	virtual std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	virtual uint32_t memory_type() const { return parent_->memory_type(); }
	virtual ELFSegment *parent() const { return parent_; }
	virtual uint32_t flags() const { return static_cast<uint32_t>(flags_); }
	virtual void Rebase(uint64_t delta_base);
	virtual void update_type(uint32_t mt) { }
	uint32_t type() const { return type_; }
	uint32_t link() const { return link_; }
	uint64_t entry_size() const { return entry_size_; }
	void set_physical_offset(uint32_t physical_offset) { physical_offset_ = physical_offset; }
	void set_size(uint32_t size) { size_ = size; }
	uint32_t alignment() const { return addralign_; }
	uint32_t info() const { return info_; }
	void set_info(uint32_t info) { info_ = info; }
	void set_link(uint32_t link) { link_ = link; }
	void set_entry_size(uint64_t entry_size) { entry_size_ = entry_size; }
	void RemapLinks(const std::map<size_t, size_t> &index_map);
private:
	ELFSegment *parent_;
	uint64_t address_;
	uint32_t size_;
	uint32_t type_;
	uint32_t physical_offset_;
	uint32_t name_idx_;
	std::string name_;
	uint32_t link_;
	uint32_t info_;
	uint32_t addralign_;
	uint64_t flags_;
	uint64_t entry_size_;
};

class ELFStringTable
{
public:
	ELFStringTable *Clone();
	std::string GetString(uint32_t pos) const;
	uint32_t AddString(const std::string &str);
	void clear();
	void ReadFromFile(ELFArchitecture &file);
	void ReadFromFile(ELFArchitecture &file, const ELFSection &section);
	size_t WriteToFile(ELFArchitecture &file);
	size_t size() const { return data_.size(); }
private:
	std::vector<char> data_;
	std::map<std::string, uint32_t> map_;
};

class ELFSectionList : public BaseSectionList
{
public:
	explicit ELFSectionList(ELFArchitecture *owner);
	explicit ELFSectionList(ELFArchitecture *owner, const ELFSectionList &src);
	ELFSection *item(size_t index) const;
	virtual ELFSectionList *Clone(ELFArchitecture *owner) const;
	void ReadFromFile(ELFArchitecture &file, size_t count);
	uint64_t WriteToFile(ELFArchitecture &file);
	ELFSection *GetSectionByType(uint32_t type) const;
	ELFSection *GetSectionByAddress(uint64_t address) const;
	ELFSection *GetSectionByName(const std::string &name) const;
	ELFSection *Add(uint64_t address, uint32_t size, uint32_t physical_offset, uint32_t flags, uint32_t type, const std::string &name);
	ELFStringTable *string_table() { return &string_table_; }
	void RemapLinks(const std::map<size_t, size_t> &index_map);
private:
	ELFSection *Add();

	ELFStringTable string_table_;
};

class ELFImportFunction : public BaseImportFunction
{
public:
	explicit ELFImportFunction(ELFImport *owner, uint64_t address, const std::string &name, ELFSymbol *symbol);
	explicit ELFImportFunction(ELFImport *owner, uint64_t address, APIType type, MapFunction *map_function);
	explicit ELFImportFunction(ELFImport *owner, const ELFImportFunction &src);
	virtual ELFImportFunction *Clone(IImport *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual std::string name() const { return name_; }
	virtual std::string display_name(bool show_ret = true) const;
	virtual void Rebase(uint64_t delta_base);
	void set_address(uint64_t address) { address_ = address; }
	ELFSymbol *symbol() const { return symbol_; }
	void set_symbol(ELFSymbol *symbol) { symbol_ = symbol; }
private:
	uint64_t address_;
	std::string name_;
	ELFSymbol *symbol_;
};

class ELFImport : public BaseImport
{
public:
	explicit ELFImport(ELFImportList *owner, bool is_sdk = false);
	explicit ELFImport(ELFImportList *owner, const std::string &name);
	explicit ELFImport(ELFImportList *owner, const ELFImport &src);
	ELFImportFunction *item(size_t index) const;
	ELFImportFunction *GetFunctionBySymbol(ELFSymbol *symbol) const;
	virtual ELFImport *Clone(IImportList *owner) const;
	virtual std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	virtual bool is_sdk() const { return is_sdk_; }
	void set_is_sdk(bool is_sdk) { is_sdk_ = is_sdk; }
	ELFImportFunction *Add(uint64_t address, const std::string &name, ELFSymbol *symbol);
protected:
	virtual	IImportFunction *Add(uint64_t address, APIType type, MapFunction *map_function);
private:
	std::string name_;
	bool is_sdk_;
};

class ELFImportList : public BaseImportList
{
public:
	explicit ELFImportList(ELFArchitecture *owner);
	explicit ELFImportList(ELFArchitecture *owner, const ELFImportList &src);
	virtual ELFImportList *Clone(ELFArchitecture *owner) const;
	ELFImport *item(size_t index) const;
	virtual ELFImportFunction *GetFunctionByAddress(uint64_t address) const;
	virtual ELFImport *GetImportByName(const std::string &name) const;
	void ReadFromFile(ELFArchitecture &file);
	void Pack();
	void WriteToFile(ELFArchitecture &file);
protected:
	virtual ELFImport *AddSDK();
private:
	ELFImport *Add(const std::string &name);
};

class ELFFixup : public BaseFixup
{
public:
	explicit ELFFixup(ELFFixupList *owner, uint64_t address, OperandSize size);
	explicit ELFFixup(ELFFixupList *owner, const ELFFixup &src);
	virtual ELFFixup *Clone(IFixupList *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual FixupType type() const { return ftHighLow; }
	virtual OperandSize size() const { return size_; }
	virtual void set_address(uint64_t address) { address_ = address; }
	virtual void Rebase(IArchitecture &file, uint64_t delta_base);
private:
	uint64_t address_;
	OperandSize size_;
};

class ELFFixupList : public BaseFixupList
{
public:
	explicit ELFFixupList();
	explicit ELFFixupList(const ELFFixupList &src);
	virtual ELFFixupList *Clone() const;
	ELFFixup *item(size_t index) const;
	virtual IFixup *AddDefault(OperandSize cpu_address_size, bool is_code);
	ELFFixup *Add(uint64_t address, OperandSize size);
	void WriteToData(Data &data, uint64_t image_base);
};

class ELFExport : public BaseExport
{
public:
	explicit ELFExport(IExportList *parent, uint64_t address);
	explicit ELFExport(IExportList *parent, ELFSymbol *symbol);
	explicit ELFExport(IExportList *parent, const ELFExport &src);
	~ELFExport();
	virtual ELFExport *Clone(IExportList *parent) const;
	virtual uint64_t address() const { return address_; }
	virtual std::string name() const { return name_; }
	virtual std::string forwarded_name() const { return std::string(); }
	virtual std::string display_name(bool show_ret = true) const;
	virtual APIType type() const { return type_; }
	virtual void set_type(APIType type) { type_ = type; }
	ELFSymbol *symbol() const { return symbol_; }
	void set_symbol(ELFSymbol *symbol) { symbol_ = symbol; }
	virtual void Rebase(uint64_t delta_base);
	void set_address(uint64_t value) { address_ = value; }
private:
	ELFSymbol *symbol_;
	uint64_t address_;
	std::string name_;
	APIType type_;
};

class ELFExportList : public BaseExportList
{
public:
	explicit ELFExportList(ELFArchitecture *owner);
	explicit ELFExportList(ELFArchitecture *owner, const ELFExportList &src);
	virtual ELFExportList *Clone(ELFArchitecture *owner) const;
	ELFExport *item(size_t index) const;
	virtual std::string name() const { return std::string(); }
	void ReadFromFile(ELFArchitecture &file);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	ELFExport *GetExportByAddress(uint64_t address) const;
protected:
	virtual IExport *Add(uint64_t address);
private:
	ELFExport *Add(ELFSymbol *symbol);
};

class ELFSymbol : public ISymbol
{
public:
	explicit ELFSymbol(ELFSymbolList *owner);
	explicit ELFSymbol(ELFSymbolList *owner, const ELFSymbol &src);
	~ELFSymbol();
	virtual ELFSymbol *Clone(ELFSymbolList *owner) const;
	void ReadFromFile(ELFArchitecture &file, const ELFStringTable &strtab);
	size_t WriteToFile(ELFArchitecture &file, ELFStringTable &string_table);
	void Rebase(uint64_t delta_base);
	uint8_t type() const { return (info_ & 0x0f); }
	uint8_t bind() const { return (info_ >> 4); }
	void set_bind(uint8_t bind) { info_ = (bind << 4) | (info_ & 0x0f); }
	uint64_t address() const { return address_; }
	uint64_t value() const { return value_; }
	std::string name() const { return name_; }
	uint16_t section_idx() const { return section_idx_; }
	uint32_t name_idx() const { return name_idx_; }
	bool is_deleted() const { return is_deleted_; }
	void set_deleted(bool value) { is_deleted_ = value; }
	ELFSymbolList *owner() const { return owner_; }
	uint16_t version() const { return version_; }
	void set_version(uint16_t version) { version_ = version; }
	bool need_hash() const { return (type() == STT_TLS) || (value_ && section_idx_); }
	size_t size() const { return static_cast<size_t>(size_); }
	virtual std::string display_name(bool show_ret = true) const;
private:
	ELFSymbolList *owner_;
	uint64_t address_;
	std::string name_;
	uint8_t info_;
	uint8_t other_;
	uint16_t section_idx_;
	uint32_t name_idx_;
	uint64_t value_;
	uint64_t size_;
	bool is_deleted_;
	uint16_t version_;
};

class ELFSymbolList : public ObjectList<ELFSymbol>
{
public:
	explicit ELFSymbolList(bool is_dynamic);
	explicit ELFSymbolList(const ELFSymbolList &src);
	virtual ELFSymbolList *Clone() const;
	void ReadFromFile(ELFArchitecture &file);
	void WriteToFile(ELFArchitecture &file);
	void Pack();
	void Rebase(uint64_t delta_base);
	ELFStringTable *string_table() { return &string_table_; }
private:
	ELFSymbol *Add();
	size_t WriteHash(ELFArchitecture &file);
	template <typename T>
	size_t WriteGNUHash(ELFArchitecture &file);
	size_t WriteVersym(ELFArchitecture &file);

	bool is_dynamic_;
	ELFStringTable string_table_;

	// no assignment op
	ELFSymbolList &operator =(const ELFSymbolList &);
};

class ELFVernaux : public IObject
{
public:
	ELFVernaux(ELFVerneed *owner);
	ELFVernaux(ELFVerneed *owner, const ELFVernaux &src);
	~ELFVernaux();
	ELFVernaux *Clone(ELFVerneed *owner) const;
	void ReadFromFile(ELFArchitecture &file);
	size_t WriteToFile(ELFArchitecture &file);
	void WriteStrings(ELFStringTable &string_table);
	uint32_t next() const { return next_; }
	uint32_t hash() const { return hash_; }
	uint16_t other() const { return other_; }
	void set_other(uint16_t other) { other_ = other; }
private:
	ELFVerneed *owner_;
	uint32_t hash_;
	uint16_t flags_;
	uint16_t other_;
	uint32_t name_pos_;
	uint32_t next_;
	std::string name_;
};

class ELFVerneed : public ObjectList<ELFVernaux>
{
public:
	ELFVerneed(ELFVerneedList *owner);
	ELFVerneed(ELFVerneedList *owner, const ELFVerneed &src);
	~ELFVerneed();
	ELFVerneed *Clone(ELFVerneedList *owner) const;
	void ReadFromFile(ELFArchitecture &file);
	size_t WriteToFile(ELFArchitecture &file);
	void WriteStrings(ELFStringTable &string_table);
	ELFVernaux *GetVernaux(uint32_t hash) const;
	uint32_t next() const { return next_; }
	std::string file() const { return file_; }
private:
	ELFVernaux *Add();

	ELFVerneedList *owner_;
	uint16_t version_;
	uint32_t file_pos_;
	uint32_t next_;
	std::string file_;
};

class ELFVerneedList : public ObjectList<ELFVerneed>
{
public:
	explicit ELFVerneedList();
	explicit ELFVerneedList(const ELFVerneedList &src);
	ELFVerneedList *Clone() const;
	void ReadFromFile(ELFArchitecture &file);
	void WriteToFile(ELFArchitecture &file);
	void WriteStrings(ELFStringTable &string_table);
	ELFVerneed *GetVerneed(const std::string &name) const;
private:
	ELFVerneed *Add();
};

class ELFVerdaux : public IObject
{
public:
	ELFVerdaux(ELFVerdef *owner);
	ELFVerdaux(ELFVerdef *owner, const ELFVerdaux &src);
	~ELFVerdaux();
	ELFVerdaux *Clone(ELFVerdef *owner) const;
	void ReadFromFile(ELFArchitecture &file);
	size_t WriteToFile(ELFArchitecture &file);
	void WriteStrings(ELFStringTable &string_table);
	uint32_t next() const { return next_; }
private:
	ELFVerdef *owner_;
	uint32_t name_pos_;
	uint32_t next_;
	std::string name_;
};

class ELFVerdef : public ObjectList<ELFVerdaux>
{
public:
	ELFVerdef(ELFVerdefList *owner);
	ELFVerdef(ELFVerdefList *owner, const ELFVerdef &src);
	~ELFVerdef();
	ELFVerdef *Clone(ELFVerdefList *owner) const;
	void ReadFromFile(ELFArchitecture &file);
	size_t WriteToFile(ELFArchitecture &file);
	void WriteStrings(ELFStringTable &string_table);
	uint32_t next() const { return next_; }
private:
	ELFVerdaux *Add();

	ELFVerdefList *owner_;
	uint16_t version_;
	uint16_t flags_;
	uint16_t ndx_;
	uint32_t hash_;
	uint32_t next_;
};

class ELFVerdefList : public ObjectList<ELFVerdef>
{
public:
	explicit ELFVerdefList();
	explicit ELFVerdefList(const ELFVerdefList &src);
	ELFVerdefList *Clone() const;
	void ReadFromFile(ELFArchitecture &file);
	void WriteToFile(ELFArchitecture &file);
	void WriteStrings(ELFStringTable &string_table);
private:
	ELFVerdef *Add();
};

class ELFRelocation : public BaseRelocation
{
public:
	explicit ELFRelocation(ELFRelocationList *owner, bool is_rela, uint64_t address, OperandSize size, uint32_t type, ELFSymbol *symbol, uint64_t addend);
	explicit ELFRelocation(ELFRelocationList *owner, const ELFRelocation &src);
	ELFRelocation *Clone(IRelocationList *owner) const;
	size_t WriteToFile(ELFArchitecture &file);
	virtual void Rebase(IArchitecture &file, uint64_t delta_base);
	ELFSymbol *symbol() const { return symbol_; }
	void set_symbol(ELFSymbol *symbol) { symbol_ = symbol; }
	bool is_rela() const { return is_rela_; }
	uint32_t type() const { return type_; }
	void set_type(uint32_t type) { type_ = type; }
	uint64_t addend() const { return addend_; }
	uint64_t value() const { return value_; }
	void set_value(uint64_t value) { value_ = value; }
private:
	bool is_rela_;
	uint32_t type_;
	uint64_t addend_;
	ELFSymbol *symbol_;
	uint64_t value_;
};

class ELFRelocationList : public BaseRelocationList
{
public:
	explicit ELFRelocationList();
	explicit ELFRelocationList(const ELFRelocationList &src);
	virtual ELFRelocationList *Clone() const;
	ELFRelocation *item(size_t index) const;
	ELFRelocation *GetRelocationByAddress(uint64_t address) const;
	void ReadFromFile(ELFArchitecture &file);
	void WriteToFile(ELFArchitecture &file);
	void Pack();
private:
	ELFRelocation *Add(bool is_rela, uint64_t address, OperandSize size, uint32_t type, ELFSymbol *symbol, uint64_t addend);

	// no assignment op
	ELFRelocationList &operator =(const ELFRelocationList &);
};

class ELFDirectory : public BaseLoadCommand
{
public:
	explicit ELFDirectory(ELFDirectoryList *owner);
	explicit ELFDirectory(ELFDirectoryList *owner, size_t type);
	explicit ELFDirectory(ELFDirectoryList *owner, const ELFDirectory &src);
	virtual ELFDirectory *Clone(ILoadCommandList *owner) const;
	virtual uint64_t address() const { return value_; }
	virtual uint32_t size() const { return 0; }
	virtual uint32_t type() const { return static_cast<uint32_t>(type_); }
	virtual std::string name() const;
	uint64_t value() const { return value_; }
	void set_value(uint64_t value) { value_ = value; }
	std::string str_value() const { return str_value_; }
	void set_str_value(const std::string &str_value) { str_value_ = str_value; }
	void ReadFromFile(ELFArchitecture &file);
	size_t WriteToFile(ELFArchitecture &file);
	void ReadStrings(ELFStringTable &string_table);
	void WriteStrings(ELFStringTable &string_table);
	virtual void Rebase(uint64_t delta_base);
private:
	uint64_t type_;
	uint64_t value_;
	std::string str_value_;
};

class ELFDirectoryList : public BaseCommandList
{
public:
	explicit ELFDirectoryList(ELFArchitecture *owner);
	explicit ELFDirectoryList(ELFArchitecture *owner, const ELFDirectoryList &src);
	ELFDirectory *item(size_t index) const;
	virtual ELFDirectoryList *Clone(ELFArchitecture *owner) const;
	void ReadFromFile(ELFArchitecture &file);
	void WriteToFile(ELFArchitecture &file);
	ELFDirectory *Add(size_t type);
	ELFDirectory *GetCommandByType(uint32_t type) const;
	void ReadStrings(ELFStringTable &string_table);
	void WriteStrings(ELFStringTable &string_table);
private:
	ELFDirectory *Add();

	// no assignment op
	ELFRelocationList &operator =(const ELFRelocationList &);
};

class ELFRuntimeFunction : public BaseRuntimeFunction
{
public:
	explicit ELFRuntimeFunction(ELFRuntimeFunctionList *owner, uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, CommonInformationEntry *cie,
		const std::vector<uint8_t> &call_frame_instructions);
	explicit ELFRuntimeFunction(ELFRuntimeFunctionList *owner, const ELFRuntimeFunction &src);
	virtual ELFRuntimeFunction *Clone(IRuntimeFunctionList *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual uint64_t begin() const { return begin_; }
	virtual uint64_t end() const { return end_; }
	virtual uint64_t unwind_address() const { return unwind_address_; }
	virtual void set_begin(uint64_t begin) { begin_ = begin; }
	virtual void set_end(uint64_t end) { end_ = end; }
	virtual void set_unwind_address(uint64_t unwind_address) { unwind_address_ = unwind_address; }
	void set_address(uint64_t address) { address_ = address; }
	CommonInformationEntry *cie() const { return cie_; }
	void set_cie(CommonInformationEntry *cie) { cie_ = cie; }
	std::vector<uint8_t> call_frame_instructions() const { return call_frame_instructions_; }
	virtual void Parse(IArchitecture &file, IFunction &dest);
	void Rebase(uint64_t delta_base);
private:
	uint64_t address_;
	uint64_t begin_;
	uint64_t end_;
	uint64_t unwind_address_;
	CommonInformationEntry *cie_;
	std::vector<uint8_t> call_frame_instructions_;
};

class ELFRuntimeFunctionList : public BaseRuntimeFunctionList
{
public:
	explicit ELFRuntimeFunctionList();
	explicit ELFRuntimeFunctionList(const ELFRuntimeFunctionList &src);
	~ELFRuntimeFunctionList();
	virtual void clear();
	ELFRuntimeFunctionList *Clone() const;
	ELFRuntimeFunction *item(size_t index) const;
	void ReadFromFile(ELFArchitecture &file);
	void WriteToFile(ELFArchitecture &file);
	void Rebase(uint64_t delta_base);
	virtual ELFRuntimeFunction *Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions);
	virtual ELFRuntimeFunction *GetFunctionByAddress(uint64_t address) const;
	CommonInformationEntryList *cie_list() const { return cie_list_; }
private:
	ELFRuntimeFunction *Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, CommonInformationEntry *cie, const std::vector<uint8_t> &call_frame_instructions);

	CommonInformationEntryList *cie_list_;
	uint8_t version_;
	uint8_t eh_frame_encoding_;
	uint8_t fde_count_encoding_;
	uint8_t fde_table_encoding_;

	// no assignment op
	ELFRuntimeFunctionList &operator =(const ELFRuntimeFunctionList &);
};

class ELFArchitecture : public BaseArchitecture
{
public:
	explicit ELFArchitecture(ELFFile *owner, uint64_t offset, uint64_t size);
	explicit ELFArchitecture(ELFFile *owner, const ELFArchitecture &src);
	virtual ~ELFArchitecture();
	virtual std::string name() const;
	virtual uint32_t type() const { return cpu_; }
	virtual OperandSize cpu_address_size() const { return cpu_address_size_; }
	virtual uint64_t entry_point() const { return entry_point_; }
	virtual uint64_t image_base() const { return image_base_; }
	uint16_t file_type() const { return file_type_; }
	uint32_t segment_alignment() const { return segment_alignment_; }
	uint32_t file_alignment() const { return file_alignment_; }
	virtual ELFDirectoryList *command_list() const { return directory_list_; }
	virtual ELFSegmentList *segment_list() const { return segment_list_; }
	virtual ELFImportList *import_list() const { return import_list_; }
	virtual ELFExportList *export_list() const { return export_list_; }
	virtual ELFFixupList *fixup_list() const { return fixup_list_; }
	virtual ELFRelocationList *relocation_list() const { return relocation_list_; }
	virtual IResourceList *resource_list() const { return NULL; }
	virtual ISEHandlerList *seh_handler_list() const { return NULL; }
	virtual ELFRuntimeFunctionList *runtime_function_list() const { return runtime_function_list_; }
	virtual IFunctionList *function_list() const { return function_list_; }
	virtual IVirtualMachineList *virtual_machine_list() const { return virtual_machine_list_; }
	virtual ELFSectionList *section_list() const { return section_list_; }
	OpenStatus ReadFromFile(uint32_t mode);
	virtual bool WriteToFile();
	virtual IArchitecture *Clone(IFile *file) const;
	virtual ELFArchitecture *Clone(ELFFile *file) const;
	virtual void Save(CompileContext &ctx);
	virtual bool is_executable() const;
	size_t shstrndx() { return shstrndx_; }
	virtual CallingConvention calling_convention() const { return cpu_address_size() == osDWord ? ccCdecl : ccABIx64; }
	void Rebase(uint64_t delta_base);
	ELFSymbolList *dynsymbol_list() const { return dynsymbol_list_; }
	ELFSymbolList *symbol_list() const { return symbol_list_; }
	ELFVerneedList *verneed_list() const { return verneed_list_; }
	ELFVerdefList *verdef_list() const { return verdef_list_; }
	ELFSegment *header_segment() const { return header_segment_; }
	uint32_t max_header_size() const { return header_size_ + resize_header_; }
protected:
	virtual bool Prepare(CompileContext &ctx);
private:
	ELFDirectoryList *directory_list_;
	ELFSegmentList *segment_list_;
	ELFSectionList *section_list_;
	ELFImportList *import_list_;
	ELFExportList *export_list_;
	ELFFixupList *fixup_list_;
	ELFRelocationList *relocation_list_;
	ELFRuntimeFunctionList *runtime_function_list_;
	IFunctionList *function_list_;
	IVirtualMachineList *virtual_machine_list_;

	uint16_t cpu_;
	uint16_t file_type_;
	OperandSize cpu_address_size_;
	uint64_t image_base_;
	uint64_t entry_point_;
	uint32_t segment_alignment_;
	uint32_t file_alignment_;
	uint16_t shstrndx_;
	uint64_t shoff_;
	uint32_t header_offset_;
	uint32_t header_size_;
	uint32_t resize_header_;
	ELFSymbolList *dynsymbol_list_;
	ELFSymbolList *symbol_list_;
	ELFVerneedList *verneed_list_;
	ELFVerdefList *verdef_list_;
	ELFSegment *header_segment_;
	uint64_t overlay_offset_;
	
	// no copy ctr or assignment op
	ELFArchitecture(const ELFArchitecture &);
	ELFArchitecture &operator =(const ELFArchitecture &);
};

class ELFFile : public IFile
{
public:
	explicit ELFFile(ILog *log);
	explicit ELFFile(const ELFFile &src, const char *file_name);
	virtual ~ELFFile();
	virtual std::string format_name() const;
	ELFArchitecture *item(size_t index) const;
	ELFFile *Clone(const char *file_name) const;
	virtual bool Compile(CompileOptions &options);
	virtual bool is_executable() const;
	virtual uint32_t disable_options() const;
protected:
	virtual OpenStatus ReadHeader(uint32_t open_mode);
	virtual IFile *runtime() const { return runtime_; }
private:
	ELFArchitecture *Add(uint64_t offset, uint64_t size);

	ELFFile *runtime_;

	// no copy ctr or assignment op
	ELFFile(const ELFFile &);
	ELFFile &operator =(const ELFFile &);
};

#endif