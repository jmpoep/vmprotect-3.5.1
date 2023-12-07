#ifndef PROCESSORS_H
#define PROCESSORS_H

#include "osutils.h"

class CommandLink;
class Buffer;
class CommandBlock;
class IVirtualMachine;
class SignatureList;
class ICommand;
class IFunction;
class Folder;
class MemoryManager;
class CommandInfoList;
class FunctionInfoList;
class FunctionInfo;

enum OperandType : uint16_t {
	otNone = 0x0000,
	otValue = 0x0001,
	otRegistr = 0x0002,
	otMemory = 0x0004,
	otSegmentRegistr = 0x0008,
	otControlRegistr = 0x0010,
	otDebugRegistr = 0x0020,
	otFPURegistr = 0x0040,
	otHiPartRegistr = 0x0080,
	otBaseRegistr = 0x0100,
	otMMXRegistr = 0x0200,
	otXMMRegistr = 0x0400,
	// otAbsolute = 0x0800,
};

enum CommandOption {
	roInverseFlag = 0x0001,
	roLockPrefix = 0x0002,
	roFar = 0x0004,
	roVexPrefix = 0x0008,
	roBreaked = 0x0010,
	roClearOriginalCode = 0x0020,
	roNeedCompile = 0x0040,
	roCreateNewBlock = 0x0080,
	roFillNop = 0x0100,
	roInternal = 0x0200,
	roNoNative = 0x0400,
	roNoSaveFlags = 0x0800,
	roWritable = 0x1000,
	roUseAsJmp = 0x2000,
	roNoProgress = 0x4000,
	roExternal = 0x8000,
	roNeedCRC = 0x10000,
	roInvalidOpcode = 0x20000,
	roDataSegment = 0x40000,
	roImportSegment = 0x80000,
};

enum VMRegistr {
	regEFX = 16,
	regETX,
	regERX,
	regEIX,
	regEmpty,
	regExtended = 0x80
};

typedef uint32_t CommandType;

enum LinkType {
	ltNone,
	ltSEHBlock,
	ltFinallyBlock,
	ltDualSEHBlock,
	ltFilterSEHBlock,
	ltJmp,
	ltJmpWithFlag,
	ltJmpWithFlagNSFS,
	ltJmpWithFlagNSNA,
	ltJmpWithFlagNSNS,
	ltCall,
	ltCase,
	ltSwitch,
	ltNative,
	ltOffset,
	ltGateOffset,
	ltExtSEHBlock,
	ltMemSEHBlock,
	ltExtSEHHandler,
	ltVBMemSEHBlock,
	ltDelta
};

enum SectionOption {
	rtNone = 0x0000,
	rtLinkedToInt = 0x0001,
	rtLinkedToExt = 0x0002,
	rtLinkedFrom = 0x0004,
	rtLinkedNext = 0x0008,
    rtBeginSection = 0x0010,
	rtEndSection = 0x0020,
	rtCloseSection = 0x0040,
    rtNoInverseResult = 0x0080,
	rtInverseResult = 0x0100,
	rtNoSaveFlags = 0x0200,
	rtInverseWrite = 0x0400,
	rtLinkedFromOtherType = 0x0800,
	rtBackwardDirection = 0x1000
};

class IVMCommand: public IObject
{
public:
	virtual void WriteToFile(IArchitecture &file) = 0;
	virtual void Compile() = 0;
	virtual size_t dump_size() const = 0;
	virtual void set_address(uint64_t address) = 0;
	virtual uint64_t address() const = 0;
	virtual ICommand *owner() const = 0;
	virtual bool is_end() const = 0;
};

enum VMCommandOption {
	voNone = 0x0000,
	voLinkCommand = 0x0001,
	voFixup = 0x0002,
	voSectionCommand = 0x0004,
	voInverseValue = 0x0008,
	voUseBeginSectionCryptor = 0x0010,
	voUseEndSectionCryptor = 0x0020,
	voBeginOffset = 0x0040,
	voEndOffset = 0x0080,
	voInitOffset = 0x0100,
	voNoCRC = 0x0200,
	voNoCryptValue = 0x0400
};

class BaseVMCommand: public IVMCommand
{
public:
	explicit BaseVMCommand(ICommand *owner);
	~BaseVMCommand();
	virtual ICommand *owner() const { return owner_; }
	void set_owner(ICommand *owner) { owner_ = owner; }
private:
	ICommand *owner_;
};

enum CommentType : uint8_t {
	ttUnknown,
	ttNone,
	ttJmp,
	ttFunction,
	ttImport,
	ttString,
	ttVariable,
	ttComment,
	ttExport,
	ttMarker
};

struct CommentInfo {
	std::string value;
	CommentType type;
	CommentInfo() : type(ttUnknown) {}
	CommentInfo(CommentType type_, const std::string &value_) : type(type_), value(value_) {}
	std::string display_value() const 
	{
		std::string res;
		if (value.empty())
			return std::string();
		return (value[0] < 5) ? value[0] + DisplayString(value.substr(1)) : DisplayString(value);
	}
};

class AddressRange;
class ISEHandler;
struct CompileContext;

class ICommand: public ObjectList<IVMCommand>
{
public:
	virtual uint64_t address() const = 0;
	virtual uint64_t next_address() const = 0;
	virtual CommandType type() const = 0;
	virtual std::string text() const = 0;
	virtual CommentInfo comment() = 0;
	virtual void set_comment(const CommentInfo &value) = 0;
	virtual uint32_t options() const = 0;
	virtual CommandLink *link() const = 0;
	virtual void set_link(CommandLink *link) = 0;
	virtual uint8_t dump(size_t index) const = 0;
	virtual size_t dump_size() const = 0;
	virtual std::string dump_str() const = 0;
	virtual size_t original_dump_size() const = 0;
	virtual size_t vm_dump_size() const = 0;
	virtual void clear() = 0;
	virtual void CompileToNative() = 0;
	virtual void CompileLink(const CompileContext &ctx) = 0;
	virtual void PrepareLink(const CompileContext &ctx) = 0;
	virtual void CompileInfo() = 0;
	virtual void set_operand_value(size_t operand_index, uint64_t value) = 0;
	virtual void set_link_value(size_t link_index, uint64_t value) = 0;
	virtual void set_jmp_value(size_t link_index, uint64_t value) = 0;
	virtual void set_address(uint64_t address) = 0;
	virtual uint64_t vm_address() const = 0;
	virtual uint64_t ext_vm_address() const = 0;
	virtual void set_vm_address(uint64_t address) = 0;
	virtual void WriteToFile(IArchitecture &file) = 0;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file) = 0;
	virtual size_t alignment() const = 0;
	virtual CommandBlock *block() const = 0;
	virtual void set_block(CommandBlock *block) = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual IFunction *owner() const = 0;
	virtual ICommand *Clone(IFunction *owner) const = 0;
	virtual CommandLink *AddLink(int operand_index, LinkType type, ICommand *to_command) = 0;
	virtual CommandLink *AddLink(int operand_index, LinkType type, uint64_t to_address = 0) = 0;
	virtual void include_section_option(SectionOption option) = 0;
	virtual uint32_t section_options() const = 0;
	virtual ISEHandler *seh_handler() const = 0;
	virtual AddressRange *address_range() const = 0;
	virtual void set_address_range(AddressRange *address_range) = 0;
	virtual bool Merge(ICommand *command) = 0;
	virtual bool is_data() const = 0;
	virtual bool is_end() const = 0;
	virtual void include_option(CommandOption value) = 0;
	virtual void exclude_option(CommandOption value) = 0;
	virtual std::string display_address() const = 0;
	virtual void set_tag(uint8_t tag) = 0;
	virtual uint8_t tag() const = 0;

	using IObject::CompareWith;
	int CompareWith(const ICommand &obj) const
	{
		if (address() < obj.address())
			return -1;
		if (address() > obj.address())
			return 1;
		return 0;
	}
#ifdef CHECKED
	virtual bool check_hash() const = 0;
#endif
};

enum AccessType : uint8_t {
	atRead,
	atWrite
};

class CommandInfo : public IObject
{
public:
	explicit CommandInfo(CommandInfoList *owner, AccessType type, uint8_t value, OperandType operand_type, OperandSize size);
	~CommandInfo();
	AccessType type() const { return type_; }
	OperandType operand_type() const { return operand_type_; }
	uint8_t value() const { return value_; }
	OperandSize size() const { return size_; }
	void set_size(OperandSize size) { size_ = size; }
private:
	CommandInfoList *owner_;
	OperandType operand_type_;
	AccessType type_;
	uint8_t value_;
	OperandSize size_;
};

class CommandInfoList: public ObjectList<CommandInfo>
{
public:
	explicit CommandInfoList();
	virtual void Add(AccessType access_type, uint8_t value, OperandType operand_type, OperandSize size);
	void set_need_flags(uint16_t flags) { need_flags_ = flags; }
	void set_change_flags(uint16_t flags) { change_flags_ = flags; }
	CommandInfo *GetInfo(AccessType type, OperandType operand_type, uint8_t value) const;
	CommandInfo *GetInfo(AccessType type, OperandType operand_type) const;
	CommandInfo *GetInfo(OperandType operand_type) const;
	uint16_t need_flags() const { return need_flags_; }
	uint16_t change_flags() const { return change_flags_; }
	virtual void clear();
private:
	uint16_t need_flags_;
	uint16_t change_flags_;
};

class BaseCommand: public ICommand
{
public:
	explicit BaseCommand(IFunction *owner);
	explicit BaseCommand(IFunction *owner, const std::string &value);
	explicit BaseCommand(IFunction *owner, const os::unicode_string &value);
	explicit BaseCommand(IFunction *owner, const Data &value);
	explicit BaseCommand(IFunction *owner, const BaseCommand &src);
	~BaseCommand();
	virtual uint64_t next_address() const { return address() + dump_size(); }
	virtual uint8_t dump(size_t index) const { return dump_[index]; }
	uint64_t dump_value(size_t pos, OperandSize size) const;
	Data &raw_dump(void) { return dump_; };
	virtual size_t dump_size() const { return dump_.size(); }
	virtual std::string dump_str() const;
	virtual size_t vm_dump_size() const;
	virtual void clear();
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void WriteToFile(IArchitecture &file);
	virtual CommandLink *link() const { return link_; }
	virtual void set_link(CommandLink *link) { link_ = link; }
	virtual CommandBlock *block() const { return block_; }
	virtual void set_block(CommandBlock *block) { block_ = block; }
	virtual IFunction *owner() const { return owner_; }
	virtual CommandLink *AddLink(int operand_index, LinkType type, ICommand *to_command);
	virtual CommandLink *AddLink(int operand_index, LinkType type, uint64_t to_address = 0);
	virtual uint64_t vm_address() const { return vm_address_; }
	virtual uint64_t ext_vm_address() const { return vm_address_; }
	virtual void set_vm_address(uint64_t address);
	bool CompareDump(const uint8_t *buffer, size_t size) const;
	void set_dump(const void *buffer, size_t size);
	virtual ISEHandler *seh_handler() const { return NULL; }
	virtual CommentInfo comment() { return comment_; }
	virtual void set_comment(const CommentInfo &comment) { comment_ = comment; }
	virtual AddressRange *address_range() const { return address_range_; }
	virtual void set_address_range(AddressRange *address_range) { address_range_ = address_range; }
	virtual void CompileInfo();
	virtual size_t alignment() const { return alignment_; }
	void set_alignment(size_t alignment) { alignment_ = alignment; }
	virtual uint32_t options() const { return options_; }
	virtual void include_option(CommandOption value) { options_ |= value; }
	virtual void exclude_option(CommandOption value) { options_ &= ~value; }
#ifdef CHECKED
	virtual bool check_hash() const { return true; }
#endif
	virtual void set_tag(uint8_t tag) { tag_ = tag; }
	virtual uint8_t tag() const { return tag_; }
protected:
	void Read(IArchitecture &file, size_t len);
	uint8_t ReadByte(IArchitecture &file);
	uint16_t ReadWord(IArchitecture &file);
	uint32_t ReadDWord(IArchitecture &file);
	uint64_t ReadQWord(IArchitecture &file);

	void PushByte(uint8_t value);
	void PushWord(uint16_t value);
	void PushDWord(uint32_t value);
	void PushQWord(uint64_t value);
	void InsertByte(size_t position, uint8_t value);
	void WriteDWord(size_t position, uint32_t value);
	std::string comment_text() const { return comment_.value; }
private:
	Data dump_;
	IFunction *owner_;
	CommandLink *link_;
	CommandBlock *block_;
	uint64_t vm_address_;
	AddressRange *address_range_;
	CommentInfo comment_;
	size_t alignment_;
	uint32_t options_;
	uint8_t tag_;
};

class InternalLinkList;

enum InternalLinkType {
	vlNone,
	vlCRCTableAddress,
	vlCRCTableCount,
	vlCRCValue
};

class InternalLink : public IObject
{
public:
	InternalLink(InternalLinkList *owner, InternalLinkType type, IVMCommand *from_command, IObject *to_command);
	~InternalLink();
	InternalLinkType type() const { return type_; }
	IVMCommand *from_command() const { return from_command_; }
	IObject *to_command() const { return to_command_; }
private:
	InternalLinkList *owner_;
	InternalLinkType type_;
	IVMCommand *from_command_;
	IObject *to_command_;
};

class InternalLinkList : public ObjectList<InternalLink>
{
public:
	InternalLink *Add(InternalLinkType type, IVMCommand *from_command, IObject *to_command);
};

enum CryptCommandType : uint8_t {
	ccAdd,
	ccSub,
	ccXor,
	ccInc,
	ccDec,
	ccBswap,
	ccRol,
	ccRor,
	ccNot,
	ccNeg,
	ccUnknown
};

class ValueCryptor;

class ValueCommand: public IObject
{
public:
	explicit ValueCommand(ValueCryptor *owner, OperandSize size, CryptCommandType type, uint64_t value);
	explicit ValueCommand(ValueCryptor *owner, const ValueCommand &src);
	~ValueCommand();
	ValueCommand *Clone(ValueCryptor *owner) const;
	CryptCommandType type(bool is_decrypt = false) const;
	uint64_t Encrypt(uint64_t value);
	uint64_t Decrypt(uint64_t value);
	uint64_t value() const { return value_; }
	OperandSize size() const { return size_; }
private:
	uint64_t value_;
	ValueCryptor *owner_;
	uint64_t Calc(uint64_t value, bool is_decrypt);
	CryptCommandType type_;
	OperandSize size_;
};

class ValueCryptor : public ObjectList<ValueCommand>
{
public:
	ValueCryptor();
	ValueCryptor(const ValueCryptor &src);
	ValueCryptor *Clone() const;
	virtual void Init(OperandSize size);
	uint64_t Encrypt(uint64_t value);
	uint64_t Decrypt(uint64_t value);
	OperandSize size() const { return size_; }
	void set_size(OperandSize size) { size_ = size; }
	void Add(CryptCommandType command, uint64_t value);
private:
	OperandSize size_;
	// no assignment op
	ValueCryptor &operator =(const ValueCryptor &);
};

class OpcodeCryptor : public ValueCryptor
{
public:
	OpcodeCryptor();
	virtual void Init(OperandSize size);
	uint64_t EncryptOpcode(uint64_t value1, uint64_t value2);
	uint64_t DecryptOpcode(uint64_t value1, uint64_t value2);
	CryptCommandType type() const { return type_; }
private:
	uint64_t Calc(uint64_t value1, uint64_t value2, bool is_decrypt);
	CryptCommandType type_;
};

class CommandLinkList;
class FunctionInfo;

class CommandLink: public IObject
{
public:
	explicit CommandLink(CommandLinkList *owner, ICommand *from_command, int operand_index, LinkType type, uint64_t to_address);
	explicit CommandLink(CommandLinkList *owner, ICommand *from_command, int operand_index, LinkType type, ICommand *to_command);
	explicit CommandLink(CommandLinkList *owner, const CommandLink &src);
	virtual ~CommandLink();
	CommandLink *Clone(CommandLinkList *owner) const;
	ICommand *from_command() const { return from_command_; }
	ICommand *parent_command() const { return parent_command_; }
	ICommand *to_command() const { return to_command_; }
	ICommand *next_command() const { return next_command_; }
	uint64_t to_address() const { return to_address_; }
	LinkType type() const { return type_; }
	bool parsed() const { return parsed_; }
	int operand_index() const { return operand_index_; }
	void set_operand_index(int value) { operand_index_ = value; }
	void set_parsed(bool parsed) { parsed_ = parsed; }
	void set_type(LinkType type) { type_ = type; }
	void set_parent_command(ICommand *parent_command) { parent_command_ = parent_command; }
	void set_from_command(ICommand *command);
	void set_to_command(ICommand *command) { to_command_ = command; }
	void set_next_command(ICommand *command) { next_command_ = command; }
	void set_sub_value(uint64_t value) { sub_value_ = value; }
	uint64_t sub_value() const { return sub_value_; }
	ICommand *gate_command(size_t index) const { return gate_commands_[index]; }
	void Rebase(uint64_t delta_base);
	void AddGateCommand(ICommand *command) { gate_commands_.push_back(command); }
	uint64_t Encrypt(uint64_t value) const;
	ValueCryptor *cryptor() const { return cryptor_; }
	void set_cryptor(ValueCryptor *cryptor);
	FunctionInfo *base_function_info() const { return base_function_info_; }
	void set_base_function_info(FunctionInfo *base_function_info) { base_function_info_ = base_function_info; }
	void set_is_inverse(bool is_inverse) { is_inverse_ = is_inverse; }
private:
	CommandLinkList *owner_;
	bool parsed_;
	ICommand *from_command_;
	ICommand *parent_command_;
	ICommand *to_command_;
	ICommand *next_command_;
	LinkType type_;
	uint64_t to_address_;
	int operand_index_;
	uint64_t sub_value_;
	std::vector<ICommand *> gate_commands_;
	ValueCryptor *cryptor_;
	FunctionInfo *base_function_info_;
	bool is_inverse_;
	
	// no copy ctr or assignment op
	CommandLink(const CommandLink &);
	CommandLink &operator =(const CommandLink &);
};

class CommandLinkList : public ObjectList<CommandLink>
{
public:
	explicit CommandLinkList();
	explicit CommandLinkList(const CommandLinkList &src);
	virtual CommandLinkList *Clone() const;
	CommandLink *Add(ICommand *from_command, int operand_index, LinkType type, uint64_t to_address = 0);
	CommandLink *Add(ICommand *from_command, int operand_index, LinkType type, ICommand *to_command);
	CommandLink *GetLinkByToAddress(LinkType type, uint64_t to_address);
	void Rebase(uint64_t delta_base);
private:
	// no assignment op
	CommandLinkList &operator =(const CommandLinkList &);
};

class ExtCommandList;

class ExtCommand: public IObject
{
public:
	explicit ExtCommand(ExtCommandList *owner, uint64_t address, ICommand *command, bool use_call);
	explicit ExtCommand(ExtCommandList *owner, const ExtCommand &src);
	virtual ~ExtCommand();
	virtual ExtCommand *Clone(ExtCommandList *owner) const;

	using IObject::CompareWith;
	int CompareWith(const ExtCommand &obj) const;
	uint64_t address() const { return address_; }
	ICommand *command() const { return command_; }
	bool use_call() const { return use_call_; }
	void set_command(ICommand *command) { command_ = command; }
	ExtCommandList *owner() const { return owner_; }
private:
	ExtCommandList *owner_;
	uint64_t address_;
	ICommand *command_;
	bool use_call_;
};

class ExtCommandList : public ObjectList<ExtCommand>
{
public:
	explicit ExtCommandList(IFunction *owner);
	explicit ExtCommandList(IFunction *owner, const ExtCommandList &src);
	virtual ExtCommandList *Clone(IFunction *owner) const;
	ExtCommand *Add(uint64_t address);
	ExtCommand *Add(uint64_t address, ICommand *command, bool use_call = false);
	ExtCommand *GetCommandByAddress(uint64_t address) const;
	virtual void AddObject(ExtCommand *ext_command);
	virtual void RemoveObject(ExtCommand *ext_command);
	IFunction *owner() const { return owner_; }
private:
	IFunction *owner_;
};

class CommandBlockList;

class CommandBlock: public AddressableObject
{
public:
	explicit CommandBlock(CommandBlockList *owner, uint32_t type, size_t start_index);
	explicit CommandBlock(CommandBlockList *owner, const CommandBlock &src);
	~CommandBlock();
	CommandBlock *Clone(CommandBlockList *owner);
	size_t start_index() const { return start_index_; }
	size_t end_index() const { return end_index_; }
	uint32_t type() const { return type_; }
	void set_start_index(size_t start_index) { start_index_ = start_index; }
	void set_end_index(size_t end_index) { end_index_ = end_index; }
	void Compile(MemoryManager &manager);
	void CompileLinks(const CompileContext &ctx);
	void CompileInfo();
	size_t WriteToFile(IArchitecture &file);
	uint8_t GetRegistr(OperandSize size, uint8_t registr, bool is_write);
	void AddCorrectCommand(IVMCommand *command) { correct_command_list_.push_back(command); }
	std::vector<IVMCommand *> correct_command_list() const { return correct_command_list_; }
	IFunction *function() const;
	IVirtualMachine *virtual_machine() const { return virtual_machine_; }
	void set_virtual_machine(IVirtualMachine *value) { virtual_machine_ = value; }
	size_t sort_index() const { return sort_index_; }
	void set_sort_index(size_t sort_index) { sort_index_ = sort_index; }
	CommandBlockList *owner() { return owner_; };
private:
	CommandBlockList *owner_;
	uint32_t type_;
	size_t start_index_;
	size_t end_index_;
	uint8_t registr_indexes_[24];
	size_t registr_count_;
	std::vector<IVMCommand *> correct_command_list_;
	IVirtualMachine *virtual_machine_;
	size_t sort_index_;
};

class CommandBlockList : public ObjectList<CommandBlock>
{
public:
	explicit CommandBlockList(IFunction *owner);
	explicit CommandBlockList(IFunction *owner, const CommandBlockList &src);
	CommandBlockList *Clone(IFunction *owner) const;
	CommandBlock *Add(uint32_t memory_type, size_t start_index);
	void CompileBlocks(MemoryManager &manager);
	void CompileLinks(const CompileContext &ctx);
	void CompileInfo();
	size_t WriteToFile(IArchitecture &file);
	IFunction *owner() const { return owner_; }
private:
	IFunction *owner_;
};

enum EntryType : uint8_t {
	etDefault,
	etRandomAddress,
	etNone
};

enum CompilationOption {
	coLockToKey = 0x2000
};

enum FunctionTag {
	ftNone,
	ftLicensing,
	ftBundler,
	ftRegistry,
	ftResources,
	ftLoader,
	ftProcessor
};

enum CommandTag {
	cmdtNone,
	cmdtMutant
};

class AddressRange : public IObject
{
public:
	explicit AddressRange(FunctionInfo *owner, uint64_t begin, uint64_t end, ICommand *begin_entry, ICommand *end_entry, ICommand *size_entry);
	explicit AddressRange(FunctionInfo *owner, const AddressRange &src);
	~AddressRange();
	uint64_t begin() const { return begin_; }
	uint64_t end() const { return end_; }
	uint64_t original_begin() const { return original_begin_ ? original_begin_ : begin_; }
	uint64_t original_end() const { return original_end_ ? original_end_ : end_; }
	ICommand *begin_entry() const { return begin_entry_; }
	ICommand *end_entry() const { return end_entry_; }
	ICommand *size_entry() const { return size_entry_; }
	void set_begin_entry(ICommand *begin_entry) { begin_entry_ = begin_entry; }
	void set_end_entry(ICommand *end_entry) { end_entry_ = end_entry; }
	void set_size_entry(ICommand *size_entry) { size_entry_ = size_entry; }
	AddressRange *Clone(FunctionInfo *owner) const;
	void Add(uint64_t address, size_t size);
	void Prepare();
	void Rebase(uint64_t delta_base);
	FunctionInfo *owner() const { return owner_; }
	void set_begin(uint64_t value) { begin_ = value; }
	void set_end(uint64_t value) { end_ = value; }
	FunctionInfo *link_info() const { return link_info_; }
	void set_link_info(FunctionInfo *info) { link_info_ = info; }
	void AddLink(AddressRange *link) { link_list_.push_back(link); }
private:
	FunctionInfo *owner_;
	uint64_t begin_;
	uint64_t end_;
	uint64_t original_begin_;
	uint64_t original_end_;
	ICommand *begin_entry_;
	ICommand *end_entry_;
	ICommand *size_entry_;
	FunctionInfo *link_info_;
	std::vector<AddressRange *> link_list_;
};

enum AddressBaseType {
	btValue,
	btImageBase,
	btFunctionBegin
};

class FunctionInfo : public ObjectList<AddressRange>
{
public:
	explicit FunctionInfo();
	explicit FunctionInfo(FunctionInfoList *owner, uint64_t begin, uint64_t end, AddressBaseType base_type, uint64_t base_value, size_t prolog_size, 
		uint8_t frame_registr, IRuntimeFunction *source, ICommand *entry);
	explicit FunctionInfo(FunctionInfoList *owner, const FunctionInfo &src);
	~FunctionInfo();
	AddressRange *Add(uint64_t begin, uint64_t end, ICommand *begin_entry, ICommand *end_entry, ICommand *size_entry);
	AddressRange *GetRangeByAddress(uint64_t address) const;
	uint64_t begin() const { return begin_; }
	uint64_t end() const { return end_; }
	size_t prolog_size() const { return prolog_size_; }
	AddressBaseType base_type() const { return base_type_; }
	uint64_t base_value() const { return base_value_; }
	ICommand *entry() const { return entry_; }
	void set_entry(ICommand *entry) { entry_ = entry; }
	ICommand *data_entry() const { return data_entry_; }
	void set_data_entry(ICommand *data_entry) { data_entry_ = data_entry; }
	FunctionInfo *Clone(FunctionInfoList *owner) const;
	void Prepare();
	void Compile();
	void WriteToFile(IArchitecture &file);
	void Rebase(uint64_t delta_base);
	uint8_t frame_registr() const { return frame_registr_; }
	IRuntimeFunction *source() const { return source_; }
	void set_source(IRuntimeFunction *source) { source_ = source; }
	std::vector<ICommand *> *unwind_opcodes() { return &unwind_opcodes_; }
	void set_unwind_opcodes(const std::vector<ICommand *> &unwind_opcodes) { unwind_opcodes_ = unwind_opcodes; }
private:
	FunctionInfoList *owner_;
	uint64_t begin_;
	uint64_t end_;
	size_t prolog_size_;
	ICommand *entry_;
	ICommand *data_entry_;
	uint8_t frame_registr_;
	AddressBaseType base_type_;
	uint64_t base_value_;
	IRuntimeFunction *source_;
	std::vector<ICommand *> unwind_opcodes_;
};

class FunctionInfoList : public ObjectList<FunctionInfo>
{
public:
	explicit FunctionInfoList();
	explicit FunctionInfoList(const FunctionInfoList &src);
	FunctionInfoList *Clone() const;
	FunctionInfo *GetItemByAddress(uint64_t address) const;
	AddressRange *GetRangeByAddress(uint64_t address) const;
	FunctionInfo *Add(uint64_t begin, uint64_t end, AddressBaseType base_type, uint64_t base_value, size_t prolog_size, uint8_t frame_registr, 
		IRuntimeFunction *source, ICommand *entry);
	void Prepare();
	void Compile();
	void WriteToFile(IArchitecture &file);
	void Rebase(uint64_t delta_base);
private:
	// no assignment op
	FunctionInfoList &operator =(const FunctionInfoList &);
};

class IFunctionList;

class IFunction : public ObjectList<ICommand>
{
public:
	virtual uint64_t address() const = 0;
	virtual uint64_t break_address() const = 0;
	virtual ObjectType type() const = 0;
	virtual EntryType entry_type() const = 0;
	virtual ICommand *entry() const = 0;
	virtual std::string name() const = 0;
	virtual std::string display_name() const = 0;
	virtual FunctionName full_name() const = 0;
	virtual OperandSize cpu_address_size() const = 0;
	virtual CommandLinkList *link_list() const = 0;
	virtual ExtCommandList *ext_command_list() const = 0;
	virtual CommandBlockList *block_list() const = 0;
	virtual bool need_compile() const = 0;
	virtual CompilationType compilation_type() const = 0;
	virtual CompilationType default_compilation_type() const = 0;
	virtual uint32_t compilation_options() const = 0;
	virtual Folder *folder() const = 0;
	virtual void set_break_address(uint64_t break_address) = 0;
	virtual bool is_breaked_address(uint64_t address) const = 0;
	virtual void set_compilation_type(CompilationType compilation_type) = 0;
	virtual void set_compilation_options(uint32_t compilation_options) = 0;
	virtual void set_need_compile(bool need_compile) = 0;
	virtual void set_folder(Folder *folder) = 0;
	virtual void set_tag(uint8_t tag) = 0;
	virtual bool from_runtime() const = 0;
	virtual void set_from_runtime(bool from_runtime) = 0;
	virtual IFunction *Clone(IFunctionList *owner) const = 0;
	virtual size_t ReadFromFile(IArchitecture &file, uint64_t address) = 0;
	virtual size_t WriteToFile(IArchitecture &file) = 0;
	virtual bool Init(const CompileContext &ctx) = 0;
	virtual bool Prepare(const CompileContext &ctx) = 0;
	virtual bool PrepareExtCommands(const CompileContext &ctx) = 0;
	virtual bool PrepareLinks(const CompileContext &ctx) = 0;
	virtual bool Compile(const CompileContext &ctx) = 0;
	virtual void AfterCompile(const CompileContext &ctx) = 0;
	virtual void CompileLinks(const CompileContext &ctx) = 0;
	virtual void CompileInfo(const CompileContext &ctx) = 0;
	virtual ICommand *GetCommandByAddress(uint64_t address) const = 0;
	virtual ICommand *GetCommandByNearAddress(uint64_t address) const = 0;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file) = 0;
	virtual uint8_t tag() const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual uint32_t memory_type() const = 0;
	virtual void set_memory_type(uint32_t memory_type) = 0;
	virtual ICommand *ParseCommand(IArchitecture &file, uint64_t address, bool dump_mode = false) = 0;
	virtual IFunctionList *owner() const = 0;
	virtual IFunction *parent() const = 0;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const = 0;
	virtual CommandBlock *AddBlock(size_t start_index, bool is_executable = false) = 0;
	virtual ICommand *AddCommand(const Data &value) = 0;
	virtual ICommand *AddCommand(OperandSize value_size, uint64_t value) = 0;
	virtual std::string display_address(const std::string &arch_name) const = 0;
	virtual Data hash() const = 0;
	virtual FunctionInfoList *function_info_list() const = 0;
#ifdef CHECKED
	virtual bool check_hash() const = 0;
#endif
protected:
	virtual ICommand *CreateCommand() = 0;
};

struct CommandBlockListCompareHelper {
	bool operator () (const CommandBlock *block1, const CommandBlock *block2) const;
};

class BaseFunction : public IFunction
{
public:
	explicit BaseFunction(IFunctionList *owner, const FunctionName &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder);
	explicit BaseFunction(IFunctionList *owner, OperandSize cpu_address_size, IFunction *parent);
	explicit BaseFunction(IFunctionList *owner, const BaseFunction &source);
	virtual ~BaseFunction();
	virtual void RemoveObject(ICommand *obj);
	virtual void clear();
	virtual uint64_t address() const { return address_; }
	virtual ObjectType type() const { return type_; }
	virtual EntryType entry_type() const { return entry_type_; }
	virtual void set_entry_type(EntryType entry_type) { entry_type_ = entry_type; }
	virtual ICommand *entry() const { return entry_; }
	virtual std::string name() const { return name_.name(); }
	virtual std::string display_name() const { return name_.display_name(); }
	virtual FunctionName full_name() const { return name_; }
	virtual OperandSize cpu_address_size() const { return cpu_address_size_; }
	virtual CommandLinkList *link_list() const { return link_list_; }
	virtual ExtCommandList *ext_command_list() const { return ext_command_list_; }
	virtual CommandBlockList *block_list() const { return block_list_; }
	virtual bool need_compile() const { return need_compile_; }
	virtual CompilationType compilation_type() const { return (default_compilation_type_ == ctNone) ? compilation_type_ : default_compilation_type_; }
	virtual CompilationType default_compilation_type() const { return default_compilation_type_; }
	virtual uint32_t compilation_options() const { return compilation_options_ | (internal_lock_to_key_ ? coLockToKey : 0); }
	virtual Folder *folder() const { return folder_; }
	virtual uint32_t memory_type() const { return memory_type_; }
	virtual void set_memory_type(uint32_t memory_type) { memory_type_ = memory_type; } 
	virtual uint8_t tag() const { return tag_; }
	virtual void set_compilation_type(CompilationType compilation_type);
	virtual void set_compilation_options(uint32_t compilation_options);
	virtual void set_need_compile(bool need_compile);
	virtual void set_folder(Folder *folder);
	virtual void set_tag(uint8_t tag) { tag_ = tag; }
	virtual bool from_runtime() const { return from_runtime_; }
	virtual void set_from_runtime(bool from_runtime) { from_runtime_ = from_runtime; }
	virtual size_t ReadFromFile(IArchitecture &file, uint64_t address);
	virtual size_t WriteToFile(IArchitecture &file);
	virtual ICommand *GetCommandByAddress(uint64_t address) const;
	virtual ICommand *GetCommandByNearAddress(uint64_t address) const;
	virtual bool Init(const CompileContext &ctx);
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool PrepareExtCommands(const CompileContext &ctx);
	virtual bool PrepareLinks(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	virtual void AfterCompile(const CompileContext &ctx);
	virtual void CompileLinks(const CompileContext &ctx);
	virtual void CompileInfo(const CompileContext &ctx);
	void AddWatermark(Watermark *watermark, int copy_count);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void Rebase(uint64_t delta_base);
	bool FreeByManager(const CompileContext &ctx);
	virtual IFunctionList *owner() const { return owner_; }
	virtual uint64_t break_address() const { return break_address_; }
	virtual void set_break_address(uint64_t break_address);
	virtual bool is_breaked_address(uint64_t address) const { return break_address_ ? address >= break_address_ : false; }
	virtual IFunction *parent() const { return parent_; }
	virtual CommandBlock *AddBlock(size_t start_index, bool is_executable = false);
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	virtual FunctionInfo *range_list() const { return range_list_; }
	virtual FunctionInfoList *function_info_list() const { return function_info_list_; }
	virtual std::string display_address(const std::string &arch_name) const;
	virtual Data hash() const;
	virtual IVirtualMachine *virtual_machine(IVirtualMachineList *virtual_machine_list, ICommand *command) const;
	virtual void AddObject(ICommand *command);
#ifdef CHECKED
	virtual bool check_hash() const;
#endif
protected:
	virtual ICommand *ParseString(IArchitecture & /*file*/, uint64_t /*address*/, size_t /*len*/) { return NULL; }
	virtual void ParseBeginCommands(IArchitecture & /*file*/) { return; }
	virtual void ParseEndCommands(IArchitecture & /*file*/) { return; }
	void set_entry(ICommand *entry) { entry_ = entry; }
	ICommand *GetCommandByLowerAddress(uint64_t address) const;
	ICommand *GetCommandByUpperAddress(uint64_t address) const;
	virtual uint64_t GetNextAddress(IArchitecture &file);
	virtual IFunction *CreateFunction(IFunction *parent = NULL) = 0;
	void ClearItems();
private:
	typedef std::map<uint64_t, ICommand *> map_command_list_t;

	map_command_list_t map_;
	FunctionName name_;
	IFunction *parent_;

	uint64_t address_;
	uint64_t break_address_;

	IFunctionList *owner_; 
	CommandLinkList *link_list_;
	ExtCommandList *ext_command_list_;
	CommandBlockList *block_list_;
	Folder *folder_;
	ICommand *entry_;
	FunctionInfo *range_list_;
	FunctionInfoList *function_info_list_;

	uint32_t compilation_options_;
	uint32_t memory_type_;

	ObjectType type_;
	OperandSize cpu_address_size_;
	CompilationType compilation_type_;
	CompilationType default_compilation_type_;
	uint8_t tag_;
	EntryType entry_type_;
	bool from_runtime_;
	bool internal_lock_to_key_;
	bool need_compile_;

	// no copy ctr or assignment op
	BaseFunction(const BaseFunction &);
	BaseFunction &operator =(const BaseFunction &);
};

class Signature: public IObject
{
public:
	explicit Signature(SignatureList *owner, const std::string &value, uint32_t tag);
	~Signature();
	size_t size() const { return dump_.size(); }
	uint8_t dump(size_t index) const { return dump_[index]; }
	uint32_t tag() const { return tag_; }
	void InitSearch() { pos_.clear(); }
	bool SearchByte(uint8_t value);
private:
	void Init();

	SignatureList *owner_;
	std::string value_;
	std::vector<uint8_t> dump_;
	std::vector<uint8_t> mask_;
	std::vector<size_t> pos_;
	uint32_t tag_;
};

class SignatureList: public ObjectList<Signature>
{
public:
	explicit SignatureList();
	Signature *Add(const std::string &value, uint32_t tag = 0);
	void InitSearch();
};

class IFunctionList : public ObjectList<IFunction>
{
public:
	virtual IFunction *Add(const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder) = 0;
	virtual IFunction *GetFunctionByAddress(uint64_t address) const = 0;
	virtual IFunction *GetFunctionByName(const std::string &name) const = 0;
	virtual IFunction *GetUnknownByName(const std::string &name) const = 0;
	virtual ICommand *GetCommandByAddress(uint64_t address, bool need_compile) const = 0;
	virtual ICommand *GetCommandByNearAddress(uint64_t address, bool need_compile) const = 0;
	virtual IFunction *AddUnknown(const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder) = 0;
	virtual IFunction *AddByAddress(uint64_t address, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder) = 0;
	virtual IFunctionList *Clone(IArchitecture *owner) const = 0;
	virtual bool Prepare(const CompileContext &ctx) = 0;
	virtual bool Compile(const CompileContext &ctx) = 0;
	virtual void CompileLinks(const CompileContext &ctx) = 0;
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file) = 0;
	virtual IFunction *crc_table() const = 0;
	virtual ValueCryptor *crc_cryptor() const = 0;
	virtual void Rebase(uint64_t delta_base) = 0;
	virtual IFunction *CreateFunction(OperandSize cpu_address_size = osDefault) = 0;
	virtual IArchitecture *owner() const = 0;
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const = 0;
	virtual bool GetRuntimeOptions() const = 0;
	virtual std::vector<IFunction *> processor_list() const = 0;
#ifdef CHECKED
	virtual bool check_hash() const = 0;
#endif
};

class BaseFunctionList : public IFunctionList
{
public:
	explicit BaseFunctionList(IArchitecture *owner);
	explicit BaseFunctionList(IArchitecture *owner, const BaseFunctionList &src);
	virtual IFunction *GetFunctionByAddress(uint64_t address) const;
	virtual IFunction *GetFunctionByName(const std::string &name) const;
	virtual IFunction *GetUnknownByName(const std::string &name) const;
	virtual ICommand *GetCommandByAddress(uint64_t address, bool need_compile) const;
	virtual ICommand *GetCommandByNearAddress(uint64_t address, bool need_compile) const;
	virtual IFunction *AddUnknown(const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder);
	virtual IFunction *AddByAddress(uint64_t address, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder);
	virtual bool Prepare(const CompileContext &ctx);
	virtual bool Compile(const CompileContext &ctx);
	virtual void CompileLinks(const CompileContext &ctx);
	virtual void CompileInfo(const CompileContext &ctx);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
	virtual void Rebase(uint64_t delta_base);
	virtual IArchitecture *owner() const { return owner_; }
	virtual void RemoveObject(IFunction *func);
	virtual void Notify(MessageType type, IObject *sender, const std::string &message = "") const;
	virtual std::vector<IFunction *> processor_list() const;
#ifdef CHECKED
	virtual bool check_hash() const;
#endif
private:
	IArchitecture *owner_;
};

typedef std::vector<uint8_t> ByteList;

class IVirtualMachine : public IObject
{
public:
	virtual uint8_t id() const = 0;
	virtual ByteList *registr_order() = 0;
	virtual bool backward_direction() const = 0;
	virtual IFunction *processor() const = 0;
};

class IVirtualMachineList : public ObjectList<IVirtualMachine>
{
public:
	virtual IVirtualMachineList *Clone() const = 0;
	virtual void Prepare(const CompileContext &ctx) = 0;
};

class BaseVirtualMachine : public IVirtualMachine
{
public:
	BaseVirtualMachine(IVirtualMachineList *owner, uint8_t id);
	~BaseVirtualMachine();
	virtual uint8_t id() const { return id_; }
private:
	IVirtualMachineList *owner_;
	uint8_t id_;
};

#endif
