/**
 * Support of Mach-O executable files.
 */

#ifndef MACFILE_H
#define MACFILE_H

class MacFile;
class MacArchitecture;
class MacLoadCommandList;
class MacSegmentList;
class MacSectionList;
class MacImport;
class MacImportList;
class MacFixup;
class MacFixupList;
class MacRuntimeFunctionList;
class CommonInformationEntry;
class CommonInformationEntryList;
class EncodedData;

class MacLoadCommand : public BaseLoadCommand
{
public:
	explicit MacLoadCommand(MacLoadCommandList *owner);
	explicit MacLoadCommand(MacLoadCommandList *owner, uint64_t address, uint32_t size, uint32_t type);
	explicit MacLoadCommand(MacLoadCommandList *owner, uint32_t type, IObject *object);
	explicit MacLoadCommand(MacLoadCommandList *owner, const MacLoadCommand &src);
	virtual uint64_t address() const { return address_; }
	virtual uint32_t size() const { return size_; }
	virtual uint32_t type() const { return type_; }
	virtual std::string name() const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	virtual MacLoadCommand *Clone(ILoadCommandList *owner) const;
	void Rebase(uint64_t delta_base);
	void set_offset(uint32_t offset) { offset_ = offset; }
	void *object() const { return object_; }
private:
	uint64_t address_;
	uint32_t size_;
	uint32_t type_;
	IObject *object_;
	uint32_t offset_;
};

class MacLoadCommandList : public BaseCommandList
{
public:
	explicit MacLoadCommandList(MacArchitecture *owner);
	explicit MacLoadCommandList(MacArchitecture *owner, const MacLoadCommandList &src);
	virtual MacLoadCommandList *Clone(MacArchitecture *owner) const;
	void ReadFromFile(MacArchitecture &file, size_t count);
	void WriteToFile(MacArchitecture &file);
	MacLoadCommand *item(size_t index) const;
	void Pack();
	void Add(uint32_t type, IObject *object);
	MacLoadCommand *GetCommandByObject(void *object) const;
};

class MacSegment : public BaseSection
{
public:
	explicit MacSegment(MacSegmentList *owner);
	explicit MacSegment(MacSegmentList *owner, uint64_t address, uint64_t size, uint32_t physical_offset, 
		uint32_t physical_size, uint32_t initprot, const std::string &name);
	explicit MacSegment(MacSegmentList *owner, const MacSegment &src);
	virtual MacSegment *Clone(ISectionList *owner) const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	virtual uint64_t address() const { return address_; }
	virtual uint64_t size() const { return size_; }
	virtual uint32_t physical_offset() const { return physical_offset_; }
	virtual uint32_t physical_size() const { return physical_size_; }
	virtual std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	virtual uint32_t memory_type() const;
	virtual void update_type(uint32_t mt);
	void set_address(uint64_t address) { address_ = address; }
	void set_size(uint64_t size) { size_ = size; }
	void set_physical_offset(uint32_t offset) { physical_offset_ = offset; }
	void set_physical_size(uint32_t size) { physical_size_ = size; }
	virtual uint32_t flags() const { return initprot_; }
	void set_flags(uint32_t flags) { initprot_ = flags; }
	virtual void Rebase(uint64_t delta_base);
	void include_maxprot(vm_prot_t value) { maxprot_ |= value; }
private:
	uint64_t address_;
	uint64_t size_;
	uint32_t physical_offset_;
	uint32_t physical_size_;
	uint32_t nsects_;
	vm_prot_t maxprot_;
	vm_prot_t initprot_;
	std::string name_;
};

class MacSegmentList : public BaseSectionList
{
public:
	explicit MacSegmentList(MacArchitecture *owner);
	explicit MacSegmentList(MacArchitecture *owner, const MacSegmentList &src);
	virtual MacSegmentList *Clone(MacArchitecture *owner) const;
	MacSegment *GetSectionByAddress(uint64_t address) const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	MacSegment *item(size_t index) const;
	MacSegment *last() const;
	MacSegment *Add(uint64_t address, uint64_t size, uint32_t physical_offset, uint32_t physical_size, uint32_t initprot, const std::string &name);
	MacSegment *GetSectionByName(const std::string &name) const;
	MacSegment *GetBaseSegment() const;
private:
	MacSegment *Add();
};

class MacStringTable
{
public:
	std::string GetString(uint32_t pos) const;
	uint32_t AddString(const std::string &str);
	void clear();
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
private:
	std::vector<char> data_;
};

class MacSymbolList;

class MacSymbol : public IObject
{
public:
	explicit MacSymbol(MacSymbolList *owner);
	explicit MacSymbol(MacSymbolList *owner, const MacSymbol &src);
	~MacSymbol();
	virtual MacSymbol *Clone(MacSymbolList *owner) const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	uint8_t type() const { return type_; }
	uint8_t sect() const { return sect_; }
	uint16_t desc() const { return desc_; }
	uint64_t value() const { return value_; }
	std::string name() const { return name_; }
	bool is_deleted() const { return is_deleted_; }
	void set_deleted(bool value) { is_deleted_ = value; }
	uint8_t library_ordinal() const;
	void set_library_ordinal(uint8_t library_ordinal);
	void set_value(uint64_t value) { value_ = value; }
private:
	MacSymbolList *owner_;
	uint8_t type_;
	uint8_t sect_;
	uint16_t desc_;
	uint64_t value_;
	std::string name_;
	bool is_deleted_;
};

class MacSymbolList : public ObjectList<MacSymbol>
{
public:
	explicit MacSymbolList();
	explicit MacSymbolList(const MacSymbolList &src);
	virtual MacSymbolList *Clone() const;
	void ReadFromFile(MacArchitecture &file, size_t count);
	void WriteToFile(MacArchitecture &file);
	MacSymbol *GetSymbol(const std::string &name, int library_ordinal) const;
	void Pack();
private:
	// no assignment op
	MacSymbolList &operator =(const MacSymbolList &);
};

class MacSection : public BaseSection
{
public:
	explicit MacSection(MacSectionList *owner, MacSegment *parent);
	explicit MacSection(MacSectionList *owner, MacSegment *parent, uint64_t address, uint32_t size, uint32_t offset, uint32_t flags, const std::string &name, const std::string &segment_name);
	explicit MacSection(MacSectionList *owner, const MacSection &src);
	~MacSection();
	uint32_t type() const { return flags_ & SECTION_TYPE; }
	uint32_t reserved1() const { return reserved1_; }
	uint32_t reserved2() const { return reserved2_; }
	virtual uint64_t address() const { return address_; }
	virtual uint64_t size() const { return size_; }
	virtual uint32_t physical_offset() const { return offset_; }
	virtual uint32_t physical_size() const { return size_; }
	virtual std::string name() const { return name_; }
	virtual uint32_t memory_type() const { return parent_->memory_type(); }
	virtual MacSegment *parent() const { return parent_; }
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	virtual MacSection *Clone(ISectionList *owner) const;
	void set_parent(MacSegment *parent) { parent_ = parent; }
	virtual void update_type(uint32_t mt) { }
	virtual uint32_t flags() const { return flags_; }
	void set_name(const std::string &name) { name_ = name; }
	void set_address(uint64_t address) { address_ = address; }
	void set_physical_offset(uint32_t physical_offset) { offset_ = physical_offset; }
	void set_physical_size(uint32_t physical_size) { size_ = physical_size; }
	void set_flags(uint32_t flags) { flags_ = flags; }
	void set_reserved1(uint32_t reserved1) { reserved1_ = reserved1; }
	void set_reserved2(uint32_t reserved2) { reserved2_ = reserved2; }
	void set_alignment(size_t value);
	virtual void Rebase(uint64_t delta_base);
	std::string segment_name() const { return segment_name_; }
private:
	std::string name_;
	std::string segment_name_;
	uint64_t address_;
	uint32_t size_;
	uint32_t offset_;
	uint32_t align_;
	uint32_t reloff_;
	uint32_t nreloc_;
	uint32_t flags_;
	uint32_t reserved1_;
	uint32_t reserved2_;
	MacSegment *parent_;
};

class MacSectionList : public BaseSectionList
{
public:
	explicit MacSectionList(MacArchitecture *owner);
	explicit MacSectionList(MacArchitecture *owner, const MacSectionList &src);
	virtual MacSectionList *Clone(MacArchitecture *owner) const;
	void ReadFromFile(MacArchitecture &file, size_t count, MacSegment *segment);
	MacSection *item(size_t index) const;
	MacSection *GetSectionByAddress(uint64_t address) const;
	MacSection *GetSectionByName(const std::string &name) const;
	MacSection *GetSectionByName(ISection *segment, const std::string &name) const;
	MacSection *Add(MacSegment *segment, uint64_t address, uint32_t size, uint32_t offset, uint32_t flags, const std::string &name, const std::string &segment_name);
};

class MacImportFunction : public BaseImportFunction
{
public:
	explicit MacImportFunction(IImport *owner, uint64_t address, APIType type, MapFunction *map_function);
	explicit MacImportFunction(IImport *owner, uint64_t address, uint8_t bind_type, size_t bind_offset, 
		const std::string &name, uint32_t flags, int64_t addend, bool is_lazy, MacSymbol *symbol);
	explicit MacImportFunction(IImport *owner, const MacImportFunction &src);
	virtual MacImportFunction *Clone(IImport *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual std::string name() const { return name_; }
	uint32_t flags() const { return flags_; }
	int64_t addend() const { return addend_; }
	MacSymbol *symbol() const { return symbol_; }
	void set_symbol(MacSymbol *symbol) { symbol_ = symbol; }
	uint8_t bind_type() const { return bind_type_; }
	bool is_lazy() const { return is_lazy_; }
	bool is_weak() const;
	virtual void Rebase(uint64_t delta_base);
	int library_ordinal() const;
	size_t bind_offset() const { return bind_offset_; }
	virtual std::string display_name(bool show_ret = true) const;
	void set_address(uint64_t address) { address_ = address; }
	void set_bind_offset(size_t bind_offset) { bind_offset_ = bind_offset; }
private:
	uint64_t address_;
	uint8_t bind_type_;
	std::string name_;
	uint32_t flags_;
	int64_t addend_;
	bool is_lazy_;
	MacSymbol *symbol_;
	size_t bind_offset_;
};

class MacImport : public BaseImport
{
public:
	explicit MacImport(MacImportList *owner, int library_ordinal, bool is_sdk = false);
	explicit MacImport(MacImportList *owner, int library_ordinal, const std::string &name, uint32_t current_version, uint32_t compatibility_version);
	explicit MacImport(MacImportList *owner, const MacImport &src);
	virtual MacImport *Clone(IImportList *owner) const;
	virtual std::string name() const { return name_; }
	virtual bool is_sdk() const { return is_sdk_; }
	void set_is_sdk(bool is_sdk) { is_sdk_ = is_sdk; }
	int library_ordinal() const { return library_ordinal_; }
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	MacImportFunction *Add(uint64_t address, uint8_t bind_type, size_t bind_offset, const std::string &name, uint32_t flags, int64_t addend, bool is_lazy, MacSymbol *symbol);
	MacImportFunction *item(size_t index) const;
	virtual MacImportFunction *GetFunctionByAddress(uint64_t address) const;
	uint32_t current_version() const { return current_version_; }
	uint32_t compatibility_version() const { return compatibility_version_; }
	void set_library_ordinal(int library_ordinal) { library_ordinal_ = library_ordinal; }
	void set_name(const std::string &name) { name_ = name; }
	bool is_weak() const { return is_weak_; }
	void set_is_weak(bool is_weak) { is_weak_ = is_weak; }
protected:
	virtual	MacImportFunction *Add(uint64_t address, APIType type, MapFunction *map_function);
private:
	int library_ordinal_;
	bool is_weak_;
	std::string name_;
	bool is_sdk_;
    uint32_t timestamp_;
    uint32_t current_version_;
    uint32_t compatibility_version_;
};

class MacImportList : public BaseImportList
{
public:
	explicit MacImportList(MacArchitecture *owner);
	explicit MacImportList(MacArchitecture *owner, const MacImportList &src);
	virtual MacImportList *Clone(MacArchitecture *owner) const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	MacImport *item(size_t index) const;
	MacImportFunction *GetFunctionByAddress(uint64_t address) const;
	void Pack();
	MacImport *GetLibraryByOrdinal(int library_ordinal) const;
	virtual MacImport *GetImportByName(const std::string &name) const;
	void RebaseBindInfo(MacArchitecture &file, size_t delta_base);
	virtual const ImportInfo *GetSDKInfo(const std::string &name) const;
	int GetMaxLibraryOrdinal() const;
protected:
	virtual MacImport *AddSDK();
private:
	void ReadBindInfo(MacArchitecture &file, uint32_t bind_off, uint32_t bind_size);
	void ReadLazyBindInfo(MacArchitecture &file, uint32_t lazy_bind_off, uint32_t lazy_bind_size);
	size_t WriteBindInfo(MacArchitecture &file);
	size_t WriteWeakBindInfo(MacArchitecture &file);
	size_t WriteLazyBindInfo(MacArchitecture &file);
	MacImport *Add(int library_ordinal);

	struct BindInfoHelper {
		bool operator () (const MacImportFunction *left, const MacImportFunction *right) const
		{
			// sort by library, symbol, type, then address
			if (left->library_ordinal() != right->library_ordinal())
				return (left->library_ordinal() < right->library_ordinal());
			if (left->name() != right->name())
				return (left->name() < right->name());
			if (left->bind_type() != right->bind_type())
				return (left->bind_type() < right->bind_type());
			return (left->address() < right->address());
		}
	};

	struct WeakBindInfoHelper {
		bool operator () (const MacImportFunction *left, const MacImportFunction *right) const
		{
			// sort by symbol, type, address
			if (left->name() != right->name())
				return (left->name() < right->name());
			if (left->bind_type() != right->bind_type())
				return (left->bind_type() < right->bind_type());
			return  (left->address() < right->address());
		}
	};

	struct LazyBindInfoHelper {
		bool operator () (const MacImportFunction *left, const MacImportFunction *right) const
		{
			return (left->bind_offset() < right->bind_offset());
		}
	};

};

class MacIndirectSymbolList;

class MacIndirectSymbol : public IObject
{
public:
	explicit MacIndirectSymbol(MacIndirectSymbolList *owner, uint64_t address, uint32_t value, MacSymbol *symbol);
	explicit MacIndirectSymbol(MacIndirectSymbolList *owner, const MacIndirectSymbol &src);
	~MacIndirectSymbol();
	virtual MacIndirectSymbol *Clone(MacIndirectSymbolList *owner) const;
	uint64_t address() const { return address_; }
	uint32_t value() const { return value_; }
	MacSymbol *symbol() const { return symbol_; }
	void set_symbol(MacSymbol *symbol) { symbol_ = symbol; }
	void set_value(uint32_t value) { value_ = value; }
	void Rebase(uint64_t delta_base);
private:
	MacIndirectSymbolList *owner_;
	uint64_t address_;
	uint32_t value_;
	MacSymbol *symbol_;
};

class MacIndirectSymbolList : public ObjectList<MacIndirectSymbol>
{
public:
	explicit MacIndirectSymbolList();
	explicit MacIndirectSymbolList(const MacIndirectSymbolList &src);
	virtual MacIndirectSymbolList *Clone() const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	void Pack();
	MacIndirectSymbol *Add(uint64_t address, uint32_t value, MacSymbol *symbol);
	MacIndirectSymbol *GetSymbol(MacSymbol *symbol) const;
	void Rebase(uint64_t delta_base);
private:
	// no assignment op
	MacIndirectSymbolList &operator =(const MacIndirectSymbolList &);
};

class MacExtRefSymbolList;

class MacExtRefSymbol : public IObject
{
public:
	explicit MacExtRefSymbol(MacExtRefSymbolList *owner, MacSymbol *symbol, uint8_t flags);
	explicit MacExtRefSymbol(MacExtRefSymbolList *owner, const MacExtRefSymbol &src);
	~MacExtRefSymbol();
	virtual MacExtRefSymbol *Clone(MacExtRefSymbolList *owner) const;
	MacSymbol *symbol() const { return symbol_; }
	uint8_t flags() const { return flags_; }
	void set_symbol(MacSymbol *symbol) { symbol_ = symbol; }
private:
	MacExtRefSymbolList *owner_;
	MacSymbol *symbol_;
	uint8_t flags_;
};

class MacExtRefSymbolList : public ObjectList<MacExtRefSymbol>
{
public:
	explicit MacExtRefSymbolList();
	explicit MacExtRefSymbolList(const MacExtRefSymbolList &src);
	virtual MacExtRefSymbolList *Clone() const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	void Pack();
	MacExtRefSymbol *Add(MacSymbol *symbol, uint8_t flags);
	MacExtRefSymbol *GetSymbol(MacSymbol *symbol) const;
private:
	// no assignment op
	MacExtRefSymbolList &operator =(const MacExtRefSymbolList &);
};

class MacExport : public BaseExport
{
public:
	explicit MacExport(IExportList *parent, uint64_t address, const std::string &name, uint64_t flags, uint64_t other);
	explicit MacExport(IExportList *parent, MacSymbol *symbol);
	explicit MacExport(IExportList *parent, const MacExport &src);
	virtual MacExport *Clone(IExportList *parent) const;
	virtual uint64_t address() const { return address_; }
	void set_address(uint64_t address);
	virtual std::string name() const { return name_; }
	virtual std::string forwarded_name() const { return forwarded_name_; }
	uint64_t flags() const { return flags_; }
	uint64_t other() const { return other_; }
	void set_forwarded_name(const std::string &forwarded_name) { forwarded_name_ = forwarded_name; }
	virtual std::string display_name(bool show_ret = true) const;
	MacSymbol *symbol() const { return symbol_; }
	void set_symbol(MacSymbol *symbol) { symbol_ = symbol; }
	virtual void Rebase(uint64_t delta_base);
private:
	MacSymbol *symbol_;
	uint64_t address_;
	std::string name_;
	std::string forwarded_name_;
	uint64_t flags_;
	uint64_t other_;
};

class MacExportList : public BaseExportList
{
public:
	explicit MacExportList(MacArchitecture *owner);
	explicit MacExportList(MacArchitecture *owner, const MacExportList &src);
	virtual MacExportList *Clone(MacArchitecture *owner) const;
	virtual std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	MacExport *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file);
	void Pack();
	void WriteToFile(MacArchitecture &file);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	MacExport *GetExportByAddress(uint64_t address) const;
protected:
	virtual MacExport *Add(uint64_t address) { return Add(address, std::string(), 0, 0); }
private:
	MacExport *Add(uint64_t address, const std::string &name, uint64_t flags, uint64_t other);
	MacExport *Add(MacSymbol *symbol);
	void ParseExportNode(const EncodedData &buf, size_t pos, const std::string &name, uint64_t base_address);

	std::string name_;
};

class MacExportNode : public ObjectList<MacExportNode>
{
public:
	explicit MacExportNode(MacExportNode *owner = NULL);
	explicit MacExportNode(MacExportNode *owner, MacExport *symbol, const std::string &cummulative_string_);
	~MacExportNode();
	void set_owner(MacExportNode *owner);
	void AddSymbol(MacExport *symbol);
	std::string cummulative_string() const { return cummulative_string_; }
	void WriteToData(EncodedData &data, uint64_t base_address);
	uint32_t offset() const { return offset_; } 
private:
	MacExportNode *Add(MacExport *symbol, const std::string &cummulative_string_);

	MacExportNode *owner_;
	MacExport *symbol_;
	std::string cummulative_string_;
	uint32_t offset_;
};

class MacFixup : public BaseFixup
{
public:
	explicit MacFixup(MacFixupList *owner, uint64_t address, uint32_t data, OperandSize size, bool is_relocation);
	explicit MacFixup(MacFixupList *owner, const MacFixup &src);
	virtual MacFixup *Clone(IFixupList *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual FixupType type() const;
	uint8_t internal_type() const;
	virtual OperandSize size() const { return size_; }
	virtual void set_address(uint64_t address) { address_ = address; }
	uint8_t bind_type() const { return static_cast<uint8_t>(data_); }
	relocation_info relocation() const;
	bool is_relocation() const { return is_relocation_; }
	void set_is_relocation(bool is_relocation);
	MacSymbol *symbol() const { return symbol_; }
	void set_symbol(MacSymbol *symbol) { symbol_ = symbol; }
	virtual void Rebase(IArchitecture &file, uint64_t delta_base);
private:
	uint64_t address_;
	MacSymbol *symbol_;
	uint32_t data_;
	OperandSize size_;
	bool is_relocation_;
};

class MacFixupList : public BaseFixupList
{
public:
	explicit MacFixupList();
	explicit MacFixupList(const MacFixupList &src);
	virtual MacFixupList *Clone() const;
	MacFixup *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file);
	void WriteToFile(MacArchitecture &file);
	virtual MacFixup *AddDefault(OperandSize cpu_address_size, bool is_code);
	virtual size_t Pack();
	void WriteToData(Data &data, uint64_t image_base);
	MacFixup *AddRelocation(uint64_t address, MacSymbol *symbol, OperandSize size);
private:
	MacFixup *Add(uint64_t address, uint32_t data, OperandSize size, bool is_relocation);
	void ReadRebaseInfo(MacArchitecture &file, uint32_t rebase_off, uint32_t rebase_size);
	void ReadRelocations(MacArchitecture &file, uint32_t offset, uint32_t count, bool need_external);
	size_t WriteRebaseInfo(MacArchitecture &file);
	size_t WriteRelocations(MacArchitecture &file, bool need_external);

	// no assignment op
	MacFixupList &operator =(const MacFixupList &);

	struct RebaseInfoHelper {
		bool operator () (const MacFixup *left, const MacFixup *right) const;
	};

	struct RebaseInfo {
		uint8_t opcode;
		uint64_t operand1;
		uint64_t operand2;
		RebaseInfo(uint8_t opcode_, uint64_t operand1_, uint64_t operand2_ = 0)
		{
			opcode = opcode_;
			operand1 = operand1_;
			operand2 = operand2_;
		}
	};
};

class MacRuntimeFunction : public BaseRuntimeFunction
{
public:
	explicit MacRuntimeFunction(MacRuntimeFunctionList *owner, uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, CommonInformationEntry *cie,
		const std::vector<uint8_t> &call_frame_instructions, uint32_t compact_encoding);
	explicit MacRuntimeFunction(MacRuntimeFunctionList *owner, const MacRuntimeFunction &src);
	virtual MacRuntimeFunction *Clone(IRuntimeFunctionList *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual uint64_t begin() const { return begin_; }
	virtual uint64_t end() const { return end_; }
	virtual uint64_t unwind_address() const { return unwind_address_; }
	uint32_t compact_encoding() const { return compact_encoding_; }
	virtual void set_begin(uint64_t begin) { begin_ = begin; }
	virtual void set_end(uint64_t end) { end_ = end; }
	virtual void set_unwind_address(uint64_t unwind_address) { unwind_address_ = unwind_address; }
	CommonInformationEntry *cie() const { return cie_; }
	void set_cie(CommonInformationEntry *cie) { cie_ = cie; }
	std::vector<uint8_t> call_frame_instructions() const { return call_frame_instructions_; }
	virtual void Parse(IArchitecture &file, IFunction &dest);
	void Rebase(uint64_t delta_base);
private:
	void ParseBorland(IArchitecture &file, IFunction &func);
	void ParseDwarf(IArchitecture &file, IFunction &func);

	uint64_t address_;
	uint64_t begin_;
	uint64_t end_;
	uint64_t unwind_address_;
	uint32_t compact_encoding_;
	CommonInformationEntry *cie_;
	std::vector<uint8_t> call_frame_instructions_;
};

class MacRuntimeFunctionList : public BaseRuntimeFunctionList
{
public:
	explicit MacRuntimeFunctionList();
	explicit MacRuntimeFunctionList(const MacRuntimeFunctionList &src);
	~MacRuntimeFunctionList();
	virtual void clear();
	MacRuntimeFunctionList *Clone() const;
	MacRuntimeFunction *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file);
	size_t WriteToFile(MacArchitecture &file, bool compact_info);
	void Rebase(uint64_t delta_base);
	virtual MacRuntimeFunction *Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions);
	virtual MacRuntimeFunction *GetFunctionByAddress(uint64_t address) const;
	CommonInformationEntryList *cie_list() const { return cie_list_; }
private:
	void ReadBorlandInfo(MacArchitecture &file, uint64_t address);
	void ReadCompactInfo(MacArchitecture &file, uint64_t address, uint32_t size);
	void ReadDwarfInfo(MacArchitecture &file, uint64_t address, uint32_t size);
	MacRuntimeFunction *Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, CommonInformationEntry *cie, const std::vector<uint8_t> &call_frame_instructions, uint32_t compact_encoding);

	size_t WriteDwarfInfo(MacArchitecture &file);
	size_t WriteCompactInfo(MacArchitecture &file);
	uint64_t address_;
	CommonInformationEntryList *cie_list_;

	// no assignment op
	MacRuntimeFunctionList &operator =(const MacRuntimeFunctionList &);
};

class MacArchitecture : public BaseArchitecture
{
public:
	explicit MacArchitecture(MacFile *owner, uint64_t offset, uint64_t size);
	explicit MacArchitecture(MacFile *owner, const MacArchitecture &src);
	virtual ~MacArchitecture();
	virtual std::string name() const;
	virtual uint32_t type() const { return cpu_type_; }
	uint32_t cpu_subtype() const { return cpu_subtype_; }
	virtual OperandSize cpu_address_size() const { return cpu_address_size_; }
	uint32_t flags() const { return flags_; }
	virtual uint64_t entry_point() const;
	virtual uint64_t image_base() const { return image_base_; }
	uint32_t segment_alignment() const { return 0x1000; }
	uint32_t file_alignment() const { return 0x1000; }
	virtual MacLoadCommandList *command_list() const { return command_list_; }
	virtual MacSegmentList *segment_list() const { return segment_list_; }
	virtual MacImportList *import_list() const { return import_list_; }
	virtual MacExportList *export_list() const { return export_list_; }
	virtual MacFixupList *fixup_list() const { return fixup_list_; }
	virtual IRelocationList *relocation_list() const { return NULL; }
	virtual IResourceList *resource_list() const { return NULL; }
	virtual ISEHandlerList *seh_handler_list() const { return NULL; }
	virtual MacRuntimeFunctionList *runtime_function_list() const { return runtime_function_list_; }
	virtual IFunctionList *function_list() const { return function_list_; }
	virtual IVirtualMachineList *virtual_machine_list() const { return virtual_machine_list_; }
	virtual MacSectionList *section_list() const { return section_list_; }
	MacSymbolList *symbol_list() const { return symbol_list_; }
	MacIndirectSymbolList *indirect_symbol_list() const { return indirect_symbol_list_; }
	OpenStatus ReadFromFile(uint32_t mode);
	virtual bool WriteToFile();
	virtual MacArchitecture *Clone(IFile *file) const;
	virtual bool Compile(CompileOptions &options, IArchitecture *runtime);
	virtual void Save(CompileContext &ctx);
	virtual bool is_executable() const;
	MacStringTable *string_table() { return &string_table_; }
	symtab_command *symtab() { return &symtab_; }
	dysymtab_command *dysymtab() { return &dysymtab_; }
	dyld_info_command *dyld_info() { return &dyld_info_; }
	uint64_t GetRelocBase() const;
	virtual CallingConvention calling_convention() const { return cpu_address_size() == osDWord ? ccCdecl : ccABIx64; }
	void Rebase(uint64_t delta_base, size_t delta_bind_info);
	MacSegment *header_segment() const { return header_segment_; }
	uint32_t max_header_size() const { return max_header_size_; }
	uint32_t file_type() const { return file_type_; }
	MacSection *runtime_functions_section() const { return runtime_functions_section_; }
	MacSection *unwind_info_section() const { return unwind_info_section_; }
protected:
	virtual bool Prepare(CompileContext &ctx);
private:
	MacLoadCommandList *command_list_;
	MacSegmentList *segment_list_;
	MacSectionList *section_list_;
	MacSymbolList *symbol_list_;
	MacImportList *import_list_;
	MacExportList *export_list_;
	MacIndirectSymbolList *indirect_symbol_list_;
	MacExtRefSymbolList *ext_ref_symbol_list_;
	MacFixupList *fixup_list_;
	MacRuntimeFunctionList *runtime_function_list_;
	IFunctionList *function_list_;
	IVirtualMachineList *virtual_machine_list_;

	cpu_type_t cpu_type_;
	cpu_subtype_t cpu_subtype_;
	OperandSize cpu_address_size_;
	uint64_t image_base_;
	symtab_command symtab_;
	dysymtab_command dysymtab_;
	dyld_info_command dyld_info_;
	MacStringTable string_table_;
	uint64_t entry_point_;
	uint32_t file_type_;
	uint32_t cmds_size_;
	uint32_t flags_;
	uint32_t header_size_;
	uint32_t segment_alignment_;
	uint32_t file_alignment_;
	uint32_t sdk_;
	MacSegment *linkedit_segment_;
	size_t optimized_segment_count_;
	MacSegment *header_segment_;
	uint32_t max_header_size_;
	MacSection *runtime_functions_section_;
	MacSection *unwind_info_section_;
	
	// no copy ctr or assignment op
	MacArchitecture(const MacArchitecture &);
	MacArchitecture &operator =(const MacArchitecture &);
};

class MacFile : public IFile
{
public:
	explicit MacFile(ILog *log);
	explicit MacFile(const MacFile &src, const char *file_name);
	virtual ~MacFile();
	virtual std::string format_name() const;
	MacArchitecture *item(size_t index) const;
	MacFile *Clone(const char *file_name) const;
	virtual bool Compile(CompileOptions &options);
	virtual bool is_executable() const;
	virtual uint32_t disable_options() const;
protected:
	virtual OpenStatus ReadHeader(uint32_t open_mode);
	virtual IFile *runtime() const { return runtime_; }
private:
	MacArchitecture *Add(uint64_t offset, uint64_t size);

	uint32_t fat_magic_;
	MacFile *runtime_;

	// no copy ctr or assignment op
	MacFile(const MacFile &);
	MacFile &operator =(const MacFile &);
};

#endif