#include "../runtime/crypto.h"
#include "objects.h"
#include "files.h"
#include "dwarf.h"

/**
 * EncodedData
 */

EncodedData::EncodedData(uint64_t address, OperandSize pointer_size)
	: std::vector<uint8_t>(), address_(address), pointer_size_(pointer_size)
{

}

void EncodedData::Read(void *buf, size_t count, size_t *pos) const
{
	if (*pos + count > size())
		throw std::runtime_error("buffer index out of bounds");
	memcpy(buf, data() + *pos, count);
	*pos += count;
}

void EncodedData::ReadFromFile(IArchitecture &file, size_t size)
{
	resize(size);
	file.Read(data(), size);
}

uint8_t EncodedData::ReadByte(size_t *pos) const
{
	uint8_t res;
	Read(&res, sizeof(res), pos);
	return res;
}

uint16_t EncodedData::ReadWord(size_t *pos) const
{
	uint16_t res;
	Read(&res, sizeof(res), pos);
	return res;
}

uint32_t EncodedData::ReadDWord(size_t *pos) const
{
	uint32_t res;
	Read(&res, sizeof(res), pos);
	return res;
}

uint64_t EncodedData::ReadQWord(size_t *pos) const
{
	uint64_t res;
	Read(&res, sizeof(res), pos);
	return res;
}

uint64_t EncodedData::ReadUleb128(size_t *pos) const
{
	uint64_t result = 0, slice;
	int bit = 0;
	uint8_t byte;

	do {
		byte = ReadByte(pos);
		slice = byte & 0x7f;
		if (bit > 63) {
			throw std::runtime_error("uleb128 too big for uint64");
		} else {
			result |= (slice << bit);
			bit += 7;
		}
	} while (byte & 0x80);
	return result;
}

int64_t EncodedData::ReadSleb128(size_t *pos) const
{
	int64_t result = 0;
	int bit = 0;
	uint8_t byte;

	do {
		byte = ReadByte(pos);
		result |= (int64_t(byte & 0x7f) << bit);
		bit += 7;
	} while (byte & 0x80);
	/* Sign extend negative numbers. */
	if ((byte & 0x40) != 0)
		result |= (-1LL) << bit; //-V610
	return result;
}

std::string EncodedData::ReadString(size_t *pos) const
{
	std::string res;

	while (true) {
		char c = ReadByte(pos);
		if (c == '\0')
			break;
		res.push_back(c);
	}

	return res;
}

uint32_t EncodedData::ReadUnsigned(size_t *pos) const
{
	static const uint32_t len_tab[0x10] = {
		1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 5
	};

	uint8_t c = at(*pos) & 0xf;
	size_t size = len_tab[c];
	*pos += size;
	uint32_t res = 0;
	bool need_shift = true;
	if (size > 4) {
		size = 4;
		need_shift = false;
	}
	memcpy(&res, data() + *pos - size, size);
	if (need_shift)
		res >>= len_tab[c];
	return res;
}

uint64_t EncodedData::ReadEncoding(uint8_t encoding, size_t *pos) const
{
	uint64_t base;
	switch (encoding & 0x70) {
	case DW_EH_PE_pcrel:
		base = address_ + *pos;
		break;
	case DW_EH_PE_datarel:
		base = address_;
		break;
	default:
		base = 0;
		break;
	}

	switch (encoding & 0x0f) {
	case DW_EH_PE_absptr:
		if (pointer_size_ == osDWord)
			return base + static_cast<int32_t>(ReadDWord(pos));
		if (pointer_size_ == osQWord)
			return base + static_cast<int64_t>(ReadQWord(pos));
		break;
	case DW_EH_PE_uleb128:
		return base + ReadUleb128(pos);
	case DW_EH_PE_sleb128:
		return base + ReadSleb128(pos);
	case DW_EH_PE_udata2:
		return base + ReadWord(pos);
	case DW_EH_PE_sdata2:
		return base + static_cast<int16_t>(ReadWord(pos));
	case DW_EH_PE_udata4:
		return base + ReadDWord(pos);
	case DW_EH_PE_sdata4:
		return base + static_cast<int32_t>(ReadDWord(pos));
	case DW_EH_PE_udata8:
		return base + ReadQWord(pos);
	case DW_EH_PE_sdata8:
		return base + static_cast<int64_t>(ReadQWord(pos));
	}
	throw std::runtime_error("Invalid encoding");
}

void EncodedData::WriteUleb128(uint64_t value)
{
	uint8_t byte;
	do {
		byte = value & 0x7F;
		value &= ~(uint64_t)0x7F;		
		if ( value != 0 )
			byte |= 0x80;
		push_back(byte);
		value = value >> 7;
	} while( byte >= 0x80 );
}

void EncodedData::WriteSleb128(int64_t value) 
{
	bool is_neg = ( value < 0 );
	uint8_t byte;
	bool more;
	do {
		byte = value & 0x7F;
		value = value >> 7;
		if (is_neg) 
			more = ((value != -1) || ((byte & 0x40) == 0));
		else
			more = ((value != 0) || ((byte & 0x40) != 0));
		if (more)
			byte |= 0x80;
		push_back(byte);
	} 
	while( more );
}

void EncodedData::WriteString(const std::string &str)
{
	Write(str.c_str(), str.size() + 1);
}

void EncodedData::WriteByte(uint8_t value)
{
	push_back(value);
}

void EncodedData::WriteWord(uint16_t value)
{
	Write(&value, sizeof(value));
}

void EncodedData::WriteDWord(uint32_t value)
{
	Write(&value, sizeof(value));
}

void EncodedData::WriteQWord(uint64_t value)
{
	Write(&value, sizeof(value));
}

void EncodedData::Write(const void *buf, size_t count)
{
	insert(end(), static_cast<const uint8_t *>(buf), static_cast<const uint8_t *>(buf) + count);
}

void EncodedData::WriteEncoding(uint8_t encoding, uint64_t value)
{
	switch (encoding & 0x70) {
	case DW_EH_PE_pcrel:
		value -= address_ + size();
		break;
	case DW_EH_PE_datarel:
		value -= address_;
		break;
	}

	switch (encoding & 0x0f) {
	case DW_EH_PE_absptr:
		if (pointer_size_ == osDWord) {
			WriteDWord(static_cast<uint32_t>(value));
			return;
		}
		if (pointer_size_ == osQWord) {
			WriteQWord(value);
			return;
		}
		break;
	case DW_EH_PE_uleb128:
		WriteUleb128(value);
		return;
	case DW_EH_PE_sleb128:
		WriteSleb128(value);
		return;
	case DW_EH_PE_udata2:
	case DW_EH_PE_sdata2:
		WriteWord(static_cast<uint16_t>(value));
		return;
	case DW_EH_PE_udata4:
	case DW_EH_PE_sdata4:
		WriteDWord(static_cast<uint32_t>(value));
		return;
	case DW_EH_PE_udata8:
	case DW_EH_PE_sdata8:
		WriteQWord(value);
		return;
	}
	throw std::runtime_error("Invalid encoding");
}

size_t EncodedData::encoding_size(uint8_t encoding) const
{
	switch (encoding & 0x0f) {
	case DW_EH_PE_absptr:
		return OperandSizeToValue(pointer_size_);
	case DW_EH_PE_udata2:
	case DW_EH_PE_sdata2:
		return sizeof(uint16_t);
	case DW_EH_PE_udata4:
	case DW_EH_PE_sdata4:
		return sizeof(uint32_t);
	case DW_EH_PE_udata8:
	case DW_EH_PE_sdata8:
		return sizeof(uint64_t);
	}
	throw std::runtime_error("Invalid encoding");
}

/**
 * CommonInformationEntry
 */

CommonInformationEntry::CommonInformationEntry(CommonInformationEntryList *owner, uint8_t version, const std::string &augmentation,
	uint64_t code_alignment_factor,	uint64_t data_alignment_factor, uint8_t return_address_register, uint8_t fde_encoding,
	uint8_t lsda_encoding, uint8_t personality_encoding, uint64_t personality_routine, const std::vector<uint8_t> &initial_instructions)
	: IObject(), owner_(owner), version_(version), augmentation_(augmentation), code_alignment_factor_(code_alignment_factor), 
	data_alignment_factor_(data_alignment_factor), return_address_register_(return_address_register), fde_encoding_(fde_encoding), 
	lsda_encoding_(lsda_encoding), personality_encoding_(personality_encoding), personality_routine_(personality_routine), initial_instructions_(initial_instructions)
{

}

CommonInformationEntry::CommonInformationEntry(CommonInformationEntryList *owner, const CommonInformationEntry &src)
	: IObject(), owner_(owner)
{
	version_ = src.version_;
	code_alignment_factor_ = src.code_alignment_factor_;
	data_alignment_factor_ = src.data_alignment_factor_;
	return_address_register_ = src.return_address_register_;
	augmentation_ = src.augmentation_;
	fde_encoding_ = src.fde_encoding_;
	lsda_encoding_ = src.lsda_encoding_;
	personality_encoding_ = src.personality_encoding_;
	personality_routine_ = src.personality_routine_;
	initial_instructions_ = src.initial_instructions_;
}

CommonInformationEntry *CommonInformationEntry::Clone(CommonInformationEntryList *owner) const
{
	CommonInformationEntry *entry = new CommonInformationEntry(owner, *this);
	return entry;
}

CommonInformationEntry::~CommonInformationEntry()
{
	if (owner_)
		owner_->RemoveObject(this);
}

void CommonInformationEntry::Rebase(uint64_t delta_base)
{
	if (personality_routine_)
		personality_routine_ += delta_base;
}

/**
 * CommonInformationEntryList
 */

CommonInformationEntryList::CommonInformationEntryList()
	: ObjectList<CommonInformationEntry>()
{

}

CommonInformationEntryList::CommonInformationEntryList(const CommonInformationEntryList &src)
	: ObjectList<CommonInformationEntry>()
{
	for (size_t i = 0; i < src.count(); i++) {
		AddObject(src.item(i)->Clone(this));
	}
}

CommonInformationEntry *CommonInformationEntryList::Add(uint8_t version, const std::string &augmentation, 
	uint64_t code_alignment_factor,	uint64_t data_alignment_factor, uint8_t return_address_register, uint8_t fde_encoding,
	uint8_t lsda_encoding, uint8_t personality_encoding, uint64_t personality_routine, const std::vector<uint8_t> &call_frame_instructions)
{
	CommonInformationEntry *entry = new CommonInformationEntry(this, version, augmentation, code_alignment_factor,
		data_alignment_factor, return_address_register, fde_encoding, lsda_encoding, personality_encoding, personality_routine, call_frame_instructions);
	AddObject(entry);
	return entry;
}

CommonInformationEntryList *CommonInformationEntryList::Clone() const
{
	CommonInformationEntryList *list = new CommonInformationEntryList(*this);
	return list;
}

void CommonInformationEntryList::Rebase(uint64_t delta_base)
{
	for (size_t i = 0; i < count(); i++) {
		item(i)->Rebase(delta_base);
	}
}

/**
 * DwarfParser
 */

uint32_t DwarfParser::CreateCompactEncoding(IArchitecture &file, const std::vector<uint8_t> &fde_instructions, CommonInformationEntry *cie_, uint64_t start) 
{
	PrologInfo info = PrologInfo();
	if (ParseInstructions(cie_->initial_instructions(), cie_, info) && ParseInstructions(fde_instructions, cie_, info))
		return (file.cpu_address_size() == osDWord) ? CreateCompactEncodingForX32(file, start, info) : CreateCompactEncodingForX64(file, start, info);
	return UNWIND_X86_MODE_DWARF;
}

bool DwarfParser::ParseInstructions(const std::vector<uint8_t> &instructions, CommonInformationEntry *cie_, PrologInfo &info)
{
	EncodedData data;
	data.Write(instructions.data(), instructions.size());

	uint64_t reg, reg2, length;
	uint64_t code_offset = 0;
	int64_t offset;
	PrologInfo initial_state = info;

	for (size_t pos = 0; pos < data.size(); ) {
		uint8_t opcode = data.ReadByte(&pos);
		uint8_t operand;
		switch (opcode) {
		case DW_CFA_nop:
			break;
		case DW_CFA_set_loc:
			code_offset = data.ReadEncoding(cie_->fde_encoding(), &pos);
			break;
		case DW_CFA_advance_loc1:
			code_offset += data.ReadByte(&pos) * cie_->code_alignment_factor();
			break;
		case DW_CFA_advance_loc2:
			code_offset += data.ReadWord(&pos) * cie_->code_alignment_factor();
			break;
		case DW_CFA_advance_loc4:
			code_offset += data.ReadDWord(&pos) * cie_->code_alignment_factor();
			break;
		case DW_CFA_offset_extended:
			reg = data.ReadUleb128(&pos);
			offset = data.ReadUleb128(&pos) * cie_->data_alignment_factor();
			if (reg >= kMaxRegisterNumber)
				return false;
			if (info.savedRegisters[reg].location != kRegisterUnused) 
				info.registerSavedMoreThanOnce = true;
			info.savedRegisters[reg].location = kRegisterInCFA;
			info.savedRegisters[reg].value = offset;
			break;
		case DW_CFA_restore_extended:
			reg = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg] = initial_state.savedRegisters[reg];
			break;
		case DW_CFA_undefined:
			reg = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg].location = kRegisterUnused;
			break;
		case DW_CFA_same_value:
			reg = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg].location = kRegisterUnused;
			info.sameValueUsed = true;
			break;
		case DW_CFA_register:
			reg = data.ReadUleb128(&pos);
			reg2 = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			if (reg2 >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg].location = kRegisterInRegister;
			info.savedRegisters[reg].value = reg2;
			info.registersInOtherRegisters = true;
			break;
		case DW_CFA_remember_state:
			break;
		case DW_CFA_restore_state:
			break;
		case DW_CFA_def_cfa:
			reg = data.ReadUleb128(&pos);
			offset = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			info.cfaRegister = static_cast<uint32_t>(reg);
			info.cfaRegisterOffset = static_cast<int32_t>(offset);
			if (offset > 0x80000000)
				info.cfaOffsetWasNegative = true;
			break;
		case DW_CFA_def_cfa_register:
			reg = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			info.cfaRegister = static_cast<uint32_t>(reg);
			break;
		case DW_CFA_def_cfa_offset:
			info.cfaRegisterOffset = static_cast<int32_t>(data.ReadUleb128(&pos));
			info.codeOffsetAtStackDecrement = static_cast<uint32_t>(code_offset);
			break;
		case DW_CFA_def_cfa_expression:
			info.cfaRegister = 0;
			info.cfaExpression = pos;
			length = data.ReadUleb128(&pos);
			pos += static_cast<size_t>(length);
			break;
		case DW_CFA_expression:
			reg = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg].location = kRegisterAtExpression;
			info.savedRegisters[reg].value = pos;
			length = data.ReadUleb128(&pos);
			pos += static_cast<size_t>(length);
			break;
		case DW_CFA_offset_extended_sf:
			reg = data.ReadUleb128(&pos);
			offset = data.ReadSleb128(&pos) * cie_->data_alignment_factor();
			if (reg >= kMaxRegisterNumber)
				return false;
			if (info.savedRegisters[reg].location != kRegisterUnused)
				info.registerSavedMoreThanOnce = true;
			info.savedRegisters[reg].location = kRegisterInCFA;
			info.savedRegisters[reg].value = offset;
			break;
		case DW_CFA_def_cfa_sf:
			reg = data.ReadUleb128(&pos);
			offset = data.ReadSleb128(&pos) * cie_->data_alignment_factor();
			if (reg >= kMaxRegisterNumber)
				return false;
			info.cfaRegister = static_cast<uint32_t>(reg);
			info.cfaRegisterOffset = static_cast<int32_t>(offset);
			break;
		case DW_CFA_def_cfa_offset_sf:
			info.cfaRegisterOffset = static_cast<int32_t>(data.ReadSleb128(&pos) * cie_->data_alignment_factor());
			info.codeOffsetAtStackDecrement = static_cast<uint32_t>(code_offset);
			break;
		case DW_CFA_val_offset:
			reg = data.ReadUleb128(&pos);
			offset = data.ReadUleb128(&pos) * cie_->data_alignment_factor();
			if (reg >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg].location = kRegisterOffsetFromCFA;
			info.savedRegisters[reg].value = offset;
			break;
		case DW_CFA_val_offset_sf:
			reg = data.ReadUleb128(&pos);
			offset = data.ReadSleb128(&pos) * cie_->data_alignment_factor();
			if (reg >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg].location = kRegisterOffsetFromCFA;
			info.savedRegisters[reg].value = offset;
			break;
		case DW_CFA_val_expression:
			reg = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			info.savedRegisters[reg].location = kRegisterIsExpression;
			info.savedRegisters[reg].value = pos;
			length = data.ReadUleb128(&pos);
			pos += static_cast<size_t>(length);
			break;
		case DW_CFA_GNU_args_size:
			offset = data.ReadUleb128(&pos);
			info.spExtraArgSize = static_cast<uint32_t>(offset);
			break;
		case DW_CFA_GNU_negative_offset_extended:
			reg = data.ReadUleb128(&pos);
			if (reg >= kMaxRegisterNumber)
				return false;
			offset = data.ReadUleb128(&pos) * cie_->data_alignment_factor();
			if (info.savedRegisters[reg].location != kRegisterUnused) 
				info.registerSavedMoreThanOnce = true;
			info.savedRegisters[reg].location = kRegisterInCFA;
			info.savedRegisters[reg].value = -offset;
			break;
		default:
			operand = opcode & 0x3F;
			switch ( opcode & 0xC0 ) {
				case DW_CFA_offset:
					reg = operand;
					offset = data.ReadUleb128(&pos) * cie_->data_alignment_factor();
					if (info.savedRegisters[reg].location != kRegisterUnused) {
						/*
							if ( (pcoffset == (pint_t)(-1)) 
								&& (results->savedRegisters[reg].location == kRegisterInCFA) 
								&& (results->savedRegisters[reg].value == offset)  )
								results->registerSavedTwiceInCIE = reg;
							else
								results->registerSavedMoreThanOnce = true;
						*/
					}
					info.savedRegisters[reg].location = kRegisterInCFA;
					info.savedRegisters[reg].value = offset;
					break;
				case DW_CFA_advance_loc:
					code_offset += operand * cie_->code_alignment_factor();
					break;
				case DW_CFA_restore:
					reg = operand;
					info.savedRegisters[reg] = initial_state.savedRegisters[reg];
					break;
				default:
					return false;
			}
		}
	}

	return true;
}

uint32_t DwarfParser::GetRBPEncodedRegister(uint32_t reg, int32_t regOffsetFromBaseOffset, bool &failure)
{
	if ( (regOffsetFromBaseOffset < 0) || (regOffsetFromBaseOffset > 32) ) {
		failure = true;
		return 0;
	}

	unsigned int slotIndex = regOffsetFromBaseOffset/8;

	switch ( reg ) {
		case UNW_X86_64_RBX:
			return UNWIND_X86_64_REG_RBX << (slotIndex*3);
		case UNW_X86_64_R12:
			return UNWIND_X86_64_REG_R12 << (slotIndex*3);
		case UNW_X86_64_R13:
			return UNWIND_X86_64_REG_R13 << (slotIndex*3);
		case UNW_X86_64_R14:
			return UNWIND_X86_64_REG_R14 << (slotIndex*3);
		case UNW_X86_64_R15:
			return UNWIND_X86_64_REG_R15 << (slotIndex*3);
	}

	// invalid register
	failure = true;
	return 0;
}

uint32_t DwarfParser::CreateCompactEncodingForX64(IArchitecture &file, uint64_t start, PrologInfo &prolog)
{
	if (prolog.registerSavedTwiceInCIE == DW_X86_64_RET_ADDR)
		return UNWIND_X86_64_MODE_DWARF;
	// don't create compact unwind info for unsupported dwarf kinds
	if (prolog.registerSavedMoreThanOnce)
		return UNWIND_X86_64_MODE_DWARF;
	if (prolog.cfaOffsetWasNegative)
		return UNWIND_X86_64_MODE_DWARF;
	if (prolog.spExtraArgSize != 0)
		return UNWIND_X86_64_MODE_DWARF;
	if (prolog.sameValueUsed)
		return UNWIND_X86_64_MODE_DWARF;

	// figure out which kind of frame this function uses
	bool standardRBPframe = ( 
		 (prolog.cfaRegister == UNW_X86_64_RBP) 
	  && (prolog.cfaRegisterOffset == 16)
	  && (prolog.savedRegisters[UNW_X86_64_RBP].location == kRegisterInCFA)
	  && (prolog.savedRegisters[UNW_X86_64_RBP].value == -16) );
	bool standardRSPframe = (prolog.cfaRegister == UNW_X86_64_RSP);
	if (!standardRBPframe && !standardRSPframe)
		return UNWIND_X86_64_MODE_DWARF;
	
	// scan which registers are saved
	int saveRegisterCount = 0;
	bool rbxSaved = false;
	bool r12Saved = false;
	bool r13Saved = false;
	bool r14Saved = false;
	bool r15Saved = false;
	bool rbpSaved = false;
	for (int i=0; i < 64; ++i) {
		if ( prolog.savedRegisters[i].location != kRegisterUnused ) {
			if ( prolog.savedRegisters[i].location != kRegisterInCFA )
				return UNWIND_X86_64_MODE_DWARF;
			switch (i) {
				case UNW_X86_64_RBX:
					rbxSaved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_64_R12:
					r12Saved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_64_R13:
					r13Saved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_64_R14:
					r14Saved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_64_R15:
					r15Saved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_64_RBP:
					rbpSaved = true;
					++saveRegisterCount;
					break;
				case DW_X86_64_RET_ADDR:
					break;
				default:
					return UNWIND_X86_64_MODE_DWARF;
			}
		}
	}
	const int64_t cfaOffsetRBX = prolog.savedRegisters[UNW_X86_64_RBX].value;
	const int64_t cfaOffsetR12 = prolog.savedRegisters[UNW_X86_64_R12].value;
	const int64_t cfaOffsetR13 = prolog.savedRegisters[UNW_X86_64_R13].value;
	const int64_t cfaOffsetR14 = prolog.savedRegisters[UNW_X86_64_R14].value;
	const int64_t cfaOffsetR15 = prolog.savedRegisters[UNW_X86_64_R15].value;
	const int64_t cfaOffsetRBP = prolog.savedRegisters[UNW_X86_64_RBP].value;
	
	// encode standard RBP frames
	uint32_t  encoding = 0;
	if ( standardRBPframe ) {
		//		|              |
		//		+--------------+   <- CFA
		//		|   ret addr   |
		//		+--------------+
		//		|     rbp      |
		//		+--------------+   <- rbp
		//		~              ~
		//		+--------------+   
		//		|  saved reg3  |
		//		+--------------+   <- CFA - offset+16
		//		|  saved reg2  |
		//		+--------------+   <- CFA - offset+8
		//		|  saved reg1  |
		//		+--------------+   <- CFA - offset
		//		|              |
		//		+--------------+
		//		|              |
		//						   <- rsp
		//
		encoding = UNWIND_X86_64_MODE_RBP_FRAME;
		
		// find save location of farthest register from rbp
		int64_t furthestCfaOffset = 0;
		if ( rbxSaved & (cfaOffsetRBX < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetRBX;
		if ( r12Saved & (cfaOffsetR12 < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetR12;
		if ( r13Saved & (cfaOffsetR13 < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetR13;
		if ( r14Saved & (cfaOffsetR14 < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetR14;
		if ( r15Saved & (cfaOffsetR15 < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetR15;
		
		if ( furthestCfaOffset == 0 ) {
			// no registers saved, nothing more to encode
			return encoding;
		}
		
		// add stack offset to encoding
		int64_t rbpOffset = furthestCfaOffset + 16;
		int64_t encodedOffset = rbpOffset/(-8);
		if ( encodedOffset > 255 )
			return UNWIND_X86_64_MODE_DWARF;
		encoding |= (encodedOffset << __builtin_ctz(UNWIND_X86_64_RBP_FRAME_OFFSET));
		
		// add register saved from each stack location
		bool encodingFailure = false;
		if ( rbxSaved )
			encoding |= GetRBPEncodedRegister(UNW_X86_64_RBX, static_cast<int32_t>(cfaOffsetRBX - furthestCfaOffset), encodingFailure);
		if ( r12Saved )
			encoding |= GetRBPEncodedRegister(UNW_X86_64_R12, static_cast<int32_t>(cfaOffsetR12 - furthestCfaOffset), encodingFailure);
		if ( r13Saved )
			encoding |= GetRBPEncodedRegister(UNW_X86_64_R13, static_cast<int32_t>(cfaOffsetR13 - furthestCfaOffset), encodingFailure);
		if ( r14Saved )
			encoding |= GetRBPEncodedRegister(UNW_X86_64_R14, static_cast<int32_t>(cfaOffsetR14 - furthestCfaOffset), encodingFailure);
		if ( r15Saved )
			encoding |= GetRBPEncodedRegister(UNW_X86_64_R15, static_cast<int32_t>(cfaOffsetR15 - furthestCfaOffset), encodingFailure);
		
		if ( encodingFailure )
			return UNWIND_X86_64_MODE_DWARF;

		return encoding;
	}
	else {
		//		|              |
		//		+--------------+   <- CFA
		//		|   ret addr   |
		//		+--------------+
		//		|  saved reg1  |
		//		+--------------+   <- CFA - 16
		//		|  saved reg2  |
		//		+--------------+   <- CFA - 24
		//		|  saved reg3  |
		//		+--------------+   <- CFA - 32
		//		|  saved reg4  |
		//		+--------------+   <- CFA - 40
		//		|  saved reg5  |
		//		+--------------+   <- CFA - 48
		//		|  saved reg6  |
		//		+--------------+   <- CFA - 56
		//		|              |
		//						   <- esp
		//

		// for RSP based frames we need to encode stack size in unwind info
		encoding = UNWIND_X86_64_MODE_STACK_IMMD;
		uint64_t stackValue = prolog.cfaRegisterOffset / 8;
		uint32_t stackAdjust = 0;
		bool immedStackSize = true;
		if ( stackValue > (UNWIND_X86_64_FRAMELESS_STACK_SIZE >> __builtin_ctz(UNWIND_X86_64_FRAMELESS_STACK_SIZE)) ) {
			// stack size is too big to fit as an immediate value, so encode offset of subq instruction in function
			if	(prolog.codeOffsetAtStackDecrement == 0)
				return UNWIND_X86_64_MODE_DWARF;

			uint64_t functionContentAdjustStackIns = start + prolog.codeOffsetAtStackDecrement - 4;
			uint64_t pos = file.Tell();
			if (!file.AddressSeek(functionContentAdjustStackIns))
				return UNWIND_X86_64_MODE_DWARF;

			uint32_t stackDecrementInCode = file.ReadDWord();
			file.Seek(pos);
			stackAdjust = (prolog.cfaRegisterOffset - stackDecrementInCode)/8;
			stackValue = functionContentAdjustStackIns - start;
			immedStackSize = false;
			if ( stackAdjust > 7 )
				return UNWIND_X86_64_MODE_DWARF;
			encoding = UNWIND_X86_64_MODE_STACK_IND;
		}	
		
		// validate that saved registers are all within 6 slots abutting return address
		int registers[6];
		for (int i=0; i < 6;++i)
			registers[i] = 0;
		if ( r15Saved ) {
			if ( cfaOffsetR15 < -56 )
				return UNWIND_X86_64_MODE_DWARF;
			registers[(cfaOffsetR15+56)/8] = UNWIND_X86_64_REG_R15;
		}
		if ( r14Saved ) {
			if ( cfaOffsetR14 < -56 )
				return UNWIND_X86_64_MODE_DWARF;
			registers[(cfaOffsetR14+56)/8] = UNWIND_X86_64_REG_R14;
		}
		if ( r13Saved ) {
			if ( cfaOffsetR13 < -56 )
				return UNWIND_X86_64_MODE_DWARF;
			registers[(cfaOffsetR13+56)/8] = UNWIND_X86_64_REG_R13;
		}
		if ( r12Saved ) {
			if ( cfaOffsetR12 < -56 )
				return UNWIND_X86_64_MODE_DWARF;
			registers[(cfaOffsetR12+56)/8] = UNWIND_X86_64_REG_R12;
		}
		if ( rbxSaved ) {
			if ( cfaOffsetRBX < -56 )
				return UNWIND_X86_64_MODE_DWARF;
			registers[(cfaOffsetRBX+56)/8] = UNWIND_X86_64_REG_RBX;
		}
		if ( rbpSaved ) {
			if ( cfaOffsetRBP < -56 )
				return UNWIND_X86_64_MODE_DWARF;
			registers[(cfaOffsetRBP+56)/8] = UNWIND_X86_64_REG_RBP;
		}
		
		// validate that saved registers are contiguous and abut return address on stack
		for (int i=0; i < saveRegisterCount; ++i) {
			if ( registers[5-i] == 0 ) {
				return UNWIND_X86_64_MODE_DWARF;
			}
		}
				
		// encode register permutation
		// the 10-bits are encoded differently depending on the number of registers saved
		int renumregs[6];
		for (int i=6-saveRegisterCount; i < 6; ++i) {
			int countless = 0;
			for (int j=6-saveRegisterCount; j < i; ++j) {
				if ( registers[j] < registers[i] )
					++countless;
			}
			renumregs[i] = registers[i] - countless -1;
		}
		uint32_t permutationEncoding = 0;
		switch ( saveRegisterCount ) {
			case 6:
				permutationEncoding |= (120*renumregs[0] + 24*renumregs[1] + 6*renumregs[2] + 2*renumregs[3] + renumregs[4]);
				break;
			case 5:
				permutationEncoding |= (120*renumregs[1] + 24*renumregs[2] + 6*renumregs[3] + 2*renumregs[4] + renumregs[5]);
				break;
			case 4:
				permutationEncoding |= (60*renumregs[2] + 12*renumregs[3] + 3*renumregs[4] + renumregs[5]);
				break;
			case 3:
				permutationEncoding |= (20*renumregs[3] + 4*renumregs[4] + renumregs[5]);
				break;
			case 2:
				permutationEncoding |= (5*renumregs[4] + renumregs[5]);
				break;
			case 1:
				permutationEncoding |= (renumregs[5]);
				break;
		}
		
		encoding |= (stackValue << __builtin_ctz(UNWIND_X86_64_FRAMELESS_STACK_SIZE));
		encoding |= (stackAdjust << __builtin_ctz(UNWIND_X86_64_FRAMELESS_STACK_ADJUST));
		encoding |= (saveRegisterCount << __builtin_ctz(UNWIND_X86_64_FRAMELESS_STACK_REG_COUNT));
		encoding |= (permutationEncoding << __builtin_ctz(UNWIND_X86_64_FRAMELESS_STACK_REG_PERMUTATION));
		return encoding;
	}
}

uint32_t DwarfParser::GetEBPEncodedRegister(uint32_t reg, int32_t regOffsetFromBaseOffset, bool& failure)
{
	if ( (regOffsetFromBaseOffset < 0) || (regOffsetFromBaseOffset > 16) ) {
		failure = true;
		return 0;
	}
	unsigned int slotIndex = regOffsetFromBaseOffset/4;
	
	switch ( reg ) {
		case UNW_X86_EBX:
			return UNWIND_X86_REG_EBX << (slotIndex*3);
		case UNW_X86_ECX:
			return UNWIND_X86_REG_ECX << (slotIndex*3);
		case UNW_X86_EDX:
			return UNWIND_X86_REG_EDX << (slotIndex*3);
		case UNW_X86_EDI:
			return UNWIND_X86_REG_EDI << (slotIndex*3);
		case UNW_X86_ESI:
			return UNWIND_X86_REG_ESI << (slotIndex*3);
	}
	
	// invalid register
	failure = true;
	return 0;
}

uint32_t DwarfParser::CreateCompactEncodingForX32(IArchitecture &file, uint64_t start, PrologInfo &prolog)
{
	if ( prolog.registerSavedTwiceInCIE == DW_X86_RET_ADDR )
		return UNWIND_X86_64_MODE_DWARF;
	// don't create compact unwind info for unsupported dwarf kinds
	if ( prolog.registerSavedMoreThanOnce )
		return UNWIND_X86_MODE_DWARF;
	if ( prolog.spExtraArgSize != 0 )
		return UNWIND_X86_MODE_DWARF;
	if ( prolog.sameValueUsed )
		return UNWIND_X86_MODE_DWARF;
	
	// figure out which kind of frame this function uses
	bool standardEBPframe = ( 
		 (prolog.cfaRegister == UNW_X86_EBP) 
	  && (prolog.cfaRegisterOffset == 8)
	  && (prolog.savedRegisters[UNW_X86_EBP].location == kRegisterInCFA)
	  && (prolog.savedRegisters[UNW_X86_EBP].value == -8) );
	bool standardESPframe = (prolog.cfaRegister == UNW_X86_ESP);
	if ( !standardEBPframe && !standardESPframe ) {
		// no compact encoding for this
		return UNWIND_X86_MODE_DWARF;
	}
	
	// scan which registers are saved
	int saveRegisterCount = 0;
	bool ebxSaved = false;
	bool ecxSaved = false;
	bool edxSaved = false;
	bool esiSaved = false;
	bool ediSaved = false;
	bool ebpSaved = false;
	for (int i=0; i < 64; ++i) {
		if ( prolog.savedRegisters[i].location != kRegisterUnused ) {
			if ( prolog.savedRegisters[i].location != kRegisterInCFA )
				return UNWIND_X86_MODE_DWARF;
			switch (i) {
				case UNW_X86_EBX:
					ebxSaved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_ECX:
					ecxSaved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_EDX:
					edxSaved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_ESI:
					esiSaved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_EDI:
					ediSaved = true;
					++saveRegisterCount;
					break;
				case UNW_X86_EBP:
					ebpSaved = true;
					++saveRegisterCount;
					break;
				case DW_X86_RET_ADDR:
					break;
				default:
					return UNWIND_X86_MODE_DWARF;
			}
		}
	}
	const int32_t cfaOffsetEBX = static_cast<int32_t>(prolog.savedRegisters[UNW_X86_EBX].value);
	const int32_t cfaOffsetECX = static_cast<int32_t>(prolog.savedRegisters[UNW_X86_ECX].value);
	const int32_t cfaOffsetEDX = static_cast<int32_t>(prolog.savedRegisters[UNW_X86_EDX].value);
	const int32_t cfaOffsetEDI = static_cast<int32_t>(prolog.savedRegisters[UNW_X86_EDI].value);
	const int32_t cfaOffsetESI = static_cast<int32_t>(prolog.savedRegisters[UNW_X86_ESI].value);
	const int32_t cfaOffsetEBP = static_cast<int32_t>(prolog.savedRegisters[UNW_X86_EBP].value);
	
	// encode standard RBP frames
	uint32_t encoding = 0;
	if ( standardEBPframe ) {
		//		|              |
		//		+--------------+   <- CFA
		//		|   ret addr   |
		//		+--------------+
		//		|     ebp      |
		//		+--------------+   <- ebp
		//		~              ~
		//		+--------------+   
		//		|  saved reg3  |
		//		+--------------+   <- CFA - offset+8
		//		|  saved reg2  |
		//		+--------------+   <- CFA - offset+e
		//		|  saved reg1  |
		//		+--------------+   <- CFA - offset
		//		|              |
		//		+--------------+
		//		|              |
		//						   <- esp
		//
		encoding = UNWIND_X86_MODE_EBP_FRAME;
		
		// find save location of farthest register from ebp
		int32_t furthestCfaOffset = 0;
		if ( ebxSaved & (cfaOffsetEBX < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetEBX;
		if ( ecxSaved & (cfaOffsetECX < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetECX;
		if ( edxSaved & (cfaOffsetEDX < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetEDX;
		if ( ediSaved & (cfaOffsetEDI < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetEDI;
		if ( esiSaved & (cfaOffsetESI < furthestCfaOffset) )
			furthestCfaOffset = cfaOffsetESI;
		
		if ( furthestCfaOffset == 0 ) {
			// no registers saved, nothing more to encode
			return encoding;
		}
		
		// add stack offset to encoding
		int ebpOffset = furthestCfaOffset + 8;
		int encodedOffset = ebpOffset/(-4);
		if ( encodedOffset > 255 )
			return UNWIND_X86_MODE_DWARF;
		encoding |= (encodedOffset << __builtin_ctz(UNWIND_X86_EBP_FRAME_OFFSET));
		
		// add register saved from each stack location
		bool encodingFailure = false;
		if ( ebxSaved )
			encoding |= GetEBPEncodedRegister(UNW_X86_EBX, cfaOffsetEBX - furthestCfaOffset, encodingFailure);
		if ( ecxSaved )
			encoding |= GetEBPEncodedRegister(UNW_X86_ECX, cfaOffsetECX - furthestCfaOffset, encodingFailure);
		if ( edxSaved )
			encoding |= GetEBPEncodedRegister(UNW_X86_EDX, cfaOffsetEDX - furthestCfaOffset, encodingFailure);
		if ( ediSaved )
			encoding |= GetEBPEncodedRegister(UNW_X86_EDI, cfaOffsetEDI - furthestCfaOffset, encodingFailure);
		if ( esiSaved )
			encoding |= GetEBPEncodedRegister(UNW_X86_ESI, cfaOffsetESI - furthestCfaOffset, encodingFailure);
		
		if ( encodingFailure )
			return UNWIND_X86_MODE_DWARF;

		return encoding;
	}
	else {
		//		|              |
		//		+--------------+   <- CFA
		//		|   ret addr   |
		//		+--------------+
		//		|  saved reg1  |
		//		+--------------+   <- CFA - 8
		//		|  saved reg2  |
		//		+--------------+   <- CFA - 12
		//		|  saved reg3  |
		//		+--------------+   <- CFA - 16
		//		|  saved reg4  |
		//		+--------------+   <- CFA - 20
		//		|  saved reg5  |
		//		+--------------+   <- CFA - 24
		//		|  saved reg6  |
		//		+--------------+   <- CFA - 28
		//		|              |
		//						   <- esp
		//

		// for ESP based frames we need to encode stack size in unwind info
		encoding = UNWIND_X86_MODE_STACK_IMMD;
		uint64_t stackValue = prolog.cfaRegisterOffset / 4;
		uint32_t stackAdjust = 0;
		bool immedStackSize = true;
		if ( stackValue > (UNWIND_X86_FRAMELESS_STACK_SIZE >> __builtin_ctz(UNWIND_X86_FRAMELESS_STACK_SIZE)) ) {
			// stack size is too big to fit as an immediate value, so encode offset of subq instruction in function
			uint64_t functionContentAdjustStackIns = start + prolog.codeOffsetAtStackDecrement - 4;		
			uint64_t pos = file.Tell();
			if (!file.AddressSeek(functionContentAdjustStackIns))
				return UNWIND_X86_64_MODE_DWARF;

			uint32_t stackDecrementInCode = file.ReadDWord();
			file.Seek(pos);
            stackAdjust = (prolog.cfaRegisterOffset - stackDecrementInCode)/4;
			stackValue = functionContentAdjustStackIns - start;
			immedStackSize = false;
			if ( stackAdjust > 7 )
				return UNWIND_X86_MODE_DWARF;
			encoding = UNWIND_X86_MODE_STACK_IND;
		}	
		
		
		// validate that saved registers are all within 6 slots abutting return address
		int registers[6];
		for (int i=0; i < 6;++i)
			registers[i] = 0;
		if ( ebxSaved ) {
			if ( cfaOffsetEBX < -28 )
				return UNWIND_X86_MODE_DWARF;
			registers[(cfaOffsetEBX+28)/4] = UNWIND_X86_REG_EBX;
		}
		if ( ecxSaved ) {
			if ( cfaOffsetECX < -28 )
				return UNWIND_X86_MODE_DWARF;
			registers[(cfaOffsetECX+28)/4] = UNWIND_X86_REG_ECX;
		}
		if ( edxSaved ) {
			if ( cfaOffsetEDX < -28 )
				return UNWIND_X86_MODE_DWARF;
			registers[(cfaOffsetEDX+28)/4] = UNWIND_X86_REG_EDX;
		}
		if ( ediSaved ) {
			if ( cfaOffsetEDI < -28 )
				return UNWIND_X86_MODE_DWARF;
			registers[(cfaOffsetEDI+28)/4] = UNWIND_X86_REG_EDI;
		}
		if ( esiSaved ) {
			if ( cfaOffsetESI < -28 )
				return UNWIND_X86_MODE_DWARF;
			registers[(cfaOffsetESI+28)/4] = UNWIND_X86_REG_ESI;
		}
		if ( ebpSaved ) {
			if ( cfaOffsetEBP < -28 )
				return UNWIND_X86_MODE_DWARF;
			registers[(cfaOffsetEBP+28)/4] = UNWIND_X86_REG_EBP;
		}
		
		// validate that saved registers are contiguous and abut return address on stack
		for (int i=0; i < saveRegisterCount; ++i) {
			if ( registers[5-i] == 0 )
				return UNWIND_X86_MODE_DWARF;
		}
				
		// encode register permutation
		// the 10-bits are encoded differently depending on the number of registers saved
		int renumregs[6];
		for (int i=6-saveRegisterCount; i < 6; ++i) {
			int countless = 0;
			for (int j=6-saveRegisterCount; j < i; ++j) {
				if ( registers[j] < registers[i] )
					++countless;
			}
			renumregs[i] = registers[i] - countless -1;
		}
		uint32_t permutationEncoding = 0;
		switch ( saveRegisterCount ) {
			case 6:
				permutationEncoding |= (120*renumregs[0] + 24*renumregs[1] + 6*renumregs[2] + 2*renumregs[3] + renumregs[4]);
				break;
			case 5:
				permutationEncoding |= (120*renumregs[1] + 24*renumregs[2] + 6*renumregs[3] + 2*renumregs[4] + renumregs[5]);
				break;
			case 4:
				permutationEncoding |= (60*renumregs[2] + 12*renumregs[3] + 3*renumregs[4] + renumregs[5]);
				break;
			case 3:
				permutationEncoding |= (20*renumregs[3] + 4*renumregs[4] + renumregs[5]);
				break;
			case 2:
				permutationEncoding |= (5*renumregs[4] + renumregs[5]);
				break;
			case 1:
				permutationEncoding |= (renumregs[5]);
				break;
		}
		
		encoding |= (stackValue << __builtin_ctz(UNWIND_X86_FRAMELESS_STACK_SIZE));
		encoding |= (stackAdjust << __builtin_ctz(UNWIND_X86_FRAMELESS_STACK_ADJUST));
		encoding |= (saveRegisterCount << __builtin_ctz(UNWIND_X86_FRAMELESS_STACK_REG_COUNT));
		encoding |= (permutationEncoding << __builtin_ctz(UNWIND_X86_FRAMELESS_STACK_REG_PERMUTATION));
		return encoding;
	}
}