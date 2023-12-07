/**
 * Support of PE executable files.
 */

#ifndef PEFILE_H
#define PEFILE_H

class PEArchitecture;
class PEDirectoryList;
class PESegmentList;
class PEImportList;
class PEFixupList;
class PERelocationList;
class PELoadConfigDirectory;
class PERuntimeFunctionList;

class PEDirectory : public BaseLoadCommand
{
public:
	explicit PEDirectory(PEDirectoryList *owner, uint32_t type);
	explicit PEDirectory(PEDirectoryList *owner, const PEDirectory &src);
	virtual uint64_t address() const { return address_; }
	virtual uint32_t size() const { return size_; }
	virtual uint32_t type() const { return type_; }
	uint32_t physical_size() const { return physical_size_ ? physical_size_ : size_; }
	void set_physical_size(uint32_t physical_size) { physical_size_ = physical_size; }
	virtual std::string name() const;
	void ReadFromFile(PEArchitecture &file);
	void WriteToFile(PEArchitecture &file) const;
	virtual PEDirectory *Clone(ILoadCommandList *owner) const;
	void clear();
	void set_address(uint64_t address) { address_ = address; }
	void set_size(uint32_t size) { size_ = size; }
	void Rebase(uint64_t delta_base);
	virtual bool visible() const { return (address_ || size_); }
	void FreeByManager(MemoryManager &manager);
private:
	uint64_t address_;
	uint32_t size_;
	uint32_t type_;
	uint32_t physical_size_;
};

class PEDirectoryList : public BaseCommandList
{
public:
	explicit PEDirectoryList(PEArchitecture *owner);
	explicit PEDirectoryList(PEArchitecture *owner, const PEDirectoryList &src);
	PEDirectoryList *Clone(PEArchitecture *owner) const;
	PEDirectory *item(size_t index) const;
	void ReadFromFile(PEArchitecture &file, uint32_t count);
	void WriteToFile(PEArchitecture &file) const;
	PEDirectory *GetCommandByType(uint32_t type) const;
	PEDirectory *GetCommandByAddress(uint64_t address) const;
private:
	PEDirectory *Add(uint32_t type);
};

class PESegment : public BaseSection
{
public:
	explicit PESegment(PESegmentList *owner);
	explicit PESegment(PESegmentList *owner, uint64_t address, uint32_t size, uint32_t physical_offset, 
		uint32_t physical_size, uint32_t flags, const std::string &name);
	explicit PESegment(PESegmentList *owner, const PESegment &src);
	virtual uint64_t address() const { return address_; }
	virtual uint64_t size() const { return size_; }
	virtual uint32_t physical_offset() const { return physical_offset_; }
	virtual uint32_t physical_size() const { return physical_size_; }
	virtual std::string name() const { return name_; }
	virtual uint32_t memory_type() const;
	virtual uint32_t flags() const { return flags_; }
	void set_flags(uint32_t flags) { flags_ = flags; }
	void set_size(uint32_t size) { size_ = size; }
	void set_physical_size(uint32_t size) { physical_size_ = size; }
	void set_physical_offset(uint32_t offset) { physical_offset_ = offset; }
	void set_name(const std::string &name) { name_ = name; }
	void ReadFromFile(PEArchitecture &file);
	void WriteToFile(PEArchitecture &file) const;
	virtual PESegment *Clone(ISectionList *owner) const;
	virtual void update_type(uint32_t mt);
	virtual void Rebase(uint64_t delta_base);
private:
	std::string name_;
	uint64_t address_;
	uint32_t size_;
	uint32_t physical_offset_;
	uint32_t physical_size_;
	uint32_t flags_;
};

class PESegmentList : public BaseSectionList
{
public:
	explicit PESegmentList(PEArchitecture *owner);
	explicit PESegmentList(PEArchitecture *owner, const PESegmentList &src);
	~PESegmentList();
	PESegmentList *Clone(PEArchitecture *owner) const;
	PESegment *item(size_t index) const;
	PESegment *GetSectionByAddress(uint64_t address) const;
	void ReadFromFile(PEArchitecture &file, uint32_t count);
	void WriteToFile(PEArchitecture &file) const;
	PESegment *last() const;
	PESegment *Add(uint64_t address, uint32_t size, uint32_t physical_offset, uint32_t physical_size, uint32_t flags, const std::string &name);
	PESegment *header_segment() const { return header_segment_; }
private:
	PESegment *Add();

	PESegment *header_segment_;

	// no copy ctr or assignment op
	PESegmentList(const PESegmentList &);
	PESegmentList &operator =(const PESegmentList &);
};

class PESectionList;

class PESection : public BaseSection
{
public:
	explicit PESection(PESectionList *owner, PESegment *parent, uint64_t address, uint64_t size, const std::string &name);
	explicit PESection(PESectionList *owner, const PESection &src);
	virtual std::string name() const { return name_; }
	virtual uint64_t address() const { return address_; }
	virtual uint64_t size() const { return size_; }
	virtual uint32_t physical_offset() const { return parent_->physical_offset() + static_cast<uint32_t>(address_ - parent_->address()); }
	virtual uint32_t physical_size() const { return static_cast<uint32_t>(size_); }
	virtual uint32_t memory_type() const { return parent_->memory_type(); }
	virtual uint32_t flags() const { return 0; }
	virtual void update_type(uint32_t mt) {}
	virtual PESection *Clone(ISectionList *owner) const;
	virtual void Rebase(uint64_t delta_base);
	virtual PESegment *parent() const { return parent_; }
	void set_parent(PESegment *parent) { parent_ = parent; }
private:
	std::string name_;
	uint64_t address_;
	uint64_t size_;
	PESegment *parent_;
};

class PESectionList : public BaseSectionList
{
public:
	explicit PESectionList(PEArchitecture *owner);
	explicit PESectionList(PEArchitecture *owner, const PESectionList &src);
	virtual PESectionList *Clone(PEArchitecture *owner) const;
	PESection *item(size_t index) const;
	PESection *Add(PESegment *parent, uint64_t address, uint64_t size, const std::string &name);
};

class PEImport;

class PEImportFunction : public BaseImportFunction
{
public:
	explicit PEImportFunction(PEImport *owner);
	explicit PEImportFunction(PEImport *owner, const std::string &name);
	explicit PEImportFunction(PEImport *owner, uint64_t address, APIType type, MapFunction *map_function);
	explicit PEImportFunction(PEImport *owner, const PEImportFunction &src);
	virtual PEImportFunction *Clone(IImport *owner) const;
	bool ReadFromFile(PEArchitecture &arch, uint32_t &rva);
	virtual uint64_t address() const { return address_; }
	virtual std::string name() const { return name_; }
	bool is_ordinal() const { return is_ordinal_; }
	uint32_t ordinal() const { return ordinal_; }
	void FreeByManager(MemoryManager &manager, bool free_iat);
	virtual void Rebase(uint64_t delta_base);
	virtual std::string display_name(bool show_ret = true) const;
	bool IsInternal(const CompileContext &ctx) const;
private:
	std::string name_;
	uint64_t name_address_;
	uint64_t address_;
	bool is_ordinal_;
	uint32_t ordinal_;
};

class PEImport : public BaseImport
{
public:
	explicit PEImport(PEImportList *owner);
	explicit PEImport(PEImportList *owner, bool is_sdk);
	explicit PEImport(PEImportList *owner, const std::string &name);
	explicit PEImport(PEImportList *owner, const PEImport &src);
	virtual PEImport *Clone(IImportList *owner) const;
	PEImportFunction *item(size_t index) const;
	bool ReadFromFile(PEArchitecture &file);
	void WriteToFile(PEArchitecture &file) const;
	virtual std::string name() const { return name_; }
	virtual bool is_sdk() const { return is_sdk_; }
	bool FreeByManager(MemoryManager &manager, bool free_iat);
	virtual void Rebase(uint64_t delta_base);
	void set_name(const std::string &name) { name_ = name; }
protected:
	virtual	PEImportFunction *Add(uint64_t address, APIType type, MapFunction *map_function);
private:
	std::string name_;
	uint64_t name_address_;
	bool is_sdk_;
	uint64_t original_first_thunk_address_;
	uint64_t first_thunk_address_;
	uint32_t time_stamp_;
	uint32_t forwarder_chain_;
};

class PEImportList : public BaseImportList
{
public:
	explicit PEImportList(PEArchitecture *owner);
	explicit PEImportList(PEArchitecture *owner, const PEImportList &src);
	virtual PEImportList *Clone(PEArchitecture *owner) const;
	PEImport *item(size_t index) const;
	virtual PEImportFunction *GetFunctionByAddress(uint64_t address) const;
	void ReadFromFile(PEArchitecture &file, PEDirectory &dir);
	void FreeByManager(MemoryManager &manager, bool free_iat);
	virtual void Rebase(uint64_t delta_base);
	void WriteToFile(PEArchitecture &file, bool skip_sdk = false) const;
protected:
	virtual PEImport *AddSDK();
private:
	uint64_t address_;
};

class PEDelayImport;
class PEDelayImportList;

class PEDelayImportFunction : public IObject
{
public:
	explicit PEDelayImportFunction(PEDelayImport *owner);
	explicit PEDelayImportFunction(PEDelayImport *owner, const PEDelayImportFunction &src);
	~PEDelayImportFunction();
	PEDelayImportFunction *Clone(PEDelayImport *owner) const;
	bool ReadFromFile(PEArchitecture &file, uint64_t add_value);
	std::string name() const { return name_; }
	bool is_ordinal() const { return is_ordinal_; }
	uint32_t ordinal() const { return ordinal_; }
private:
	PEDelayImport *owner_;

	std::string name_;
	bool is_ordinal_;
	uint32_t ordinal_;
};

class PEDelayImport : public ObjectList<PEDelayImportFunction>
{
public:
	explicit PEDelayImport(PEDelayImportList *owner);
	explicit PEDelayImport(PEDelayImportList *owner, const PEDelayImport &src);
	~PEDelayImport();
	PEDelayImport *Clone(PEDelayImportList *owner) const;
	bool ReadFromFile(PEArchitecture &file);
	virtual std::string name() const { return name_; }
	uint32_t flags() const { return flags_; }
	uint64_t module() const { return module_; }
	uint64_t iat() const { return iat_; }
	uint64_t bound_iat() const { return bound_iat_; }
	uint64_t unload_iat() const { return unload_iat_; }
	uint32_t time_stamp() const { return time_stamp_; }
private:
	PEDelayImportList *owner_;

	std::string name_;
	uint32_t flags_;
	uint64_t module_;
	uint64_t iat_;
	uint64_t bound_iat_;
	uint64_t unload_iat_;
	uint32_t time_stamp_;
};

class PEDelayImportList : public ObjectList<PEDelayImport>
{
public:
	explicit PEDelayImportList();
	explicit PEDelayImportList(const PEDelayImportList &src);
	PEDelayImportList *Clone() const;
	void ReadFromFile(PEArchitecture &file, PEDirectory &dir);
};

class PEExportList;

class PEExport : public BaseExport
{
public:
	explicit PEExport(PEExportList *owner, uint64_t address, uint32_t ordinal);
	explicit PEExport(PEExportList *owner, const PEExport &src);
	virtual uint64_t address() const { return address_; }
	virtual std::string name() const { return name_; }
	virtual std::string forwarded_name() const { return forwarded_name_; }
	virtual std::string display_name(bool show_ret = true) const;
	uint32_t ordinal() const { return ordinal_; }
	void set_name(const std::string &name) { name_ = name; }
	void set_forwarded_name(const std::string &forwarded_name) { forwarded_name_ = forwarded_name; }
	virtual PEExport *Clone(IExportList *owner) const;
	/*virtual*/ int CompareWith(const IObject &obj) const;
	void FreeByManager(MemoryManager &manager);
	void ReadFromFile(PEArchitecture &file, uint64_t address_of_name, bool is_forwarded);
	virtual void Rebase(uint64_t delta_base);
private:
	uint64_t address_;
	uint32_t ordinal_;
	uint64_t address_of_name_;
	std::string name_;
	std::string forwarded_name_;
};

class PEExportList : public BaseExportList
{
public:
	explicit PEExportList(PEArchitecture *owner);
	explicit PEExportList(PEArchitecture *owner, const PEExportList &src);
	virtual PEExportList *Clone(PEArchitecture *owner) const;
	virtual std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	uint32_t characteristics() const { return characteristics_; }
	uint32_t time_date_stamp() const { return time_date_stamp_; }
	uint16_t major_version() const { return major_version_; }
	uint16_t minor_version() const { return minor_version_; }
	PEExport *item(size_t index) const;
	void ReadFromFile(PEArchitecture &arch, PEDirectory &dir);
	void FreeByManager(MemoryManager &manager);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	uint32_t WriteToData(IFunction &data, uint64_t image_base);
	void AddAntidebug();
protected:
	virtual PEExport *Add(uint64_t address) { return Add(address, 0); }
private:
	PEExport *Add(uint64_t address, uint32_t ordinal);
	PEExport *GetExportByOrdinal(uint32_t ordinal);

	uint64_t address_;
	uint64_t name_address_;
	uint32_t characteristics_;
	uint32_t time_date_stamp_;
	uint16_t major_version_;
	uint16_t minor_version_;
	std::string name_;
	uint32_t number_of_functions_;
	uint64_t address_of_functions_;
	uint32_t number_of_names_;
	uint64_t address_of_names_;
	uint64_t address_of_name_ordinals_;

	struct NameInfo {
		uint32_t ordinal_index;
		uint32_t address_of_name;
		bool operator == (uint16_t ordinal_index_) const
		{
			return (ordinal_index == ordinal_index_);
		}
	};

	struct ExportInfo {
		PEExport *export_function;
		ExportInfo(PEExport *export_function_) : export_function(export_function_) {}
		bool operator< (const ExportInfo &obj) const
		{
			return (export_function->name().compare(obj.export_function->name()) < 0);
		}
	};

};

class PEFixup : public BaseFixup
{
public:
	explicit PEFixup(PEFixupList *owner, uint64_t address, uint8_t type);
	explicit PEFixup(PEFixupList *owner, const PEFixup &src);
	virtual uint64_t address() const { return address_; }
	virtual FixupType type() const;
	virtual OperandSize size() const;
	virtual PEFixup *Clone(IFixupList *owner) const;
	uint8_t internal_type() const { return type_; }
	virtual void set_address(uint64_t address) { address_ = address; }
	virtual void Rebase(IArchitecture &file, uint64_t delta_base);
private:
	uint64_t address_;
	uint8_t type_;
};

class PEFixupList : public BaseFixupList
{
public:
	explicit PEFixupList();
	explicit PEFixupList(const PEFixupList &src);
	virtual PEFixupList *Clone() const;
	PEFixup *item(size_t index) const;
	void ReadFromFile(PEArchitecture &file, PEDirectory &dir);
	void WriteToData(Data &data, uint64_t image_base);
	size_t WriteToFile(PEArchitecture &file);
	virtual IFixup *AddDefault(OperandSize cpu_address_size, bool is_code);
private:
	PEFixup *Add(uint64_t address, uint8_t type);

	// no assignment op
	PEFixupList &operator =(const PEFixupList &);
};

class PERelocation : public BaseRelocation
{
public:
	explicit PERelocation(PERelocationList *owner, uint64_t address, uint64_t source, OperandSize size, uint32_t addend);
	explicit PERelocation(PERelocationList *owner, const PERelocation &src);
	virtual PERelocation *Clone(IRelocationList *owner) const;
	uint64_t source() const { return source_; }
	uint32_t addend() const { return addend_; }
	virtual ISymbol *symbol() const { return NULL; }
private:
	uint64_t source_;
	uint32_t addend_;
};

class PERelocationList : public BaseRelocationList
{
public:
	explicit PERelocationList();
	explicit PERelocationList(const PERelocationList &src);
	virtual PERelocationList *Clone() const;
	PERelocation *item(size_t index) const;
	void ReadFromFile(PEArchitecture &file);
	void WriteToData(Data &data, uint64_t image_base);
private:
	void ParseMinGW(PEArchitecture &file, uint64_t address, uint64_t start, uint64_t end);
	PERelocation *Add(uint64_t address, uint64_t source, OperandSize size, uint32_t addend);
	uint64_t address_;
	uint64_t mem_address_;

	// no assignment op
	PERelocationList &operator =(const PERelocationList &);
};

class PESEHandler : public BaseSEHandler
{
public:
	explicit PESEHandler(ISEHandlerList *owner, uint64_t address);
	explicit PESEHandler(ISEHandlerList *owner, const PESEHandler &src);
	virtual PESEHandler *Clone(ISEHandlerList *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual void set_address(uint64_t address) { address_ = address; }
	virtual bool is_deleted() const { return deleted_; }
	virtual void set_deleted(bool deleted) { deleted_ = deleted; }
	void Rebase(uint64_t delta_base);
private:
	uint64_t address_;
	bool deleted_;
};

class PESEHandlerList : public BaseSEHandlerList
{
public:
	explicit PESEHandlerList();
	explicit PESEHandlerList(const PESEHandlerList &src);
	virtual PESEHandlerList *Clone() const;
	PESEHandler *item(size_t index) const;
	virtual PESEHandler *Add(uint64_t address);
	void Rebase(uint64_t delta_base);
	void Pack();
};

class PECFGAddressTable;

class PECFGAddress : public IObject
{
public:
	explicit PECFGAddress(PECFGAddressTable *owner, uint64_t address);
	explicit PECFGAddress(PECFGAddressTable *owner, const PECFGAddress &src);
	~PECFGAddress();
	PECFGAddress *Clone(PECFGAddressTable *owner) const;
	void Rebase(uint64_t delta_base);
	uint64_t address() const { return address_; }
	void set_data(std::vector<uint8_t> value) { data_ = value; }
	std::vector<uint8_t> data() const { return data_; }
private:
	PECFGAddressTable *owner_;
	uint64_t address_;
	std::vector<uint8_t> data_;
};

class PECFGAddressTable : public ObjectList<PECFGAddress>
{
public:
	explicit PECFGAddressTable();
	explicit PECFGAddressTable(const PECFGAddressTable &src);
	PECFGAddressTable *Clone() const;
	PECFGAddress *Add(uint64_t address);
	void Rebase(uint64_t delta_base);
};

class PELoadConfigDirectory : public IObject
{
public:
	explicit PELoadConfigDirectory();
	explicit PELoadConfigDirectory(const PELoadConfigDirectory &src);
	~PELoadConfigDirectory();
	virtual PELoadConfigDirectory *Clone() const;
	void ReadFromFile(PEArchitecture &file, PEDirectory &dir);
	size_t WriteToFile(PEArchitecture &file);
	void FreeByManager(MemoryManager &manager);
	void Rebase(uint64_t delta_base);
	uint64_t security_cookie() const { return security_cookie_; }
	void set_security_cookie(uint64_t value) { security_cookie_ = value; }
	uint64_t cfg_check_function() const { return cfg_check_function_; }
	void set_cfg_check_function(uint64_t value) { cfg_check_function_ = value; }
	PESEHandlerList *seh_handler_list() const { return seh_handler_list_; }
	PECFGAddressTable *cfg_address_list() const { return cfg_address_list_; }
	uint64_t seh_table_address() const { return seh_table_address_; }
	uint64_t cfg_table_address() const { return cfg_table_address_; }
private:
	uint64_t seh_table_address_;
	uint64_t security_cookie_;
	uint64_t cfg_table_address_;
	uint64_t cfg_check_function_;
	uint32_t guard_flags_;
	PESEHandlerList *seh_handler_list_;
	PECFGAddressTable *cfg_address_list_;

	// no assignment op
	PELoadConfigDirectory &operator =(const PELoadConfigDirectory &);
};

enum PEResourceType {
	rtUnknown,
	rtCursor = 1,
	rtBitmap = 2,
	rtIcon = 3,
	rtMenu = 4,
	rtDialog = 5,
	rtStringTable = 6,
	rtFontDir = 7,
	rtFont = 8,
	rtAccelerators = 9,
	rtRCData = 10,
	rtMessageTable = 11,
	rtGroupCursor = 12,
	rtGroupIcon = 14,
	rtVersionInfo = 16,
	rtDlgInclude = 17,
	rtPlugPlay = 19,
	rtVXD = 20,
	rtAniCursor = 21,
	rtAniIcon = 22,
	rtHTML = 23,
	rtManifest = 24,
	rtDialogInit = 240,
	rtToolbar = 241
};

class PEResource : public BaseResource
{
public:
	explicit PEResource(IResource *owner, PEResourceType type, uint32_t name_offset, uint32_t data_offset);
	explicit PEResource(IResource *owner, const PEResource &src);
	virtual PEResource *Clone(IResource *owner) const;
	PEResource *item(size_t index) const;
	virtual uint32_t type() const { return type_; }
	virtual uint64_t address() const { return is_directory() ? 0 : address_; }
	virtual size_t size() const { return is_directory() ? 0 : data_.item.Size; }
	virtual std::string name() const { return has_name() ? "\"" + name_ + "\"" : name_; }
	virtual bool is_directory() const { return (data_offset_ & IMAGE_RESOURCE_DATA_IS_DIRECTORY) != 0; }
	virtual PEResource *GetResourceByName(const std::string &name) const;
	void set_name(const std::string &name) { name_ = name; }
	bool has_name() const { return (name_offset_ & IMAGE_RESOURCE_NAME_IS_STRING) != 0; }
	bool need_store() const;
	virtual std::string id() const;
	void ReadFromFile(PEArchitecture &file, uint64_t root_address);
	// PE format
	void WriteHeader(Data &data);
	void WriteEntry(Data &data);
	void WriteName(Data &data);
	size_t WriteData(Data &data, PEArchitecture &file);
	// ResourceManager format
	void WriteHeader(IFunction &data);
	void WriteEntry(IFunction &data);
	void WriteName(IFunction &data, size_t root_index, uint32_t key);
	void WriteData(IFunction &data, PEArchitecture &file, uint32_t key);
private:
	PEResource *Add(PEResourceType type, uint32_t name_offset, uint32_t data_offset);

	PEResourceType type_;
	uint32_t name_offset_;
	uint32_t data_offset_;
	union {
		IMAGE_RESOURCE_DIRECTORY dir;
		IMAGE_RESOURCE_DATA_ENTRY item;
	} data_;
	std::string name_;
	uint64_t address_;
	size_t entry_offset_;
	size_t data_entry_offset_;
};

class PEResourceList : public BaseResourceList
{
public:
	explicit PEResourceList(PEArchitecture *owner);
	explicit PEResourceList(PEArchitecture *owner, const PEResourceList &src);

	using BaseResourceList::Clone;
	PEResourceList *Clone(PEArchitecture *owner) const;
	PEResource *item(size_t index) const;
	void ReadFromFile(PEArchitecture &file, PEDirectory &dir);
	size_t WriteToFile(PEArchitecture &file, uint64_t address);
	void Compile(PEArchitecture &file, bool for_packing);
	size_t size() const { return data_.size(); }
	size_t store_size() const { return store_size_; }
	void WritePackData(Data &data);
	void CreateCommands(PEArchitecture &file, IFunction &data);
private:
	PEResource *Add(PEResourceType type, uint32_t name_offset, uint32_t data_offset);
	IMAGE_RESOURCE_DIRECTORY dir_;
	Data data_;
	std::vector<size_t> link_list_;
	size_t store_size_;
};

class PERuntimeFunction : public BaseRuntimeFunction
{
public:
	explicit PERuntimeFunction(PERuntimeFunctionList *owner, uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address);
	explicit PERuntimeFunction(PERuntimeFunctionList *owner, const PERuntimeFunction &src);
	virtual PERuntimeFunction *Clone(IRuntimeFunctionList *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual uint64_t begin() const { return begin_; }
	virtual uint64_t end() const { return end_; }
	virtual uint64_t unwind_address() const { return unwind_address_; }
	virtual void set_begin(uint64_t begin) { begin_ = begin; }
	virtual void set_end(uint64_t end) { end_ = end; }
	virtual void set_unwind_address(uint64_t unwind_address) { unwind_address_ = unwind_address; }
	virtual void Rebase(uint64_t delta_base);
	virtual void Parse(IArchitecture &file, IFunction &dest);
private:
	uint64_t address_;
	uint64_t begin_;
	uint64_t end_;
	uint64_t unwind_address_;
};

class PERuntimeFunctionList : public BaseRuntimeFunctionList
{
public:
	explicit PERuntimeFunctionList();
	explicit PERuntimeFunctionList(const PERuntimeFunctionList &src);
	PERuntimeFunctionList *Clone() const;
	PERuntimeFunction *item(size_t index) const;
	void ReadFromFile(PEArchitecture &file, PEDirectory &directory);
	size_t WriteToFile(PEArchitecture &file);
	virtual PERuntimeFunction *Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions);
	virtual PERuntimeFunction *GetFunctionByAddress(uint64_t address) const;
	void RebaseByFile(IArchitecture &file, uint64_t target_image_base, uint64_t delta_base);
	void FreeByManager(MemoryManager &manager);
	uint64_t address() const { return address_; }
private:
	uint64_t RebaseDWord(IArchitecture &file, uint32_t delta_base);
	uint64_t address_;

	// no assignment op
	PERuntimeFunctionList &operator =(const PERuntimeFunctionList &);
};

class PETLSDirectory : public ReferenceList
{
public:
	explicit PETLSDirectory();
	explicit PETLSDirectory(const PETLSDirectory &src);
	PETLSDirectory *Clone() const;
	void ReadFromFile(PEArchitecture &file, PEDirectory &directory);
	void FreeByManager(MemoryManager &manager);
	uint64_t address() const { return address_; }
	uint64_t start_address_of_raw_data() const { return start_address_of_raw_data_; }
	uint64_t end_address_of_raw_data() const { return end_address_of_raw_data_; }
	uint64_t address_of_index() const { return address_of_index_; }
	uint64_t address_of_call_backs() const { return address_of_call_backs_; }
	uint32_t size_of_zero_fill() const { return size_of_zero_fill_; }
	uint32_t characteristics() const { return characteristics_; }
	void set_start_address_of_raw_data(uint64_t value) { start_address_of_raw_data_ = value; }
	void set_end_address_of_raw_data(uint64_t value) { end_address_of_raw_data_ = value; }
private:
	uint64_t address_;
	uint64_t start_address_of_raw_data_;
	uint64_t end_address_of_raw_data_;
	uint64_t address_of_index_;
	uint64_t address_of_call_backs_;
	uint32_t size_of_zero_fill_;
	uint32_t characteristics_;

	// no assignment op
	PETLSDirectory &operator =(const PETLSDirectory &);
};

class PEDebugDirectory;

class PEDebugData : public IObject
{
public:
	explicit PEDebugData(PEDebugDirectory *owner);
	explicit PEDebugData(PEDebugDirectory *owner, const PEDebugData &src);
	~PEDebugData();
	PEDebugData *Clone(PEDebugDirectory *owner) const;
	void ReadFromFile(PEArchitecture &file);
	void WriteToFile(PEArchitecture &file);
	uint64_t address() const { return address_; }
	uint32_t offset() const { return offset_; }
	uint32_t size() const { return size_; }
	uint32_t type() const { return type_; }
	void set_address(uint64_t address) { address_ = address; }
	void set_offset(uint32_t offset) { offset_ = offset; }
private:
	PEDebugDirectory *owner_;
    uint32_t characteristics_;
    uint32_t time_date_stamp_;
    uint16_t major_version_;
    uint16_t minor_version_;
    uint32_t type_;
    uint32_t size_;
    uint64_t address_;
	uint32_t offset_;
};

class PEDebugDirectory : public ObjectList<PEDebugData>
{
public:
	explicit PEDebugDirectory();
	explicit PEDebugDirectory(const PEDebugDirectory &src);
	PEDebugDirectory *Clone() const;
	uint64_t address() const { return address_; }
	void ReadFromFile(PEArchitecture &file, PEDirectory &directory);
	void WriteToFile(PEArchitecture &file);
	void FreeByManager(MemoryManager &manager) const;
private:
	PEDebugData *Add();
	uint64_t address_;

	// not impl
	PEDebugDirectory &operator =(const PEDebugDirectory &);
};

class pdb_reader;

class PDBFile : public BaseMapFile
{
public:
	explicit PDBFile();
	virtual bool Parse(const char *file_name, const std::vector<uint64_t> &segments);
	virtual std::string file_name() const { return file_name_; }
	virtual uint64_t time_stamp() const { return time_stamp_; }
	std::vector<uint8_t> guid() const { return guid_; }
	void set_time_stamp(uint64_t value) { time_stamp_ = value; }
private:
	bool ReadSymbols(pdb_reader &reader);
	void codeview_dump_symbols(const std::vector<uint8_t> &root, size_t offset);
	std::string GetTypeName(size_t type, const std::string &name);
	void AddSymbol(size_t segment, size_t offset, const std::string &name);
	void AddSection(size_t segment, size_t offset, uint64_t size, const std::string &name);

	std::string file_name_;
	uint64_t time_stamp_;
	std::vector<uint8_t> guid_;
	std::vector<uint64_t> segments_;
	size_t types_first_index_;
	std::vector<uint8_t> types_data_;
	std::vector<const union codeview_type *> types_offset_;
	std::set<std::pair<uint64_t, std::string> > map_;
};

class COFFStringTable
{
public:
	std::string GetString(uint32_t pos) const;
	void ReadFromFile(PEArchitecture &file);
	void ReadFromFile(FileStream &file);
private:
	std::vector<char> data_;
};

class COFFFile : public BaseMapFile
{
public:
	bool Parse(const char *file_name, const std::vector<uint64_t> &segments);
	virtual std::string file_name() const { return file_name_; }
	virtual uint64_t time_stamp() const { return time_stamp_; }
private:
	void AddSymbol(size_t segment, size_t offset, const std::string &name);
	uint64_t time_stamp_;
	std::string file_name_;
	std::vector<uint64_t> segments_;
};

class PEFile;

enum ImageType {
	itExe,
	itLibrary,
	itDriver
};

class PEArchitecture : public BaseArchitecture
{
public:
	explicit PEArchitecture(PEFile *owner, uint64_t offset, uint64_t size);
	explicit PEArchitecture(PEFile *owner, const PEArchitecture &src);
	virtual ~PEArchitecture();
	virtual PEArchitecture *Clone(IFile *file) const;
	OpenStatus ReadFromFile(uint32_t mode);
	virtual void ReadFromBuffer(Buffer &buffer);
	void WriteCheckSum();
	virtual bool WriteToFile();
	virtual bool is_executable() const;
	virtual std::string name() const;
	virtual uint32_t type() const { return cpu_; }
	virtual OperandSize cpu_address_size() const { return cpu_address_size_; }
	virtual uint64_t entry_point() const { return entry_point_; }
	void set_entry_point(uint64_t entry_point) { entry_point_ = entry_point; }
	virtual uint32_t segment_alignment() const { return segment_alignment_; }
	virtual uint32_t file_alignment() const { return file_alignment_; }
	virtual uint64_t image_base() const { return image_base_; }
	virtual PEDirectoryList *command_list() const { return directory_list_; }
	virtual PESegmentList *segment_list() const { return segment_list_; }
	virtual PESectionList *section_list() const { return section_list_; }
	virtual PEImportList *import_list() const { return import_list_; }
	virtual PEExportList *export_list() const { return export_list_; }
	virtual PEFixupList *fixup_list() const { return fixup_list_; }
	virtual PERelocationList *relocation_list() const { return relocation_list_; }
	virtual PEResourceList *resource_list() const { return resource_list_; }
	virtual PESEHandlerList *seh_handler_list() const { return load_config_directory_->seh_handler_list(); }
	PETLSDirectory *tls_directory() const { return tls_directory_; }
	PELoadConfigDirectory *load_config_directory() const { return load_config_directory_; }
	PEDelayImportList *delay_import_list() const { return delay_import_list_; }
	virtual IFunctionList *function_list() const { return function_list_; }
	virtual IVirtualMachineList *virtual_machine_list() const { return virtual_machine_list_; }
	virtual PERuntimeFunctionList *runtime_function_list() const { return runtime_function_list_; }
	virtual bool Compile(CompileOptions &options, IArchitecture *runtime);
	virtual void Save(CompileContext &ctx);
	ImageType image_type() const { return image_type_; }
	void Rebase(uint64_t target_image_base, uint64_t delta_base);
	virtual CallingConvention calling_convention() const { return (cpu_address_size() == osDWord) ? ccStdcall : ccMSx64; }
	virtual uint64_t time_stamp() const { return time_stamp_; }
	PESegment *resource_section() const { return resource_section_; }
	PESegment *fixup_section() const { return fixup_section_; }
	uint32_t header_offset() const { return header_offset_; }
	uint32_t header_size() const { return header_size_; }
	virtual std::string ANSIToUTF8(const std::string &str) const;
	uint16_t dll_characteristics() const { return dll_characteristics_; }
	std::string pdb_file_name() const;
	uint32_t operating_system_version() const { return operating_system_version_; }
protected:
	virtual bool Prepare(CompileContext &ctx);
	virtual bool ReadMapFile(IMapFile &map_file);
private:
	enum {
		MIN_HEADER_OFFSET = 0x80
	};
	PEDirectoryList *directory_list_;
	PESegmentList *segment_list_;
	PESectionList *section_list_;
	PEImportList *import_list_;
	PEExportList *export_list_;
	PEFixupList *fixup_list_;
	PERelocationList *relocation_list_;
	IFunctionList *function_list_;
	PEResourceList *resource_list_;
	PELoadConfigDirectory *load_config_directory_;
	IVirtualMachineList *virtual_machine_list_;
	PERuntimeFunctionList *runtime_function_list_;
	PETLSDirectory *tls_directory_;
	PEDebugDirectory *debug_directory_;
	PEDelayImportList *delay_import_list_;
	uint32_t cpu_;
	OperandSize cpu_address_size_;
	uint64_t time_stamp_;
	uint64_t entry_point_;
	uint64_t image_base_;
	uint32_t header_offset_;
	uint32_t header_size_;
	uint32_t segment_alignment_;
	uint32_t file_alignment_;
	PESegment *resource_section_;
	PESegment *fixup_section_;
	size_t optimized_section_count_;
	ImageType image_type_;
	uint16_t characterictics_;
	uint32_t check_sum_;
	uint32_t low_resize_header_;
	uint32_t resize_header_;
	uint32_t operating_system_version_;
	uint32_t subsystem_version_;
	uint16_t dll_characteristics_;
	
	// no copy ctr or assignment op
	PEArchitecture(const PEArchitecture &);
	PEArchitecture &operator =(const PEArchitecture &);
};

class NETArchitecture;

class PEFile : public IFile
{
public:
	explicit PEFile(ILog *log = NULL);
	explicit PEFile(const PEFile &src, const char *file_name);
	virtual ~PEFile();
	virtual std::string format_name() const;
	virtual PEFile *Clone(const char *file_name) const;
	virtual bool Compile(CompileOptions &options);
	virtual std::string version() const;
	virtual bool is_executable() const;
	virtual uint32_t disable_options() const;
	bool GetCheckSum(uint32_t *check_sum);
	PEArchitecture *arch_pe() const { return count() > 0 ? dynamic_cast<PEArchitecture *>(item(0)) : NULL; }
	virtual std::string exec_command() const;
protected:
	virtual OpenStatus ReadHeader(uint32_t open_mode);
	bool WriteHeader();
	virtual IFile *runtime() const { return runtime_; };
private:
	PEFile *runtime_;

	// no copy ctr or assignment op
	PEFile(const PEFile &);
	PEFile &operator =(const PEFile &);
};

#endif