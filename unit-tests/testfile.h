#ifndef TESTFILE_H
#define TESTFILE_H

template<class TTestConfig> class TestSegmentListT;

template<class TTestConfig> 
class TestSegmentT : public TTestConfig::Segment
{
public:
	explicit TestSegmentT(typename TTestConfig::SegmentList *owner, uint64_t address, uint32_t size, const char *name, uint32_t type)
		: TTestConfig::Segment(owner), address_(address), size_(size), name_(name), type_(type), physical_size_(0), physical_offset_(0) {}
	virtual uint64_t address() const { return address_; }
	virtual uint64_t size() const { return size_; }
	virtual uint32_t physical_offset() const { return physical_offset_; }
	virtual uint32_t physical_size() const { return physical_size_; }
	virtual std::string name() const { return name_; }
	virtual uint32_t memory_type() const { return type_; }
	void set_physical_size(uint32_t physical_size) { physical_size_ = physical_size; }
	void set_physical_offset(uint32_t physical_offset) { physical_offset_ = physical_offset; }
	virtual void update_type(uint32_t mt) { }
	virtual uint32_t flags() const { return 0; }
	virtual void Rebase(uint64_t delta_base) { address_ += delta_base; }
	virtual TestSegmentT<TTestConfig> *Clone(ISectionList *owner) const { return new TestSegmentT<TTestConfig>(dynamic_cast<typename TTestConfig::SegmentList *>(owner), address_, size_, name_.c_str(), type_); }
private:
	uint64_t address_;
	uint32_t size_;
	std::string name_;
	uint32_t type_;
	uint32_t physical_size_;
	uint32_t physical_offset_;
};

template<class TTestConfig> 
class TestSegmentListT : public TTestConfig::SegmentList
{
public:
	explicit TestSegmentListT(typename TTestConfig::Architecture *owner)
		: TTestConfig::SegmentList(owner) {}

	TestSegmentT<TTestConfig> *Add(uint64_t address, uint32_t size, const char *name, uint32_t type)
	{
		TestSegmentT<TTestConfig> *seg = new TestSegmentT<TTestConfig>(this, address, size, name, type);
		this->AddObject(seg);
		return seg;
	}
};

template<class TTestConfig> 
class TestExportListT : public TTestConfig::ExportList
{
public:
	explicit TestExportListT(typename TTestConfig::Architecture *owner)
		: TTestConfig::ExportList(owner) {}
	virtual std::string name() const { return ""; }
protected:
	virtual typename TTestConfig::Export *Add(uint64_t /*address*/) { return NULL; }
};

template<class TTestConfig> 
class TestFixupT : public TTestConfig::Fixup
{
public:
	explicit TestFixupT(typename TTestConfig::FixupList *owner);
	virtual uint64_t address() const { return 0; }
	virtual FixupType type() const { return ftUnknown; }
	virtual OperandSize size() const { return osDWord; }
	virtual void set_address(uint64_t /*address*/) { return; }
	virtual void Rebase(IArchitecture & /*file*/, uint64_t /*delta_base*/) { return; }
	virtual TestFixupT<TTestConfig> *Clone(IFixupList *owner) const { return new TestFixupT<TTestConfig>(reinterpret_cast<typename TTestConfig::FixupList *>(owner)); }
};

template<class TTestConfig> 
class TestFixupListT : public TTestConfig::FixupList
{
public:
	explicit TestFixupListT()
		: TTestConfig::FixupList() {}
	virtual IFixup *AddDefault(OperandSize /*cpu_address_size*/,  bool /*is_code*/) 
	{ 
		TestFixupT<TTestConfig> *fixup = new TestFixupT<TTestConfig>(this);
		this->AddObject(fixup);
		return fixup; 
	}
};

template<class TTestConfig> 
class TestImportListT : public TTestConfig::ImportList
{
public:
	explicit TestImportListT(typename TTestConfig::Architecture *owner)
		: TTestConfig::ImportList(owner) {}
protected:
	virtual typename TTestConfig::Import *AddSDK() { return NULL; }
};

class TestMapFile : public MapFile
{
public:
	bool ParseEx(const char *file_name, const std::vector<uint64_t> &segments, uint64_t time_stamp)
	{
		bool res = MapFile::Parse(file_name, segments);
		set_time_stamp(time_stamp);
		return res;
	}
};

template<class TTestConfig>
class TestArchitectureT : public TTestConfig::Architecture
{
public:
	explicit TestArchitectureT(typename TTestConfig::File *owner, OperandSize cpu_address_size) 
		: TTestConfig::Architecture(owner, 0, -1), function_list_(NULL), cpu_address_size_(cpu_address_size), virtual_machine_list_(NULL)
	{
		segment_list_ = new TestSegmentListT<TTestConfig>(this);
		export_list_ = new TestExportListT<TTestConfig>(this);
		fixup_list_ = new TestFixupListT<TTestConfig>();
		import_list_ = new TestImportListT<TTestConfig>(this);
		time_stamp_ = 1;

		function_list_ = new typename TTestConfig::FunctionList(this);
		runtime_function_list_ = new typename TTestConfig::RuntimeFunctionList();
		virtual_machine_list_ = new typename TTestConfig::VirtualMachineList();
	}

	virtual ~TestArchitectureT()
	{
		delete segment_list_;
		delete export_list_;
		delete function_list_;
		delete fixup_list_;
		delete import_list_;
		delete virtual_machine_list_;
		delete runtime_function_list_;
	}; 

	virtual std::string format_name() const { return "TEST"; }
	virtual std::string name() const { return "TEST"; }
	virtual uint32_t type() const { return 0; }
	virtual OperandSize cpu_address_size() const { return cpu_address_size_; }
	virtual uint64_t entry_point() const { return 0; };
	virtual uint32_t segment_alignment() const { return 0x1000; }
	virtual TestSegmentListT<TTestConfig> *segment_list() const { return segment_list_; }
	virtual typename TTestConfig::SectionList *section_list() const { return NULL; }
	virtual TestImportListT<TTestConfig> *import_list() const { return import_list_; }
	virtual TestExportListT<TTestConfig> *export_list() const { return export_list_; }
	virtual TestFixupListT<TTestConfig> *fixup_list() const { return fixup_list_; }
	virtual typename TTestConfig::RelocationList *relocation_list() const { return NULL; }
	virtual typename TTestConfig::ResourceList *resource_list() const { return NULL; }
	virtual IFunctionList *function_list() const { return function_list_; }
	virtual IVirtualMachineList *virtual_machine_list() const { return virtual_machine_list_; }
	virtual typename TTestConfig::SEHandlerList *seh_handler_list() const { return NULL; }
	virtual typename TTestConfig::RuntimeFunctionList *runtime_function_list() const { return runtime_function_list_; }
	virtual uint64_t image_base() const { return 0; }
	virtual CallingConvention calling_convention() const { return ccStdcall; }
	virtual uint64_t time_stamp() const { return time_stamp_; }

	void ReadTestMapFile(const char *file_name)
	{
		TestMapFile map_file;
		std::vector<uint64_t> segments;
		for (size_t i = 0; i < segment_list()->count(); i++) {
			segments.push_back(segment_list()->item(i)->address());
		}
		if (std::find(segments.begin(), segments.end(), 0) == segments.end())
			segments.insert(segments.begin(), 0);
		if (map_file.ParseEx(file_name, segments, time_stamp()))
			this->ReadMapFile(map_file);
		this->map_function_list()->ReadFromFile(*this);
	};
	virtual void Save(CompileContext & /*c*/) 
	{ 
		ISection *last_section = segment_list_->item(0);
		uint64_t address = AlignValue(last_section->address() + last_section->size(), segment_alignment());
		uint64_t pos = this->size();
		auto *vmp_section = segment_list_->Add(address, -1, ".vmp", mtReadable | mtExecutable);
		vmp_section->set_physical_size(-1);
		vmp_section->set_physical_offset(static_cast<uint32_t>(pos));

		for (size_t i = 0; i < function_list_->count(); i++) {
			function_list_->item(i)->WriteToFile(*this);
		}
	};

	bool Prepare(CompileContext &ctx)
	{
		ISectionList *seg_list = segment_list();
		if (seg_list->count() == 0)
			return false;

		ISection *section = seg_list->item(seg_list->count() - 1);
		ctx.manager->Add(AlignValue(section->address() + section->size(), segment_alignment()), -1, mtReadable | mtExecutable | mtWritable | mtNotPaged);

		return true;
	};
protected:
	virtual bool ReadHeader(uint32_t /*open_mode*/) { return true; }
private:
	TestSegmentListT<TTestConfig> *segment_list_;
	TestExportListT<TTestConfig> *export_list_;
	IFunctionList *function_list_;
	TestFixupListT<TTestConfig> *fixup_list_;
	TestImportListT<TTestConfig> *import_list_;
	OperandSize cpu_address_size_;
	IVirtualMachineList *virtual_machine_list_;
	typename TTestConfig::RuntimeFunctionList *runtime_function_list_;
	uint64_t time_stamp_;

	// no copy ctr or assignment op
	TestArchitectureT(const TestArchitectureT &);
	TestArchitectureT &operator =(const TestArchitectureT &);
};

template<class TTestConfig>
class TestFileT : public TTestConfig::File
{
public:
	explicit TestFileT(OperandSize cpu_address_size) 
		: TTestConfig::File(NULL)
	{
		Add(cpu_address_size);
	}

	explicit TestFileT(const typename TTestConfig::File &src, const char *file_name)
		: TTestConfig::File(src, file_name) {}

	void OpenFromMemory(const void *buf, uint32_t len)
	{
		this->CloseStream();
		this->stream_ = new MemoryStream;
		/*size_t res = */this->stream_->Write(buf, len);

		ISection *segment = item(0)->segment_list()->item(0);
		uint8_t b = 0xcc;
		for (size_t i = len; i < segment->physical_size(); i++) {
			this->stream_->Write(&b, sizeof(b));
		}

		typename TTestConfig::FileHelper helper;
		helper.Parse(*item(0));
	}

	TestArchitectureT<TTestConfig> *item(size_t index) const { return reinterpret_cast<TestArchitectureT<TTestConfig> *>(TTestConfig::File::item(index)); }

	IArchitecture *Add(OperandSize cpu_address_size) 
	{
		auto arch = new TestArchitectureT<TTestConfig>(this, cpu_address_size);
		this->AddObject(arch);
		return arch;
	}

	TestFileT *Clone(const char *file_name) const
	{
		TestFileT *file = new TestFileT(*this, file_name);
		return file;
	}

protected:
	virtual OpenStatus ReadHeader(uint32_t /*open_mode*/) { return osSuccess; };
	virtual IFile *runtime() const { return NULL; }
private:
	std::string map_file_name_;
};

class TestLog: public ILog
{
public:
	virtual void AddMessage(MessageType /*type*/, IObject * /*sender*/, const std::string & /*message*/) { return; }
	virtual void StartProgress(const std::string & /*caption*/, unsigned long long /*max*/) { return; }
	virtual void StepProgress(unsigned long long /*value*/ = 1ull, bool /*is_project*/ = false) { return; }
	virtual void EndProgress() { return; }
};

#endif