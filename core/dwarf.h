#ifndef DWARF_H
#define DWARF_H

class CommonInformationEntryList;
class IArchitecture;

class EncodedData : public std::vector<uint8_t>
{
public:
	EncodedData(uint64_t address = 0, OperandSize pointer_size = osByte);
	void ReadFromFile(IArchitecture &file, size_t size);
	uint8_t ReadByte(size_t *pos) const;
	uint16_t ReadWord(size_t *pos) const;
	uint32_t ReadDWord(size_t *pos) const;
	uint64_t ReadQWord(size_t *pos) const;
	uint64_t ReadUleb128(size_t *pos) const;
	int64_t ReadSleb128(size_t *pos) const;
	std::string ReadString(size_t *pos) const;
	uint32_t ReadUnsigned(size_t *pos) const;
	uint64_t ReadEncoding(uint8_t encoding, size_t *pos) const;
	void WriteString(const std::string &str);
	void WriteUleb128(uint64_t value);
	void WriteSleb128(int64_t value);
	void WriteByte(uint8_t value);
	void WriteWord(uint16_t value);
	void WriteDWord(uint32_t value);
	void WriteQWord(uint64_t value);
	void WriteEncoding(uint8_t encoding, uint64_t value);
	uint64_t address() const { return address_; }
	OperandSize pointer_size() const { return pointer_size_; }
	void Read(void *buf, size_t count, size_t *pos) const;
	void Write(const void *buf, size_t count);
	size_t encoding_size(uint8_t encoding) const;
private:
	uint64_t address_;
	OperandSize pointer_size_;
};

class CommonInformationEntry : public IObject
{
public:
	explicit CommonInformationEntry(CommonInformationEntryList *owner, uint8_t version, const std::string &augmentation, 
		uint64_t code_alignment_factor,	uint64_t data_alignment_factor, uint8_t return_adddress_register, uint8_t fde_encoding,
		uint8_t lsda_encoding, uint8_t personality_encoding, uint64_t personality_routine, const std::vector<uint8_t> &initial_instructions);
	explicit CommonInformationEntry(CommonInformationEntryList *owner, const CommonInformationEntry &src);
	~CommonInformationEntry();
	CommonInformationEntry *Clone(CommonInformationEntryList *owner) const;
	uint8_t version() const { return version_; }
	std::string augmentation() const { return augmentation_; }
	uint64_t code_alignment_factor() const { return code_alignment_factor_; }
	uint64_t data_alignment_factor() const { return data_alignment_factor_; }
	uint8_t return_address_register() const { return return_address_register_; }
	uint8_t fde_encoding() const { return fde_encoding_; }
	uint8_t lsda_encoding() const { return lsda_encoding_; }
	uint8_t personality_encoding() const { return personality_encoding_; }
	uint64_t personality_routine() const { return personality_routine_; }
	std::vector<uint8_t> initial_instructions() const { return initial_instructions_; }
	void Rebase(uint64_t delta_base);
private:
	CommonInformationEntryList *owner_;
	uint8_t version_;
	std::string augmentation_;
	uint64_t code_alignment_factor_;
	uint64_t data_alignment_factor_;
	uint8_t return_address_register_;
	uint8_t fde_encoding_;
	uint8_t lsda_encoding_;
	uint8_t personality_encoding_;
	uint64_t personality_routine_;
	std::vector<uint8_t> initial_instructions_;
};

class CommonInformationEntryList : public ObjectList<CommonInformationEntry>
{
public:
	explicit CommonInformationEntryList();
	explicit CommonInformationEntryList(const CommonInformationEntryList &src);
	CommonInformationEntry *Add(uint8_t version, const std::string &augmentation, uint64_t code_alignment_factor,
		uint64_t data_alignment_factor, uint8_t return_address_register, uint8_t fde_encoding,
		uint8_t lsda_encoding, uint8_t personality_encoding, uint64_t personality_routine, const std::vector<uint8_t> &call_frame_instructions);
	CommonInformationEntryList *Clone() const;
	void Rebase(uint64_t delta_base);
private:
	// no assignment op
	CommonInformationEntryList &operator =(const CommonInformationEntryList &);
};

class DwarfParser
{
public:
	static uint32_t CreateCompactEncoding(IArchitecture &file, const std::vector<uint8_t> &fde_instructions, CommonInformationEntry *cie_, uint64_t start);
private:
	enum { kMaxRegisterNumber = 120 };
	enum RegisterSavedWhere { kRegisterUnused, kRegisterInCFA, kRegisterOffsetFromCFA,
								kRegisterInRegister, kRegisterAtExpression, kRegisterIsExpression } ;
	struct RegisterLocation {
		RegisterSavedWhere	location;
		int64_t				value;
	};

	struct PrologInfo {
		uint32_t			cfaRegister;		
		int32_t				cfaRegisterOffset;	// CFA = (cfaRegister)+cfaRegisterOffset
		int64_t				cfaExpression;		// CFA = expression
		uint32_t			spExtraArgSize;
		uint32_t			codeOffsetAtStackDecrement;
		uint8_t				registerSavedTwiceInCIE;
		bool				registersInOtherRegisters;
		bool				registerSavedMoreThanOnce;
		bool				cfaOffsetWasNegative;
		bool				sameValueUsed;
		RegisterLocation	savedRegisters[kMaxRegisterNumber];	// from where to restore registers
	};
	static bool ParseInstructions(const std::vector<uint8_t> &instructions, CommonInformationEntry *cie_, PrologInfo &info);
	static uint32_t CreateCompactEncodingForX32(IArchitecture &file, uint64_t start, PrologInfo &info);
	static uint32_t CreateCompactEncodingForX64(IArchitecture &file, uint64_t start, PrologInfo &info);
	static uint32_t GetRBPEncodedRegister(uint32_t reg, int32_t regOffsetFromBaseOffset, bool &failure);
	static uint32_t GetEBPEncodedRegister(uint32_t reg, int32_t regOffsetFromBaseOffset, bool &failure);
};

#endif