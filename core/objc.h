#ifndef OBJC_H
#define OBJC_H

struct objc_module {
	uint32_t	version;
	uint32_t	size;
	uint32_t	name;
	uint32_t	symtab;	
};

struct objc_module_64 {
	uint32_t	version;
	uint32_t	size;
	uint64_t	name;
	uint64_t	symtab;	
};

struct objc_symtab {
	uint32_t	sel_ref_cnt;
	uint32_t	refs;		
	uint16_t	cls_def_cnt;
	uint16_t	cat_def_cnt;
};

struct objc_symtab_64 {
	uint32_t	sel_ref_cnt;
	uint64_t	refs;		
	uint16_t	cls_def_cnt;
	uint16_t	cat_def_cnt;
};

struct objc_class {			
	uint32_t isa;	
	uint32_t super_class;	
	uint32_t name;
	uint32_t version;
	uint32_t info;
	uint32_t instance_size;
	uint32_t ivars;
	uint32_t methods;
	uint32_t cache;
 	uint32_t protocols;
};

struct objc_class_64 {			
	uint64_t isa;	
	uint64_t super_class;	
	uint64_t name;		
	uint64_t version;
	uint64_t info;
	uint64_t instance_size;
	uint64_t ivars;
	uint64_t methods;
	uint64_t cache;
 	uint64_t protocols;
};

struct objc_protocol {
	uint32_t isa;
	uint32_t name;
	uint32_t protocols;
	uint32_t instance_methods;
	uint32_t class_methods;
};

struct objc_protocol_64 {
	uint64_t isa;
	uint64_t name;
	uint64_t protocols;
	uint64_t instance_methods;
	uint64_t class_methods;
};

struct objc_method {
	uint32_t name;
	uint32_t types;
	uint32_t imp;
};

struct objc_method_64 {
	uint64_t name;
	uint64_t types;
	uint64_t imp;
};

struct objc_method_list {
	uint32_t next;
	uint32_t count;
	// objc_method method[1];
};

struct objc_category {
	uint32_t category_name;
	uint32_t class_name;
	uint32_t instance_methods;
	uint32_t class_methods;
	uint32_t protocols;
};

struct objc_category_64 {
	uint64_t category_name;
	uint64_t class_name;
	uint64_t instance_methods;
	uint64_t class_methods;
	uint64_t protocols;
};

struct objc2_class {
	uint32_t isa;
    uint32_t super_class;
    uint32_t cache;
    uint32_t vtable;
    uint32_t data;
};

struct objc2_class_64 {
	uint64_t isa;
	uint64_t super_class;
	uint64_t cache;
	uint64_t vtable;
	uint64_t data;
};

struct objc2_class_data {
	uint32_t flags;
	uint32_t instance_start;
	uint32_t instance_size;
	uint32_t ivar_layout;
	uint32_t name;
	uint32_t base_methods;
	uint32_t base_protocols;
	uint32_t ivars;
	uint32_t weak_ivar_layout;
	uint32_t base_properties;
};

struct objc2_class_data_64 {
	uint32_t flags;
	uint32_t instance_start;
	uint32_t instance_size;
	uint32_t reserved;
	uint64_t ivar_layout;
	uint64_t name;
	uint64_t base_methods;
	uint64_t base_protocols;
	uint64_t ivars;
	uint64_t weak_ivar_layout;
	uint64_t base_properties;
};

struct objc2_method {
	uint32_t name;
	uint32_t types;
	uint32_t imp;
};

struct objc2_method_64 {
	uint64_t name;
	uint64_t types;
	uint64_t imp;
};

struct objc2_category {
	uint32_t name;
	uint32_t cls;
	uint32_t instance_methods;
	uint32_t class_methods;
	uint32_t protocols;
	uint32_t instance_properties;
};

struct objc2_category_64 {
	uint64_t name;
	uint64_t cls;
	uint64_t instance_methods;
	uint64_t class_methods;
	uint64_t protocols;
	uint64_t instance_properties;
};

struct objc2_protocol {
	uint32_t isa;
	uint32_t name;
	uint32_t protocols;
	uint32_t instance_methods;
	uint32_t class_methods;
	uint32_t optional_instance_methods;
	uint32_t optional_class_methods;
	uint32_t instance_properties;
};

struct objc2_protocol_64 {
	uint64_t isa;
	uint64_t name;
	uint64_t protocols;
	uint64_t instance_methods;
	uint64_t class_methods;
	uint64_t optional_instance_methods;
	uint64_t optional_class_methods;
	uint64_t instance_properties;
};

struct objc2_message {
	uint32_t imp;
	uint32_t name;
};

struct objc2_message_64 {
	uint64_t imp;
	uint64_t name;
};

class MacArchitecture;
class MacSegment;

class IObjcMethod : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcMethodList : public ObjectList<IObjcMethod>
{
public:
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcClass : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcClassList : public ObjectList<IObjcClass>
{
public:
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcCategory : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcCategoryList : public ObjectList<IObjcCategory>
{
public:
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcSelector : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcSelectorList : public ObjectList<IObjcSelector>
{
public:
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcProtocol : public IObject
{
public:
	virtual uint64_t address() const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcProtocolList : public ObjectList<IObjcProtocol>
{
public:
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
};

class IObjcStorage : public IObject
{
public:
	virtual void ReadFromFile(MacArchitecture &file) = 0;
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const = 0;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const = 0;
	virtual std::vector<MacSegment *> segment_list() const = 0;
};

class BaseObjcClass : public IObjcClass
{
public:
	BaseObjcClass(IObjcClassList *owner, uint64_t address);
	virtual ~BaseObjcClass();
	virtual uint64_t address() const { return address_; }
private:
	IObjcClassList *owner_;
	uint64_t address_;
};

class BaseObjcClassList : public IObjcClassList
{
public:
	virtual void AddObject(IObjcClass *object);
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
	IObjcClass *GetClassByAddress(uint64_t address) const;
private:
	std::map<uint64_t, IObjcClass *> map_;
};

class BaseObjcMethod : public IObjcMethod
{
public:
	BaseObjcMethod(IObjcMethodList *owner, uint64_t address);
	virtual ~BaseObjcMethod();
	virtual uint64_t address() const { return address_; }
private:
	IObjcMethodList *owner_;
	uint64_t address_;
};

class BaseObjcMethodList : public IObjcMethodList
{
public:
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
};

class BaseObjcCategory : public IObjcCategory
{
public:
	BaseObjcCategory(IObjcCategoryList *owner, uint64_t address);
	virtual ~BaseObjcCategory();
	virtual uint64_t address() const { return address_; }
private:
	IObjcCategoryList *owner_;
	uint64_t address_;
};

class BaseObjcCategoryList : public IObjcCategoryList
{
public:
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
};

class BaseObjcSelector : public IObjcSelector
{
public:
	BaseObjcSelector(IObjcSelectorList *owner, uint64_t address);
	virtual ~BaseObjcSelector();
	virtual uint64_t address() const { return address_; }
private:
	IObjcSelectorList *owner_;
	uint64_t address_;
};

class BaseObjcSelectorList : public IObjcSelectorList
{
public:
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
};

class BaseObjcProtocol : public IObjcProtocol
{
public:
	BaseObjcProtocol(IObjcProtocolList *owner, uint64_t address);
	virtual ~BaseObjcProtocol();
	virtual uint64_t address() const { return address_; }
private:
	IObjcProtocolList *owner_;
	uint64_t address_;
};

class BaseObjcProtocolList : public IObjcProtocolList
{
public:
	IObjcProtocol *GetProtocolByAddress(uint64_t address) const;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
};

enum ObjcMethodType {
	mtUnknown,
	mtLoad
};

class ObjcMethod : public BaseObjcMethod
{
public:
	ObjcMethod(IObjcMethodList *owner, uint64_t address);
	void ReadFromFile(MacArchitecture &file);
	void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t name_;
	uint64_t types_;
	uint64_t imp_;
	ObjcMethodType type_;
};

class ObjcMethodList : public BaseObjcMethodList
{
public:
	ObjcMethodList();
	ObjcMethod *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file, uint64_t address);
private:
	ObjcMethod *Add(uint64_t address);
};

class ObjcClass : public BaseObjcClass
{
public:
	ObjcClass(IObjcClassList *owner, uint64_t address);
	~ObjcClass();
	void ReadFromFile(MacArchitecture &file);
	uint64_t isa() const { return isa_; }
	uint64_t super_class() const { return super_class_; }
	uint64_t info() const { return info_; }
	void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t isa_;
	uint64_t super_class_;
	uint64_t name_;
	uint64_t version_;
	uint64_t info_;
	uint64_t instance_size_;
	uint64_t ivars_;
	uint64_t methods_;
	uint64_t cache_;
 	uint64_t protocols_;
	ObjcMethodList *method_list_;
	
	// no copy ctr or assignment op
	ObjcClass(const ObjcClass &);
	ObjcClass &operator =(const ObjcClass &);
};

class ObjcClassList : public BaseObjcClassList
{
public:
	ObjcClassList();
	ObjcClass *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file, size_t class_count);
private:
	ObjcClass *Add(uint64_t address);
};

class ObjcCategory: public BaseObjcCategory
{
public:
	ObjcCategory(IObjcCategoryList *owner, uint64_t address);
	~ObjcCategory();
	void ReadFromFile(MacArchitecture &file);
	void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t category_name_;
	uint64_t class_name_;
	uint64_t instance_methods_;
	uint64_t class_methods_;
 	uint64_t protocols_;
	ObjcMethodList *method_list_;
	ObjcMethodList *class_method_list_;
	
	// no copy ctr or assignment op
	ObjcCategory(const ObjcCategory &);
	ObjcCategory &operator =(const ObjcCategory &);
};

class ObjcCategoryList : public BaseObjcCategoryList
{
public:
	ObjcCategoryList();
	ObjcCategory *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file, size_t category_count);
private:
	ObjcCategory *Add(uint64_t address);
};

class ObjcSelector : public BaseObjcSelector
{
public:
	ObjcSelector(IObjcSelectorList *owner, uint64_t address);
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
};

class ObjcSelectorList : public BaseObjcSelectorList
{
public:
	ObjcSelectorList();
	void ReadFromFile(MacArchitecture &file);
private:
	ObjcSelector *Add(uint64_t address);
};

class ObjcProtocolList;

class ObjcProtocol : public BaseObjcProtocol
{
public:
	ObjcProtocol(ObjcProtocolList *owner, uint64_t address);
	void ReadFromFile(MacArchitecture &file);
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t isa_;
	uint64_t name_;
	uint64_t protocols_;
	uint64_t instance_methods_;
	uint64_t class_methods_;
};

class ObjcProtocolList : public BaseObjcProtocolList
{
public:
	ObjcProtocolList();
	ObjcProtocol *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file);
private:
	ObjcProtocol *Add(uint64_t address);
};

class ObjcStorage : public IObjcStorage
{
public:
	ObjcStorage();
	~ObjcStorage();
	virtual void ReadFromFile(MacArchitecture &file);
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
	virtual std::vector<MacSegment *> segment_list() const { return segment_list_; }
private:
	ObjcClassList *class_list_;
	ObjcCategoryList *category_list_;
	ObjcSelectorList *selector_list_;
	ObjcProtocolList *protocol_list_;
	std::vector<MacSegment *> segment_list_;
	
	// no copy ctr or assignment op
	ObjcStorage(const ObjcStorage &);
	ObjcStorage &operator =(const ObjcStorage &);
};

class Objc2Method : public BaseObjcMethod
{
public:
	Objc2Method(IObjcMethodList *owner, uint64_t address);
	void ReadFromFile(MacArchitecture &file);
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t name_;
	uint64_t types_;
	uint64_t imp_;
	ObjcMethodType type_;
};

class Objc2MethodList : public BaseObjcMethodList
{
public:
	Objc2MethodList();
	Objc2Method *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file, uint64_t address);
private:
	Objc2Method *Add(uint64_t address);
};

class Objc2Class : public BaseObjcClass
{
public:
	Objc2Class(IObjcClassList *owner, uint64_t address);
	~Objc2Class();
	void ReadFromFile(MacArchitecture &file);
	uint64_t isa() const { return isa_; }
	uint64_t super_class() const { return super_class_; }
	void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t isa_;
	uint64_t super_class_;
	uint64_t cache_;
	uint64_t vtable_;
	uint64_t data_;
	uint32_t flags_;
	uint32_t instance_start_;
	uint32_t instance_size_;
	uint64_t ivar_layout_;
	uint64_t name_;
	uint64_t base_methods_;
	uint64_t base_protocols_;
	uint64_t ivars_;
	uint64_t protocols_;
	uint64_t weak_ivar_layout_;
	uint64_t base_properties_;
	Objc2MethodList *method_list_;
	
	// no copy ctr or assignment op
	Objc2Class(const Objc2Class &);
	Objc2Class &operator =(const Objc2Class &);
};

class Objc2ClassList : public BaseObjcClassList
{
public:
	Objc2ClassList();
	Objc2Class *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file);
private:
	Objc2Class *Add(uint64_t address);
};

class Objc2Category: public BaseObjcCategory
{
public:
	Objc2Category(IObjcCategoryList *owner, uint64_t address);
	~Objc2Category();
	void ReadFromFile(MacArchitecture &file);
	void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t name_;
	uint64_t cls_;
	uint64_t instance_methods_;
	uint64_t class_methods_;
	uint64_t protocols_;
	uint64_t instance_properties_;
	Objc2MethodList *method_list_;
	Objc2MethodList *class_method_list_;
	
	// no copy ctr or assignment op
	Objc2Category(const Objc2Category &);
	Objc2Category &operator =(const Objc2Category &);
};

class Objc2CategoryList : public BaseObjcCategoryList
{
public:
	Objc2CategoryList();
	Objc2Category *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file);
private:
	Objc2Category *Add(uint64_t address);
};

class Objc2SelectorList : public BaseObjcSelectorList
{
public:
	Objc2SelectorList();
	void ReadFromFile(MacArchitecture &file);
private:
	ObjcSelector *Add(uint64_t address);
};

class Objc2ProtocolList;

class Objc2Protocol : public BaseObjcProtocol
{
public:
	Objc2Protocol(Objc2ProtocolList *owner, uint64_t address);
	~Objc2Protocol();
	void ReadFromFile(MacArchitecture &file);
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	OperandSize size_;
	uint64_t isa_;
	uint64_t name_;
	uint64_t protocols_;
	uint64_t instance_methods_;
	uint64_t class_methods_;
	uint64_t optional_instance_methods_;
	uint64_t optional_class_methods_;
	uint64_t instance_properties_;
	Objc2MethodList *method_list_;

	// no copy ctr or assignment op
	Objc2Protocol(const Objc2Protocol &);
	Objc2Protocol &operator =(const Objc2Protocol &);
};

class Objc2ProtocolList : public BaseObjcProtocolList
{
public:
	Objc2ProtocolList();
	Objc2Protocol *item(size_t index) const;
	void ReadFromFile(MacArchitecture &file);
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	Objc2Protocol *Add(uint64_t address);
};

class Objc2MessageList;

class Objc2Message : public IObject
{
public:
	Objc2Message(Objc2MessageList *owner, uint64_t address);
	~Objc2Message();
	void ReadFromFile(MacArchitecture &file);
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	Objc2MessageList *owner_;
	uint64_t address_;
	OperandSize size_;
	uint64_t imp_;
	uint64_t name_;
};

class Objc2MessageList : public ObjectList<Objc2Message>
{
public:
	Objc2MessageList();
	void ReadFromFile(MacArchitecture &file);
	void GetStringReferences(std::set<uint64_t> &address_list) const;
private:
	Objc2Message *Add(uint64_t address);
};

class Objc2Storage : public IObjcStorage
{
public:
	Objc2Storage();
	~Objc2Storage();
	virtual void ReadFromFile(MacArchitecture &file);
	virtual void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	virtual void GetStringReferences(std::set<uint64_t> &address_list) const;
	virtual std::vector<MacSegment *> segment_list() const { return segment_list_; }
private:
	Objc2ClassList *class_list_;
	Objc2CategoryList *category_list_;
	Objc2SelectorList *selector_list_;
	Objc2ProtocolList *protocol_list_;
	Objc2MessageList *message_list_;
	std::vector<MacSegment *> segment_list_;
	
	// no copy ctr or assignment op
	Objc2Storage(const Objc2Storage &);
	Objc2Storage &operator =(const Objc2Storage &);
};

class Objc : public IObject
{
public:
	Objc();
	~Objc();
	bool ReadFromFile(MacArchitecture &file);
	void GetLoadMethodReferences(std::set<uint64_t> &address_list) const;
	void GetStringReferences(std::set<uint64_t> &address_list) const;
	std::vector<MacSegment *> segment_list() const;
private:
	IObjcStorage *storage_;
	
	// no copy ctr or assignment op
	Objc(const Objc &);
	Objc &operator =(const Objc &);
};

#endif