#ifndef DOTNETFILE_H
#define DOTNETFILE_H

#include "dotnet.h"

std::string ILName(const std::string &ret, const std::string &type, const std::string &method, const std::string &signature);

enum ILTokenType {
	ttModule = (0x00 << 24),
	ttTypeRef = (0x01 << 24),
	ttTypeDef = (0x02 << 24),
	ttField = (0x04 << 24),
	ttMethodDef = (0x06 << 24),
	ttParam = (0x08 << 24),
	ttInterfaceImpl = (0x09 << 24),
	ttMemberRef = (0x0a << 24),
	ttConstant = (0x0b << 24),
	ttCustomAttribute = (0x0c << 24),
	ttFieldMarshal = (0x0d << 24),
	ttDeclSecurity = (0x0e << 24),
	ttClassLayout = (0x0f << 24),
	ttFieldLayout = (0x10 << 24),
	ttStandAloneSig = (0x11 << 24),
	ttEventMap = (0x12 << 24),
	ttEvent = (0x14 << 24),
	ttPropertyMap = (0x15 << 24),
	ttProperty = (0x17 << 24),
	ttMethodSemantics = (0x18 << 24),
	ttMethodImpl = (0x19 << 24),
	ttModuleRef = (0x1a << 24),
	ttTypeSpec = (0x1b << 24),
	ttImplMap = (0x1c << 24),
	ttFieldRVA = (0x1d << 24),
	ttEncLog = (0x1e << 24),
	ttEncMap = (0x1f << 24),
	ttAssembly = (0x20 << 24),
	ttAssemblyProcessor = (0x21 << 24),
	ttAssemblyOS = (0x22 << 24),
	ttAssemblyRef = (0x23 << 24),
	ttAssemblyRefProcessor = (0x24 << 24),
	ttAssemblyRefOS = (0x25 << 24),
	ttFile = (0x26 << 24),
	ttExportedType = (0x27 << 24),
	ttManifestResource = (0x28 << 24),
	ttNestedClass = (0x29 << 24),
	ttGenericParam = (0x2a << 24),
	ttMethodSpec = (0x2b << 24),
	ttGenericParamConstraint = (0x2c << 24),
	ttUserString = (0x70 << 24),
	ttInvalid = (0xff << 24),
	ttMask = (0xff << 24)
};

#define TOKEN_TYPE(id) ((id) & 0xff000000)
#define TOKEN_VALUE(id) ((id) & 0x00ffffff)

class ILData : public std::vector<uint8_t>
{
public:
	ILData();
	ILData(const uint8_t *data, size_t size);
	uint32_t ReadEncoded(uint32_t &pos) const;
	void WriteEncoded(uint32_t value);
	void Read(uint32_t &pos, void *buffer, uint32_t size) const;
	uint8_t ReadByte(uint32_t &pos) const;
	uint16_t ReadWord(uint32_t &pos) const;
	uint32_t ReadDWord(uint32_t &pos) const;
	uint64_t ReadQWord(uint32_t &pos) const;
	std::string ReadString(uint32_t &pos) const;
	void WriteByte(uint8_t value);
	void WriteWord(uint16_t value);
	void WriteDWord(uint32_t value);
	void WriteQWord(uint64_t value);
	void Write(const void *buffer, size_t size);
	void WriteString(const std::string &value);
};

class ILUserStringsTable;
class NETArchitecture;
class ILMetaData;
class ILTable;
class TokenReferenceList;
class ILToken;
class NETRuntimeFunctionList;

class ILStream : public BaseLoadCommand
{
public:
	explicit ILStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name);
	explicit ILStream(ILMetaData *owner, const ILStream &src);
	virtual ILStream *Clone(ILoadCommandList *owner) const;
	virtual void ReadFromFile(NETArchitecture &file) { }
	virtual void WriteToFile(NETArchitecture &file);
	virtual void Prepare() { }
	virtual uint64_t address() const { return address_; }
	virtual uint32_t size() const { return size_; }
	virtual uint32_t type() const { return 0; }
	virtual std::string name() const { return name_; }
	virtual void Rebase(uint64_t delta_base);
	void FreeByManager(MemoryManager &manager);
protected:
	void set_size(uint32_t size) { size_ = size; }
private:
	uint64_t address_;
	uint32_t size_;
	std::string name_;
};

class ILStringsStream : public ILStream
{
public:
	explicit ILStringsStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name);
	explicit ILStringsStream(ILMetaData *owner, const ILStringsStream &src);
	virtual ILStream *Clone(ILoadCommandList *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void Prepare();
	std::string GetString(uint32_t pos) const;
	uint32_t AddString(const std::string &str);
	size_t data_size() const { return data_.size(); }
private:
	std::map<std::string, uint32_t> map_;
	ILData data_;
};

class ILUserStringsStream : public ILStream
{
public:
	explicit ILUserStringsStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name);
	explicit ILUserStringsStream(ILMetaData *owner, const ILUserStringsStream &src);
	virtual ILUserStringsStream *Clone(ILoadCommandList *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void Prepare();
	std::string GetString(uint32_t pos) const;
	ILData GetData(uint32_t pos, uint64_t *address) const;
	uint32_t AddString(const std::string &str);
private:
	std::map<std::string, uint32_t> map_;
	ILData data_;
};

class ILBlobStream : public ILStream
{
public:
	explicit ILBlobStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name);
	explicit ILBlobStream(ILMetaData *owner, const ILBlobStream &src);
	virtual ILStream *Clone(ILoadCommandList *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void Prepare();
	ILData GetData(uint32_t pos) const;
	uint32_t AddData(const ILData &value);
	size_t data_size() const { return data_.size(); }
private:
	std::map<ILData, uint32_t> map_;
	ILData data_;
};

class ILGuidStream : public ILStream
{
public:
	explicit ILGuidStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name);
	explicit ILGuidStream(ILMetaData *owner, const ILGuidStream &src);
	virtual ILStream *Clone(ILoadCommandList *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void Prepare();
	ILData GetData(uint32_t pos) const;
	uint32_t AddData(const ILData &value);
	size_t data_size() const { return data_.size(); }
private:
	std::map<ILData, uint32_t> map_;
	ILData data_;
};

class ILHeapStream : public ILStream
{
public:
	explicit ILHeapStream(ILMetaData *owner, uint64_t address, uint32_t size, const std::string &name);
	explicit ILHeapStream(ILMetaData *owner, const ILHeapStream &src);
	~ILHeapStream();
	virtual ILStream *Clone(ILoadCommandList *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	uint8_t offset_sizes() const { return heap_offset_sizes_; }
	ILTable *table(size_t index) const { return table_list_[index]; }
	size_t count() const { return table_list_.size(); }
	virtual void Rebase(uint64_t delta_base);
	void UpdateTokens();
	void Pack();
private:
	ILTable *Add(ILTokenType type, uint32_t row_count);

	uint32_t reserved_;
	uint8_t major_version_;
	uint8_t minor_version_;
	uint8_t heap_offset_sizes_;
	uint8_t reserved2_;
	uint64_t mask_valid_;
	uint64_t mask_sorted_;
	std::vector<uint8_t> extra_data_;
	std::vector<ILTable *> table_list_;
};

class TokenReference : public IObject
{
public:
	explicit TokenReference(TokenReferenceList *owner, uint64_t address, uint32_t command_type);
	explicit TokenReference(TokenReferenceList *owner, const TokenReference &src);
	~TokenReference();
	uint64_t address() const { return address_; }
	void set_address(uint64_t address) { address_ = address; }
	TokenReference *Clone(TokenReferenceList *owner) const;
	void Rebase(uint64_t delta_base);
	bool is_deleted() const { return is_deleted_; }
	void set_deleted(bool is_deleted) { is_deleted_ = is_deleted; }
	TokenReferenceList *owner() const { return owner_; }
	void set_owner(TokenReferenceList *owner);
	uint32_t command_type() const { return command_type_; }
private:
	TokenReferenceList *owner_;
	uint64_t address_;
	bool is_deleted_;
	uint32_t command_type_;

	// no copy ctr or assignment op
	TokenReference(const TokenReference &);
	TokenReference &operator =(const TokenReference &);
};

class TokenReferenceList : public ObjectList<TokenReference>
{
public:
	explicit TokenReferenceList(ILToken *owner);
	explicit TokenReferenceList(ILToken *owner, const TokenReferenceList &src);
	TokenReferenceList *Clone(ILToken *owner) const;
	TokenReference *Add(uint64_t address, uint32_t command_type = 0);
	TokenReference *GetReferenceByAddress(uint64_t address) const;
	void Rebase(uint64_t delta_base);
	ILToken *owner() const { return owner_; }
private:
	ILToken *owner_;
};

enum ILSignatureType {
	stDefault,
	stC,
	stStdCall,
	stThisCall,
	stFastCall,
	stVarArg,
	stField,
	stLocal,
	stProperty,
	stUnmanaged,
	stGenericinst,
	stNativeVarArg,

	stGeneric = 0x10,
	stHasThis = 0x20,
	stExplicitThis = 0x40,
};

struct EncodingDesc {
	const ILTokenType *types;
	size_t size;
	size_t bits;
	EncodingDesc(const ILTokenType *types_, size_t size_, size_t bits_) : types(types_), size(size_), bits(bits_) {}
};

class ILToken : public IObject
{
public:
	ILToken(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILToken(ILMetaData *meta, ILTable *owner, const ILToken &src);
	~ILToken();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	uint32_t id() const { return id_; }
	ILTokenType type() const { return static_cast<ILTokenType>(TOKEN_TYPE(id_)); }
	uint32_t value() const { return TOKEN_VALUE(id_); }
	void set_value(uint32_t value, IArchitecture *file = NULL);
	void set_owner(ILMetaData *meta, ILTable *owner);
	ILToken *next() const;
	TokenReferenceList *reference_list() const { return reference_list_; }
	virtual void ReadFromFile(NETArchitecture &file) { }
	virtual void WriteToStreams(ILMetaData &data) { }
	virtual void WriteToFile(NETArchitecture &file) { }
	virtual void UpdateTokens() { }
	virtual void RemapTokens(const std::map<ILToken *, ILToken *> &token_map) { }
	virtual void Rebase(uint64_t delta_base);
	bool is_deleted() const { return is_deleted_; }
	void set_deleted(bool is_deleted) { is_deleted_ = is_deleted; }
	ILMetaData *meta() const { return meta_; }
	ILTable *owner() const { return owner_; }
	virtual bool is_exported() const { return false; }
	bool can_rename() const { return can_rename_; }
	void set_can_rename(bool value) { can_rename_ = value; }
	uint32_t Encode(const EncodingDesc &desc) const;
	using IObject::CompareWith;
	virtual int CompareWith(const ILToken &other) const;
protected:
	std::string ReadStringFromFile(NETArchitecture &file) const;
	std::string ReadUserString(uint32_t value) const;
	ILData ReadBlobFromFile(NETArchitecture &file) const;
	ILData ReadGUIDFromFile(NETArchitecture &file) const;
	ILToken *ReadTokenFromFile(NETArchitecture &file, const EncodingDesc &desc) const;
	ILToken *ReadTokenFromFile(NETArchitecture &file, ILTokenType type) const;

	void WriteStringToFile(NETArchitecture &file, uint32_t pos) const;
	void WriteGuidToFile(NETArchitecture &file, uint32_t pos) const;
	void WriteTokenToFile(NETArchitecture &file, ILTokenType type, ILToken *token) const;
	void WriteTokenListToFile(NETArchitecture &file, ILTokenType type, ILToken *token) const;
	void WriteTokenToFile(NETArchitecture &file, const EncodingDesc &desc, ILToken *token) const;
	void WriteBlobToFile(NETArchitecture &file, uint32_t pos) const;
private:
	ILMetaData *meta_;
	ILTable *owner_;
	uint32_t id_;
	TokenReferenceList *reference_list_;
	bool is_deleted_;
	bool can_rename_;

	// no copy ctr or assignment op
	ILToken(const ILToken &);
	ILToken &operator =(const ILToken &);
};

class ILSignature;
class ILElement;
class ILModuleRef;
class ILParam;
class ILTypeDef;

class ILCustomMod : public IObject
{
public:
	ILCustomMod(ILMetaData *meta);
	ILCustomMod(ILMetaData *meta, const ILCustomMod &src);
	ILCustomMod *Clone(ILMetaData *meta);
	std::string name(bool mode = false) const;
	void Parse(const ILData &data, uint32_t &id);
	void WriteToData(ILData &data);
	void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
private:
	ILMetaData *meta_;
	CorElementType type_;
	ILToken *token_;
};

class ILArrayShape : public IObject
{
public:
	ILArrayShape();
	ILArrayShape(const ILArrayShape &src);
	ILArrayShape *Clone();
	std::string name() const;
	void Parse(const ILData &data, uint32_t &id);
	void WriteToData(ILData &data) const;
private:
	uint32_t rank_;
	std::vector<uint32_t> sizes_;
	std::vector<uint32_t> lo_bounds_;

	// not implemented
	ILArrayShape &operator =(const ILArrayShape &);
};

class ILElement : public IObject
{
public:
	ILElement(ILMetaData *meta, ILSignature *owner, CorElementType type = ELEMENT_TYPE_END, ILToken *token = NULL, ILElement *next = NULL);
	ILElement(ILMetaData *meta, ILSignature *owner, const ILElement &src);
	~ILElement();
	ILElement *Clone(ILMetaData *meta, ILSignature *owner);
	CorElementType type() const { return type_; }
	std::string name(bool mode = false) const;
	void Parse(const ILData &data);
	void Parse(const ILData &data, uint32_t &id);
	void UpdateTokens();
	void WriteToData(ILData &data) const;
	ILData data() const;
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	bool is_ref() const { return byref_; }
	ILToken *token() const { return token_; }
	bool is_predicate() const { return is_predicate_; }
	void set_is_predicate(bool is_predicate) { is_predicate_ = is_predicate; }
	ILElement *next() const { return next_; }
private:
	ILCustomMod *AddMod();
	ILElement *AddElement();

	ILMetaData *meta_;
	ILSignature *owner_;
	CorElementType type_;
	bool byref_;
	bool pinned_;
	uint32_t generic_param_;
	ILElement *next_;
	ILToken *token_;
	ILSignature *method_;
	ILArrayShape *array_shape_;
	std::vector<ILCustomMod*> mod_list_;
	std::vector<ILElement*> child_list_;
	bool is_predicate_;
	
	// no copy ctr or assignment op
	ILElement(const ILElement &);
	ILElement &operator =(const ILElement &);
};

class ILSignature : public ObjectList<ILElement>
{
public:
	ILSignature(ILMetaData *meta);
	ILSignature(ILMetaData *meta, const ILSignature &src);
	~ILSignature();
	ILSignature *Clone(ILMetaData *meta);
	void Parse(const ILData &data);
	bool Parse(const ILData &data, uint32_t &id);
	bool has_this() const { return (type_ & stHasThis) != 0; }
	void set_has_this(bool value) { type_ = (ILSignatureType)(value ? (type_ | stHasThis) : (type_ & ~stHasThis)); }
	bool explicit_this() const { return (type_ & stExplicitThis) != 0; }
	std::string name(bool mode = false, ILSignature *gen_signature = NULL) const;
	std::string type_name() const;
	std::string ret_name(bool mode = false) const;
	void UpdateTokens();
	ILData data() const;
	void WriteToData(ILData &data) const;
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILElement *ret() const { return ret_; }
	bool is_method() const;
	bool is_field() const;
private:
	ILElement *Add();

	ILMetaData *meta_;
	ILSignatureType type_;
	uint32_t gen_param_count_;
	ILElement *ret_;

	// no copy ctr or assignment op
	ILSignature(const ILSignature &);
	ILSignature &operator =(const ILSignature &);
};

class ILAssembly : public ILToken
{
public:
	ILAssembly(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILAssembly(ILMetaData *meta, ILTable *owner, const std::string &name);
	ILAssembly(ILMetaData *meta, ILTable *owner, const ILAssembly &src);
	ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	std::string full_name() const;
	uint16_t major_version() const { return major_version_; }
	uint16_t minor_version() const { return minor_version_; }
	uint16_t build_number() const { return build_number_; }
	std::string name() const { return name_; }
private:
	uint32_t hash_id_;
	uint16_t major_version_;
	uint16_t minor_version_;
	uint16_t build_number_;
	uint16_t revision_number_;
	uint32_t flags_;
	ILData public_key_;
	std::string name_;
	std::string culture_;

	uint32_t public_key_pos_;
	uint32_t name_pos_;
	uint32_t culture_pos_;
};

class ILAssemblyOS : public ILToken
{
public:
	ILAssemblyOS(ILMetaData *meta, ILTable *owner, uint32_t id);
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
private:
	uint32_t os_platform_id_;
	uint32_t os_major_version_;
	uint32_t os_minor_version_;
};

class ILAssemblyProcessor : public ILToken
{
public:
	ILAssemblyProcessor(ILMetaData *meta, ILTable *owner, uint32_t id);
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
private:
	uint32_t processor_;
};

class ILAssemblyRef : public ILToken
{
public:
	ILAssemblyRef(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILAssemblyRef(ILMetaData *meta, ILTable *owner, const std::string &name);
	ILAssemblyRef(ILMetaData *meta, ILTable *owner, const ILAssemblyRef &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name() const { return name_; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	std::string full_name() const;
	uint16_t major_version() const { return major_version_; }
	uint16_t minor_version() const { return minor_version_; }
	uint16_t build_number() const { return build_number_; }
private:
	uint16_t major_version_;
	uint16_t minor_version_;
	uint16_t build_number_;
	uint16_t revision_number_;
	uint32_t flags_;
	ILData public_key_or_token_;
	std::string name_;
	std::string culture_;
	ILData hash_value_;

	uint32_t public_key_or_token_pos_;
	uint32_t name_pos_;
	uint32_t culture_pos_;
	uint32_t hash_value_pos_;
};

class ILAssemblyRefOS : public ILToken
{
public:
	ILAssemblyRefOS(ILMetaData *meta, ILTable *owner, uint32_t id);
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
private:
	uint32_t os_platform_id_;
	uint32_t os_major_version_;
	uint32_t os_minor_version_;
	ILAssemblyRef *assembly_ref_;
};

class ILAssemblyRefProcessor : public ILToken
{
public:
	ILAssemblyRefProcessor(ILMetaData *meta, ILTable *owner, uint32_t id);
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
private:
	uint32_t processor_;
	ILAssemblyRef *assembly_ref_;
};

class ILClassLayout : public ILToken
{
public:
	ILClassLayout(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILClassLayout(ILMetaData *meta, ILTable *owner, const ILClassLayout &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
private:
	uint16_t packing_size_;
	uint32_t class_size_;
	ILTypeDef *parent_;
};

class ILConstant : public ILToken
{
public:
	ILConstant(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILConstant(ILMetaData *meta, ILTable *owner, const ILConstant &src);
	ILToken *parent() const { return parent_; }
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	virtual int CompareWith(const ILToken &other) const;
private:
	uint8_t type_;
	uint8_t padding_zero_;
	ILToken *parent_;
	ILData value_;

	uint32_t value_pos_;
};

class CustomAttributeValue;

class ILCustomAttribute : public ILToken
{
public:
	ILCustomAttribute(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILCustomAttribute(ILMetaData *meta, ILTable *owner, const ILCustomAttribute &src);
	~ILCustomAttribute();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILToken *parent() const { return parent_; }
	ILToken *type() const { return type_; }
	virtual int CompareWith(const ILToken &other) const;
	CustomAttributeValue *ParseValue() const;
	void UpdateValue();
private:
	ILToken *parent_;
	ILToken *type_;
	ILData value_;

	CustomAttributeValue *parser_;
	uint32_t value_pos_;
};

class ILDeclSecurity : public ILToken
{
public:
	ILDeclSecurity(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILDeclSecurity(ILMetaData *meta, ILTable *owner, const ILDeclSecurity &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
private:
	uint16_t action_;
	ILToken *parent_;
	ILData permission_set_;

	uint32_t permission_set_pos_;
};

class ILEvent : public ILToken
{
public:
	ILEvent(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILEvent(ILMetaData *meta, ILTable *owner, const ILEvent &src);
	ILEvent *next() const { return reinterpret_cast<ILEvent *>(ILToken::next()); }
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILToken *parent() const { return parent_; }
	uint16_t flags() const { return flags_; }
	bool is_exported() const;
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	ILTypeDef *declaring_type() const { return declaring_type_; }
	void set_declaring_type(ILTypeDef *declaring_type) { declaring_type_ = declaring_type; }
private:
	uint16_t flags_;
	std::string name_;
	ILToken *parent_;
	ILTypeDef *declaring_type_;

	uint32_t name_pos_;
};

class ILEventMap : public ILToken
{
public:
	ILEventMap(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILEventMap(ILMetaData *meta, ILTable *owner, const ILEventMap &src);
	ILEventMap *next() const { return reinterpret_cast<ILEventMap *>(ILToken::next()); }
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	ILEvent *event_list() const { return event_list_; }
	void set_event_list(ILEvent *event_list) { event_list_ = event_list; }
	ILTypeDef *parent() const { return parent_; }
private:
	ILTypeDef *parent_;
	ILEvent *event_list_;
};

class ILField : public ILToken
{
public:
	ILField(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILField(ILMetaData *meta, ILTable *owner, const ILField &src);
	~ILField();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	uint16_t flags() const { return flags_; }
	ILField *next() const { return reinterpret_cast<ILField *>(ILToken::next()); }
	ILTypeDef *declaring_type() const { return declaring_type_; }
	void set_declaring_type(ILTypeDef *declaring_type) { declaring_type_ = declaring_type; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILSignature *signature() const { return signature_; }
	bool is_exported() const;
	std::string full_name(bool mode = false) const;
private:
	uint16_t flags_;
	std::string name_;
	ILSignature *signature_;
	ILTypeDef *declaring_type_;

	uint32_t name_pos_;
	uint32_t signature_pos_;
	
	// no copy ctr or assignment op
	ILField(const ILField &);
	ILField &operator =(const ILField &);
};

class ILFieldLayout : public ILToken
{
public:
	ILFieldLayout(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILFieldLayout(ILMetaData *meta, ILTable *owner, const ILFieldLayout &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILField *parent() const { return field_; }
private:
	uint32_t offset_;
	ILField *field_;
};

class ILFieldMarshal : public ILToken
{
public:
	ILFieldMarshal(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILFieldMarshal(ILMetaData *meta, ILTable *owner, const ILFieldMarshal &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
private:
	ILToken *parent_;
	ILData native_type_;

	uint32_t native_type_pos_;
};

class ILFieldRVA : public ILToken
{
public:
	ILFieldRVA(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILFieldRVA(ILMetaData *meta, ILTable *owner, const ILFieldRVA &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	ILFieldRVA *next() const { return reinterpret_cast<ILFieldRVA *>(ILToken::next()); }
	ILField *field() const { return field_; };
	virtual void Rebase(uint64_t delta_base);
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
private:
	uint64_t address_;
	ILField *field_;
};

class ILFile : public ILToken
{
public:
	ILFile(ILMetaData *meta, ILTable *owner, uint32_t id);
	std::string name() const { return name_; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
private:
	uint32_t flags_;
	std::string name_;
	ILData value_;

	uint32_t name_pos_;
	uint32_t value_pos_;
};

class ILGenericParam : public ILToken
{
public:
	ILGenericParam(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILGenericParam(ILMetaData *meta, ILTable *owner, const ILGenericParam &src);
	ILToken *parent() const { return parent_; }
	ILGenericParam *next() const { return reinterpret_cast<ILGenericParam *>(ILToken::next()); }
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	uint16_t number() const { return number_; }
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	virtual int CompareWith(const ILToken &other) const;
private:
	uint16_t number_;
	uint16_t flags_;
	ILToken *parent_;
	std::string name_;

	uint32_t name_pos_;
};

class ILGenericParamConstraint : public ILToken
{
public:
	ILGenericParamConstraint(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILGenericParamConstraint(ILMetaData *meta, ILTable *owner, const ILGenericParamConstraint &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	virtual int CompareWith(const ILToken &other) const;
private:
	ILGenericParam *parent_;
	ILToken *constraint_;
};

class ILImplMap : public ILToken
{
public:
	ILImplMap(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILImplMap(ILMetaData *meta, ILTable *owner, const ILImplMap &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	virtual int CompareWith(const ILToken &other) const;
	ILToken *member_forwarded() const { return member_forwarded_; }
	ILModuleRef *import_scope() const { return import_scope_; }
	std::string import_name() const { return import_name_; }
private:
	uint16_t mapping_flags_;
	ILToken *member_forwarded_;
	std::string import_name_;
	ILModuleRef *import_scope_;

	uint32_t import_name_pos_;
};

class ILInterfaceImpl : public ILToken
{
public:
	ILInterfaceImpl(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILInterfaceImpl(ILMetaData *meta, ILTable *owner, const ILInterfaceImpl &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	virtual int CompareWith(const ILToken &other) const;
	ILTypeDef *parent() const { return class_; }
	ILToken *_interface() const { return interface_; }
private:
	ILTypeDef *class_;
	ILToken *interface_;
};

class ILResourceList;

class ILResource : public IObject
{
public:
	ILResource(ILResourceList *owner, uint32_t type, const std::string &name, uint32_t offset, uint32_t size);
	ILResource(ILResourceList *owner, const ILResource &src);
	~ILResource();
	std::string name() const { return name_; }
	uint32_t offset() const { return offset_; }
	uint32_t size() const { return size_; }
private:
	ILResourceList *owner_;
	uint32_t type_;
	std::string name_;
	uint32_t offset_;
	uint32_t size_;
};

class ManifestResourceValue;
class BamlDocument;
class NETStream;

class ManifestResourceItem : public IObject
{
public:
	ManifestResourceItem(ManifestResourceValue *owner, const std::string &name, uint32_t type, uint64_t address, uint32_t size, const std::vector<uint8_t> &data);
	ManifestResourceItem(ManifestResourceValue *owner, const ManifestResourceItem &src);
	~ManifestResourceItem();
	ManifestResourceItem *Clone(ManifestResourceValue *owner) const;
	std::string name() const { return name_; }
	uint32_t type() const { return type_; }
	uint64_t address() const { return address_; }
	uint32_t size() const { return size_; }
	std::vector<uint8_t> data() const;
	BamlDocument *ParseBaml();
	int32_t name_hash() const;
private:
	ManifestResourceValue *owner_;
	std::string name_;
	uint32_t type_;
	uint64_t address_;
	uint32_t size_;
	BamlDocument *baml_;
	std::vector<uint8_t> data_;
};

class ManifestResourceValue : public  ObjectList<ManifestResourceItem>
{
public:
	ManifestResourceValue();
	ManifestResourceValue(const ManifestResourceValue &src);
	ManifestResourceValue *Clone() const;
	void ReadFromFile(NETArchitecture &file, uint64_t address);
	uint64_t address() const { return address_; };
	uint32_t size() const { return size_; };
	void WriteToStream(NETStream &stream);
private:
	ManifestResourceItem *Add(const std::string &name, uint32_t id, uint64_t address, uint32_t size, const std::vector<uint8_t> &data);
	uint64_t address_;
	uint32_t size_;

	uint32_t magic_;
	uint32_t header_version_;
	std::string reader_type_name_;
	std::string set_type_name_;
	uint32_t reader_version_;
	std::vector<std::string> type_names_;
};

class ILManifestResource : public ILToken
{
public:
	ILManifestResource(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILManifestResource(ILMetaData *meta, ILTable *owner, const ILManifestResource &src);
	~ILManifestResource();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	uint32_t offset() const { return offset_; }
	void set_offset(uint32_t offset) { offset_ = offset; }
	ILToken *implementation() const { return implementation_; }
	void set_implementation(ILToken *token) { implementation_ = token; }
	void set_flags(uint32_t flags) { flags_ = flags; }
	ILManifestResource *next() const { return reinterpret_cast<ILManifestResource *>(ILToken::next()); }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void UpdateValue();
	ManifestResourceValue *value() const { return value_; }
	std::vector<uint8_t> data() const { return data_; }
private:
	uint32_t offset_;
	uint32_t flags_;
	std::string name_;
	ILToken *implementation_;
	ManifestResourceValue *value_;

	uint32_t name_pos_;
	std::vector<uint8_t> data_;
};

class ILMemberRef : public ILToken
{
public:
	ILMemberRef(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILMemberRef(ILMetaData *meta, ILTable *owner, const ILMemberRef &src);
	~ILMemberRef();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name() const { return name_; }
	std::string full_name(bool mode = false, ILSignature *method_gen_signature = NULL) const;
	void set_name(const std::string &name) { name_ = name; }
	ILMemberRef *next() const { return reinterpret_cast<ILMemberRef *>(ILToken::next()); }
	ILToken *declaring_type() const { return declaring_type_; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILSignature *signature() const { return signature_; }
private:
	ILSignature *signature_;
	ILToken *declaring_type_;
	std::string name_;

	uint32_t signature_pos_;
	uint32_t name_pos_;
	
	// no copy ctr or assignment op
	ILMemberRef(const ILMemberRef &);
	ILMemberRef &operator =(const ILMemberRef &);
};

class ILStandAloneSig;

class ILMethodDef : public ILToken
{
public:
	ILMethodDef(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILMethodDef(ILMetaData *meta, ILTable *owner, const ILMethodDef &src);
	ILMethodDef(ILMetaData *meta, ILTable *owner, const std::string &name, const ILData &signature, CorMethodAttr flags = mdPrivateScope, CorMethodImpl impl_flags = miIL);
	~ILMethodDef();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	ILMethodDef *next() const { return reinterpret_cast<ILMethodDef *>(ILToken::next()); }
	ILParam *param_list() const { return param_list_; }
	void set_param_list(ILParam *param_list) { param_list_ = param_list; } 
	CorMethodImpl impl_flags() const { return impl_flags_; }
	CorMethodAttr flags() const { return flags_; }
	void set_flags(CorMethodAttr flags) { flags_ = flags; }
	uint64_t address() const { return address_; }
	void set_address(uint64_t address) { address_ = address; }
	ILTypeDef *declaring_type() const { return declaring_type_; }
	void set_declaring_type(ILTypeDef *declaring_type) { declaring_type_ = declaring_type; }
	uint32_t fat_size() const { return fat_size_; }
	uint32_t code_size() const { return code_size_; }
	ILStandAloneSig *locals() const { return locals_; }
	void set_locals(ILStandAloneSig *locals) { locals_ = locals; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void Rebase(uint64_t delta_base);
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILSignature *signature() const { return signature_; }
	bool is_exported() const;
	std::string full_name(bool mode = false, ILSignature *method_gen_signature = NULL) const;
private:
	ILSignature *signature_;
	uint64_t address_;
	CorMethodImpl impl_flags_;
	CorMethodAttr flags_;
	std::string name_;
	ILParam *param_list_;
	ILTypeDef *declaring_type_;
	uint32_t fat_size_;
	uint32_t code_size_;
	ILStandAloneSig *locals_;

	uint32_t name_pos_;
	uint32_t signature_pos_;
	
	// no copy ctr or assignment op
	ILMethodDef(const ILMethodDef &);
	ILMethodDef &operator =(const ILMethodDef &);
};

class ILMethodImpl : public ILToken
{
public:
	ILMethodImpl(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILMethodImpl(ILMetaData *meta, ILTable *owner, const ILMethodImpl &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual int CompareWith(const ILToken &other) const;
private:
	ILTypeDef *class_;
	ILToken *method_body_;
	ILToken *method_declaration_;
};

class ILMethodSemantics : public ILToken
{
public:
	ILMethodSemantics(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILMethodSemantics(ILMetaData *meta, ILTable *owner, const ILMethodSemantics &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	virtual int CompareWith(const ILToken &other) const;
	CorMethodSemanticsAttr flags() const { return flags_; }
	ILMethodDef *method() const { return method_; }
	ILToken *association() const { return association_; }
private:
	CorMethodSemanticsAttr flags_;
	ILMethodDef *method_;
	ILToken *association_;
};

class ILMethodSpec : public ILToken
{
public:
	ILMethodSpec(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILMethodSpec(ILMetaData *meta, ILTable *owner, const ILMethodSpec &src);
	~ILMethodSpec();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	ILToken *parent() const { return parent_; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILSignature *signature() const { return signature_; }
	std::string full_name(bool mode) const;
private:
	ILToken *parent_;
	ILSignature *signature_;

	uint32_t instantiation_pos_;
	
	// no copy ctr or assignment op
	ILMethodSpec(const ILMethodSpec &);
	ILMethodSpec &operator =(const ILMethodSpec &);
};

class ILModule : public ILToken
{
public:
	ILModule(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILModule(ILMetaData *meta, ILTable *owner, const std::string &name);
	ILModule(ILMetaData *meta, ILTable *owner, const ILModule &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
private:
	uint16_t generation_;
	std::string name_;
	ILData mv_id_;
	ILData enc_id_;
	ILData enc_base_id_;

	uint32_t name_pos_;
	uint32_t mv_id_pos_;
	uint32_t enc_id_pos_;
	uint32_t enc_base_id_pos_;
};

class ILModuleRef : public ILToken
{
public:
	ILModuleRef(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILModuleRef(ILMetaData *meta, ILTable *owner, const ILModuleRef &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name() const { return name_; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
private:
	std::string name_;
	uint32_t name_pos_;
};

class ILNestedClass : public ILToken
{
public:
	ILNestedClass(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILNestedClass(ILMetaData *meta, ILTable *owner, const ILNestedClass &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	ILTypeDef *nested_type() const { return nested_type_; }
	ILTypeDef *declaring_type() const { return declaring_type_; }
	ILNestedClass *next() const { return reinterpret_cast<ILNestedClass *>(ILToken::next()); }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
private:
	ILTypeDef *nested_type_;
	ILTypeDef *declaring_type_;
};

class ILParam : public ILToken
{
public:
	ILParam(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILParam(ILMetaData *meta, ILTable *owner, const ILParam &src);
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	uint16_t flags() const { return flags_; }
	ILParam *next() const { return reinterpret_cast<ILParam *>(ILToken::next()); }
	ILMethodDef *parent() const { return parent_; }
	void set_parent(ILMethodDef *parent) { parent_ = parent; }
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
private:
	uint16_t flags_;
	uint16_t sequence_;
	std::string name_;
	ILMethodDef *parent_;

	uint32_t name_pos_;
};

class ILPropertyMap;

class ILProperty : public ILToken
{
public:
	ILProperty(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILProperty(ILMetaData *meta, ILTable *owner, const ILProperty &src);
	~ILProperty();
	ILTypeDef *declaring_type() const { return declaring_type_; }
	void set_declaring_type(ILTypeDef *declaring_type) { declaring_type_ = declaring_type; }
	ILProperty *next() const { return reinterpret_cast<ILProperty *>(ILToken::next()); }
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	uint16_t flags() const { return flags_; }
	bool is_exported() const;
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
private:
	uint16_t flags_;
	std::string name_;
	ILSignature *type_;
	ILTypeDef *declaring_type_;

	uint32_t name_pos_;
	uint32_t type_pos_;

	// no copy ctr or assignment op
	ILProperty(const ILProperty &);
	ILProperty &operator =(const ILProperty &);
};

class ILPropertyMap : public ILToken
{
public:
	ILPropertyMap(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILPropertyMap(ILMetaData *meta, ILTable *owner, const ILPropertyMap &src);
	ILPropertyMap *next() const { return reinterpret_cast<ILPropertyMap *>(ILToken::next()); }
	ILTypeDef *parent() const { return parent_; }
	ILProperty *property_list() const { return property_list_; }
	void set_property_list(ILProperty *property_list) { property_list_ = property_list; }
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	bool is_exported() const;
private:
	ILTypeDef *parent_;
	ILProperty *property_list_;
};

class ILStandAloneSig : public ILToken
{
public:
	ILStandAloneSig(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILStandAloneSig(ILMetaData *meta, ILTable *owner, const ILStandAloneSig &src);
	ILStandAloneSig(ILMetaData *meta, ILTable *owner, const ILData &data);
	~ILStandAloneSig();
	virtual ILStandAloneSig *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name(bool mode = false) const;
	ILSignature *signature() const { return signature_; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
private:
	ILSignature *signature_;
	uint32_t signature_pos_;
	
	// no copy ctr or assignment op
	ILStandAloneSig(const ILStandAloneSig &);
	ILStandAloneSig &operator =(const ILStandAloneSig &);
};

class ILTypeDef : public ILToken
{
public:
	ILTypeDef(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILTypeDef(ILMetaData *meta, ILTable *owner, ILToken *base_type, const std::string &name_space, const std::string &name, CorTypeAttr flags = tdNotPublic);
	ILTypeDef(ILMetaData *meta, ILTable *owner, const ILTypeDef &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	CorTypeAttr flags() const { return flags_; }
	std::string name_space() const { return namespace_; }
	void set_namespace(const std::string &name) { namespace_ = name; }
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
	std::string full_name() const;
	std::string reflection_name() const;
	ILField *field_list() const { return field_list_; }
	void set_field_list(ILField *field_list) { field_list_ = field_list; }
	ILMethodDef *method_list() const { return method_list_; }
	void set_method_list(ILMethodDef *method_list) { method_list_ = method_list; }
	ILTypeDef *next() const { return reinterpret_cast<ILTypeDef *>(ILToken::next()); }
	ILTypeDef *declaring_type() const { return declaring_type_; }
	void set_declaring_type(ILTypeDef *declaring_type) { declaring_type_ = declaring_type; }
	ILToken *base_type() const { return base_type_; }
	uint32_t class_size() const { return class_size_; }
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	virtual bool is_exported() const;
	void AddMethod(ILMethodDef *method);
	ILField *GetField(const std::string &name) const;
	ILMethodDef *GetMethod(const std::string &name, ILSignature *signature) const;
	ILProperty *GetProperty(const std::string &name) const;
	ILEvent *GetEvent(const std::string &name) const;
	ILTypeDef *GetNested(const std::string &name) const;
	ILElement *GetEnumUnderlyingType() const;
	bool FindBaseType(const std::string &name) const;
	bool FindImplement(const std::string &name) const;
private:
	CorTypeAttr flags_;
	std::string name_;
	std::string namespace_;
	ILToken *base_type_;
	ILField *field_list_;
	ILMethodDef *method_list_;
	ILTypeDef *declaring_type_;
	uint32_t class_size_;

	uint32_t name_pos_;
	uint32_t namespace_pos_;
};

class ILTypeRef : public ILToken
{
public:
	ILTypeRef(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILTypeRef(ILMetaData *meta, ILTable *owner, const ILTypeRef &src);
	ILTypeRef(ILMetaData *meta, ILTable *owner, ILToken *resolution_scope, const std::string &name_space, const std::string &name);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name_space() const { return namespace_; }
	std::string full_name(bool mode = false) const;
	std::string module_name() const;
	ILTypeRef *declaring_type() const;
	std::string name() const { return name_; }
	ILToken *resolution_scope() const { return resolution_scope_; }  
	virtual void ReadFromFile(NETArchitecture &file);
	virtual void WriteToStreams(ILMetaData &data);
	virtual void WriteToFile(NETArchitecture &file);
	virtual void UpdateTokens();
	virtual void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILTypeDef *Resolve(bool show_error) const;
private:
	ILToken *resolution_scope_;
	std::string name_;
	std::string namespace_;

	uint32_t name_pos_;
	uint32_t namespace_pos_;
};

class ILTypeSpec : public ILToken
{
public:
	ILTypeSpec(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILTypeSpec(ILMetaData *meta, ILTable *owner, const ILTypeSpec &src);
	ILTypeSpec(ILMetaData *meta, ILTable *owner, ILData &data);
	~ILTypeSpec();
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name(bool mode = false) { return signature_->name(mode); }
	virtual void ReadFromFile(NETArchitecture &file);
	void WriteToStreams(ILMetaData &data);
	void WriteToFile(NETArchitecture &file);
	void UpdateTokens();
	void RemapTokens(const std::map<ILToken*, ILToken*> &token_map);
	ILElement *info() const { return signature_; }
private:
	ILElement *signature_;

	uint32_t signature_pos_;
	
	// no copy ctr or assignment op
	ILTypeSpec(const ILTypeSpec &);
	ILTypeSpec &operator =(const ILTypeSpec &);
};

class ILUserString : public ILToken
{
public:
	ILUserString(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILUserString(ILMetaData *meta, ILTable *owner, const ILUserString &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
private:
	std::string name_;
};

class ILExportedType : public ILToken
{
public:
	ILExportedType(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILExportedType(ILMetaData *meta, ILTable *owner, const ILExportedType &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	void WriteToStreams(ILMetaData &data);
	void WriteToFile(NETArchitecture &file);
	void UpdateTokens();
	std::string full_name() const;
	ILExportedType *declaring_type() const;
	ILToken *implementation() const { return implementation_; }
private:
	uint32_t flags_;
	uint32_t type_def_id_;
	std::string name_;
	std::string namespace_;
	ILToken *implementation_;

	uint32_t name_pos_;
	uint32_t namespace_pos_;
};

class ILEncLog : public ILToken
{
public:
	ILEncLog(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILEncLog(ILMetaData *meta, ILTable *owner, const ILEncLog &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	void WriteToFile(NETArchitecture &file);
private:
	uint32_t token_;
	uint32_t func_code_;
};

class ILEncMap : public ILToken
{
public:
	ILEncMap(ILMetaData *meta, ILTable *owner, uint32_t id);
	ILEncMap(ILMetaData *meta, ILTable *owner, const ILEncMap &src);
	virtual ILToken *Clone(ILMetaData *meta, ILTable *owner) const;
	virtual void ReadFromFile(NETArchitecture &file);
	void WriteToFile(NETArchitecture &file);
private:
	uint32_t token_;
};

class ILTable : public ObjectList<ILToken>
{
public:
	ILTable(ILMetaData *meta, ILTokenType type, uint32_t token_count);
	ILTable(ILMetaData *meta, const ILTable &src);
	ILTable *Clone(ILMetaData *meta) const;
	ILTokenType type() const { return type_; }
	void ReadFromFile(NETArchitecture &file);
	void WriteToStreams(ILMetaData &data);
	void WriteToFile(NETArchitecture &file);
	void Rebase(uint64_t delta_base);
	void Pack();
	void UpdateTokens();
protected:
	ILToken *Add(uint32_t value);
	ILMetaData *meta_;
	ILTokenType type_;
};

class ILUserStringsTable : public ILTable
{
public:
	ILUserStringsTable(ILMetaData *meta);
	ILUserStringsTable(ILMetaData *meta, const ILUserStringsTable &src);
	ILUserStringsTable *Clone(ILMetaData *meta) const;
	virtual void AddObject(ILToken *token);
	ILToken *Add(uint32_t id) { return ILTable::Add(id); }
	ILToken *GetTokenByPos(uint32_t pos) const;
private:
	std::map<uint32_t, ILToken *> map_;
};

enum FrameworkType {
	fwUnknown,
	fwFramework,
	fwCore,
	fwStandard
};

struct FrameworkVersion
{
	uint32_t major;
	uint32_t minor;
	uint32_t patch;
	FrameworkVersion()
	{
		major = 0;
		minor = 0;
		patch = 0;
	}
	FrameworkVersion(uint32_t _major, uint32_t _minor, uint32_t _patch)
	{
		major = _major;
		minor = _minor;
		patch = _patch;
	}
	bool Parse(const std::string &version);
	bool operator < (const FrameworkVersion &right) const
	{
		if (major < right.major)
			return true;
		if (major == right.major) {
			if (minor < right.minor)
				return true;
			if (minor == right.minor) {
				if (patch < right.patch)
					return true;
			}
		}
		return false;
	}
};

struct FrameworkInfo
{
	FrameworkType type;
	FrameworkVersion version;
	FrameworkInfo() : type(fwUnknown), version() {}
};

class ILMetaData : public BaseCommandList
{
public:
	explicit ILMetaData(NETArchitecture *owner);
	explicit ILMetaData(NETArchitecture *owner, const ILMetaData &src);
	~ILMetaData();
	ILMetaData *Clone(NETArchitecture *owner) const;
	void ReadFromFile(NETArchitecture &file, uint64_t address);
	void Prepare();
	void WriteToFile(NETArchitecture &file);
	void FreeByManager(MemoryManager &manager);
	uint64_t address() const { return address_; }
	uint32_t size() const { return size_; }
	std::string version() const { return version_; }
	FrameworkInfo framework() const { return framework_; }
	ILStream *item(size_t index) const;
	ILTable *table(ILTokenType type) const;
	ILUserStringsTable *us_table() const { return us_table_; }
	ILToken *token(uint32_t id) const;
	uint32_t token_count(ILTokenType type) const;
	size_t string_field_size() const { return (heap_->offset_sizes() & 1) ? sizeof(uint32_t) : sizeof(uint16_t); }
	size_t guid_field_size() const { return (heap_->offset_sizes() & 2) ? sizeof(uint32_t) : sizeof(uint16_t); }
	size_t blob_field_size() const { return (heap_->offset_sizes() & 4) ? sizeof(uint32_t) : sizeof(uint16_t); }
	size_t string_data_size() const { return strings_ ? strings_->data_size() : 0; }
	size_t guid_data_size() const { return guid_ ? guid_->data_size() : 0; }
	size_t blob_data_size() const { return blob_ ? blob_->data_size() : 0; }
	size_t field_size(ILTokenType type) const;
	size_t field_size(const EncodingDesc &desc) const;
	std::string GetUserString(uint32_t pos) const;
	ILData GetUserData(uint32_t pos, uint64_t *address) const;
	std::string GetString(uint32_t pos) const;
	ILData GetGuid(uint32_t pos) const;
	ILData GetBlob(uint32_t pos) const;
	uint32_t AddUserString(const std::string &str) const;
	uint32_t AddString(const std::string &str) const;
	uint32_t AddGuid(const ILData &data) const;
	uint32_t AddBlob(const ILData &data) const;
	ILStream *GetStreamByName(const std::string &name) const;
	void UpdateTokens();
	virtual void Rebase(uint64_t delta_base);
	ILMethodDef *GetMethod(uint64_t address) const;
	ILStandAloneSig *AddStandAloneSig(const ILData &data);
	ILToken *ImportType(CorElementType type);
	ILToken *ImportType(const ILElement &element);
	ILTypeDef *GetTypeDef(const std::string &name) const;
	ILExportedType *GetExportedType(const std::string &name) const;
	ILAssemblyRef *GetCoreLib() const;
	ILTypeRef *ImportTypeRef(ILToken *resolution_scope, const std::string &name_space, const std::string &name);
	ILTypeDef *AddTypeDef(ILToken *base_type, const std::string &name_space, const std::string &name, CorTypeAttr flags);
	ILMethodDef *AddMethod(ILTypeDef *declaring_type, const std::string &name, const ILData &signature, CorMethodAttr flags, CorMethodImpl impl_flags);
	ILAssemblyRef *GetAssemblyRef(const std::string &name) const;
	ILMetaData *ResolveAssembly(const std::string &name, bool show_error) const;
	ILTypeDef *ResolveType(const std::string &name, bool show_error);
	ILTypeDef *Resolve(const ILTypeRef *type_ref, bool show_error) const;
	ILTypeDef *Resolve(const ILExportedType *exported_type, bool show_error) const;
private:
	ILStream *Add(uint64_t address, uint32_t size, const std::string &name);
	ILToken *GetType(const ILElement &element) const;
	ILToken *AddType(const ILElement &element);
	ILTypeRef *GetTypeRef(ILToken *resolution_scope, const std::string &name_space, const std::string &name) const;
	ILTypeRef *AddTypeRef(ILToken *resolution_scope, const std::string &name_space, const std::string &name);

	uint64_t address_;
	uint32_t size_;
    uint32_t signature_;
    uint16_t major_version_;
    uint16_t minor_version_;
    uint32_t reserved_;
	uint16_t flags_;
	std::string version_;
	FrameworkInfo framework_;

	ILStringsStream *strings_;
	ILUserStringsStream *user_strings_;
	ILBlobStream *blob_;
	ILGuidStream *guid_;
	ILHeapStream *heap_;
	ILUserStringsTable *us_table_;

	// no copy ctr or assignment op
	ILMetaData(const ILMetaData &);
	ILMetaData &operator =(const ILMetaData &);
};

class NETImport;
class NETImportList;

class NETImportFunction : public BaseImportFunction
{
public:
	explicit NETImportFunction(NETImport *owner, uint32_t token, const std::string &name, size_t name_pos);
	explicit NETImportFunction(NETImport *owner, const NETImportFunction &src);
	NETImportFunction *Clone(IImport *owner) const;
	virtual std::string name() const { return name_; }
	virtual uint64_t address() const { return token_; }
	virtual std::string display_name(bool show_ret = true) const { return show_ret ? name_ : name_.substr(name_pos_); }
	virtual void Rebase(uint64_t delta_base) {}
	uint32_t token() const { return token_; }
private:
	uint32_t token_;
	std::string name_;
	size_t name_pos_;
};

class NETImport : public BaseImport
{
public:
	explicit NETImport(NETImportList *owner, uint32_t token, const std::string &name);
	explicit NETImport(NETImportList *owner, const NETImport &src);
	NETImport *Clone(IImportList *owner) const;
	NETImportFunction *item(size_t index) const;
	virtual std::string name() const { return name_; }
	uint32_t token() const { return token_; }
	virtual bool is_sdk() const { return is_sdk_; }
	void set_is_sdk(bool is_sdk) { is_sdk_ = is_sdk; }
	virtual	IImportFunction *Add(uint64_t address, APIType type, MapFunction *map_function) { return NULL; }
	NETImportFunction *Add(uint32_t token, const std::string &name, size_t name_pos);
	NETImportFunction *GetFunctionByToken(uint32_t token) const;
	NETImportFunction *GetFunctionByName(const std::string &name) const;
private:
	bool is_sdk_;
	uint32_t token_;
	std::string name_;
};

class NETImportList : public BaseImportList
{
public:
	explicit NETImportList(NETArchitecture *owner);
	explicit NETImportList(NETArchitecture *owner, const NETImportList &src);
	virtual NETImportList *Clone(NETArchitecture *owner) const;
	NETImport *item(size_t index) const;
	NETImport *GetImportByName(const std::string &name) const;
	void ReadFromFile(NETArchitecture &file);
	NETImportFunction *GetFunctionByToken(uint32_t token) const;
	void Pack();
protected:
	virtual IImport *AddSDK() { return NULL; };
private:
	NETImport *Add(uint32_t token, const std::string &name);
};

class NETExportList;

class NETExport : public BaseExport
{
public:
	explicit NETExport(NETExportList *owner, uint64_t address, const std::string &name, size_t name_pos);
	explicit NETExport(NETExportList *owner, const NETExport &src);
	NETExport *Clone(IExportList *owner) const;
	virtual uint64_t address() const { return address_; }
	virtual std::string name() const { return name_; }
	virtual std::string display_name(bool show_ret = true) const { return show_ret ? name_ : name_.substr(name_pos_); }
	virtual std::string forwarded_name() const { return ""; }
	virtual void Rebase(uint64_t delta_base);
private:
	uint64_t address_;
	std::string name_;
	size_t name_pos_;
};

class NETExportList : public BaseExportList
{
public:
	explicit NETExportList(NETArchitecture *owner);
	explicit NETExportList(NETArchitecture *owner, const NETExportList &src);
	NETExportList *Clone(NETArchitecture *owner) const;
	virtual std::string name() const { return ""; }
	void ReadFromFile(NETArchitecture &file);
	virtual void ReadFromBuffer(Buffer &buffer, IArchitecture &file);
protected:
	virtual IExport *Add(uint64_t address);
private:
	NETExport *Add(uint64_t address, const std::string &name, size_t name_pos);
};

class NETRuntimeFunction : public BaseRuntimeFunction
{
public:
	explicit NETRuntimeFunction(NETRuntimeFunctionList *owner, uint64_t begin, uint64_t end, uint64_t start, ILMethodDef *method);
	explicit NETRuntimeFunction(NETRuntimeFunctionList *owner, const NETRuntimeFunction &src);
	NETRuntimeFunction *Clone(IRuntimeFunctionList *owner) const;
	virtual uint64_t address() const { return 0; }
	virtual uint64_t begin() const { return begin_; }
	virtual uint64_t end() const { return end_; }
	uint64_t start() const { return start_; }
	ILMethodDef *method() const { return method_list_.empty() ? NULL : method_list_[0]; }
	std::vector<ILMethodDef *> method_list() const { return method_list_; }
	void set_method_list(const std::vector<ILMethodDef *> &method_list) { method_list_ = method_list; }
	virtual uint64_t unwind_address() const { return 0; }
	virtual void set_begin(uint64_t begin);
	virtual void set_end(uint64_t end) { end_ = end; }
	virtual void set_unwind_address(uint64_t unwind_address) { }
	virtual void Parse(IArchitecture &file, IFunction &dest) { }
	void Rebase(uint64_t delta_base);
	void AddMethod(ILMethodDef *method) { method_list_.push_back(method); }
private:
	uint64_t begin_;
	uint64_t end_;
	uint64_t start_;
	std::vector<ILMethodDef *> method_list_;
};

class NETRuntimeFunctionList : public BaseRuntimeFunctionList
{
public:
	explicit NETRuntimeFunctionList();
	explicit NETRuntimeFunctionList(const NETRuntimeFunctionList &src);
	NETRuntimeFunctionList *Clone() const;
	NETRuntimeFunction *item(size_t index) const;
	void ReadFromFile(NETArchitecture &file);
	virtual NETRuntimeFunction *GetFunctionByAddress(uint64_t address) const;
	virtual IRuntimeFunction *Add(uint64_t address, uint64_t begin, uint64_t end, uint64_t unwind_address, IRuntimeFunction *source, const std::vector<uint8_t> &call_frame_instructions);
	NETRuntimeFunction *Add(uint64_t begin, uint64_t end, uint64_t start, ILMethodDef *method);
};

class NETResource : public BaseResource
{
public:
	explicit NETResource(IResource *owner, const std::string &name, uint32_t id, uint64_t address, uint32_t size);
	explicit NETResource(IResource *owner, const NETResource &src);
	virtual uint32_t type() const { return 0; }
	virtual uint64_t address() const { return address_; }
	virtual size_t size() const { return size_; }
	virtual std::string name() const { return name_; }
	virtual bool need_store() const { return true; }
	virtual bool is_directory() const { return count() > 0; }
	virtual std::string id() const { return string_format("%.4X", id_); }
	virtual IResource *Clone(IResource *owner) const;
	NETResource *Add(const std::string &name, uint32_t id, uint64_t address, uint32_t size);
	NETResource *item(size_t index) const;
private:
	std::string name_;
	uint32_t id_;
	uint64_t address_;
	size_t size_;
};

class NETResourceList : public BaseResourceList
{
public:
	explicit NETResourceList(NETArchitecture *owner);
	explicit NETResourceList(NETArchitecture *owner, const NETResourceList &src);
	using BaseResourceList::Clone;
	NETResourceList *Clone(NETArchitecture *owner) const;
	void ReadFromFile(NETArchitecture &file);
	NETResource *item(size_t index) const;
private:
	NETResource *Add(const std::string &name, uint32_t id, uint64_t address, uint32_t size);
};

class NETArchitecture : public BaseArchitecture
{
public:
	explicit NETArchitecture(PEFile *owner);
	explicit NETArchitecture(PEFile *owner, const NETArchitecture &src);
	virtual ~NETArchitecture();
	virtual NETArchitecture *Clone(IFile *file) const;
	virtual std::string name() const { return ".NET"; }
	std::string full_name() const;
	virtual uint32_t type() const { return 0; }
	virtual uint64_t image_base() const { return pe_->image_base(); }
	virtual CallingConvention calling_convention() const { return ccStdcall; }
	virtual OperandSize cpu_address_size() const { return pe_->cpu_address_size(); }
	virtual uint64_t entry_point() const { return 0; }
	virtual uint32_t segment_alignment() const { return pe_->segment_alignment(); }
	virtual uint32_t file_alignment() const { return pe_->file_alignment(); }
	uint32_t header_offset() const { return pe_->header_offset(); }
	uint32_t header_size() const { return pe_->header_size(); }
	virtual ILMetaData *command_list() const { return meta_data_; }
	virtual PESegmentList *segment_list() const { return pe_->segment_list(); }
	virtual PESectionList *section_list() const { return pe_->section_list(); }
	virtual NETImportList *import_list() const { return import_list_; }
	virtual NETExportList *export_list() const { return export_list_; }
	virtual PEFixupList *fixup_list() const { return pe_->fixup_list(); }
	virtual IRelocationList *relocation_list() const { return NULL; }
	virtual IFunctionList *function_list() const { return function_list_; }
	virtual IVirtualMachineList *virtual_machine_list() const { return virtual_machine_list_; }
	virtual NETResourceList *resource_list() const { return resource_list_; }
	virtual ISEHandlerList *seh_handler_list() const { return NULL; }
	virtual NETRuntimeFunctionList *runtime_function_list() const { return runtime_function_list_; }
	virtual void Save(CompileContext &ctx);
	virtual bool WriteToFile();
	bool WriteResources(const std::string assembly_name, Data &dest_data);
	virtual bool is_executable() const;
	OpenStatus ReadFromFile(uint32_t mode);
	void Rebase(uint64_t delta_base);
	ILMethodDef *entry_point_method() const { return entry_point_; }
	void set_entry_point_method(ILMethodDef *metod) { entry_point_ = metod; }
	PEArchitecture *pe() const { return pe_; }
	IMAGE_COR20_HEADER header() const { return header_; }
	void RenameToken(ILToken *token);
protected:
	virtual bool Prepare(CompileContext &ctx);
	void RenameSymbols();
private:
	PEArchitecture *pe_;

	IMAGE_COR20_HEADER header_;
	NETImportList *import_list_;
	NETExportList *export_list_;
	NETResourceList *resource_list_;
	NETRuntimeFunctionList *runtime_function_list_;
	ILMetaData *meta_data_;
	ILMethodDef *entry_point_;
	IFunctionList *function_list_;
	IVirtualMachineList *virtual_machine_list_;

	size_t optimized_section_count_;
	uint32_t resize_header_;
	std::set<std::string> rename_map_;
};

class INameReference : public IObject
{
public:
	virtual void UpdateName() = 0;
};

class NameReferenceList;

class BaseNameReference : public INameReference
{
public:
	explicit BaseNameReference(NameReferenceList *owner);
	~BaseNameReference();
private:
	NameReferenceList *owner_;
};

class ResourceNameReference : public BaseNameReference
{
public:
	ResourceNameReference(NameReferenceList *owner, ILManifestResource *resource, ILTypeDef *type, const std::string &add);
	virtual void UpdateName();
private:
	ILManifestResource *resource_;
	ILTypeDef *type_;
	std::string add_;
};

class StringNameReference : public BaseNameReference
{
public:
	StringNameReference(NameReferenceList *owner, ILUserString *string, ILTypeDef *type);
	virtual void UpdateName();
private:
	ILUserString *string_;
	ILTypeDef *type_;
};

class ILCommand;

class CommandNameReference : public BaseNameReference
{
public:
	CommandNameReference(NameReferenceList *owner, ILCommand *command, ILTypeDef *type);
	virtual void UpdateName();
private:
	ILCommand *command_;
	ILTypeDef *type_;
};

class CustomAttributeArgument;
class CustomAttributeNamedArgument;

class CustomAttributeNameReference : public BaseNameReference
{
public:
	CustomAttributeNameReference(NameReferenceList *owner, CustomAttributeNamedArgument *arg, ILToken *token);
	virtual void UpdateName();
private:
	CustomAttributeNamedArgument *arg_;
	ILToken *token_;
};

class MemberReference : public BaseNameReference
{
public:
	MemberReference(NameReferenceList *owner, ILMemberRef *member, ILToken *token);
	virtual void UpdateName();
private:
	ILMemberRef *member_;
	ILToken *token_;
};

class MethodReference : public BaseNameReference
{
public:
	MethodReference(NameReferenceList *owner, ILMethodDef *method, ILMethodDef *source);
	virtual void UpdateName();
private:
	ILMethodDef *method_;
	ILMethodDef *source_;
};

class TypeInfoRecord;

class BamlTypeInfoRecordReference : public BaseNameReference
{
public:
	BamlTypeInfoRecordReference(NameReferenceList *owner, TypeInfoRecord *record, ILToken *token);
	virtual void UpdateName();
private:
	TypeInfoRecord *record_;
	ILToken *token_;
};

class AttributeInfoRecord;

class BamlAttributeInfoRecordReference : public BaseNameReference
{
public:
	BamlAttributeInfoRecordReference(NameReferenceList *owner, AttributeInfoRecord *record, ILToken *token);
	virtual void UpdateName();
private:
	AttributeInfoRecord *record_;
	ILToken *token_;
};

class PropertyRecord;

class BamlPropertyRecordReference : public BaseNameReference
{
public:
	BamlPropertyRecordReference(NameReferenceList *owner, PropertyRecord *record, ILToken *token);
	virtual void UpdateName();
private:
	PropertyRecord *record_;
	ILToken *token_;
};

class NameReferenceList : public ObjectList<INameReference>
{
public:
	NameReferenceList();
	void AddResourceReference(ILManifestResource *resource, ILTypeDef *type, const std::string &format);
	void AddStringReference(ILUserString *string, ILTypeDef *type);
	void AddCommandReference(ILCommand *command, ILTypeDef *type);
	void AddCustomAttributeNameReference(ILCustomAttribute *attribute, CustomAttributeNamedArgument *arg, ILToken *token);
	void AddCustomAttribute(ILCustomAttribute *attribute);
	void AddMemberReference(ILMemberRef *member, ILToken *token);
	void AddTypeInfoRecordReference(ILManifestResource *resource, TypeInfoRecord *record, ILToken *token);
	void AddAttributeInfoRecordReference(ILManifestResource *resource, AttributeInfoRecord *record, ILToken *token);
	void AddPropertyRecordReference(ILManifestResource *resource, PropertyRecord *record, ILToken *token);
	void AddManifestResource(ILManifestResource *resource);
	void AddMethodReference(ILMethodDef *method, ILMethodDef *source);
	void UpdateNames();
private:
	std::set<ILCustomAttribute *> custom_attribute_list_;
	std::set<ILManifestResource *> manifest_resource_list_;
};

class NETStream : public MemoryStream
{
public:
	NETStream();
	NETStream(AbstractStream &stream, size_t size);
	virtual size_t Read(void *Buffer, size_t Count);
	uint8_t ReadByte();
	uint16_t ReadWord();
	uint32_t ReadDWord();
	bool ReadBoolean();
	uint32_t ReadEncoded();
	uint32_t ReadEncoded7Bit();
	std::string ReadString();
	std::string ReadUnicodeString();
	void WriteByte(uint8_t value);
	void WriteWord(uint16_t value);
	void WriteDWord(uint32_t value);
	void WriteEncoded7Bit(uint32_t value);
	void WriteString(const std::string &value);
	void WriteBoolean(bool value);
	void WriteUnicodeString(const os::unicode_string &value);
	void WriteEncoded(uint32_t value);
};

class BamlDocument;

enum BamlRecordType : uint8_t {
	ClrEvent = 0x13,
	Comment = 0x17,
	AssemblyInfo = 0x1c,
	AttributeInfo = 0x1f,
	ConstructorParametersStart = 0x2a,
	ConstructorParametersEnd = 0x2b,
	ConstructorParameterType = 0x2c,
	ConnectionId = 0x2d,
	ContentProperty = 0x2e,
	DefAttribute = 0x19,
	DefAttributeKeyString = 0x26,
	DefAttributeKeyType = 0x27,
	DeferableContentStart = 0x25,
	DefTag = 0x18,
	DocumentEnd = 0x2,
	DocumentStart = 0x1,
	ElementEnd = 0x4,
	ElementStart = 0x3,
	EndAttributes = 0x1a,
	KeyElementEnd = 0x29,
	KeyElementStart = 0x28,
	LastRecordType = 0x39,
	LineNumberAndPosition = 0x35,
	LinePosition = 0x36,
	LiteralContent = 0xf,
	NamedElementStart = 0x2f,
	OptimizedStaticResource = 0x37,
	PIMapping = 0x1b,
	PresentationOptionsAttribute = 0x34,
	ProcessingInstruction = 0x16,
	Property = 0x5,
	PropertyArrayEnd = 0xa,
	PropertyArrayStart = 0x9,
	PropertyComplexEnd = 0x8,
	PropertyComplexStart = 0x7,
	PropertyCustom = 0x6,
	PropertyDictionaryEnd = 0xe,
	PropertyDictionaryStart = 0xd,
	PropertyListEnd = 0xc,
	PropertyListStart = 0xb,
	PropertyStringReference = 0x21,
	PropertyTypeReference = 0x22,
	PropertyWithConverter = 0x24,
	PropertyWithExtension = 0x23,
	PropertyWithStaticResourceId = 0x38,
	RoutedEvent = 0x12,
	StaticResourceEnd = 0x31,
	StaticResourceId = 0x32,
	StaticResourceStart = 0x30,
	StringInfo = 0x20,
	Text = 0x10,
	TextWithConverter = 0x11,
	TextWithId = 0x33,
	TypeInfo = 0x1d,
	TypeSerializerInfo = 0x1e,
	XmlAttribute = 0x15,
	XmlnsProperty = 0x14
};

class BamlRecord : public IObject
{
public:
	BamlRecord(BamlDocument *owner);
	~BamlRecord();
	virtual BamlRecordType type() const = 0;
	uint64_t position() const { return position_; }
	void set_position(uint64_t pos) { position_ = pos; }
	virtual void Read(NETStream &stream) {}
	virtual void ReadDefer(size_t index) {}
	virtual void Write(NETStream &stream) {}
	virtual void WriteDefer(size_t index, NETStream &stream) {}
	BamlDocument *owner() const { return owner_; }
protected:
	uint64_t GetPosition(size_t index);
private:
	BamlDocument *owner_;
	uint64_t position_;
};

class SizedBamlRecord : public BamlRecord
{
public:
	SizedBamlRecord(BamlDocument *owner);
	uint32_t data_size() const { return data_size_; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
	virtual void ReadData(NETStream &stream) = 0;
	virtual void WriteData(NETStream &stream) = 0;
private:
	uint32_t data_size_;
};

class DocumentStartRecord : public BamlRecord
{
public:
	DocumentStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return DocumentStart; }
	void Read(NETStream &file);
	void Write(NETStream &file);
private:
	bool load_async_;
	uint32_t max_async_records_;
	bool debug_baml_;
};

class DocumentEndRecord : public BamlRecord
{
public:
	DocumentEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return DocumentEnd; }
};

class ElementStartRecord : public BamlRecord
{
public:
	ElementStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return ElementStart; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
	uint16_t type_id() const { return type_id_; }
private:
	uint16_t type_id_;
	uint8_t flags_;
};

class ElementEndRecord : public BamlRecord
{
public:
	ElementEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return ElementEnd; }
};

class AssemblyInfoRecord : public SizedBamlRecord
{
public:
	AssemblyInfoRecord(BamlDocument *owner);
	BamlRecordType type() const { return AssemblyInfo; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
	uint16_t assembly_id() const { return assembly_id_; }
	std::string assembly_name() const { return assembly_name_; }
private:
	uint16_t assembly_id_;
	std::string assembly_name_;
};

class TypeInfoRecord : public SizedBamlRecord
{
public:
	TypeInfoRecord(BamlDocument *owner);
	BamlRecordType type() const { return TypeInfo; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
	uint16_t type_id() const { return type_id_; }
	uint16_t assembly_id() const { return assembly_id_; }
	std::string type_name() const { return type_name_; }
	void set_type_name(const std::string &name) { type_name_ = name; }
private:
	uint16_t type_id_;
	uint16_t assembly_id_;
	std::string type_name_;
};

class XmlnsPropertyRecord : public SizedBamlRecord
{
public:
	XmlnsPropertyRecord(BamlDocument *owner);
	BamlRecordType type() const { return XmlnsProperty; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	std::string prefix_;
	std::string xml_namespace_;
	std::vector<uint16_t> assembly_ids_;
};

class AttributeInfoRecord : public SizedBamlRecord
{
public:
	AttributeInfoRecord(BamlDocument *owner);
	BamlRecordType type() const { return AttributeInfo; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
	uint16_t attribute_id() const { return attribute_id_; }
	uint16_t owner_type_id() const { return owner_type_id_; }
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
private:
	uint16_t attribute_id_;
	uint16_t owner_type_id_;
	uint8_t attribute_usage_;
	std::string name_;
};

class PropertyDictionaryStartRecord : public BamlRecord
{
public:
	PropertyDictionaryStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyDictionaryStart; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t attribute_id_;
};

class PropertyDictionaryEndRecord : public BamlRecord
{
public:
	PropertyDictionaryEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyDictionaryEnd; }
};

class StringInfoRecord : public SizedBamlRecord
{
public:
	StringInfoRecord(BamlDocument *owner);
	BamlRecordType type() const { return StringInfo; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	uint16_t string_id_;
	std::string value_;
};

class DeferableContentStartRecord : public BamlRecord
{
public:
	DeferableContentStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return DeferableContentStart; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
	void ReadDefer(size_t index);
	void WriteDefer(size_t index, NETStream &stream);
private:
	uint32_t offset_;
	uint64_t offset_pos_;
	BamlRecord *record_;
};

class DefAttributeKeyStringRecord : public SizedBamlRecord
{
public:
	DefAttributeKeyStringRecord(BamlDocument *owner);
	BamlRecordType type() const { return DefAttributeKeyString; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
	void ReadDefer(size_t index);
	void WriteDefer(size_t index, NETStream &stream);
private:
	uint16_t value_id_;
	uint32_t offset_;
	uint32_t offset_pos_;
	bool shared_;
	bool shared_set_;
	BamlRecord *record_;
};

class DefAttributeKeyTypeRecord : public ElementStartRecord
{
public:
	DefAttributeKeyTypeRecord(BamlDocument *owner);
	BamlRecordType type() const { return DefAttributeKeyType; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
	void ReadDefer(size_t index);
	void WriteDefer(size_t index, NETStream &stream);
private:
	uint32_t offset_;
	bool shared_;
	bool shared_set_;
	uint64_t offset_pos_;
	BamlRecord *record_;
};

class PropertyComplexStartRecord : public BamlRecord
{
public:
	PropertyComplexStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyComplexStart; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t attribute_id_;
};

class PropertyComplexEndRecord : public BamlRecord
{
public:
	PropertyComplexEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyComplexEnd; }
};

class PropertyTypeReferenceRecord : public PropertyComplexStartRecord
{
public:
	PropertyTypeReferenceRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyTypeReference; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t type_id_;
};

class ContentPropertyRecord : public BamlRecord
{
public:
	ContentPropertyRecord(BamlDocument *owner);
	BamlRecordType type() const { return ContentProperty; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t attribute_id_;
};

class PropertyCustomRecord : public SizedBamlRecord
{
public:
	PropertyCustomRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyCustom; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	uint16_t attribute_id_;
	uint16_t serializer_type_id_;
	std::vector<uint8_t> data_;
};

class PropertyRecord : public SizedBamlRecord
{
public:
	PropertyRecord(BamlDocument *owner);
	BamlRecordType type() const { return Property; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
	uint16_t attribute_id() const { return attribute_id_; }
	std::string value() const { return value_; }
	void set_value(const std::string &value) { value_ = value; }
private:
	uint16_t attribute_id_;
	std::string value_;
};

class PropertyWithConverterRecord : public PropertyRecord
{
public:
	PropertyWithConverterRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyWithConverter; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	uint16_t converter_type_id_;
};

class PIMappingRecord : public SizedBamlRecord
{
public:
	PIMappingRecord(BamlDocument *owner);
	BamlRecordType type() const { return PIMapping; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	std::string xml_namespace_;
	std::string clr_namespace_;
	uint16_t assembly_id_;
};

class PropertyListStartRecord : public PropertyComplexStartRecord
{
public:
	PropertyListStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyListStart; }
};

class PropertyListEndRecord : public BamlRecord
{
public:
	PropertyListEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyListEnd; }
};

class ConnectionIdRecord : public BamlRecord
{
public:
	ConnectionIdRecord(BamlDocument *owner);
	BamlRecordType type() const { return ConnectionId; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint32_t connection_id_;
};

class OptimizedStaticResourceRecord : public BamlRecord
{
public:
	OptimizedStaticResourceRecord(BamlDocument *owner);
	BamlRecordType type() const { return OptimizedStaticResource; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint8_t flags_;
	uint16_t value_id_;
};

class ConstructorParametersStartRecord : public BamlRecord
{
public:
	ConstructorParametersStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return ConstructorParametersStart; }
};

class ConstructorParametersEndRecord : public BamlRecord
{
public:
	ConstructorParametersEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return ConstructorParametersEnd; }
};

class TextRecord : public SizedBamlRecord
{
public:
	TextRecord(BamlDocument *owner);
	BamlRecordType type() const { return Text; }
	std::string value() const { return value_; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	std::string value_;
};

class StaticResourceIdRecord : public BamlRecord
{
public:
	StaticResourceIdRecord(BamlDocument *owner);
	BamlRecordType type() const { return StaticResourceId; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t static_resource_id_;
};

class StaticResourceEndRecord : public BamlRecord
{
public:
	StaticResourceEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return StaticResourceEnd; }
};

class StaticResourceStartRecord : public ElementStartRecord
{
public:
	StaticResourceStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return StaticResourceStart; }
};

class PropertyWithStaticResourceIdRecord : public StaticResourceIdRecord
{
public:
	PropertyWithStaticResourceIdRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyWithStaticResourceId; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t attribute_id_;
};

class PropertyWithExtensionRecord : public BamlRecord
{
public:
	PropertyWithExtensionRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyWithExtension; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t attribute_id_;
	uint16_t flags_;
	uint16_t value_id_;
};

class DefAttributeRecord : public SizedBamlRecord
{
public:
	DefAttributeRecord(BamlDocument *owner);
	BamlRecordType type() const { return DefAttribute; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	std::string value_;
	uint16_t name_id_;
};

class KeyElementStartRecord : public DefAttributeKeyTypeRecord
{
public:
	KeyElementStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return KeyElementStart; }
};

class KeyElementEndRecord : public BamlRecord
{
public:
	KeyElementEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return KeyElementEnd; }
};

class ConstructorParameterTypeRecord : public BamlRecord
{
public:
	ConstructorParameterTypeRecord(BamlDocument *owner);
	BamlRecordType type() const { return ConstructorParameterType; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint16_t type_id_;
};

class TextWithConverterRecord : public TextRecord
{
public:
	TextWithConverterRecord(BamlDocument *owner);
	BamlRecordType type() const { return TextWithConverter; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	uint16_t converter_type_id_;
};

class TextWithIdRecord : public TextRecord
{
public:
	TextWithIdRecord(BamlDocument *owner);
	BamlRecordType type() const { return TextWithId; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	uint16_t value_id_;
};

class LineNumberAndPositionRecord : public BamlRecord
{
public:
	LineNumberAndPositionRecord(BamlDocument *owner);
	BamlRecordType type() const { return LineNumberAndPosition; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint32_t line_number_;
	uint32_t line_position_;
};

class LinePositionRecord : public BamlRecord
{
public:
	LinePositionRecord(BamlDocument *owner);
	BamlRecordType type() const { return LinePosition; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	uint32_t line_position_;
};

class LiteralContentRecord : public SizedBamlRecord
{
public:
	LiteralContentRecord(BamlDocument *owner);
	BamlRecordType type() const { return LiteralContent; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	std::string value_;
	uint32_t reserved0_;
	uint32_t reserved1_;
};

class PresentationOptionsAttributeRecord : public SizedBamlRecord
{
public:
	PresentationOptionsAttributeRecord(BamlDocument *owner);
	BamlRecordType type() const { return PresentationOptionsAttribute; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	std::string value_;
	uint16_t name_id_;
};

class PropertyArrayStartRecord : public PropertyComplexStartRecord
{
public:
	PropertyArrayStartRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyArrayStart; }
};

class PropertyArrayEndRecord : public BamlRecord
{
public:
	PropertyArrayEndRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyArrayEnd; }
};

class PropertyStringReferenceRecord : public PropertyComplexStartRecord
{
public:
	PropertyStringReferenceRecord(BamlDocument *owner);
	BamlRecordType type() const { return PropertyStringReference; }
	void Read(NETStream &stream);
	void Write(NETStream &stream);
private:
	std::string value_;
	uint16_t string_id_;
};

class RoutedEventRecord : public SizedBamlRecord
{
public:
	RoutedEventRecord(BamlDocument *owner);
	BamlRecordType type() const { return RoutedEvent; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	uint16_t attribute_id_;
	std::string value_;
};

class TypeSerializerInfoRecord : public TypeInfoRecord
{
public:
	TypeSerializerInfoRecord(BamlDocument *owner);
	BamlRecordType type() const { return TypeSerializerInfo; }
	void ReadData(NETStream &stream);
	void WriteData(NETStream &stream);
private:
	uint16_t type_id_;
};

class BamlDocument : public ObjectList<BamlRecord>
{
public:
	BamlDocument();
	void clear();
	bool ReadFromStream(NETStream &stream);
	void WriteToStream(NETStream &stream);
	BamlRecord *GetRecordByPosition(uint64_t pos) const;
	size_t GetClosedIndex(BamlRecordType start, BamlRecordType end, size_t index);
private:
	std::vector<os::unicode_char> signature_;
	uint16_t reader_version_major_;
	uint16_t reader_version_minor_;
	uint16_t updater_version_major_;
	uint16_t updater_version_minor_;
	uint16_t writer_version_major_;
	uint16_t writer_version_minor_;
};

class BamlElement : public IObject
{
public:
	BamlElement(BamlElement *parent);
	~BamlElement();
	BamlElement *parent() const { return parent_; }
	BamlRecord *header() const { return header_; }
	BamlRecord *footer() const { return footer_; }
	std::vector<BamlElement *> children() const { return children_; }
	std::vector<BamlRecord *> body() const { return body_; }
	bool Parse(BamlDocument *document);
	ILTypeDef *type() const { return type_; }
	void set_type(ILTypeDef *type) { type_ = type; }
private:
	static bool is_header(BamlRecordType type);
	static bool is_footer(BamlRecordType type);
	static bool is_match(BamlRecordType header, BamlRecordType footer);

	BamlElement *parent_;
	BamlRecord *header_;
	BamlRecord *footer_;
	std::vector<BamlElement *> children_;
	std::vector<BamlRecord *> body_;
	ILTypeDef *type_;
};

class CustomAttributeArgumentList;
class CustomAttributeNamedArgumentList;
class CustomAttributeFixedArgument;

class CustomAttributeValue : public IObject
{
public:
	CustomAttributeValue(ILMetaData *meta);
	~CustomAttributeValue();
	void clear();
	void Parse(ILToken *type, const ILData &data);
	void Write(ILData &data);
	CustomAttributeArgumentList *fixed_list() const { return fixed_list_; }
	CustomAttributeNamedArgumentList *named_list() const { return named_list_; }
private:
	ILMetaData *meta_;
	CustomAttributeArgumentList *fixed_list_;
	CustomAttributeNamedArgumentList *named_list_;
};

class CustomAttributeArgument : public IObject
{
public:
	CustomAttributeArgument(ILMetaData *meta, CustomAttributeArgumentList *owner);
	~CustomAttributeArgument();
	virtual ILElement *type() const = 0;
	ILTypeDef *reference() const { return reference_; }
	CustomAttributeArgumentList *children() const { return children_; }
	CustomAttributeArgumentList *owner() const { return owner_; }
	virtual void Read(const ILData &data, uint32_t &pos);
	virtual void Write(ILData &data) const;
	void set_value(ILData &data) { value_ = data; }
	bool ToBoolean() const;
	std::string ToString() const;
protected:
	CustomAttributeFixedArgument *AddChild(ILElement *type);
	ILElement *ReadFieldOrPropType(const ILData &data, uint32_t &id);
	void ReadEnum(const ILData &data, uint32_t &id, ILToken *type);
	void ReadObject(const ILData &data, uint32_t &id);
	void ReadArray(const ILData &data, uint32_t &pos, ILElement *type);
	void ReadValue(const ILData &data, uint32_t &pos, ILElement *type);
	std::string ReadString(const ILData &data, uint32_t &id);
	ILTypeDef *ReadType(const ILData &data, uint32_t &id);

	void WriteFieldOrPropType(ILData &data, ILElement *type) const;
	void WriteArray(ILData &data) const;
	void WriteObject(ILData &data) const;
	void WriteType(ILData &data, ILTypeDef *token) const;
	void WriteValue(ILData &data, ILElement *type) const;
private:
	ILMetaData *meta_;
	CustomAttributeArgumentList *owner_;
	ILData value_;
	ILTypeDef *reference_;
	CustomAttributeArgumentList *children_;
};

class CustomAttributeFixedArgument : public CustomAttributeArgument
{
public:
	CustomAttributeFixedArgument(ILMetaData *meta, CustomAttributeArgumentList *owner, ILElement *type);
	virtual ILElement *type() const { return type_; }
private:
	ILElement *type_;
};

class CustomAttributeArgumentList : public ObjectList<CustomAttributeArgument>
{
public:
	CustomAttributeArgumentList();
};

class CustomAttributeNamedArgument : public CustomAttributeArgument
{
public:
	CustomAttributeNamedArgument(ILMetaData *meta, CustomAttributeArgumentList *owner);
	~CustomAttributeNamedArgument();
	virtual ILElement *type() const { return type_; }
	virtual void Read(const ILData &data, uint32_t &pos);
	virtual void Write(ILData &data) const;
	bool is_field() const { return is_field_; }
	std::string name() const { return name_; }
	void set_name(const std::string &name) { name_ = name; }
private:
	bool is_field_;
	std::string name_;
	ILElement *type_;
};

class CustomAttributeNamedArgumentList : public CustomAttributeArgumentList
{
public:
	CustomAttributeNamedArgumentList();
	CustomAttributeNamedArgument *item(size_t index) const;
};

struct FrameworkPathInfo
{
	std::string path;
	FrameworkVersion version;
	FrameworkPathInfo() {}
	FrameworkPathInfo(std::string _path, FrameworkVersion _version) : path(_path), version(_version) {}
};

struct FrameworkRedirectInfo
{
	std::string public_key_token;
	std::string version;
	FrameworkRedirectInfo() {}
	FrameworkRedirectInfo(const std::string &_public_key_token, const std::string &_version) : public_key_token(_public_key_token), version(_version) {}
};

struct AssemblyName
{
	std::string name;
	std::string version;
	std::string culture;
	std::string public_key_token;
	AssemblyName(const std::string &name);
	std::string value() const;
};

class AssemblyResolver : public IObject
{
public:
	AssemblyResolver();
	~AssemblyResolver();
	ILMetaData *Resolve(const ILMetaData &source, const std::string &name);
	void Prepare();
private:
	std::vector<std::string> FindAssemblies(const ILMetaData &source, const AssemblyName &name);

	std::string win_dir_;
	std::vector<FrameworkPathInfo> dotnet_dir_list_;

	struct ci_less : std::binary_function<std::string, std::string, bool>
	{
		bool operator() (const std::string &s1, const std::string &s2) const
		{
			return _strcmpi(s1.c_str(), s2.c_str()) < 0;
		}
	};
	std::map<std::string, PEFile *, ci_less> cache_;
	std::map<std::string, FrameworkRedirectInfo, ci_less> redirect_v2_map_;
	std::map<std::string, FrameworkRedirectInfo, ci_less> redirect_v4_map_;
	std::map<std::string, FrameworkInfo> framework_path_map_;
};

#endif