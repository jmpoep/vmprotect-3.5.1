#include "objects.h"
#include "osutils.h"
#include "files.h"
#include "macfile.h"
#include "objc.h"

MacSection *GetObjcSection(MacArchitecture &file, uint8_t version, const std::string &section_name)
{
	std::set<std::string> segment_list;
	switch (version) {
	case 1:
		segment_list.insert(SEG_OBJC);
		break;
	case 2:
		segment_list.insert(SEG_DATA);
		segment_list.insert("__DATA_CONST");
		break;
	}

	for (size_t i = 0; i < file.section_list()->count(); i++) {
		MacSection *section = file.section_list()->item(i);
		if (section->parent() && segment_list.find(section->parent()->name()) != segment_list.end() && section->name() == section_name)
			return section;
	}
	return NULL;
}

/**
 * BaseObjcMethod
 */

BaseObjcMethod::BaseObjcMethod(IObjcMethodList *owner, uint64_t address)
	: IObjcMethod(), owner_(owner), address_(address)
{

}

BaseObjcMethod::~BaseObjcMethod()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
 * BaseObjcMethodList
 */

void BaseObjcMethodList::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetLoadMethodReferences(address_list);
	}
}

void BaseObjcMethodList::GetStringReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetStringReferences(address_list);
	}
}

/**
 * BaseObjcCLass
 */

BaseObjcClass::BaseObjcClass(IObjcClassList *owner, uint64_t address)
	: IObjcClass(), owner_(owner), address_(address)
{

}

BaseObjcClass::~BaseObjcClass()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
 * BaseObjcClassList
 */

void BaseObjcClassList::AddObject(IObjcClass *object)
{
	IObjcClassList::AddObject(object);
	if (object->address())
		map_[object->address()] = object;
}

void BaseObjcClassList::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetLoadMethodReferences(address_list);
	}
}

void BaseObjcClassList::GetStringReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetStringReferences(address_list);
	}
}

IObjcClass *BaseObjcClassList::GetClassByAddress(uint64_t address) const
{
	std::map<uint64_t, IObjcClass *>::const_iterator it = map_.find(address);
	if (it != map_.end())
		return it->second;

	return NULL;
}

/**
 * BaseObjcCategory
 */

BaseObjcCategory::BaseObjcCategory(IObjcCategoryList *owner, uint64_t address)
	: IObjcCategory(), owner_(owner), address_(address)
{

}

BaseObjcCategory::~BaseObjcCategory()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
 * BaseObjcCategoryList
 */

void BaseObjcCategoryList::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetLoadMethodReferences(address_list);
	}
}

void BaseObjcCategoryList::GetStringReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetStringReferences(address_list);
	}
}

/**
 * BaseObjcSelector
 */

BaseObjcSelector::BaseObjcSelector(IObjcSelectorList *owner, uint64_t address)
	: IObjcSelector(), owner_(owner), address_(address)
{

}

BaseObjcSelector::~BaseObjcSelector()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
 * BaseObjcSelectorList
 */

void BaseObjcSelectorList::GetStringReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetStringReferences(address_list);
	}
}

/**
 * BaseObjcProtocol
 */

BaseObjcProtocol::BaseObjcProtocol(IObjcProtocolList *owner, uint64_t address)
	: IObjcProtocol(), owner_(owner), address_(address)
{

}

BaseObjcProtocol::~BaseObjcProtocol()
{
	if (owner_)
		owner_->RemoveObject(this);
}

/**
 * BaseObjcProtocolList
 */

IObjcProtocol *BaseObjcProtocolList::GetProtocolByAddress(uint64_t address) const
{
	for (size_t i = 0; i < count(); i++) {
		IObjcProtocol *prot = item(i);
		if (prot->address() == address)
			return prot;
	}
	return NULL;
}

void BaseObjcProtocolList::GetStringReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetStringReferences(address_list);
	}
}

/**
 * ObjcMethod
 */

ObjcMethod::ObjcMethod(IObjcMethodList *owner, uint64_t address)
	: BaseObjcMethod(owner, address)
{
	size_ = osDWord;
	name_ = 0;
	types_ = 0;
	imp_ = 0;
	type_ = mtUnknown;
}

void ObjcMethod::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc_method method;
		file.Read(&method, sizeof(method));
		name_ = method.name;
		types_ = method.types;
		imp_ = method.imp;
	} else {
		objc_method_64 method;
		file.Read(&method, sizeof(method));
		name_ = method.name;
		types_ = method.types;
		imp_ = method.imp;
	}
	
	if (name_ && file.AddressSeek(name_)) {
		std::string name = file.ReadString();
		if (name == "load")
			type_ = mtLoad;
	}
}

void ObjcMethod::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	if (type_ == mtLoad && imp_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_method, imp) : offsetof(objc_method_64, imp)));
}

void ObjcMethod::GetStringReferences(std::set<uint64_t> &address_list) const
{
	if (name_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_method, name) : offsetof(objc_method_64, name)));
	if (types_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_method, types) : offsetof(objc_method_64, types)));
}

/**
 * ObjcMethodList
 */

ObjcMethodList::ObjcMethodList()
	: BaseObjcMethodList()
{

}

ObjcMethod *ObjcMethodList::Add(uint64_t address)
{
	ObjcMethod *objc_method = new ObjcMethod(this, address);
	AddObject(objc_method);
	return objc_method;
}

ObjcMethod *ObjcMethodList::item(size_t index) const
{
	return reinterpret_cast<ObjcMethod *>(BaseObjcMethodList::item(index));
}

void ObjcMethodList::ReadFromFile(MacArchitecture &file, uint64_t address)
{
	if (!file.AddressSeek(address))
		return;

	size_t method_size = file.cpu_address_size() == osDWord ? sizeof(objc_method) : sizeof(objc_method_64);
	objc_method_list list;

	file.Read(&list, sizeof(list));

	address += sizeof(list);
	for (size_t i = 0; i < list.count; i++) {
		Add(address);
		address += method_size;
	}

	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadFromFile(file);
	}
}

/**
 * ObjcClass
 */

ObjcClass::ObjcClass(IObjcClassList *owner, uint64_t address)
	: BaseObjcClass(owner, address), size_(osDWord), isa_(0), super_class_(0), name_(0),
	version_(0), info_(0), instance_size_(0), ivars_(0), methods_(0), cache_(0), protocols_(0)
{
	method_list_ = new ObjcMethodList();
}

ObjcClass::~ObjcClass()
{
	delete method_list_;
}

void ObjcClass::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc_class klass;
		file.Read(&klass, sizeof(klass));
		isa_ = klass.isa;
		super_class_ = klass.super_class;
		name_ = klass.name;
		version_ = klass.version;
		info_ = klass.info;
		instance_size_ = klass.instance_size;
		ivars_ = klass.ivars;
		methods_ = klass.methods;
		cache_ = klass.cache;
		protocols_ = klass.protocols;
	} else {
		objc_class_64 klass;
		file.Read(&klass, sizeof(klass));
		isa_ = klass.isa;
		super_class_ = klass.super_class;
		name_ = klass.name;
		version_ = klass.version;
		info_ = klass.info;
		instance_size_ = klass.instance_size;
		ivars_ = klass.ivars;
		methods_ = klass.methods;
		cache_ = klass.cache;
		protocols_ = klass.protocols;
	}

	if (methods_)
		method_list_->ReadFromFile(file, methods_);
}

void ObjcClass::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	method_list_->GetLoadMethodReferences(address_list);
}

void ObjcClass::GetStringReferences(std::set<uint64_t> &address_list) const
{
	if (isa_ && (info_ & (CLS_CLASS | CLS_META)) == CLS_META)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_class, isa) : offsetof(objc_class_64, isa)));
	if (super_class_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_class, super_class) : offsetof(objc_class_64, super_class)));
	if (name_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_class, name) : offsetof(objc_class_64, name)));

	method_list_->GetStringReferences(address_list);
}

/**
 * ObjcClassList
 */

ObjcClassList::ObjcClassList()
	: BaseObjcClassList()
{

}

ObjcClass *ObjcClassList::Add(uint64_t address)
{
	ObjcClass *objc_class = new ObjcClass(this, address);
	AddObject(objc_class);
	return objc_class;
}

ObjcClass *ObjcClassList::item(size_t index) const
{
	return reinterpret_cast<ObjcClass *>(BaseObjcClassList::item(index));
}

void ObjcClassList::ReadFromFile(MacArchitecture &file, size_t class_count)
{
	size_t i, old_count;
	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	uint64_t value = 0;
	old_count = count();
	for (i = 0; i < class_count; i++) {
		assert(sizeof(value) >= value_size);
		file.Read(&value, value_size);
		if (!GetClassByAddress(value))
			Add(value);
	}

	uint64_t pos = file.Tell();
	for (i = old_count; i < count(); i++) {
		ObjcClass *obj_class = item(i);
		obj_class->ReadFromFile(file);
		if (obj_class->isa() && (obj_class->info() & CLS_CLASS) && !GetClassByAddress(obj_class->isa()))
			Add(obj_class->isa());
	}
	file.Seek(pos);
}

/**
 * ObjcCategory
 */

ObjcCategory::ObjcCategory(IObjcCategoryList *owner, uint64_t address)
	: BaseObjcCategory(owner, address), size_(osDWord), category_name_(0), class_name_(0), instance_methods_(0), class_methods_(0), protocols_(0)
{
	method_list_ = new ObjcMethodList();
	class_method_list_ = new ObjcMethodList();
}

ObjcCategory::~ObjcCategory()
{
	delete method_list_;
	delete class_method_list_;
}

void ObjcCategory::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc_category category;
		file.Read(&category, sizeof(category));
		category_name_ = category.category_name;
		class_name_ = category.class_name;
		instance_methods_ = category.instance_methods;
		class_methods_ = category.class_methods;
		protocols_ = category.protocols;
	} else {
		objc_category_64 category;
		file.Read(&category, sizeof(category));
		category_name_ = category.category_name;
		class_name_ = category.class_name;
		instance_methods_ = category.instance_methods;
		class_methods_ = category.class_methods;
		protocols_ = category.protocols;
	}

	if (instance_methods_)
		method_list_->ReadFromFile(file, instance_methods_);
	if (class_methods_)
		class_method_list_->ReadFromFile(file, class_methods_);
}

void ObjcCategory::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	method_list_->GetLoadMethodReferences(address_list);
}

void ObjcCategory::GetStringReferences(std::set<uint64_t> &address_list) const
{
	if (category_name_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_category, category_name) : offsetof(objc_category_64, category_name)));
	if (class_name_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_category, class_name) : offsetof(objc_category_64, class_name)));

	method_list_->GetStringReferences(address_list);
	class_method_list_->GetStringReferences(address_list);
}

/**
 * ObjcCategoryList
 */

ObjcCategoryList::ObjcCategoryList()
	: BaseObjcCategoryList()
{

}

ObjcCategory *ObjcCategoryList::Add(uint64_t address)
{
	ObjcCategory *objc_category = new ObjcCategory(this, address);
	AddObject(objc_category);
	return objc_category;
}

ObjcCategory *ObjcCategoryList::item(size_t index) const
{
	return reinterpret_cast<ObjcCategory *>(BaseObjcCategoryList::item(index));
}

void ObjcCategoryList::ReadFromFile(MacArchitecture &file, size_t count)
{
	size_t i;
	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	uint64_t value = 0;
	for (i = 0; i < count; i++) {
		assert(sizeof(value) >= value_size);
		file.Read(&value, value_size);
		Add(value);
	}
	for (i = 0; i < count; i++) {
		item(i)->ReadFromFile(file);
	}
}

/**
 * ObjcSelector
 */

ObjcSelector::ObjcSelector(IObjcSelectorList *owner, uint64_t address)
	: BaseObjcSelector(owner, address)
{

}

void ObjcSelector::GetStringReferences(std::set<uint64_t> &address_list) const
{
	address_list.insert(address());
}

/**
 * ObjcSelectorList
 */

ObjcSelectorList::ObjcSelectorList()
	: BaseObjcSelectorList()
{

}

void ObjcSelectorList::ReadFromFile(MacArchitecture &file)
{
	MacSection *section = GetObjcSection(file, 1, "__message_refs");
	if (section && file.AddressSeek(section->address())) {
		size_t value_size = OperandSizeToValue(file.cpu_address_size());
		size_t count = static_cast<size_t>(section->size() / OperandSizeToValue(file.cpu_address_size()));
		uint64_t address = section->address();
		for (size_t i = 0; i < count; i++) {
			Add(address);
			address += value_size;
		}
	}
}

ObjcSelector *ObjcSelectorList::Add(uint64_t address)
{
	ObjcSelector *sel = new ObjcSelector(this, address);
	AddObject(sel);
	return sel;
}

/**
 * ObjcProtocol
 */

ObjcProtocol::ObjcProtocol(ObjcProtocolList *owner, uint64_t address)
	: BaseObjcProtocol(owner, address)
{
	size_ = osDWord;
	isa_ = 0;
	name_ = 0;
	protocols_ = 0;
	instance_methods_ = 0;
	class_methods_ = 0;
}

void ObjcProtocol::GetStringReferences(std::set<uint64_t> &address_list) const
{
	address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc_protocol, name) : offsetof(objc_protocol_64, name)));
}

void ObjcProtocol::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc_protocol protocol;
		file.Read(&protocol, sizeof(protocol));
		isa_ = protocol.isa;
		name_ = protocol.name;
		protocols_ = protocol.protocols;
		instance_methods_ = protocol.instance_methods;
		class_methods_ = protocol.class_methods;
	} else {
		objc_protocol_64 protocol;
		file.Read(&protocol, sizeof(protocol));
		isa_ = protocol.isa;
		name_ = protocol.name;
		protocols_ = protocol.protocols;
		instance_methods_ = protocol.instance_methods;
		class_methods_ = protocol.class_methods;
	}
}

/**
 * ObjcProtocolList
 */

ObjcProtocolList::ObjcProtocolList()
	: BaseObjcProtocolList()
{

}

ObjcProtocol *ObjcProtocolList::item(size_t index) const
{
	return reinterpret_cast<ObjcProtocol *>(BaseObjcProtocolList::item(index));
}

void ObjcProtocolList::ReadFromFile(MacArchitecture &file)
{
	MacSection *section = GetObjcSection(file, 1, "__protocol");
	if (section && file.AddressSeek(section->address())) {
		size_t protocol_size = (file.cpu_address_size() == osDWord) ? sizeof(objc_protocol) : sizeof(objc_protocol_64);
		size_t count = static_cast<size_t>(section->size() / protocol_size);
		uint64_t address = section->address();
		for (size_t i = 0; i < count; i++) {
			Add(address);
			address += protocol_size;
		}
	}
	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadFromFile(file);
	}
}

ObjcProtocol *ObjcProtocolList::Add(uint64_t address)
{
	ObjcProtocol *prot = new ObjcProtocol(this, address);
	AddObject(prot);
	return prot;
}

/**
 * ObjcStorage
 */

ObjcStorage::ObjcStorage()
	: IObjcStorage()
{
	class_list_ = new ObjcClassList();
	category_list_ = new ObjcCategoryList();
	selector_list_ = new ObjcSelectorList();
	protocol_list_ = new ObjcProtocolList();
}

ObjcStorage::~ObjcStorage()
{
	delete class_list_;
	delete category_list_;
	delete selector_list_;
	delete protocol_list_;
}

void ObjcStorage::ReadFromFile(MacArchitecture &file)
{
	size_t i;
	std::vector<uint64_t> symtab_list;

	MacSection *section = GetObjcSection(file, 1, SECT_OBJC_MODULES);
	if (section && file.AddressSeek(section->address())) {
		size_t module_size = (file.cpu_address_size() == osDWord) ? sizeof(objc_module) : sizeof(objc_module_64);
		size_t module_count = static_cast<size_t>(section->size() / module_size);
		for (i = 0; i < module_count; i++) {
			uint64_t symtab;
			if (file.cpu_address_size() == osDWord) {
				objc_module module;
				file.Read(&module, sizeof(module));
				symtab = module.symtab;
			} else {
				objc_module_64 module;
				file.Read(&module, sizeof(module));
				symtab = module.symtab;
			}
			if (symtab)
				symtab_list.push_back(symtab);
		}
	}

	for (i = 0; i < symtab_list.size(); i++) {
		if (file.AddressSeek(symtab_list[i])) {
			uint16_t cls_def_cnt;
			uint16_t cat_def_cnt;

			if (file.cpu_address_size() == osDWord) {
				objc_symtab symtab;
				file.Read(&symtab, sizeof(symtab));
				cls_def_cnt = symtab.cls_def_cnt;
				cat_def_cnt = symtab.cat_def_cnt;
			} else {
				objc_symtab_64 symtab;
				file.Read(&symtab, sizeof(symtab));
				cls_def_cnt = symtab.cls_def_cnt;
				cat_def_cnt = symtab.cat_def_cnt;
			}

			class_list_->ReadFromFile(file, cls_def_cnt);
			category_list_->ReadFromFile(file, cat_def_cnt);
		}
	}
	selector_list_->ReadFromFile(file);
	protocol_list_->ReadFromFile(file);

	MacSegment *segment = file.segment_list()->GetSectionByName(SEG_OBJC);
	if (segment)
		segment_list_.push_back(segment);
}

void ObjcStorage::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	class_list_->GetLoadMethodReferences(address_list);
	category_list_->GetLoadMethodReferences(address_list);
}

void ObjcStorage::GetStringReferences(std::set<uint64_t> &address_list) const
{
	class_list_->GetStringReferences(address_list);
	category_list_->GetStringReferences(address_list);
	selector_list_->GetStringReferences(address_list);
	protocol_list_->GetStringReferences(address_list);
}

/**
 * Objc2Method
 */

Objc2Method::Objc2Method(IObjcMethodList *owner, uint64_t address)
	: BaseObjcMethod(owner, address), size_(osDWord), name_(0), types_(0), imp_(0), type_(mtUnknown)
{
	
}

void Objc2Method::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc2_method method;
		file.Read(&method, sizeof(method));
		name_ = method.name;
		types_ = method.types;
		imp_ = method.imp;
	} else {
		objc2_method_64 method;
		file.Read(&method, sizeof(method));
		name_ = method.name;
		types_ = method.types;
		imp_ = method.imp;
	}
	
	if (name_ && file.AddressSeek(name_)) {
		std::string name = file.ReadString();
		if (name == "load")
			type_ = mtLoad;
	}
}

void Objc2Method::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	if (type_ == mtLoad && imp_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc2_method, imp) : offsetof(objc2_method_64, imp)));
}

void Objc2Method::GetStringReferences(std::set<uint64_t> &address_list) const
{
	if (name_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc2_method, name) : offsetof(objc2_method_64, name)));
	if (types_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc2_method, types) : offsetof(objc2_method_64, types)));
}

/**
 * Objc2MethodList
 */

Objc2MethodList::Objc2MethodList()
	: BaseObjcMethodList()
{

}

Objc2Method *Objc2MethodList::Add(uint64_t address)
{
	Objc2Method *objc_method = new Objc2Method(this, address);
	AddObject(objc_method);
	return objc_method;
}

Objc2Method *Objc2MethodList::item(size_t index) const
{
	return reinterpret_cast<Objc2Method *>(BaseObjcMethodList::item(index));
}

void Objc2MethodList::ReadFromFile(MacArchitecture &file, uint64_t address)
{
	if (!file.AddressSeek(address))
		return;

	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	size_t method_size = file.cpu_address_size() == osDWord ? sizeof(objc_method) : sizeof(objc_method_64);
	/*uint32_t entsize =*/ file.ReadDWord();
	uint32_t method_count = file.ReadDWord();
	address += sizeof(uint32_t)/*entsize*/ + sizeof(method_count);
	for (size_t i = 0; i < method_count; i++) {
		Add(address);
		address += method_size;
	}

	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadFromFile(file);
	}
}

/**
 * Objc2Class
 */

Objc2Class::Objc2Class(IObjcClassList *owner, uint64_t address)
	: BaseObjcClass(owner, address)
{
	size_ = osDWord;
	flags_ = 0;
	isa_ = 0;
	super_class_ = 0;
	cache_ = 0;
	vtable_ = 0;
	data_ = 0;
	ivar_layout_ = 0;
	name_ = 0;
	base_methods_ = 0;
	base_protocols_ = 0;
	ivars_ = 0;
	protocols_ = 0;
	weak_ivar_layout_ = 0;
	base_properties_ = 0;
	instance_start_ = 0;
	instance_size_ = 0;
	method_list_ = new Objc2MethodList();
}

Objc2Class::~Objc2Class()
{
	delete method_list_;
}

void Objc2Class::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();

	if (file.cpu_address_size() == osDWord) {
		objc2_class klass;
		file.Read(&klass, sizeof(klass));
		isa_ = klass.isa;
		super_class_ = klass.super_class;
		cache_ = klass.cache;
		vtable_ = klass.vtable;
		data_ = klass.data;
	} else {
		objc2_class_64 klass;
		file.Read(&klass, sizeof(klass));
		isa_ = klass.isa;
		super_class_ = klass.super_class;
		cache_ = klass.cache;
		vtable_ = klass.vtable;
		data_ = klass.data;
	}

	if (!file.AddressSeek(data_))
		return;

	if (file.cpu_address_size() == osDWord) {
		objc2_class_data data;
		file.Read(&data, sizeof(data));
		flags_ = data.flags;
		instance_start_ = data.instance_start;
		instance_size_ = data.instance_size;
		ivar_layout_ = data.ivar_layout;
		name_ = data.name;
		base_methods_ = data.base_methods;
		base_protocols_ = data.base_protocols;
		ivars_ = data.ivars;
		weak_ivar_layout_ = data.weak_ivar_layout;
		base_properties_ = data.base_properties;
	} else {
		objc2_class_data_64 data;
		file.Read(&data, sizeof(data));
		flags_ = data.flags;
		instance_start_ = data.instance_start;
		instance_size_ = data.instance_size;
		ivar_layout_ = data.ivar_layout;
		name_ = data.name;
		base_methods_ = data.base_methods;
		base_protocols_ = data.base_protocols;
		ivars_ = data.ivars;
		weak_ivar_layout_ = data.weak_ivar_layout;
		base_properties_ = data.base_properties;
	}

	if (base_methods_)
		method_list_->ReadFromFile(file, base_methods_);
};

void Objc2Class::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	method_list_->GetLoadMethodReferences(address_list);
}

void Objc2Class::GetStringReferences(std::set<uint64_t> &address_list) const
{
	if (name_)
		address_list.insert(data_ + ((size_ == osDWord) ? offsetof(objc2_class_data, name) : offsetof(objc2_class_data_64, name)));

	method_list_->GetStringReferences(address_list);
}

/**
 * Objc2ClassList
 */

Objc2ClassList::Objc2ClassList()
	: BaseObjcClassList()
{

}

Objc2Class *Objc2ClassList::Add(uint64_t address)
{
	Objc2Class *objc_class = new Objc2Class(this, address);
	AddObject(objc_class);
	return objc_class;
}

Objc2Class *Objc2ClassList::item(size_t index) const
{
	return reinterpret_cast<Objc2Class *>(BaseObjcClassList::item(index));
}

void Objc2ClassList::ReadFromFile(MacArchitecture &file)
{
	const char *classlist_section_name[] = {
		"__objc_nlclslist",
		"__objc_classlist"
	};

	for (size_t j = 0; j < _countof(classlist_section_name); j++) {
		MacSection *section = GetObjcSection(file, 2, classlist_section_name[j]);
		if (section && file.AddressSeek(section->address())) {
			size_t count = static_cast<size_t>(section->size() / OperandSizeToValue(file.cpu_address_size()));
			size_t value_size = OperandSizeToValue(file.cpu_address_size());
			uint64_t value = 0;
			for (size_t i = 0; i < count; i++) {
				assert(sizeof(value) >= value_size);
				file.Read(&value, value_size);
				if (!GetClassByAddress(value))
					Add(value);
			}
		}
	}
	for (size_t i = 0; i < count(); i++) {
		Objc2Class *obj_class = item(i);
		obj_class->ReadFromFile(file);
		if (obj_class->isa() && !GetClassByAddress(obj_class->isa()))
			Add(obj_class->isa());
		if (obj_class->super_class() && !GetClassByAddress(obj_class->super_class()))
			Add(obj_class->super_class());
	}
}

/**
 * ObjcCategory
 */

Objc2Category::Objc2Category(IObjcCategoryList *owner, uint64_t address)
	: BaseObjcCategory(owner, address), name_(0), cls_(0), instance_methods_(0), class_methods_(0), protocols_(0), instance_properties_(0), size_(osDWord)
{
	method_list_ = new Objc2MethodList();
	class_method_list_ = new Objc2MethodList();
}

Objc2Category::~Objc2Category()
{
	delete method_list_;
	delete class_method_list_;
}

void Objc2Category::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc2_category category;
		file.Read(&category, sizeof(category));
		name_ = category.name;
		cls_ = category.cls;
		instance_methods_ = category.instance_methods;
		class_methods_ = category.class_methods;
		protocols_ = category.protocols;
		instance_properties_ = category.instance_properties;
	} else {
		objc2_category_64 category;
		file.Read(&category, sizeof(category));
		name_ = category.name;
		cls_ = category.cls;
		instance_methods_ = category.instance_methods;
		class_methods_ = category.class_methods;
		protocols_ = category.protocols;
		instance_properties_ = category.instance_properties;
	}

	if (instance_methods_)
		method_list_->ReadFromFile(file, instance_methods_);
	if (class_methods_)
		class_method_list_->ReadFromFile(file, class_methods_);
}

void Objc2Category::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	method_list_->GetLoadMethodReferences(address_list);
	class_method_list_->GetStringReferences(address_list);
}

void Objc2Category::GetStringReferences(std::set<uint64_t> &address_list) const
{
	if (name_)
		address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc2_category, name) : offsetof(objc2_category_64, name)));

	method_list_->GetStringReferences(address_list);
	class_method_list_->GetStringReferences(address_list);
}

/**
 * Objc2CategoryList
 */

Objc2CategoryList::Objc2CategoryList()
	: BaseObjcCategoryList()
{

}

Objc2Category *Objc2CategoryList::Add(uint64_t address)
{
	Objc2Category *objc_category = new Objc2Category(this, address);
	AddObject(objc_category);
	return objc_category;
}

Objc2Category *Objc2CategoryList::item(size_t index) const
{
	return reinterpret_cast<Objc2Category *>(BaseObjcCategoryList::item(index));
}

void Objc2CategoryList::ReadFromFile(MacArchitecture &file)
{
	const char *catlist_section_name[] = {
		"__objc_nlcatlist",
		"__objc_catlist" 
	};

	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	uint64_t value = 0;
	assert(sizeof(value) >= value_size);
	for (size_t j = 0; j < _countof(catlist_section_name); j++) {
		MacSection *section = GetObjcSection(file, 2, catlist_section_name[j]);
		if (section && file.AddressSeek(section->address())) {
			size_t count = static_cast<size_t>(section->size()) / OperandSizeToValue(file.cpu_address_size());
			for (size_t i = 0; i < count; i++) {
				file.Read(&value, value_size);
				Add(value);
			}
		}
	}
	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadFromFile(file);
	}
}

/**
 * Objc2SelectorList
 */

Objc2SelectorList::Objc2SelectorList()
	: BaseObjcSelectorList()
{

}

void Objc2SelectorList::ReadFromFile(MacArchitecture &file)
{
	MacSection *section = GetObjcSection(file, 2, "__objc_selrefs");
	if (section && file.AddressSeek(section->address())) {
		size_t value_size = OperandSizeToValue(file.cpu_address_size());
		size_t count = static_cast<size_t>(section->size() / OperandSizeToValue(file.cpu_address_size()));
		uint64_t address = section->address();
		for (size_t i = 0; i < count; i++) {
			Add(address);
			address += value_size;
		}
	}
}

ObjcSelector *Objc2SelectorList::Add(uint64_t address)
{
	ObjcSelector *sel = new ObjcSelector(this, address);
	AddObject(sel);
	return sel;
}

/**
 * Objc2Protocol
 */

Objc2Protocol::Objc2Protocol(Objc2ProtocolList *owner, uint64_t address)
	: BaseObjcProtocol(owner, address), size_(osDWord)
{
    method_list_ = new Objc2MethodList();
}

Objc2Protocol::~Objc2Protocol()
{
	delete method_list_;
}

void Objc2Protocol::GetStringReferences(std::set<uint64_t> &address_list) const
{
	address_list.insert(address() + ((size_ == osDWord) ? offsetof(objc2_protocol, name) : offsetof(objc2_protocol_64, name)));
	method_list_->GetStringReferences(address_list);
}

void Objc2Protocol::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address()))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc2_protocol protocol;
		file.Read(&protocol, sizeof(protocol));
		isa_ = protocol.isa;
		name_ = protocol.name;
		protocols_ = protocol.protocols;
		instance_methods_ = protocol.instance_methods;
		class_methods_ = protocol.class_methods;
		optional_instance_methods_ = protocol.optional_instance_methods;
		optional_class_methods_ = protocol.optional_class_methods;
		instance_properties_ = protocol.instance_properties;
	} else {
		objc2_protocol_64 protocol;
		file.Read(&protocol, sizeof(protocol));
		isa_ = protocol.isa;
		name_ = protocol.name;
		protocols_ = protocol.protocols;
		instance_methods_ = protocol.instance_methods;
		class_methods_ = protocol.class_methods;
		optional_instance_methods_ = protocol.optional_instance_methods;
		optional_class_methods_ = protocol.optional_class_methods;
		instance_properties_ = protocol.instance_properties;
	}

	if (instance_methods_)
		method_list_->ReadFromFile(file, instance_methods_);
}

/**
 * Objc2ProtocolList
 */

Objc2ProtocolList::Objc2ProtocolList()
	: BaseObjcProtocolList()
{

}

Objc2Protocol *Objc2ProtocolList::item(size_t index) const
{
	return reinterpret_cast<Objc2Protocol *>(BaseObjcProtocolList::item(index));
}

void Objc2ProtocolList::ReadFromFile(MacArchitecture &file)
{
	const char *protolist_section_name[] = {
		"__objc_protolist",
		"__objc_protorefs"
	};

	size_t value_size = OperandSizeToValue(file.cpu_address_size());
	for (size_t j = 0; j < _countof(protolist_section_name); j++) {
		MacSection *section = GetObjcSection(file, 2, protolist_section_name[j]);
		if (section && file.AddressSeek(section->address())) {
			size_t count = static_cast<size_t>(section->size() / OperandSizeToValue(file.cpu_address_size()));
			uint64_t value = 0;
			for (size_t i = 0; i < count; i++) {
				file.Read(&value, value_size);
				if (!GetProtocolByAddress(value))
					Add(value);
			}
		}
	}
	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadFromFile(file);
	}
}

Objc2Protocol *Objc2ProtocolList::Add(uint64_t address)
{
	Objc2Protocol *prot = new Objc2Protocol(this, address);
	AddObject(prot);
	return prot;
}

void Objc2ProtocolList::GetStringReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetStringReferences(address_list);
	}
}

/**
 * Objc2Message
 */

Objc2Message::Objc2Message(Objc2MessageList *owner, uint64_t address)
	: IObject(), owner_(owner), address_(address), size_(osDefault), imp_(0), name_(0)
{

}

Objc2Message::~Objc2Message()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void Objc2Message::ReadFromFile(MacArchitecture &file)
{
	if (!file.AddressSeek(address_))
		return;

	size_ = file.cpu_address_size();
	if (file.cpu_address_size() == osDWord) {
		objc2_message message;
		file.Read(&message, sizeof(message));
		imp_ = message.imp;
		name_ = message.name;
	} else {
		objc2_message_64 message;
		file.Read(&message, sizeof(message));
		imp_ = message.imp;
		name_ = message.name;
	}
}

void Objc2Message::GetStringReferences(std::set<uint64_t> &address_list) const
{
	address_list.insert(address_ + ((size_ == osDWord) ? offsetof(objc2_message, name) : offsetof(objc2_message_64, name)));
}

/**
 * Objc2MessageList
 */

Objc2MessageList::Objc2MessageList()
	: ObjectList<Objc2Message>()
{

}

void Objc2MessageList::ReadFromFile(MacArchitecture &file)
{
	MacSection *section = GetObjcSection(file, 2, "__objc_msgrefs");
	if (section && file.AddressSeek(section->address())) {
		size_t value_size = file.cpu_address_size() == osDWord ? sizeof(objc2_message) : sizeof(objc2_message_64);
		size_t count = static_cast<size_t>(section->size() / OperandSizeToValue(file.cpu_address_size()));
		uint64_t address = section->address();
		for (size_t i = 0; i < count; i++) {
			Add(address);
			address += value_size;
		}
	}
	for (size_t i = 0; i < count(); i++) {
		item(i)->ReadFromFile(file);
	}
}

Objc2Message *Objc2MessageList::Add(uint64_t address)
{
	Objc2Message *message = new Objc2Message(this, address);
	AddObject(message);
	return message;
}

void Objc2MessageList::GetStringReferences(std::set<uint64_t> &address_list) const
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->GetStringReferences(address_list);
	}
}

/**
 * Objc2Storage
 */

Objc2Storage::Objc2Storage()
	: IObjcStorage()
{
	class_list_ = new Objc2ClassList();
	category_list_ = new Objc2CategoryList();
	selector_list_ = new Objc2SelectorList();
	protocol_list_ = new Objc2ProtocolList();
	message_list_ = new Objc2MessageList();
}

Objc2Storage::~Objc2Storage()
{
	delete class_list_;
	delete category_list_;
	delete selector_list_;
	delete protocol_list_;
	delete message_list_;
}

void Objc2Storage::ReadFromFile(MacArchitecture &file)
{
	class_list_->ReadFromFile(file);
	category_list_->ReadFromFile(file);
	selector_list_->ReadFromFile(file);
	protocol_list_->ReadFromFile(file);
	message_list_->ReadFromFile(file);

	for (size_t i = 0; i < file.section_list()->count(); i++) {
		MacSection *section = file.section_list()->item(i);
		if (!section->parent() || find(segment_list_.begin(), segment_list_.end(), section->parent()) != segment_list_.end())
			continue;

		if (section->name().substr(0, 7) == "__objc_" && GetObjcSection(file, 2, section->name()))
			segment_list_.push_back(section->parent());
	}
}

void Objc2Storage::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	class_list_->GetLoadMethodReferences(address_list);
	category_list_->GetLoadMethodReferences(address_list);
}

void Objc2Storage::GetStringReferences(std::set<uint64_t> &address_list) const
{
	class_list_->GetStringReferences(address_list);
	category_list_->GetStringReferences(address_list);
	selector_list_->GetStringReferences(address_list);
	protocol_list_->GetStringReferences(address_list);
	message_list_->GetStringReferences(address_list);
}

/**
 * Objc
 */

Objc::Objc()
	: IObject(), storage_(NULL)
{

}

Objc::~Objc()
{
	delete storage_;
}

bool Objc::ReadFromFile(MacArchitecture &file)
{
	delete storage_;
	storage_ = NULL;

	if (GetObjcSection(file, 1, "__image_info"))
		storage_ = new ObjcStorage();
	else if (GetObjcSection(file, 2, "__objc_imageinfo"))
		storage_ = new Objc2Storage();

	if (storage_) {
		storage_->ReadFromFile(file);
		return true;
	}

	return false;
}

void Objc::GetLoadMethodReferences(std::set<uint64_t> &address_list) const
{
	address_list.clear();
	if (storage_)
		storage_->GetLoadMethodReferences(address_list);
}

void Objc::GetStringReferences(std::set<uint64_t> &address_list) const
{
	address_list.clear();
	if (storage_)
		storage_->GetStringReferences(address_list);
}

std::vector<MacSegment *> Objc::segment_list() const
{
	if (storage_)
		return storage_->segment_list();

	return std::vector<MacSegment *>();
}