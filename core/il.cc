#include "../runtime/common.h"
#include "../runtime/crypto.h"
#include "objects.h"
#include "osutils.h"
#include "streams.h"
#include "files.h"
#include "processors.h"
#include "lang.h"
#include "core.h"
#include "pefile.h"
#include "packer.h"
#include "dotnet.h"
#include "dotnetfile.h"
#include "il.h"

/**
 * ILCommand
 */

ILCommand::ILCommand(IFunction *owner, OperandSize size, uint64_t address)
	: BaseCommand(owner), address_(address), size_(size), type_(icUnknown), operand_value_(0), original_dump_size_(0),
	section_options_(0), operand_pos_(0), token_reference_(NULL), ext_vm_entry_(NULL), param_(0), fixup_(NULL)
{
	if (address_)
		include_option(roClearOriginalCode);
}

ILCommand::ILCommand(IFunction *owner, OperandSize size, ILCommandType type, uint64_t operand_value, IFixup *fixup)
	: BaseCommand(owner), address_(0), size_(size), type_(type), operand_value_(operand_value), original_dump_size_(0),
	section_options_(0), operand_pos_(0), token_reference_(NULL), ext_vm_entry_(NULL), param_(0), fixup_(fixup)
{

}

ILCommand::ILCommand(IFunction *owner, OperandSize size, const std::string &value)
	: BaseCommand(owner, value), address_(0), size_(size), type_(icUnknown), operand_value_(0), original_dump_size_(0),
	section_options_(0), operand_pos_(0), token_reference_(NULL), ext_vm_entry_(NULL), param_(0), fixup_(NULL)
{
	type_ = icData;
}

ILCommand::ILCommand(IFunction *owner, OperandSize size, const Data &value)
	: BaseCommand(owner, value), address_(0), size_(size), type_(icUnknown), operand_value_(0), original_dump_size_(0),
	section_options_(0), operand_pos_(0), token_reference_(NULL), ext_vm_entry_(NULL), param_(0), fixup_(NULL)
{
	type_ = icData;
}

ILCommand::ILCommand(IFunction *owner, const ILCommand &src)
	: BaseCommand(owner, src), section_options_(0), ext_vm_entry_(NULL), fixup_(NULL)
{
	address_ = src.address_;
	size_ = src.size_;
	type_ = src.type_;
	operand_value_ = src.operand_value_;
	original_dump_size_ = src.original_dump_size_;
	operand_pos_ = src.operand_pos_;
	token_reference_ = src.token_reference_;
	param_ = src.param_;
}

void ILCommand::Init(ILCommandType type, uint64_t operand_value, TokenReference *token_reference)
{
	type_ = type;
	operand_value_ = operand_value;
	token_reference_ = token_reference;
}

void ILCommand::clear()
{
	type_ = icUnknown;
	operand_value_ = 0;
	original_dump_size_ = 0;
	operand_pos_ = 0;
	token_reference_ = NULL;
	BaseCommand::clear();
}

void ILCommand::InitUnknown()
{
	clear();
	type_ = icUnknown;
	PushByte(0);
}

void ILCommand::InitComment(const std::string &comment)
{
	type_ = icComment;
	set_comment(CommentInfo(ttNone, comment));
}

ISEHandler * ILCommand::seh_handler() const
{
	throw std::runtime_error("The method or operation is not implemented.");
}

void ILCommand::set_seh_handler(ISEHandler *handler)
{
	throw std::runtime_error("The method or operation is not implemented.");
}

std::string ILCommand::text() const
{
	std::string res;

	const ILOpCode opcode = ILOpCodes[type_];

	res = (type_ == icComment) ? comment_text() : opcode.name;
	if (type_ == icData) {
		for (size_t i = 0; i < dump_size(); i++) {
			if (i > 0)
				res.append(",");
			res.append(string_format(" %.2X", dump(i)));
		}
	} else {
		switch (opcode.operand_type) {
		case InlineNone:
		case InlinePhi:
		case Inline8:
			// do nothing
			break;
		case ShortInlineI:
		case ShortInlineVar:
			res.append(string_format(" %.2X", static_cast<uint8_t>(operand_value_)));
			break;
		case InlineVar:
			res.append(string_format(" %.4X", static_cast<uint16_t>(operand_value_)));
			break;
		case InlineI8:
		case InlineR:
			res.append(string_format(" %.16llX", operand_value_));
			break;
		case InlineSwitch:
			break;
		case ShortInlineBrTarget:
		case InlineBrTarget:
			res.append(string_format(" %.8X", static_cast<uint32_t>(operand_value_)));
			break;
		default:
			res.append(string_format(" %.8X", static_cast<uint32_t>(operand_value_)));
			break;
		}
	}

	return res;
}

CommentInfo ILCommand::comment()
{
	CommentInfo res = BaseCommand::comment();
	if (res.type != ttUnknown)
		return res;

	res.type = ttNone;
	NETArchitecture *file = owner()->owner() ? dynamic_cast<NETArchitecture *>(owner()->owner()->owner()) : NULL;
	if (file) {
		switch (ILOpCodes[type_].operand_type) {
		case InlineBrTarget:
		case ShortInlineBrTarget:
			res.value = (next_address() > operand_value_) ? char(2) : char(4);
			res.type = ttJmp;
			break;

		case InlineType:
		case InlineField:
		case InlineTok:
		case InlineSig:
		case InlineString:
		case InlineMethod:
			if (ILToken *token = token_reference_ ? token_reference_->owner()->owner() : NULL) {
				switch (token->type()) {
				case ttTypeRef:
					{
						ILTypeRef *type = reinterpret_cast<ILTypeRef *>(token);
						std::string name = type->full_name(true);
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = ttImport;
					}
					break;
				case ttTypeDef:
					{
						ILTypeDef *type = reinterpret_cast<ILTypeDef *>(token);
						std::string name = type->full_name();
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = ttFunction;
					}
					break;
				case ttTypeSpec:
					{
						ILTypeSpec *type = reinterpret_cast<ILTypeSpec *>(token);
						std::string name = type->name(true);
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = ttFunction;
					}
					break;
				case ttField:
					{
						ILField *field = reinterpret_cast<ILField *>(token);
						std::string name = field->full_name(true);
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = ttFunction;
					}
					break;
				case ttMemberRef:
					{
						ILMemberRef *member = reinterpret_cast<ILMemberRef *>(token);
						std::string name = member->full_name(true);
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = (member->declaring_type() && member->declaring_type()->type() == ttTypeRef) ?  ttImport : ttFunction;
					}
					break;
				case ttStandAloneSig:
					{
						ILStandAloneSig *signature = reinterpret_cast<ILStandAloneSig*>(token);
						std::string name = signature->name(true);
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = ttFunction;
					}
					break;
				case ttUserString:
					{
						ILUserString *str = reinterpret_cast<ILUserString*>(token);
						res.value = string_format("%c \"%s\"", 3, str->name().c_str());
						res.type = ttString;
					}
					break;
				case ttMethodDef:
					{
						ILMethodDef *method = reinterpret_cast<ILMethodDef*>(token);
						std::string name = method->full_name(true);
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = ttFunction;
					}
					break;
				case ttMethodSpec:
					{
						ILMethodSpec *method_spec = reinterpret_cast<ILMethodSpec*>(token);
						std::string name = method_spec->full_name(true);
						res.value = string_format("%c %s", 3, name.c_str());
						res.type = (method_spec->parent() && method_spec->parent()->type() == ttMethodDef) ? ttFunction : ttImport;
					}
					break;
				}
				if (res.type == ttFunction && ILOpCodes[type_].operand_type == InlineField)
					res.type = ttVariable;
			}
			break;
		}

		set_comment(res);
	}
	return res;
}

void ILCommand::CompileToNative()
{
	if (type_ == icComment || type_ == icData)
		return;

	BaseCommand::clear();

	switch (type_) {
	case icByte:		break;
	case icWord:		break;
	case icDword:		break;
	case icQword:		break;
	case icCase:		break;
	case icNop:			PushByte(0x00); break;
	case icBreak:		PushByte(0x01); break;
	case icLdarg_0:		PushByte(0x02); break;
	case icLdarg_1:		PushByte(0x03); break;
	case icLdarg_2:		PushByte(0x04); break;
	case icLdarg_3:		PushByte(0x05); break;
	case icLdloc_0:		PushByte(0x06); break;
	case icLdloc_1:		PushByte(0x07); break;
	case icLdloc_2:		PushByte(0x08); break;
	case icLdloc_3:		PushByte(0x09); break;
	case icStloc_0:		PushByte(0x0a); break;
	case icStloc_1:		PushByte(0x0b); break;
	case icStloc_2:		PushByte(0x0c); break;
	case icStloc_3:		PushByte(0x0d); break;
	case icLdarg_s:		PushByte(0x0e); break;
	case icLdarga_s:	PushByte(0x0f); break;
	case icStarg_s:		PushByte(0x10); break;
	case icLdloc_s:		PushByte(0x11); break;
	case icLdloca_s:	PushByte(0x12); break;
	case icStloc_s:		PushByte(0x13); break;
	case icLdnull:		PushByte(0x14); break;
	case icLdc_i4_m1:	PushByte(0x15); break;
	case icLdc_i4_0:	PushByte(0x16); break;
	case icLdc_i4_1:	PushByte(0x17); break;
	case icLdc_i4_2:	PushByte(0x18); break;
	case icLdc_i4_3:	PushByte(0x19); break;
	case icLdc_i4_4:	PushByte(0x1a); break;
	case icLdc_i4_5:	PushByte(0x1b); break;
	case icLdc_i4_6:	PushByte(0x1c); break;
	case icLdc_i4_7:	PushByte(0x1d); break;
	case icLdc_i4_8:	PushByte(0x1e); break;
	case icLdc_i4_s:	PushByte(0x1f); break;
	case icLdc_i4:		PushByte(0x20); break;
	case icLdc_i8:		PushByte(0x21); break;
	case icLdc_r4:		PushByte(0x22); break;
	case icLdc_r8:		PushByte(0x23); break;
	case icDup:			PushByte(0x25); break;
	case icPop:			PushByte(0x26); break;
	case icJmp:			PushByte(0x27); break;
	case icCall:		PushByte(0x28); break;
	case icCalli:		PushByte(0x29); break;
	case icRet:			PushByte(0x2a); break;
	case icBr_s:		PushByte(0x2b); break;
	case icBrfalse_s:	PushByte(0x2c); break;
	case icBrtrue_s:	PushByte(0x2d); break;
	case icBeq_s:		PushByte(0x2e); break;
	case icBge_s:		PushByte(0x2f); break;
	case icBgt_s:		PushByte(0x30); break;
	case icBle_s:		PushByte(0x31); break;
	case icBlt_s:		PushByte(0x32); break;
	case icBne_un_s:	PushByte(0x33); break;
	case icBge_un_s:	PushByte(0x34); break;
	case icBgt_un_s:	PushByte(0x35); break;
	case icBle_un_s:	PushByte(0x36); break;
	case icBlt_un_s:	PushByte(0x37); break;
	case icBr:			PushByte(0x38); break;
	case icBrfalse:		PushByte(0x39); break;
	case icBrtrue:		PushByte(0x3a); break;
	case icBeq:			PushByte(0x3b); break;
	case icBge:			PushByte(0x3c); break;
	case icBgt:			PushByte(0x3d); break;
	case icBle:			PushByte(0x3e); break;
	case icBlt:			PushByte(0x3f); break;
	case icBne_un:		PushByte(0x40); break;
	case icBge_un:		PushByte(0x41); break;
	case icBgt_un:		PushByte(0x42); break;
	case icBle_un:		PushByte(0x43); break;
	case icBlt_un:		PushByte(0x44); break;
	case icSwitch:		PushByte(0x45); break;
	case icLdind_i1:	PushByte(0x46); break;
	case icLdind_u1:	PushByte(0x47); break;
	case icLdind_i2:	PushByte(0x48); break;
	case icLdind_u2:	PushByte(0x49); break;
	case icLdind_i4:	PushByte(0x4a); break;
	case icLdind_u4:	PushByte(0x4b); break;
	case icLdind_i8:	PushByte(0x4c); break;
	case icLdind_i:		PushByte(0x4d); break;
	case icLdind_r4:	PushByte(0x4e); break;
	case icLdind_r8:	PushByte(0x4f); break;
	case icLdind_ref:	PushByte(0x50); break;
	case icStind_ref:	PushByte(0x51); break;
	case icStind_i1:	PushByte(0x52); break;
	case icStind_i2:	PushByte(0x53); break;
	case icStind_i4:	PushByte(0x54); break;
	case icStind_i8:	PushByte(0x55); break;
	case icStind_r4:	PushByte(0x56); break;
	case icStind_r8:	PushByte(0x57); break;
	case icAdd:			PushByte(0x58); break;
	case icSub:			PushByte(0x59); break;
	case icMul:			PushByte(0x5a); break;
	case icDiv:			PushByte(0x5b); break;
	case icDiv_un:		PushByte(0x5c); break;
	case icRem:			PushByte(0x5d); break;
	case icRem_un:		PushByte(0x5e); break;
	case icAnd:			PushByte(0x5f); break;
	case icOr:			PushByte(0x60); break;
	case icXor:			PushByte(0x61); break;
	case icShl:			PushByte(0x62); break;
	case icShr:			PushByte(0x63); break;
	case icShr_un:		PushByte(0x64); break;
	case icNeg:			PushByte(0x65); break;
	case icNot:			PushByte(0x66); break;
	case icConv_i1:		PushByte(0x67); break;
	case icConv_i2:		PushByte(0x68); break;
	case icConv_i4:		PushByte(0x69); break;
	case icConv_i8:		PushByte(0x6a); break;
	case icConv_r4:		PushByte(0x6b); break;
	case icConv_r8:		PushByte(0x6c); break;
	case icConv_u4:		PushByte(0x6d); break;
	case icConv_u8:		PushByte(0x6e); break;
	case icCallvirt:	PushByte(0x6f); break;
	case icCpobj:		PushByte(0x70); break;
	case icLdobj:		PushByte(0x71); break;
	case icLdstr:		PushByte(0x72); break;
	case icNewobj:		PushByte(0x73); break;
	case icCastclass:	PushByte(0x74); break;
	case icIsinst:		PushByte(0x75); break;
	case icConv_r_un:	PushByte(0x76); break;
	case icUnbox:		PushByte(0x79); break;
	case icThrow:		PushByte(0x7a); break;
	case icLdfld:		PushByte(0x7b); break;
	case icLdflda:		PushByte(0x7c); break;
	case icStfld:		PushByte(0x7d); break;
	case icLdsfld:		PushByte(0x7e); break;
	case icLdsflda:		PushByte(0x7f); break;
	case icStsfld:		PushByte(0x80); break;
	case icStobj:		PushByte(0x81); break;
	case icConv_ovf_i1_un: PushByte(0x82); break;
	case icConv_ovf_i2_un: PushByte(0x83); break;
	case icConv_ovf_i4_un: PushByte(0x84); break;
	case icConv_ovf_i8_un: PushByte(0x85); break;
	case icConv_ovf_u1_un: PushByte(0x86); break;
	case icConv_ovf_u2_un: PushByte(0x87); break;
	case icConv_ovf_u4_un: PushByte(0x88); break;
	case icConv_ovf_u8_un: PushByte(0x89); break;
	case icConv_ovf_i_un: PushByte(0x8a); break;
	case icConv_ovf_u_un: PushByte(0x8b); break;
	case icBox:			PushByte(0x8c); break;
	case icNewarr:		PushByte(0x8d); break;
	case icLdlen:		PushByte(0x8e); break;
	case icLdelema:		PushByte(0x8f); break;
	case icLdelem_i1:	PushByte(0x90); break;
	case icLdelem_u1:	PushByte(0x91); break;
	case icLdelem_i2:	PushByte(0x92); break;
	case icLdelem_u2:	PushByte(0x93); break;
	case icLdelem_i4:	PushByte(0x94); break;
	case icLdelem_u4:	PushByte(0x95); break;
	case icLdelem_i8:	PushByte(0x96); break;
	case icLdelem_i:	PushByte(0x97); break;
	case icLdelem_r4:	PushByte(0x98); break;
	case icLdelem_r8:	PushByte(0x99); break;
	case icLdelem_ref:	PushByte(0x9a); break;
	case icStelem_i:	PushByte(0x9b); break;
	case icStelem_i1:	PushByte(0x9c); break;
	case icStelem_i2:	PushByte(0x9d); break;
	case icStelem_i4:	PushByte(0x9e); break;
	case icStelem_i8:	PushByte(0x9f); break;
	case icStelem_r4:	PushByte(0xa0); break;
	case icStelem_r8:	PushByte(0xa1); break;
	case icStelem_ref:	PushByte(0xa2); break;
	case icLdelem:		PushByte(0xa3); break;
	case icStelem:		PushByte(0xa4); break;
	case icUnbox_any:	PushByte(0xa5); break;
	case icConv_ovf_i1:	PushByte(0xb3); break;
	case icConv_ovf_u1:	PushByte(0xb4); break;
	case icConv_ovf_i2:	PushByte(0xb5); break;
	case icConv_ovf_u2:	PushByte(0xb6); break;
	case icConv_ovf_i4:	PushByte(0xb7); break;
	case icConv_ovf_u4:	PushByte(0xb8); break;
	case icConv_ovf_i8:	PushByte(0xb9); break;
	case icConv_ovf_u8:	PushByte(0xba); break;
	case icLdtoken:		PushByte(0xd0); break;
	case icConv_u2:		PushByte(0xd1); break;
	case icConv_u1:		PushByte(0xd2); break;
	case icConv_i:		PushByte(0xd3); break;
	case icConv_ovf_i:	PushByte(0xd4); break;
	case icConv_ovf_u:	PushByte(0xd5); break;
	case icAdd_ovf:		PushByte(0xd6); break;
	case icAdd_ovf_un:	PushByte(0xd7); break;
	case icMul_ovf:		PushByte(0xd8); break;
	case icMul_ovf_un:	PushByte(0xd9); break;
	case icSub_ovf:		PushByte(0xda); break;
	case icSub_ovf_un:	PushByte(0xdb); break;
	case icEndfinally:	PushByte(0xdc); break;
	case icLeave:		PushByte(0xdd); break;
	case icLeave_s:		PushByte(0xde); break;
	case icStind_i:		PushByte(0xdf); break;
	case icConv_u:		PushByte(0xe0); break;
	case icArglist:		PushByte(0xfe); PushByte(0x00); break;
	case icCeq:			PushByte(0xfe); PushByte(0x01); break;
	case icCgt:			PushByte(0xfe); PushByte(0x02); break;
	case icCgt_un:		PushByte(0xfe); PushByte(0x03); break;
	case icClt:			PushByte(0xfe); PushByte(0x04); break;
	case icClt_un:		PushByte(0xfe); PushByte(0x05); break;
	case icLdftn:		PushByte(0xfe); PushByte(0x06); break;
	case icLdvirtftn:	PushByte(0xfe); PushByte(0x07); break;
	case icLdarg:		PushByte(0xfe); PushByte(0x09); break;
	case icLdarga:		PushByte(0xfe); PushByte(0x0a); break;
	case icStarg:		PushByte(0xfe); PushByte(0x0b); break;
	case icLdloc:		PushByte(0xfe); PushByte(0x0c); break;
	case icLdloca:		PushByte(0xfe); PushByte(0x0d); break;
	case icStloc:		PushByte(0xfe); PushByte(0x0e); break;
	case icLocalloc:	PushByte(0xfe); PushByte(0x0f); break;
	case icEndfilter:	PushByte(0xfe); PushByte(0x11); break;
	case icUnaligned:	PushByte(0xfe); PushByte(0x12); break;
	case icVolatile:	PushByte(0xfe); PushByte(0x13); break;
	case icTail:		PushByte(0xfe); PushByte(0x14); break;
	case icInitobj:		PushByte(0xfe); PushByte(0x15); break;
	case icConstrained:	PushByte(0xfe); PushByte(0x16); break;
	case icCpblk:		PushByte(0xfe); PushByte(0x17); break;
	case icInitblk:		PushByte(0xfe); PushByte(0x18); break;
	case icNo:			PushByte(0xfe); PushByte(0x19); break;
	case icRethrow:		PushByte(0xfe); PushByte(0x1a); break;
	case icSizeof:		PushByte(0xfe); PushByte(0x1c); break;
	case icRefanytype:	PushByte(0xfe); PushByte(0x1d); break;
	case icReadonly:	PushByte(0xfe); PushByte(0x1e); break;
	default:
		throw std::runtime_error("Runtime error at CompileToNative: " + text());
	}

	operand_pos_ = dump_size();

	switch (ILOpCodes[type_].operand_type) {
	case InlineNone:
	case InlinePhi:
	case Inline8:
		// do nothing
		break;
	case ShortInlineI:
	case ShortInlineVar:
		PushByte(static_cast<uint8_t>(operand_value_));
		break;
	case InlineVar:
		PushWord(static_cast<uint16_t>(operand_value_));
		break;
	case InlineI8:
	case InlineR:
		PushQWord(operand_value_);
		break;
	case InlineSwitch:
		PushDWord(static_cast<uint32_t>(operand_value_));
		break;
	case ShortInlineBrTarget:
		PushByte(static_cast<uint8_t>(operand_value_ - next_address() - 1));
		break;
	case InlineBrTarget:
		PushDWord(static_cast<uint32_t>(operand_value_ - next_address() - 4));
		break;
	default:
		PushDWord(static_cast<uint32_t>(operand_value_));
		break;
	}

	if (options() & roFillNop) {
		for (size_t j = dump_size(); j < original_dump_size_; j++) {
			PushByte(0x00);
		}
	}
}

void ILCommand::CompileLink(const CompileContext &ctx)
{
	uint64_t value;

	if (block()->type() & mtExecutable) {
		// native block
		if (!link() || link()->operand_index() == -1)
			return;

		ICommand *to_command = link()->to_command();
		if (to_command) {
			value = (to_command->block()->type() & mtExecutable) ? to_command->address() : to_command->ext_vm_address();
		} else if (link()->type() == ltDelta) {
			value = link()->to_address();
		} else {
			return;
		}

		switch (link()->type()) {
		case ltDelta:
			value -= address();
			break;
		case ltCase:
			value -= link()->parent_command()->next_address() + reinterpret_cast<ILCommand *>(link()->parent_command())->operand_value() * sizeof(uint32_t);
			break;
		}
		set_operand_value(link()->operand_index(), link()->Encrypt(value));
		CompileToNative();
	} else {
		// VM block
		for (size_t i = 0; i < internal_links_.count(); i++) {
			InternalLink *internal_link = internal_links_.item(i);
			ILVMCommand *vm_command = reinterpret_cast<ILVMCommand *>(internal_link->from_command());

			switch (internal_link->type()) {
			case vlCRCTableAddress:
				value = reinterpret_cast<ILFunctionList *>(ctx.file->function_list())->runtime_crc_table()->entry()->address() - ctx.file->image_base();
				break;
			case vlCRCTableCount:
				value = reinterpret_cast<ILFunctionList *>(ctx.file->function_list())->runtime_crc_table()->region_count();
				break;
			default:
				ILCommand *command = reinterpret_cast<ILCommand*>(internal_link->to_command());
				if (!command)
					continue;

				value = command->vm_address() - ctx.file->image_base();
			}
			vm_command->set_value(value);
			vm_command->Compile();
		}

		if (vm_links_.empty())
			return;

		ICommand *to_command = link()->to_command();
		ICommand *next_command = link()->next_command();
		uint64_t image_base = ctx.file->image_base();

		switch (link()->type()) {
		case ltJmp:
		case ltCase:
			set_link_value(0, to_command->vm_address() - image_base);
			break;
		case ltJmpWithFlag:
			set_link_value(0, to_command->vm_address() - image_base);
			set_link_value(1, next_command->vm_address() - image_base);
			break;
		case ltOffset:
			if (to_command) {
				value = (section_options_ & rtLinkedFromOtherType) ? to_command->address() : to_command->vm_address();
				set_link_value(0, link()->Encrypt(value));
			}
			break;
		case ltSwitch:
			set_link_value(0, vm_links_[4]->address() - image_base);
			set_link_value(1, vm_links_[2]->address() - image_base);
			set_link_value(3, next_command->vm_address() - image_base);
			set_link_value(5, to_command->vm_address() - image_base);
			break;
		case ltCall:
			if (section_options_ & rtLinkedFrom)
				set_link_value(0, to_command->vm_address() - image_base);
			break;
		}
	}
}

void ILCommand::PrepareLink(const CompileContext &ctx)
{
	bool from_native = block() && (block()->type() & mtExecutable) ? true : owner()->compilation_type() == ctMutation;
	ICommand *to_command = link()->to_command();
	if (to_command) {
		bool to_native = to_command->block() && (to_command->block()->type() & mtExecutable) ? true : to_command->owner()->compilation_type() == ctMutation;
		if (from_native == to_native) {
			section_options_ |= rtLinkedFrom;
		} else {
			section_options_ |= rtLinkedFromOtherType;
		}

		if ((section_options_ & rtLinkedFromOtherType)
			|| link()->type() == ltSEHBlock
			|| link()->type() == ltFinallyBlock
			|| (link()->type() == ltJmp && (type_ == icLeave || type_ == icLeave_s))) {
			to_command->include_section_option(rtLinkedToExt);
		}
		else {
			to_command->include_section_option(rtLinkedToInt);
		}
	}

	if (from_native)
		return;

	bool need_next_command = false;
	ICommand *next_command;

	switch (link()->type()) {
	case ltJmpWithFlag:
	case ltSwitch:
		need_next_command = true;
		break;
	}

	if (need_next_command) {
		size_t j = owner()->IndexOf(this) + 1;
		if (type_ == icSwitch)
			j += static_cast<uint32_t>(operand_value_);
		if (j < owner()->count()) {
			next_command = owner()->item(j);
			include_section_option(rtLinkedNext);
			link()->set_next_command(next_command);
			next_command->include_section_option(rtLinkedToInt);
		}
	}
}

void ILCommand::set_link_value(size_t link_index, uint64_t value)
{
	ILVMCommand *vm_command = vm_links_[link_index];
	vm_command->set_value(value);
	vm_command->Compile();
}

void ILCommand::set_operand_value(size_t operand_index, uint64_t value)
{
	operand_value_ = value;
}

void ILCommand::set_operand_fixup(IFixup *fixup)
{
	fixup_ = fixup;
}

void ILCommand::set_address(uint64_t address)
{
	address_ = address;

	switch (ILOpCodes[type_].operand_type) {
	case ShortInlineBrTarget:
	case InlineBrTarget:
		CompileToNative();
		break;
	}
}

void ILCommand::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	size_t i, r;

	r = buffer.ReadByte();
	address_ = buffer.ReadDWord() + file.image_base();
	type_ = static_cast<ILCommandType>(buffer.ReadWord());
	BaseCommand::ReadFromBuffer(buffer, file);

	original_dump_size_ = (r & 0x10) ? buffer.ReadWord() : buffer.ReadByte();
	if (type_ == icComment) {
		for (i = 0; i < original_dump_size_; i++) {
			PushByte(buffer.ReadByte());
		}
	} else {
		for (i = 0; i < original_dump_size_; i++) {
			PushByte(0);
		}
	}
	if (r & 1) {
		operand_value_ = buffer.ReadQWord();
	}
	if (r & 0x08) {
		uint32_t value = static_cast<uint32_t>(operand_value_);
		ILMetaData *meta = dynamic_cast<NETArchitecture &>(file).command_list();
		ILToken *token = meta->token(value);
		if (!token && TOKEN_TYPE(value) == ttUserString)
			token = meta->us_table()->Add(value);
		if (token) {
			uint64_t reference_address = address() + original_dump_size_ - sizeof(uint32_t);
			if (type_ == icComment) {
				token_reference_ = token->reference_list()->GetReferenceByAddress(reference_address);
				if (!token_reference_)
					throw std::runtime_error("Invalid token reference");
			}
			else
				token_reference_ = token->reference_list()->Add(reference_address);
		}
	}
}

void ILCommand::WriteToFile(IArchitecture &file)
{
	if (fixup_) {
		if (fixup_ == NEED_FIXUP) {
			ISection *segment = file.segment_list()->GetSectionByAddress(address_);
			fixup_ = file.fixup_list()->AddDefault(file.cpu_address_size(), segment && (segment->memory_type() & mtExecutable) != 0);
		}
		fixup_->set_address(address_ + operand_pos_);
	}

	BaseCommand::WriteToFile(file);
}

void ILCommand::Rebase(uint64_t delta_base)
{
	if (!address_)
		return;

	switch (ILOpCodes[type_].operand_type) {
	case ShortInlineBrTarget:
	case InlineBrTarget:
		operand_value_ += delta_base;
		break;
	}

	address_ += delta_base;
}

ILCommand * ILCommand::Clone(IFunction *owner) const
{
	ILCommand *command = new ILCommand(owner, *this);
	return command;
}

bool ILCommand::Merge(ICommand *command)
{
	return false;
}

bool ILCommand::is_data() const
{
	return (type_ == icByte || type_ == icWord || type_ == icDword || type_ == icData);
}

bool ILCommand::is_end() const
{
	ILFlowControl flow_type = ILOpCodes[type_].flow_type;
	return (flow_type == Branch || flow_type == Return || flow_type == Throw);
}

bool ILCommand::is_prefix() const
{
	return (ILOpCodes[type_].opcode_type == Prefix);
}

std::string ILCommand::dump_str() const
{
	std::string res;
	if (type_ == icUnknown) {
		for (size_t i = 0; i < dump_size(); i++) {
			res += "??";
		}
		return res;
	}

	res = BaseCommand::dump_str();

	size_t value_size;
	const ILOpCode opcode = ILOpCodes[type_];
	switch (opcode.operand_type) {
	case InlineNone:
	case InlinePhi:
	case Inline8:
		value_size = 0;
		break;
	case ShortInlineI:
	case ShortInlineVar:
	case ShortInlineBrTarget:
		value_size = sizeof(uint8_t);
		break;
	case InlineVar:
		value_size = sizeof(uint16_t);
		break;
	case InlineI8:
	case InlineR:
		value_size = sizeof(uint64_t);
		break;
	default:
		value_size = sizeof(uint32_t);
		break;
	}

	if (value_size) {
		size_t end = dump_size();
		while (end > operand_pos_) {
			res.insert((end - value_size) * 2, " ");
			end -= value_size;
		}
	}

	return res;
}

std::string ILCommand::display_address() const
{
	return DisplayValue(size(), address());
}

size_t ILCommand::ReadFromFile(IArchitecture & file)
{
	clear();

	switch (ReadByte(file)) {
	case 0x00: type_ = icNop; break;
	case 0x01: type_ = icBreak;	break;
	case 0x02: type_ = icLdarg_0; break;
	case 0x03: type_ = icLdarg_1; break;
	case 0x04: type_ = icLdarg_2; break;
	case 0x05: type_ = icLdarg_3; break;
	case 0x06: type_ = icLdloc_0; break;
	case 0x07: type_ = icLdloc_1; break;
	case 0x08: type_ = icLdloc_2; break;
	case 0x09: type_ = icLdloc_3; break;
	case 0x0a: type_ = icStloc_0; break;
	case 0x0b: type_ = icStloc_1; break;
	case 0x0c: type_ = icStloc_2; break;
	case 0x0d: type_ = icStloc_3; break;
	case 0x0e: type_ = icLdarg_s; break;
	case 0x0f: type_ = icLdarga_s; break;
	case 0x10: type_ = icStarg_s; break;
	case 0x11: type_ = icLdloc_s; break;
	case 0x12: type_ = icLdloca_s; break;
	case 0x13: type_ = icStloc_s; break;
	case 0x14: type_ = icLdnull; break;
	case 0x15: type_ = icLdc_i4_m1; break;
	case 0x16: type_ = icLdc_i4_0; break;
	case 0x17: type_ = icLdc_i4_1; break;
	case 0x18: type_ = icLdc_i4_2; break;
	case 0x19: type_ = icLdc_i4_3; break;
	case 0x1a: type_ = icLdc_i4_4; break;
	case 0x1b: type_ = icLdc_i4_5; break;
	case 0x1c: type_ = icLdc_i4_6; break;
	case 0x1d: type_ = icLdc_i4_7; break;
	case 0x1e: type_ = icLdc_i4_8; break;
	case 0x1f: type_ = icLdc_i4_s; break;
	case 0x20: type_ = icLdc_i4; break;
	case 0x21: type_ = icLdc_i8; break;
	case 0x22: type_ = icLdc_r4; break;
	case 0x23: type_ = icLdc_r8; break;
	// 0x24 reserved for standardization
	case 0x25: type_ = icDup; break;
	case 0x26: type_ = icPop; break;
	case 0x27: type_ = icJmp; break;
	case 0x28: type_ = icCall; break;
	case 0x29: type_ = icCalli; break;
	case 0x2a: type_ = icRet; break;
	case 0x2b: type_ = icBr_s; break;
	case 0x2c: type_ = icBrfalse_s; break;
	case 0x2d: type_ = icBrtrue_s; break;
	case 0x2e: type_ = icBeq_s; break;
	case 0x2f: type_ = icBge_s; break;
	case 0x30: type_ = icBgt_s; break;
	case 0x31: type_ = icBle_s; break;
	case 0x32: type_ = icBlt_s; break;
	case 0x33: type_ = icBne_un_s; break;
	case 0x34: type_ = icBge_un_s; break;
	case 0x35: type_ = icBgt_un_s; break;
	case 0x36: type_ = icBle_un_s; break;
	case 0x37: type_ = icBlt_un_s; break;
	case 0x38: type_ = icBr; break;
	case 0x39: type_ = icBrfalse; break;
	case 0x3a: type_ = icBrtrue; break;
	case 0x3b: type_ = icBeq; break;
	case 0x3c: type_ = icBge; break;
	case 0x3d: type_ = icBgt; break;
	case 0x3e: type_ = icBle; break;
	case 0x3f: type_ = icBlt; break;
	case 0x40: type_ = icBne_un; break;
	case 0x41: type_ = icBge_un; break;
	case 0x42: type_ = icBgt_un; break;
	case 0x43: type_ = icBle_un; break;
	case 0x44: type_ = icBlt_un; break;
	case 0x45: type_ = icSwitch; break;
	case 0x46: type_ = icLdind_i1; break;
	case 0x47: type_ = icLdind_u1; break;
	case 0x48: type_ = icLdind_i2; break;
	case 0x49: type_ = icLdind_u2; break;
	case 0x4a: type_ = icLdind_i4; break;
	case 0x4b: type_ = icLdind_u4; break;
	case 0x4c: type_ = icLdind_i8; break;
	case 0x4d: type_ = icLdind_i; break;
	case 0x4e: type_ = icLdind_r4; break;
	case 0x4f: type_ = icLdind_r8; break;
	case 0x50: type_ = icLdind_ref; break;
	case 0x51: type_ = icStind_ref; break;
	case 0x52: type_ = icStind_i1; break;
	case 0x53: type_ = icStind_i2; break;
	case 0x54: type_ = icStind_i4; break;
	case 0x55: type_ = icStind_i8; break;
	case 0x56: type_ = icStind_r4; break;
	case 0x57: type_ = icStind_r8; break;
	case 0x58: type_ = icAdd; break;
	case 0x59: type_ = icSub; break;
	case 0x5a: type_ = icMul; break;
	case 0x5b: type_ = icDiv; break;
	case 0x5c: type_ = icDiv_un; break;
	case 0x5d: type_ = icRem; break;
	case 0x5e: type_ = icRem_un; break;
	case 0x5f: type_ = icAnd; break;
	case 0x60: type_ = icOr; break;
	case 0x61: type_ = icXor; break;
	case 0x62: type_ = icShl; break;
	case 0x63: type_ = icShr; break;
	case 0x64: type_ = icShr_un; break;
	case 0x65: type_ = icNeg; break;
	case 0x66: type_ = icNot; break;
	case 0x67: type_ = icConv_i1; break;
	case 0x68: type_ = icConv_i2; break;
	case 0x69: type_ = icConv_i4; break;
	case 0x6a: type_ = icConv_i8; break;
	case 0x6b: type_ = icConv_r4; break;
	case 0x6c: type_ = icConv_r8; break;
	case 0x6d: type_ = icConv_u4; break;
	case 0x6e: type_ = icConv_u8; break;
	case 0x6f: type_ = icCallvirt; break;
	case 0x70: type_ = icCpobj; break;
	case 0x71: type_ = icLdobj; break;
	case 0x72: type_ = icLdstr; break;
	case 0x73: type_ = icNewobj; break;
	case 0x74: type_ = icCastclass; break;
	case 0x75: type_ = icIsinst; break;
	case 0x76: type_ = icConv_r_un; break;
	// 0x77-0x78 reserved for standardization
	case 0x79: type_ = icUnbox; break;
	case 0x7a: type_ = icThrow; break;
	case 0x7b: type_ = icLdfld; break;
	case 0x7c: type_ = icLdflda; break;
	case 0x7d: type_ = icStfld; break;
	case 0x7e: type_ = icLdsfld; break;
	case 0x7f: type_ = icLdsflda; break;
	case 0x80: type_ = icStsfld; break;
	case 0x81: type_ = icStobj; break;
	case 0x82: type_ = icConv_ovf_i1_un; break;
	case 0x83: type_ = icConv_ovf_i2_un; break;
	case 0x84: type_ = icConv_ovf_i4_un; break;
	case 0x85: type_ = icConv_ovf_i8_un; break;
	case 0x86: type_ = icConv_ovf_u1_un; break;
	case 0x87: type_ = icConv_ovf_u2_un; break;
	case 0x88: type_ = icConv_ovf_u4_un; break;
	case 0x89: type_ = icConv_ovf_u8_un; break;
	case 0x8a: type_ = icConv_ovf_i_un; break;
	case 0x8b: type_ = icConv_ovf_u_un; break;
	case 0x8c: type_ = icBox; break;
	case 0x8d: type_ = icNewarr; break;
	case 0x8e: type_ = icLdlen; break;
	case 0x8f: type_ = icLdelema; break;
	case 0x90: type_ = icLdelem_i1; break;
	case 0x91: type_ = icLdelem_u1; break;
	case 0x92: type_ = icLdelem_i2; break;
	case 0x93: type_ = icLdelem_u2; break;
	case 0x94: type_ = icLdelem_i4; break;
	case 0x95: type_ = icLdelem_u4; break;
	case 0x96: type_ = icLdelem_i8; break;
	case 0x97: type_ = icLdelem_i; break;
	case 0x98: type_ = icLdelem_r4; break;
	case 0x99: type_ = icLdelem_r8; break;
	case 0x9a: type_ = icLdelem_ref; break;
	case 0x9b: type_ = icStelem_i; break;
	case 0x9c: type_ = icStelem_i1; break;
	case 0x9d: type_ = icStelem_i2; break;
	case 0x9e: type_ = icStelem_i4; break;
	case 0x9f: type_ = icStelem_i8; break;
	case 0xa0: type_ = icStelem_r4; break;
	case 0xa1: type_ = icStelem_r8; break;
	case 0xa2: type_ = icStelem_ref; break;
	case 0xa3: type_ = icLdelem; break;
	case 0xa4: type_ = icStelem; break;
	case 0xa5: type_ = icUnbox_any; break;
	// 0xA6-0xB2 reserved for standardization
	case 0xb3: type_ = icConv_ovf_i1; break;
	case 0xb4: type_ = icConv_ovf_u1; break;
	case 0xb5: type_ = icConv_ovf_i2; break;
	case 0xb6: type_ = icConv_ovf_u2; break;
	case 0xb7: type_ = icConv_ovf_i4; break;
	case 0xb8: type_ = icConv_ovf_u4; break;
	case 0xb9: type_ = icConv_ovf_i8; break;
	case 0xba: type_ = icConv_ovf_u8; break;
	// 0xBB-0xC1 reserved for standardization
	case 0xc2: type_ = icRefanyval; break;
	case 0xc3: type_ = icCkfinite; break;
	// 0xC4-0xC5 reserved for standardization
	case 0xc6: type_ = icMkrefany; break;
	// 0xC7-0xCF reserved for standardization
	case 0xd0: type_ = icLdtoken; break;
	case 0xd1: type_ = icConv_u2; break;
	case 0xd2: type_ = icConv_u1; break;
	case 0xd3: type_ = icConv_i; break;
	case 0xd4: type_ = icConv_ovf_i; break;
	case 0xd5: type_ = icConv_ovf_u; break;
	case 0xd6: type_ = icAdd_ovf; break;
	case 0xd7: type_ = icAdd_ovf_un; break;
	case 0xd8: type_ = icMul_ovf; break;
	case 0xd9: type_ = icMul_ovf_un; break;
	case 0xda: type_ = icSub_ovf; break;
	case 0xdb: type_ = icSub_ovf_un; break;
	case 0xdc: type_ = icEndfinally; break;
	case 0xdd: type_ = icLeave; break;
	case 0xde: type_ = icLeave_s; break;
	case 0xdf: type_ = icStind_i; break;
	case 0xe0: type_ = icConv_u; break;
	// 0xE1-0xEF reserved for standardization
	// 0xF0-0xFB are ecma_experimental
	// 0xFC-0xFD reserved for standardization
	case 0xfe:
		//two-byte opcodes
		switch (ReadByte(file)) {
			case 0x00: type_ = icArglist; break;
			case 0x01: type_ = icCeq; break;
			case 0x02: type_ = icCgt; break;
			case 0x03: type_ = icCgt_un; break;
			case 0x04: type_ = icClt; break;
			case 0x05: type_ = icClt_un; break;
			case 0x06: type_ = icLdftn; break;
			case 0x07: type_ = icLdvirtftn; break;
			// 0xFE08 reserved for standardization
			case 0x09: type_ = icLdarg; break;
			case 0x0a: type_ = icLdarga; break;
			case 0x0b: type_ = icStarg; break;
			case 0x0c: type_ = icLdloc; break;
			case 0x0d: type_ = icLdloca; break;
			case 0x0e: type_ = icStloc; break;
			case 0x0f: type_ = icLocalloc; break;
			// 0xFE10 reserved for standardization
			case 0x11: type_ = icEndfilter; break;
			case 0x12: type_ = icUnaligned; break;
			case 0x13: type_ = icVolatile; break;
			case 0x14: type_ = icTail; break;
			case 0x15: type_ = icInitobj; break;
			case 0x16: type_ = icConstrained; break;
			case 0x17: type_ = icCpblk; break;
			case 0x18: type_ = icInitblk; break;
			case 0x19: type_ = icNo; break;
			case 0x1a: type_ = icRethrow; break;
			// 0xFE1B reserved for standardization
			case 0x1c: type_ = icSizeof; break;
			case 0x1d: type_ = icRefanytype; break;
			case 0x1e: type_ = icReadonly; break;
		}
		break;
	// 0xFF reserved for standardization
	}

	operand_pos_ = dump_size();

	switch (ILOpCodes[type_].operand_type)
	{
	case InlineNone:			// No operand
	case InlinePhi:				// Obsolete. The operand is reserved and should not be used.
	case Inline8:				// not used
		// all read already
		break;
	case ShortInlineI:			// 8-bit integer
		operand_value_ = static_cast<int8_t>(ReadByte(file));
		break;
	case ShortInlineVar:		// 8-bit integer containing the ordinal of a local variable or an argument
		operand_value_ = ReadByte(file);
		break;
	case InlineVar:				// 16-bit integer containing the ordinal of a local variable or an argument
		operand_value_ = ReadWord(file);
		break;
	case InlineI8:				// 64-bit integer
	case InlineR:				// 64-bit IEEE floating point number
		operand_value_ = ReadQWord(file);
		break;
	case InlineSwitch:
		operand_value_ = ReadDWord(file);
		break;
	case ShortInlineBrTarget:	// 8-bit integer branch target
		operand_value_ = static_cast<int8_t>(ReadByte(file));
		operand_value_ += next_address();
		break;
	case InlineBrTarget: // 32-bit integer branch target
		operand_value_ = static_cast<int32_t>(ReadDWord(file));
		operand_value_ += next_address();
		break;
	case InlineField: 
	case InlineMethod:
	case InlineSig:
	case InlineTok:
	case InlineType:
	case InlineString:
		operand_value_ = ReadDWord(file);
		{
			ILToken *token = dynamic_cast<NETArchitecture &>(file).command_list()->token(static_cast<uint32_t>(operand_value_));
			if (token)
				token_reference_ = token->reference_list()->GetReferenceByAddress(address() + operand_pos_);
		}
		break;
	default: // 4 byte
		operand_value_ = ReadDWord(file);
		break;
	}

	original_dump_size_ = dump_size();

	return original_dump_size_; 
}

uint64_t ILCommand::ReadValueFromFile(IArchitecture &file, OperandSize value_size, bool is_token)
{
	switch (value_size) {
	case osByte:
		type_ = icByte;
		operand_value_ = ReadByte(file);
		break;
	case osWord:
		type_ = icWord;
		operand_value_ = ReadWord(file);
		break;
	case osDWord:
		type_ = icDword;
		operand_value_ = ReadDWord(file);
		break;
	default:
		throw std::runtime_error("Invalid value size");
	}

	if (is_token) {
		ILToken *token = dynamic_cast<NETArchitecture &>(file).command_list()->token(static_cast<uint32_t>(operand_value_));
		if (token)
			token_reference_ = token->reference_list()->GetReferenceByAddress(address_);
	}

	return operand_value_;
}

void ILCommand::ReadString(IArchitecture &file, size_t len)
{
	type_ = icData;
	Read(file, len);
}

void ILCommand::ReadCaseCommand(IArchitecture &file)
{
	type_ = icCase;
	operand_value_ = DWordToInt64(ReadDWord(file));
}

int ILCommand::GetStackLevel(size_t *pop_ref) const
{
	if (type_ == icRet)
		return 0;

	size_t push = 0;
	size_t pop = 0;
	if (ILOpCodes[type_].flow_type == Call) {
		ILToken *token = token_reference_->owner()->owner();
		if (token->type() == ttMethodSpec)
			token = reinterpret_cast<ILMethodSpec *>(token)->parent();

		ILSignature *signature;
		switch (token->type()) {
		case ttMethodDef:
			signature = reinterpret_cast<ILMethodDef *>(token)->signature();
			break;
		case ttMemberRef:
			signature = reinterpret_cast<ILMemberRef *>(token)->signature();
			break;
		case ttStandAloneSig:
			signature = reinterpret_cast<ILStandAloneSig *>(token)->signature();
			break;
		default:
			throw std::runtime_error("Invalid token type: " + text());
		}
		if (type_ != icNewobj && signature->has_this() && !signature->explicit_this())
			pop += 1;
		if (type_ == icCalli)
			pop += 1;
		pop += static_cast<int>(signature->count());
		if (type_ == icNewobj || signature->ret()->type() != ELEMENT_TYPE_VOID)
			push += 1;
	}
	else {
		switch (ILOpCodes[type_].pop) {
		case Pop0:
			break;
		case Pop1:
		case Popi:
		case Popref:
			pop += 1;
			break;
		case Pop1_pop1:
		case Popi_pop1:
		case Popi_popi:
		case Popi_popi8:
		case Popi_popr4:
		case Popi_popr8:
		case Popref_pop1:
		case Popref_popi:
			pop += 2;
			break;
		case Popi_popi_popi:
		case Popref_popi_popi:
		case Popref_popi_popi8:
		case Popref_popi_popr4:
		case Popref_popi_popr8:
		case Popref_popi_popref:
		case Popref_popi_pop1:
			pop += 3;
			break;
		}

		switch (ILOpCodes[type_].push) {
		case Push0:
			break;
		case Push1:
		case Pushi:
		case Pushi8:
		case Pushr4:
		case Pushr8:
		case Pushref:
			push += 1;
			break;
		case Push1_push1:
			push += 2;
			break;
		}
	}

	if (pop_ref)
		*pop_ref = pop;

	return (int)(push - pop);
}

ILVMCommand *ILCommand::AddVMCommand(const CompileContext &ctx, ILCommandType command_type, uint64_t value, uint32_t options, TokenReference *token_reference, ILCommand *to_command)
{
	ILVMCommand *vm_command = NULL;
#ifdef ULTIMATE
	if ((owner()->compilation_options() & coLockToKey) && command_type == icLdc_i4 && (options & voNoCryptValue) == 0) {
		vm_command = new ILVMCommand(this, command_type, value, token_reference);
		vm_command->set_crypt_command(icAdd, osDWord, ctx.options.licensing_manager->product_code());
	}
#endif
	if (!vm_command)
		vm_command = new ILVMCommand(this, command_type, value, token_reference);
	AddObject(vm_command);
	if (options & voLinkCommand)
		vm_links_.push_back(vm_command);

	switch (command_type) {
	case icBr:
	case icRet:
		section_options_ |= rtCloseSection;
		break;
	}

	if (to_command)
		internal_links_.Add(vlNone, vm_command, to_command);

	uint32_t new_options = (options & voSectionCommand);
	if (vm_command->crypt_command() == icAdd) {
		new_options |= voNoCryptValue;

		size_t i;
		ILVMCommand *cur_command = vm_command;
		ILToken *type = reinterpret_cast<NETArchitecture *>(ctx.file)->command_list()->ImportType(ELEMENT_TYPE_U4);
		// add session key
		uint64_t address = ctx.runtime->export_list()->GetAddressByType(atLoaderData);
		ILFunction *loader_data_func = reinterpret_cast<ILFunction *>(ctx.runtime->function_list()->GetFunctionByAddress(address));
		ILCommand *field_command = loader_data_func->item(1);
		AddVMCommand(ctx, icLdnull, 0, new_options);
		AddVMCommand(ctx, icLdc_i4, field_command->operand_value(), new_options, field_command->token_reference());
		AddVMCommand(ctx, icLdfld, 0, new_options);
		AddVMCommand(ctx, icLdc_i4, ctx.runtime_var_index[VAR_SESSION_KEY], new_options);
		AddVMCommand(ctx, icLdc_i4, type->id(), new_options, type->reference_list()->Add(0));
		AddVMCommand(ctx, icLdelem_ref, 0, new_options);
		AddVMCommand(ctx, icAdd, 0, new_options);
		for (i = 1; i < 4; i++) {
			cur_command->set_link_command(AddVMCommand(ctx, icLdc_i4, 0, new_options));
			// add session key
			AddVMCommand(ctx, icLdnull, 0, new_options);
			AddVMCommand(ctx, icLdc_i4, field_command->operand_value(), new_options, field_command->token_reference());
			AddVMCommand(ctx, icLdfld, 0, new_options);
			AddVMCommand(ctx, icLdc_i4, ctx.runtime_var_index[VAR_SESSION_KEY], new_options);
			AddVMCommand(ctx, icLdc_i4, type->id(), new_options, type->reference_list()->Add(0));
			AddVMCommand(ctx, icLdelem_ref, 0, new_options);
			AddVMCommand(ctx, icAdd, 0, new_options);
			cur_command = cur_command->link_command();
		}
		address = ctx.runtime->export_list()->GetAddressByType(atDecryptBuffer);
		ILVMCommand *from_command = AddVMCommand(ctx, icLdc_i4, 0, new_options);
		ICommand *to_command = ctx.file->function_list()->GetCommandByAddress(address, true);
		if (to_command)
			internal_links_.Add(vlNone, from_command, to_command);
		AddVMCommand(ctx, icCallvm, 0, new_options);
		// add session key
		AddVMCommand(ctx, icLdnull, 0, new_options);
		AddVMCommand(ctx, icLdc_i4, field_command->operand_value(), new_options, field_command->token_reference());
		AddVMCommand(ctx, icLdfld, 0, new_options);
		AddVMCommand(ctx, icLdc_i4, ctx.runtime_var_index[VAR_SESSION_KEY], new_options);
		AddVMCommand(ctx, icLdc_i4, type->id(), new_options, type->reference_list()->Add(0));
		AddVMCommand(ctx, icLdelem_ref, 0, new_options);
		AddVMCommand(ctx, icAdd, 0, new_options);
	}

	return vm_command;
}

void ILCommand::AddCmpSection(const CompileContext &ctx, ILCommandType cmp_command)
{
	ILCommandType cmp_type;
	switch (cmp_command) {
	case icBne_un:
	case icClt_un:
	case icBlt_un:
	case icBle_un:
	case icCgt_un:
	case icBgt_un:
	case icBge_un:
		cmp_type = icCmp_un;
		break;
	default:
		cmp_type = icCmp;
		break;
	}

	switch (cmp_command) {
	case icCeq:
	case icBeq:
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, cmp_type, 0);
		AddVMCommand(ctx, icNot, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAnd, 0);
		break;
	case icBne_un:
		AddVMCommand(ctx, icLdc_i4, 0);
		AddVMCommand(ctx, cmp_type, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAnd, 0);
		break;
	case icClt:
	case icClt_un:
	case icBlt:
	case icBlt_un:
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, cmp_type, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icShr_un, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAnd, 0);
		break;
	case icBle:
	case icBle_un:
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, cmp_type, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAdd, 0);
		AddVMCommand(ctx, icNot, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icShr_un, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAnd, 0);
		break;
	case icCgt:
	case icCgt_un:
	case icBgt:
	case icBgt_un:
		AddVMCommand(ctx, icLdc_i4, static_cast<uint32_t>(-1));
		AddVMCommand(ctx, cmp_type, 0);
		AddVMCommand(ctx, icNot, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAdd, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icShr_un, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAnd, 0);
		break;
	case icBge:
	case icBge_un:
		AddVMCommand(ctx, icLdc_i4, static_cast<uint32_t>(-1));
		AddVMCommand(ctx, cmp_type, 0);
		AddVMCommand(ctx, icNot, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icShr_un, 0);
		AddVMCommand(ctx, icLdc_i4, 1);
		AddVMCommand(ctx, icAnd, 0);
		break;
	}
}

void ILCommand::AddJmpWithFlagSection(const CompileContext &ctx, bool is_true)
{
	ILToken *type = reinterpret_cast<NETArchitecture *>(ctx.file)->command_list()->ImportType(ELEMENT_TYPE_BOOLEAN);

	AddVMCommand(ctx, icLdc_i4, type->id(), 0, type->reference_list()->Add(0));
	AddVMCommand(ctx, icConv, 0);
	AddVMCommand(ctx, icNeg, 0);
	if (!is_true)
		AddVMCommand(ctx, icNot, 0);
	AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
	AddVMCommand(ctx, icAnd, 0);
	AddVMCommand(ctx, icDup, 0);
	AddVMCommand(ctx, icLdc_i4, type->id(), 0, type->reference_list()->Add(0));
	AddVMCommand(ctx, icConv, 0);
	AddVMCommand(ctx, icNeg, 0);
	AddVMCommand(ctx, icNot, 0);
	AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
	AddVMCommand(ctx, icAnd, 0);
	AddVMCommand(ctx, icAdd, 0);
	AddVMCommand(ctx, icBr, 0);
}

void ILCommand::CompileToVM(const CompileContext &ctx)
{
	ILToken *internal_type = NULL;
	TokenReference *internal_token_reference = NULL;

	switch (type_) {
	case icLdelem_i:
	case icLdelem_i1:
	case icLdelem_i2:
	case icLdelem_i4:
	case icLdelem_i8:
	case icLdelem_u1:
	case icLdelem_u2:
	case icLdelem_u4:
	case icLdelem_r4:
	case icLdelem_r8:
	case icLdelem_ref:
	case icStelem_i:
	case icStelem_i1:
	case icStelem_i2:
	case icStelem_i4:
	case icStelem_i8:
	case icStelem_r4:
	case icStelem_r8:
	case icStelem_ref:
	case icLdind_i:
	case icLdind_i1:
	case icLdind_i2:
	case icLdind_i4:
	case icLdind_i8:
	case icLdind_u1:
	case icLdind_u2:
	case icLdind_u4:
	case icLdind_r4:
	case icLdind_r8:
	case icLdind_ref:
	case icStind_i:
	case icStind_i1:
	case icStind_i2:
	case icStind_i4:
	case icStind_i8:
	case icStind_r4:
	case icStind_r8:
	case icStind_ref:
	case icConv_i:
	case icConv_u:
	case icConv_i1:
	case icConv_u1:
	case icConv_i2:
	case icConv_u2:
	case icConv_i4:
	case icConv_u4:
	case icConv_i8:
	case icConv_u8:
	case icConv_r4:
	case icConv_r8:
	case icConv_r_un:
	case icConv_ovf_i:
	case icConv_ovf_u:
	case icConv_ovf_i1:
	case icConv_ovf_u1:
	case icConv_ovf_i2:
	case icConv_ovf_u2:
	case icConv_ovf_i4:
	case icConv_ovf_u4:
	case icConv_ovf_i8:
	case icConv_ovf_u8:
	case icConv_ovf_i_un:
	case icConv_ovf_u_un:
	case icConv_ovf_i1_un:
	case icConv_ovf_u1_un:
	case icConv_ovf_i2_un:
	case icConv_ovf_u2_un:
	case icConv_ovf_i4_un:
	case icConv_ovf_u4_un:
	case icConv_ovf_i8_un:
	case icConv_ovf_u8_un:
		{
			CorElementType element_type;
			switch (type_) {
			case icLdelem_i1:
			case icStelem_i1:
			case icLdind_i1:
			case icStind_i1:
			case icConv_i1:
			case icConv_ovf_i1:
			case icConv_ovf_i1_un:
				element_type = ELEMENT_TYPE_I1;
				break;
			case icLdelem_i2:
			case icStelem_i2:
			case icLdind_i2:
			case icStind_i2:
			case icConv_i2:
			case icConv_ovf_i2:
			case icConv_ovf_i2_un:
				element_type = ELEMENT_TYPE_I2;
				break;
			case icLdelem_i4:
			case icStelem_i4:
			case icLdind_i4:
			case icStind_i4:
			case icConv_i4:
			case icConv_ovf_i4:
			case icConv_ovf_i4_un:
				element_type = ELEMENT_TYPE_I4;
				break;
			case icLdelem_i8:
			case icStelem_i8:
			case icLdind_i8:
			case icStind_i8:
			case icConv_i8:
			case icConv_ovf_i8:
			case icConv_ovf_i8_un:
				element_type = ELEMENT_TYPE_I8;
				break;
			case icLdelem_u1:
			case icLdind_u1:
			case icConv_u1:
			case icConv_ovf_u1:
			case icConv_ovf_u1_un:
				element_type = ELEMENT_TYPE_U1;
				break;
			case icLdelem_u2:
			case icLdind_u2:
			case icConv_u2:
			case icConv_ovf_u2:
			case icConv_ovf_u2_un:
				element_type = ELEMENT_TYPE_U2;
				break;
			case icLdelem_u4:
			case icLdind_u4:
			case icConv_u4:
			case icConv_ovf_u4:
			case icConv_ovf_u4_un:
				element_type = ELEMENT_TYPE_U4;
				break;
			case icConv_u8:
			case icConv_ovf_u8:
			case icConv_ovf_u8_un:
				element_type = ELEMENT_TYPE_U8;
				break;
			case icLdelem_r4:
			case icStelem_r4:
			case icLdind_r4:
			case icStind_r4:
			case icConv_r4:
				element_type = ELEMENT_TYPE_R4;
				break;
			case icLdelem_r8:
			case icStelem_r8:
			case icLdind_r8:
			case icStind_r8:
			case icConv_r8:
			case icConv_r_un:
				element_type = ELEMENT_TYPE_R8;
				break;
			case icLdelem_ref:
			case icStelem_ref:
			case icLdind_ref:
			case icStind_ref:
				element_type = ELEMENT_TYPE_OBJECT;
				break;
			case icConv_u:
			case icConv_ovf_u:
			case icConv_ovf_u_un:
				element_type = ELEMENT_TYPE_U;
				break;
			default:
				element_type = ELEMENT_TYPE_I;
				break;
			}
			internal_type = reinterpret_cast<NETArchitecture *>(ctx.file)->command_list()->ImportType(element_type);
			internal_token_reference = internal_type->reference_list()->Add(0);
		}
		break;
	}

	switch (type_) {
	case icNop:
		break;
	case icLdarg:
	case icLdarg_s:
	case icLdarg_0:
	case icLdarg_1:
	case icLdarg_2:
	case icLdarg_3:
		AddVMCommand(ctx, icLdarg, param_);
		break;
	case icLdarga:
	case icLdarga_s:
		AddVMCommand(ctx, icLdarga, param_);
		break;
	case icStarg:
	case icStarg_s:
		AddVMCommand(ctx, icStarg, param_);
		break;
	case icLdloc:
	case icLdloc_s:
	case icLdloc_0:
	case icLdloc_1:
	case icLdloc_2:
	case icLdloc_3:
		AddVMCommand(ctx, icLdarg, param_);
		break;
	case icLdloca:
	case icLdloca_s:
		AddVMCommand(ctx, icLdarga, param_);
		break;
	case icStloc:
	case icStloc_s:
	case icStloc_0:
	case icStloc_1:
	case icStloc_2:
	case icStloc_3:
		AddVMCommand(ctx, icStarg, param_);
		break;
	case icLdc_i4_0:
		AddVMCommand(ctx, icLdc_i4, 0);
		break;
	case icLdc_i4_1:
		AddVMCommand(ctx, icLdc_i4, 1);
		break;
	case icLdc_i4_2:
		AddVMCommand(ctx, icLdc_i4, 2);
		break;
	case icLdc_i4_3:
		AddVMCommand(ctx, icLdc_i4, 3);
		break;
	case icLdc_i4_4:
		AddVMCommand(ctx, icLdc_i4, 4);
		break;
	case icLdc_i4_5:
		AddVMCommand(ctx, icLdc_i4, 5);
		break;
	case icLdc_i4_6:
		AddVMCommand(ctx, icLdc_i4, 6);
		break;
	case icLdc_i4_7:
		AddVMCommand(ctx, icLdc_i4, 7);
		break;
	case icLdc_i4_8:
		AddVMCommand(ctx, icLdc_i4, 8);
		break;
	case icLdc_i4:
	case icLdc_i4_s:
		AddVMCommand(ctx, icLdc_i4, operand_value_, (link() && link()->operand_index() == 0) ? voLinkCommand : 0);
		break;
	case icLdc_i4_m1:
		AddVMCommand(ctx, icLdc_i4, static_cast<uint32_t>(-1));
		break;
	case icLdc_i8:
		AddVMCommand(ctx, icLdc_i8, operand_value_);
		break;
	case icLdc_r4:
		AddVMCommand(ctx, icLdc_r4, operand_value_);
		break;
	case icLdc_r8:
		AddVMCommand(ctx, icLdc_r8, operand_value_);
		break;
	case icLdnull:
		AddVMCommand(ctx, icLdnull, 0);
		break;
	case icConv_i:
	case icConv_u:
	case icConv_i1:
	case icConv_u1:
	case icConv_i2:
	case icConv_u2:
	case icConv_i4:
	case icConv_u4:
	case icConv_i8:
	case icConv_u8:
	case icConv_r4:
	case icConv_r8:
		AddVMCommand(ctx, icLdc_i4, internal_type->id(), 0, internal_token_reference);
		AddVMCommand(ctx, icConv, 0);
		break;
	case icConv_ovf_i:
	case icConv_ovf_u:
	case icConv_ovf_i1:
	case icConv_ovf_u1:
	case icConv_ovf_i2:
	case icConv_ovf_u2:
	case icConv_ovf_i4:
	case icConv_ovf_u4:
	case icConv_ovf_i8:
	case icConv_ovf_u8:
		AddVMCommand(ctx, icLdc_i4, internal_type->id(), 0, internal_token_reference);
		AddVMCommand(ctx, icConv_ovf, 0);
		break;
	case icConv_ovf_i_un:
	case icConv_ovf_u_un:
	case icConv_ovf_i1_un:
	case icConv_ovf_u1_un:
	case icConv_ovf_i2_un:
	case icConv_ovf_u2_un:
	case icConv_ovf_i4_un:
	case icConv_ovf_u4_un:
	case icConv_ovf_i8_un:
	case icConv_ovf_u8_un:
	case icConv_r_un:
		AddVMCommand(ctx, icLdc_i4, internal_type->id(), 0, internal_token_reference);
		AddVMCommand(ctx, icConv_ovf_un, 0);
		break;
	case icRem:
		AddVMCommand(ctx, icRem, 0);
		break;
	case icRem_un:
		AddVMCommand(ctx, icRem_un, 0);
		break;
	case icAdd:
		AddVMCommand(ctx, icAdd, 0);
		break;
	case icSub:
		AddVMCommand(ctx, icSub, 0);
		break;
	case icDiv:
		AddVMCommand(ctx, icDiv, 0);
		break;
	case icDiv_un:
		AddVMCommand(ctx, icDiv_un, 0);
		break;
	case icMul:
		AddVMCommand(ctx, icMul, 0);
		break;
	case icAnd:
		AddVMCommand(ctx, icAnd, 0);
		break;
	case icOr:
		AddVMCommand(ctx, icOr, 0);
		break;
	case icXor:
		AddVMCommand(ctx, icXor, 0);
		break;
	case icShl:
		AddVMCommand(ctx, icShl, 0);
		break;
	case icShr:
		AddVMCommand(ctx, icShr, 0);
		break;
	case icShr_un:
		AddVMCommand(ctx, icShr_un, 0);
		break;
	case icNeg:
		AddVMCommand(ctx, icNeg, 0);
		break;
	case icNot:
		AddVMCommand(ctx, icNot, 0);
		break;
	case icAdd_ovf:
		AddVMCommand(ctx, icAdd_ovf, 0);
		break;
	case icAdd_ovf_un:
		AddVMCommand(ctx, icAdd_ovf_un, 0);
		break;
	case icMul_ovf:
		AddVMCommand(ctx, icMul_ovf, 0);
		break;
	case icMul_ovf_un:
		AddVMCommand(ctx, icMul_ovf_un, 0);
		break;
	case icSub_ovf:
		AddVMCommand(ctx, icSub_ovf, 0);
		break;
	case icSub_ovf_un:
		AddVMCommand(ctx, icSub_ovf_un, 0);
		break;
	case icCeq:
		AddCmpSection(ctx, icCeq);
		break;
	case icClt:
		AddCmpSection(ctx, icClt);
		break;
	case icClt_un:
		AddCmpSection(ctx, icClt_un);
		break;
	case icCgt:
		AddCmpSection(ctx, icCgt);
		break;
	case icCgt_un:
		AddCmpSection(ctx, icCgt_un);
		break;
	case icCall:
		if (section_options_ & rtLinkedFrom) {
			AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
			AddVMCommand(ctx, icCallvm, 0);
		}
		else {
			AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
			AddVMCommand(ctx, icCall, 0);
		}
		break;
	case icCallvirt:
		if (section_options_ & rtLinkedFrom) {
			AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
			AddVMCommand(ctx, icCallvmvirt, 0);
		}
		else {
			AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
			AddVMCommand(ctx, icCallvirt, 0);
		}
		break;
	case icNewobj:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icNewobj, 0);
		break;
	case icNewarr:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icNewarr, 0);
		break;
	case icInitobj:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icInitobj, 0);
		break;
	case icConstrained:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icConstrained, 0);
		break;
	case icStelem_i:
	case icStelem_i1:
	case icStelem_i2:
	case icStelem_i4:
	case icStelem_i8:
	case icStelem_r4:
	case icStelem_r8:
	case icStelem_ref:
		AddVMCommand(ctx, icLdc_i4, internal_type->id(), 0, internal_token_reference);
		AddVMCommand(ctx, icStelem_ref, 0);
		break;
	case icStelem:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icStelem_ref, 0);
		break;
	case icLdelem_i:
	case icLdelem_i1:
	case icLdelem_u1:
	case icLdelem_i2:
	case icLdelem_u2:
	case icLdelem_i4:
	case icLdelem_u4:
	case icLdelem_i8:
	case icLdelem_r4:
	case icLdelem_r8:
	case icLdelem_ref:
		AddVMCommand(ctx, icLdc_i4, internal_type->id(), 0, internal_token_reference);
		AddVMCommand(ctx, icLdelem_ref, 0);
		break;
	case icLdelem:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdelem_ref, 0);
		break;
	case icLdelema:
		AddVMCommand(ctx, icLdelema, 0);
		break;
	case icLdlen:
		AddVMCommand(ctx, icLdlen, 0);
		break;
	case icLdfld:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdfld, 0);
		break;
	case icLdsfld:
		AddVMCommand(ctx, icLdnull, 0);
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdfld, 0);
		break;
	case icLdflda:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdflda, 0);
		break;
	case icLdsflda:
		AddVMCommand(ctx, icLdnull, 0);
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdflda, 0);
		break;
	case icStfld:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icStfld, 0);
		break;
	case icStsfld:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icStsfld, 0);
		break;
	case icLdstr:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdstr, 0);
		break;
	case icLdftn:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdftn, 0);
		break;
	case icLdvirtftn:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdvirtftn, 0);
		break;
	case icStind_i:
	case icStind_i1:
	case icStind_i2:
	case icStind_i4:
	case icStind_i8:
	case icStind_r4:
	case icStind_r8:
	case icStind_ref:
		AddVMCommand(ctx, icLdc_i4, internal_type->id(), 0, internal_token_reference);
		AddVMCommand(ctx, icStind_ref, 0);
		break;
	case icStobj:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icStind_ref, 0);
		break;
	case icLdind_i:
	case icLdind_i1:
	case icLdind_i2:
	case icLdind_i4:
	case icLdind_i8:
	case icLdind_u1:
	case icLdind_u2:
	case icLdind_u4:
	case icLdind_r4:
	case icLdind_r8:
	case icLdind_ref:
		AddVMCommand(ctx, icLdc_i4, internal_type->id(), 0, internal_token_reference);
		AddVMCommand(ctx, icLdind_ref, 0);
		break;
	case icLdobj:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdind_ref, 0);
		break;
	case icPop:
		AddVMCommand(ctx, icPop, 0);
		break;
	case icDup:
		AddVMCommand(ctx, icDup, 0);
		break;
	case icBox:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icBox, 0);
		break;
	case icUnbox:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icUnbox, 0);
		break;
	case icUnbox_any:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icUnbox_any, 0);
		break;
	case icLdtoken:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icLdtoken, 0);
		break;
	case icBr:
	case icBr_s:
		AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
		AddVMCommand(ctx, icBr, 0);
		break;
	case icBeq:
	case icBeq_s:
		AddCmpSection(ctx, icBeq);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBne_un:
	case icBne_un_s:
		AddCmpSection(ctx, icBne_un);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBlt:
	case icBlt_s:
		AddCmpSection(ctx, icBlt);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBlt_un:
	case icBlt_un_s:
		AddCmpSection(ctx, icBlt_un);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBle:
	case icBle_s:
		AddCmpSection(ctx, icBle);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBle_un:
	case icBle_un_s:
		AddCmpSection(ctx, icBle_un);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBgt:
	case icBgt_s:
		AddCmpSection(ctx, icBgt);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBgt_un:
	case icBgt_un_s:
		AddCmpSection(ctx, icBgt_un);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBge:
	case icBge_s:
		AddCmpSection(ctx, icBge);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBge_un:
	case icBge_un_s:
		AddCmpSection(ctx, icBge_un);
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBrtrue:
	case icBrtrue_s:
		AddJmpWithFlagSection(ctx, true);
		break;
	case icBrfalse:
	case icBrfalse_s:
		AddJmpWithFlagSection(ctx, false);
		break;
	case icLeave:
	case icLeave_s:
		AddVMCommand(ctx, icLdc_i4, static_cast<uint32_t>(operand_value_ - ctx.file->image_base()));
		AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
		AddVMCommand(ctx, icLeave, 0);
		break;
	case icSwitch:
		AddVMCommand(ctx, icDup, 0);
		AddVMCommand(ctx, icLdc_i4, operand_value_);
		AddCmpSection(ctx, icBlt_un);
		AddJmpWithFlagSection(ctx, true);

		AddVMCommand(ctx, icPop, 0, voLinkCommand);
		AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
		AddVMCommand(ctx, icBr, 0);

		AddVMCommand(ctx, icLdc_i4, 2, voLinkCommand);
		AddVMCommand(ctx, icShl, 0);
		AddVMCommand(ctx, icLdc_i4, 0, voLinkCommand);
		AddVMCommand(ctx, icAdd, 0);
		AddVMCommand(ctx, icLdmem_i4, 0);
		AddVMCommand(ctx, icBr, 0);
		break;
	case icCase:
		AddVMCommand(ctx, icDword, 0, voLinkCommand);
		break;
	case icEndfinally:
		AddVMCommand(ctx, icEndfinally, 0);
		break;
	case icEndfilter:
		AddVMCommand(ctx, icEndfilter, 0);
		break;
	case icRet:
		if (operand_value_) {
			AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
			AddVMCommand(ctx, icBox, 0);
		}
		else
			AddVMCommand(ctx, icLdnull, 0);

		AddVMCommand(ctx, icLdc_i4, 0);
		if ((ctx.options.flags & cpMemoryProtection) && owner()->tag() != ftLoader) {
			ILVMCommand *vm_command = AddVMCommand(ctx, icLdc_i4, 0);
			uint64_t address = ctx.runtime->export_list()->GetAddressByType(atRandom);
			ICommand *to_command = ctx.file->function_list()->GetCommandByAddress(address, true);
			internal_links_.Add(vlNone, vm_command, to_command);
			AddVMCommand(ctx, icCallvm, 0);
			AddVMCommand(ctx, icLdc_i4, rand32());
			AddVMCommand(ctx, icAdd, 0);
			vm_command = AddVMCommand(ctx, icLdc_i4, 0);
			internal_links_.Add(vlCRCTableCount, vm_command, NULL);
			AddVMCommand(ctx, icRem_un, 0);
			AddVMCommand(ctx, icLdc_i4, sizeof(CRCInfo::POD));
			AddVMCommand(ctx, icMul, 0);
			vm_command = AddVMCommand(ctx, icLdc_i4, 0);
			internal_links_.Add(vlCRCTableAddress, vm_command, NULL);
			AddVMCommand(ctx, icAdd, 0);
			AddVMCommand(ctx, icStarg, param_);
			AddVMCommand(ctx, icLdarg, param_);
			AddVMCommand(ctx, icLdmem_i4, 0);
			AddCryptorSection(ctx, ctx.file->function_list()->crc_cryptor(), true);
			AddVMCommand(ctx, icLdarg, param_);
			AddVMCommand(ctx, icLdc_i4, offsetof(CRCInfo::POD, size));
			AddVMCommand(ctx, icAdd, 0);
			AddVMCommand(ctx, icLdmem_i4, 0);
			AddCryptorSection(ctx, ctx.file->function_list()->crc_cryptor(), true);
			vm_command = AddVMCommand(ctx, icLdc_i4, 0);
			address = ctx.runtime->export_list()->GetAddressByType(atCalcCRC);
			to_command = ctx.file->function_list()->GetCommandByAddress(address, true);
			internal_links_.Add(vlNone, vm_command, to_command);
			AddVMCommand(ctx, icCallvm, 0);
			AddVMCommand(ctx, icLdarg, param_);
			AddVMCommand(ctx, icLdc_i4, offsetof(CRCInfo::POD, hash));
			AddVMCommand(ctx, icAdd, 0);
			AddVMCommand(ctx, icLdmem_i4, 0);
			AddVMCommand(ctx, icAdd, 0);
			AddVMCommand(ctx, icAdd, 0);
		}
		AddVMCommand(ctx, icBr, 0);
		break;
	case icThrow:
		AddVMCommand(ctx, icThrow, 0);
		break;
	case icRethrow:
		AddVMCommand(ctx, icRethrow, 0);
		break;
	case icCastclass:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icCastclass, 0);
		break;
	case icIsinst:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icIsinst, 0);
		break;
	case icCkfinite:
		AddVMCommand(ctx, icCkfinite, 0);
		break;
	case icLocalloc:
		AddVMCommand(ctx, icLocalloc, 0);
		break;
	case icSizeof:
		AddVMCommand(ctx, icLdc_i4, operand_value_, 0, token_reference_);
		AddVMCommand(ctx, icSizeof, 0);
		break;
	default:
		throw std::runtime_error("Runtime error at CompileToVM: " + text());
	}
}

void ILCommand::AddExtSection(const CompileContext &ctx)
{
	ext_vm_entry_ = AddVMCommand(ctx, icLdc_i4, 0);
	AddVMCommand(ctx, icPop, 0);
}

void ILCommand::AddCryptorSection(const CompileContext &ctx, ValueCryptor *cryptor, bool is_decrypt)
{
	if (!cryptor)
		return;

	CompileContext new_ctx = ctx;
	new_ctx.options.flags &= ~cpMemoryProtection;

	for (size_t i = 0; i < cryptor->count(); i++) {
		ValueCommand *value_command = cryptor->item(i);
		AddVMCommand(new_ctx, icLdc_i4, value_command->value());
		switch (value_command->type(is_decrypt)) {
		case ccDec:
			AddVMCommand(new_ctx, icSub, 0);
			break;
		case ccInc:
			AddVMCommand(new_ctx, icAdd, 0);
			break;
		case ccSub:
			AddVMCommand(new_ctx, icSub, 0);
			break;
		case ccAdd:
			AddVMCommand(new_ctx, icAdd, 0);
			break;
		case ccXor:
			AddVMCommand(new_ctx, icXor, 0);
			break;
		default:
			throw 1;
		}
	}
}

/**
* ILVMCommand
*/

ILVMCommand::ILVMCommand(ILCommand *owner, ILCommandType command_type, uint64_t value, TokenReference *token_reference)
	: BaseVMCommand(owner), command_type_(command_type), value_(value), token_reference_(token_reference), address_(0),
	crypt_command_(icUnknown), crypt_size_(osDWord), crypt_key_(0), link_command_(NULL)
{

}

void ILVMCommand::Compile()
{
	reinterpret_cast<ILVirtualMachine *>(owner()->block()->virtual_machine())->CompileCommand(*this);
}

void ILVMCommand::WriteToFile(IArchitecture &file)
{
	if (!dump_.size())
		return;

	file.Write(dump_.data(), dump_.size());
}

/**
 * ILFunction
 */

ILFunction::ILFunction(IFunctionList *owner, const FunctionName &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder)
	: BaseFunction(owner, name, compilation_type, compilation_options, need_compile, folder)
{

}

ILFunction::ILFunction(IFunctionList *owner /*= NULL*/)
	: BaseFunction(owner, FunctionName(""), ctVirtualization, 0, true, NULL)
{

}

ILFunction::ILFunction(IFunctionList *owner, OperandSize cpu_address_size, IFunction *parent)
	: BaseFunction(owner, cpu_address_size, parent)
{

}

ILFunction::ILFunction(IFunctionList *owner, const ILFunction &src)
	: BaseFunction(owner, src)
{

}

bool ILFunction::Prepare(const CompileContext &ctx)
{
	size_t i, j;

	if (type() == otString) {
		IArchitecture *file = from_runtime() ? ctx.runtime : ctx.file;
		MapFunction *map_function = file->map_function_list()->GetFunctionByAddress(address());
		if (map_function) {
			uint64_t address = ctx.runtime->export_list()->GetAddressByType(atDecryptStringA);
			if (!address)
				return false;

			NETRuntimeFunctionList *runtime_function_list = reinterpret_cast<NETRuntimeFunctionList *>(ctx.file->runtime_function_list());
			ILMetaData *meta = reinterpret_cast<ILMetaData *>(ctx.runtime->command_list());
			ILMethodDef *decrypt_method = meta->GetMethod(address);
			if (!decrypt_method)
				return false;

			ILFunction *decrypt_func = reinterpret_cast<ILFunction*>(ctx.runtime->function_list()->GetFunctionByAddress(address));
			if (!decrypt_func)
				return false;

			size_t orig_count = count();
			ILTable *table = meta->table(ttMethodDef);
			for (i = 0; i < orig_count; i++) {
				ILMethodDef *method = reinterpret_cast<ILMethodDef*>(decrypt_method->Clone(meta, table));
				method->set_deleted(false);
				method->declaring_type()->AddMethod(method);

				size_t old_count = count();
				for (j = 0; j < decrypt_func->count(); j++) {
					ILCommand *src_command = decrypt_func->item(j);
					ILCommand *dst_command = reinterpret_cast<ILCommand*>(src_command->Clone(this));
					dst_command->set_address(0);
					if (static_cast<uint32_t>(dst_command->operand_value()) == FACE_STORAGE_INFO)
						dst_command->set_operand_value(0, item(i)->address() - ctx.file->image_base());
					AddObject(dst_command);

					CommandLink *src_link = src_command->link();
					if (src_link) {
						CommandLink *dst_link = src_link->Clone(link_list());
						dst_link->set_from_command(dst_command);
						link_list()->AddObject(dst_link);
				
						if (src_link->parent_command())
							dst_link->set_parent_command(GetCommandByAddress(src_link->parent_command()->address()));
					}

					if (dst_command->token_reference()) {
						TokenReference *token_reference = dst_command->token_reference()->Clone(dst_command->token_reference()->owner());
						token_reference->owner()->AddObject(token_reference);
						dst_command->set_token_reference(token_reference);
					}
				}

				for (j = 0; j < decrypt_func->function_info_list()->count(); j++) {
					FunctionInfo *src_info = decrypt_func->function_info_list()->item(j);
					FunctionInfo *dst_info = src_info->Clone(function_info_list());
					function_info_list()->AddObject(dst_info);
					if (dst_info->entry())
						dst_info->set_entry(item(old_count + decrypt_func->IndexOf(dst_info->entry())));
					dst_info->set_source(runtime_function_list->Add(0, 0, 0, method));
				}

				AddressRange *address_range = function_info_list()->item(0)->Add(0, 0, NULL, NULL, NULL);
				for (j = old_count; j < count(); j++) {
					ILCommand *command = item(j);
					command->set_address_range(address_range);
					command->CompileToNative();
				}

				ILCommand *src_command = item(i);
				for (j = 0; j < map_function->reference_list()->count(); j++) {
					Reference *reference = map_function->reference_list()->item(j);
					if (reference->operand_address() != src_command->address())
						continue;

					address = reference->address();
					ILCommand *command = reinterpret_cast<ILCommand *>(ctx.file->function_list()->GetCommandByAddress(address, true));
					if (!command) {
						if (!ctx.file->AddressSeek(address))
							return false;

						CommandBlock *block = AddBlock(count(), true);
						block->set_address(address);

						command = Add(address);
						command->ReadFromFile(*file);
						command->set_block(block);
						command->exclude_option(roClearOriginalCode);
					}
					if (command->token_reference())
						command->token_reference()->set_deleted(true);
					command->Init(icCall, method->id());
					command->set_token_reference(method->reference_list()->Add(address + 1));
					command->CompileToNative();
				}
			}
		}
	}
	else {
		for (i = 0; i < function_info_list()->count(); i++) {
			FunctionInfo *info = function_info_list()->item(i);
			if (!info->source())
				continue;

			std::map<size_t, size_t> variable_map;
			NETRuntimeFunction *runtime_func = reinterpret_cast<NETRuntimeFunction *>(info->source());

			ILStandAloneSig *locals = runtime_func->method()->locals();
			std::vector<ILMethodDef *> method_list = runtime_func->method_list();
			if (locals) {
				if (locals->reference_list()->count() > 1) {
					ILStandAloneSig *old_locals = locals;
					locals = locals->Clone(locals->meta(), locals->owner());
					locals->owner()->AddObject(locals);
					locals->reference_list()->clear();
					for (j = 0; j < method_list.size(); j++) {
						method_list[j]->set_locals(locals);
					}
					for (j = old_locals->reference_list()->count(); j > 0; j--) {
						TokenReference *token_reference = old_locals->reference_list()->item(j - 1);
						if (item(IndexOf(info->entry()) + 1)->token_reference() == token_reference)
							token_reference->set_owner(locals->reference_list());
					}
				}
			}
			else if (compilation_type() != ctVirtualization) {
				ILData data;
				data.push_back(stLocal);
				data.push_back(0);
				locals = runtime_func->method()->meta()->AddStandAloneSig(data);
				for (j = 0; j < method_list.size(); j++) {
					method_list[j]->set_locals(locals);
				}

				ILCommand *command = new ILCommand(this, osDWord, icComment, 0);
				size_t insert_index = IndexOf(info->entry()) + 1;
				InsertObject(insert_index, command);
				command->set_token_reference(locals->reference_list()->Add(0));
				command->set_address_range(info->entry()->address_range());
				uint32_t dump = 0;
				command->set_dump(&dump, sizeof(dump));
				for (j = 0; j < block_list()->count(); j++) {
					CommandBlock *block = block_list()->item(j);
					if (block->start_index() >= insert_index) {
						block->set_start_index(block->start_index() + 1);
						block->set_end_index(block->end_index() + 1);
					}
				}
			}
			else
				continue;

			if (compilation_type() != ctMutation)
				locals->set_deleted(true);

			ILSignature *signature = locals->signature();
			size_t old_count = signature->count();
			ILElement *random_variable = new ILElement(locals->meta(), signature);
			{
				ILData data;
				data.push_back(ELEMENT_TYPE_U4);
				random_variable->Parse(data);
			}
			random_variable->set_is_predicate(true);
			signature->AddObject(random_variable);

			std::vector<ILElement *> element_list;
			std::vector<size_t> index_list;
			while (signature->count()) {
				ILElement *element = signature->item(0);
				signature->RemoveObject(element);
				index_list.push_back(element_list.size());
				element_list.push_back(element);
			}

			for (j = 0; j < index_list.size(); j++) {
				std::swap(index_list[j], index_list[rand() % index_list.size()]);
			}

			for (j = 0; j < index_list.size(); j++) {
				size_t old_index = index_list[j];
				if (old_index < old_count)
					variable_map[old_index] = j;
				signature->AddObject(element_list[old_index]);
			}

			if (variable_map.empty())
				continue;

			std::map<size_t, size_t>::const_iterator it;
			for (j = IndexOf(info->entry()) + 2; j < count(); j++) {
				ILCommand *command = item(j);
				switch (command->type()) {
				case icLdloc:
				case icLdloc_s:
					it = variable_map.find(static_cast<uint16_t>(command->operand_value()));
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icLdloc, it->second);
					command->CompileToNative();
					break;
				case icLdloca:
				case icLdloca_s:
					it = variable_map.find(static_cast<uint16_t>(command->operand_value()));
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icLdloca, it->second);
					command->CompileToNative();
					break;
				case icStloc:
				case icStloc_s:
					it = variable_map.find(static_cast<uint16_t>(command->operand_value()));
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icStloc, it->second);
					command->CompileToNative();
					break;
				case icLdloc_0:
					it = variable_map.find(0);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icLdloc, it->second);
					command->CompileToNative();
					break;
				case icLdloc_1:
					it = variable_map.find(1);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icLdloc, it->second);
					command->CompileToNative();
					break;
				case icLdloc_2:
					it = variable_map.find(2);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icLdloc, it->second);
					command->CompileToNative();
					break;
				case icLdloc_3:
					it = variable_map.find(3);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icLdloc, it->second);
					command->CompileToNative();
					break;
				case icStloc_0:
					it = variable_map.find(0);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icStloc, it->second);
					command->CompileToNative();
					break;
				case icStloc_1:
					it = variable_map.find(1);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icStloc, it->second);
					command->CompileToNative();
					break;
				case icStloc_2:
					it = variable_map.find(2);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icStloc, it->second);
					command->CompileToNative();
					break;
				case icStloc_3:
					it = variable_map.find(3);
					if (it == variable_map.end())
						throw std::runtime_error("Invalid variable index");
					command->Init(icStloc, it->second);
					command->CompileToNative();
					break;
				}

				if (command->is_data() || command->type() == icComment)
					break;
			}
		}
	}

	if (tag() == ftProcessor) {
		if (ctx.runtime)
			return true;
	}

	return BaseFunction::Prepare(ctx);
}

void ILFunction::CompileLinks(const CompileContext &ctx)
{
	BaseFunction::CompileLinks(ctx);
}

void ILFunction::CompileInfo(const CompileContext &ctx)
{
	BaseFunction::CompileInfo(ctx);

	size_t i;
	FunctionInfo *info;
	AddressRange *range;
	ILCommand *command;

	for (i = 0; i < function_info_list()->count(); i++) {
		info = function_info_list()->item(i);
		if (!info->begin())
			continue;

		if (info->entry()) {
			uint32_t code_size = static_cast<uint32_t>(info->end() - info->begin());
			command = reinterpret_cast<ILCommand *>(info->entry());
			Data data;
			for (size_t k = 0; k < command->dump_size(); k++) {
				data.PushByte(command->dump(k));
			}
			if (command->dump_size() == 1) {
				code_size--;
				data[0] = CorILMethod_TinyFormat | (code_size << CorILMethod_FormatShift);
			} else {
				code_size -= (data[1] >> 4) * sizeof(uint32_t);
				data.WriteDWord(offsetof(IMAGE_COR_ILMETHOD_FAT, CodeSize), code_size);
			}
			command->set_dump(data.data(), data.size());
		}
	}

	for (i = 0; i < range_list()->count(); i++) {
		range = range_list()->item(i);
		info = function_info_list()->GetItemByAddress(range->begin());
		if (!info)
			continue;

		uint64_t base_value = info->begin() + info->base_value();

		if (range->begin_entry()) {
			command = reinterpret_cast<ILCommand *>(range->begin_entry());
			command->set_operand_value(0, range->begin() - base_value);
			command->CompileToNative();
		}
		if (range->size_entry()) {
			command = reinterpret_cast<ILCommand *>(range->size_entry());
			command->set_operand_value(0, range->end() - range->begin());
			command->CompileToNative();
		}
	}
}

void ILFunction::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	BaseFunction::ReadFromBuffer(buffer, file);
}

void ILFunction::ParseBeginCommands(IArchitecture &)
{

}

void ILFunction::ParseEndCommands(IArchitecture &file)
{
	if (type() != otString) {
		size_t i;
		size_t use_address_count = 0;
		for (i = 0; i < file.map_function_list()->count(); i++) {
			if (file.map_function_list()->item(i)->address() == address()) {
				use_address_count++;
			}
		}

		if (use_address_count > 1) {
			for (i = 0; i < count(); i++) {
				item(i)->exclude_option(roClearOriginalCode);
			}
		}
	}
}

void ILFunction::CreateBlocks()
{
	CommandBlock *cur_block = NULL;
	for (size_t i = 0; i < count(); i++) {
		ILCommand *command = item(i);
		if (command->block() || (command->options() & roNeedCompile) == 0) {
			cur_block = NULL;
			continue;
		}

		if ((!cur_block || (command->options() & roCreateNewBlock) || item(cur_block->end_index())->is_data() != command->is_data()))
			cur_block = AddBlock(i, true);

		cur_block->set_end_index(i);

		command->set_block(cur_block);

		if (command->is_end())
			cur_block = NULL;
	}
}

void ILFunction::CalcStack(std::map<ILCommand *, int> &stack_map)
{
	size_t i, j;
	ILCommand *command, *entry, *link_command;
	std::set<ILCommand *> entry_stack, link_list;

	for (i = 0; i < function_info_list()->count(); i++) {
		FunctionInfo *info = function_info_list()->item(i);
		if (!info->source())
			continue;

		entry = reinterpret_cast<ILCommand *>(info->entry());
		entry_stack.insert(entry);
		stack_map[entry] = 0;
	}
	for (i = 0; i < this->link_list()->count(); i++) {
		CommandLink *link = this->link_list()->item(i);
		switch (link->type()) {
		case ltSEHBlock:
			entry = reinterpret_cast<ILCommand *>(link->to_command());
			entry_stack.insert(entry);
			stack_map[entry] = 1;
			break;
		case ltFinallyBlock:
			entry = reinterpret_cast<ILCommand *>(link->to_command());
			entry_stack.insert(entry);
			stack_map[entry] = 0;
			break;
		}
	}

	int stack;
	while (!entry_stack.empty()) {
		entry = *entry_stack.begin();
		j = IndexOf(entry);
		std::map<ILCommand *, int>::const_iterator s = stack_map.find(entry);
		if (s == stack_map.end())
			throw std::runtime_error("Incorrect stack");
		stack = s->second;
		for (i = j; i < count(); i++) {
			command = item(i);
			std::set<ILCommand *>::const_iterator c = entry_stack.find(command);
			if (c != entry_stack.end())
				entry_stack.erase(c);

			if (command->type() == icComment || command->type() == icCase || command->is_data())
				continue;

			s = stack_map.find(command);
			if (s != stack_map.end()) {
				if (s->second != stack)
					throw std::runtime_error("Incorrect stack");
			}
			else
				stack_map[command] = stack;

			stack += command->GetStackLevel();

			link_list.clear();
			if (command->type() == icSwitch) {
				size_t case_count = static_cast<uint32_t>(command->operand_value());
				for (j = 0; j < case_count; j++) {
					ILCommand *case_command = item(i + 1 + j);
					link_list.insert(reinterpret_cast<ILCommand *>(case_command->link()->to_command()));
				}
			}
			else {
				if (command->link() && (command->link()->type() == ltJmp || command->link()->type() == ltJmpWithFlag))
					link_list.insert(reinterpret_cast<ILCommand *>(command->link()->to_command()));
			}
			for (std::set<ILCommand *>::const_iterator l = link_list.begin(); l != link_list.end(); l++) {
				link_command = *l;
				s = stack_map.find(link_command);
				if (s != stack_map.end()) {
					if (s->second != stack)
						throw std::runtime_error("Incorrect stack");
				}
				else {
					entry_stack.insert(link_command);
					stack_map[link_command] = stack;
				}
			}

			if (command->is_end())
				break;
		}
	}
}

void ILFunction::Mutate(const CompileContext &ctx)
{
	size_t i, j, index;
	ILCommand *command, *insert_command, *entry;
	std::map<ILCommand *, ILSignature *> variables_map;
	uint32_t value, old_value, bit_mask;
	std::map<ILCommand *, int> stack_map;

	for (i = 0; i < function_info_list()->count(); i++) {
		FunctionInfo *info = function_info_list()->item(i);
		if (!info->source())
			continue;

		ILStandAloneSig *locals = reinterpret_cast<NETRuntimeFunction *>(info->source())->method()->locals();
		entry = reinterpret_cast<ILCommand *>(info->entry());
		variables_map[entry] = locals ? locals->signature() : NULL;
		stack_map[entry] = 0;
	}

	CalcStack(stack_map);

	int stack;
	std::list<ICommand *> new_command_list;
	std::vector<ILCommand *> insert_command_list;
	std::vector<size_t> predicate_list;
	std::map<size_t, uint32_t> value_list;
	std::map<ILCommand *, std::map<size_t, uint32_t> > value_list_map;
	ILSignature *variables = NULL;
	size_t orig_count = count();
	std::vector<CommandLink *> insert_link_list;
	for (i = 0; i < orig_count; i++) {
		command = item(i);
		new_command_list.push_back(command);

		if ((command->options() & roNoProgress) == 0)
			ctx.file->StepProgress();

		if (is_breaked_address(command->address()))
			continue;

		if (command->block()) {
			CommandBlock *block = command->block();
			size_t insert_count = new_command_list.size() - 1 - i;
			for (j = block->start_index() + 1; j <= block->end_index(); j++) {
				new_command_list.push_back(item(j));
			}
			i = block->end_index();
			if (insert_count) {
				block->set_start_index(block->start_index() + insert_count);
				block->set_end_index(block->end_index() + insert_count);
			}
			ctx.file->StepProgress(block->end_index() - block->start_index());
			continue;
		}
		else if ((command->options() & roNeedCompile) == 0)
			continue;

		std::map<ILCommand *, ILSignature *>::const_iterator it = variables_map.find(command);
		if (it != variables_map.end()) {
			predicate_list.clear();
			variables = it->second;
			if (variables) {
				for (j = 0; j < variables->count(); j++) {
					ILElement *variable = variables->item(j);
					if (variable->is_predicate())
						predicate_list.push_back(j);
				}
			}
		}

		if (command->type() == icComment || command->type() == icCase || command->is_data())
			continue;

		insert_command_list.clear();
		new_command_list.pop_back();
		AddressRange *address_range = command->address_range();
		ILCommand *link_command = (command->link() && (command->link()->type() == ltJmp || command->link()->type() == ltJmpWithFlag)) ? reinterpret_cast<ILCommand *>(command->link()->to_command()) : NULL;

		if (command->section_options() & (rtLinkedToInt | rtLinkedToExt)) {
			std::map<size_t, uint32_t> dst_value_list;
			if ((command->section_options() & rtLinkedToExt) == 0) {
				std::map<ILCommand *, std::map<size_t, uint32_t> >::iterator it = value_list_map.find(command);
				if (it != value_list_map.end()) {
					dst_value_list = it->second;
					if (!new_command_list.empty() && !new_command_list.back()->is_end()) {
						for (std::map<size_t, uint32_t>::const_iterator d = dst_value_list.begin(); d != dst_value_list.end(); d++) {
							index = d->first;
							value = d->second;
							std::map<size_t, uint32_t>::const_iterator v = value_list.find(index);
							if (v == value_list.end()) {
								insert_command_list.push_back(AddCommand(icLdc_i4, value));
							}
							else {
								insert_command_list.push_back(AddCommand(icLdloc, index));
								old_value = v->second;
								if (rand() & 1) {
									insert_command_list.push_back(AddCommand(icLdc_i4, value - old_value));
									insert_command_list.push_back(AddCommand(icAdd, 0));
								}
								else {
									insert_command_list.push_back(AddCommand(icLdc_i4, value ^ old_value));
									insert_command_list.push_back(AddCommand(icXor, 0));
								}
							}
							insert_command_list.push_back(AddCommand(icStloc, index));
						}
					}
					for (j = 0; j < insert_command_list.size(); j++) {
						insert_command = insert_command_list[j];
						insert_command->set_address_range(address_range);
						insert_command->CompileToNative();
						insert_command->include_option(roNoProgress);
						new_command_list.push_back(insert_command);
					}
					if (!insert_command_list.empty()) {
						insert_command = insert_command_list[0];
						for (j = 0; j < link_list()->count(); j++) {
							CommandLink *link = link_list()->item(j);
							if (link->next_command() == command)
								link->set_next_command(insert_command);
						}
					}
					insert_command_list.clear();
				}
				else if (!value_list.empty()) {
					value_list_map[command] = value_list;
					dst_value_list = value_list;
				}
			}
			value_list = dst_value_list;
		}

		if (!predicate_list.empty() && !command->is_end()) {
			if (rand() & 1) {
				// modify predicate
				value = rand32();
				bit_mask = 0x1f;
				index = predicate_list[rand() % predicate_list.size()];
				std::map<size_t, uint32_t>::const_iterator it = value_list.find(index);
				if (it != value_list.end()) {
					old_value = it->second;
					if (rand() & 1) {
						insert_command_list.push_back(AddCommand(icLdc_i4, value));
						insert_command_list.push_back(AddCommand(icLdloc, index));
					}
					else {
						insert_command_list.push_back(AddCommand(icLdloc, index));
						insert_command_list.push_back(AddCommand(icLdc_i4, value));
						std::swap(value, old_value);
					}
					switch (rand() % (8 + (old_value != 0 ? 2 : 0))) {
					case 0:
						value += old_value;
						insert_command_list.push_back(AddCommand(icAdd, 0));
						break;
					case 1:
						value -= old_value;
						insert_command_list.push_back(AddCommand(icSub, 0));
						break;
					case 2:
						old_value &= bit_mask;
						if (insert_command_list.back()->type() == icLdc_i4)
							insert_command_list.back()->set_operand_value(0, old_value);
						else {
							insert_command_list.push_back(AddCommand(icLdc_i4, bit_mask));
							insert_command_list.push_back(AddCommand(icAnd, 0));
						}
						value >>= old_value;
						insert_command_list.push_back(AddCommand(icShr_un, 0));
						break;
					case 3:
						old_value &= bit_mask;
						if (insert_command_list.back()->type() == icLdc_i4)
							insert_command_list.back()->set_operand_value(0, old_value);
						else {
							insert_command_list.push_back(AddCommand(icLdc_i4, bit_mask));
							insert_command_list.push_back(AddCommand(icAnd, 0));
						}
						value <<= old_value;
						insert_command_list.push_back(AddCommand(icShl, 0));
						break;
					case 4:
						value |= old_value;
						insert_command_list.push_back(AddCommand(icOr, 0));
						break;
					case 5:
						value &= old_value;
						insert_command_list.push_back(AddCommand(icAnd, 0));
						break;
					case 6:
						value *= old_value;
						insert_command_list.push_back(AddCommand(icMul, 0));
						break;
					case 8:
						value /= old_value;
						insert_command_list.push_back(AddCommand(icDiv_un, 0));
						break;
					case 9:
						value %= old_value;
						insert_command_list.push_back(AddCommand(icRem_un, 0));
						break;
					default:
						value ^= old_value;
						insert_command_list.push_back(AddCommand(icXor, 0));
						break;
					}
				}
				else {
					insert_command_list.push_back(AddCommand(icLdc_i4, value));
				}
				insert_command_list.push_back(AddCommand(icStloc, index));
				value_list[index] = value;
			}
		}

		if (!value_list.empty()) {
			index = NOT_ID;
			for (std::map<size_t, uint32_t>::const_iterator it = value_list.begin(); it != value_list.end(); it++) {
				index = it->first;
				if (rand() & 1)
					break;
			}
			value = value_list[index];
			if (command->link() == NULL && (command->type() == icLdc_i4 || command->type() == icLdc_i4_s
				|| command->type() == icLdc_i4_0 || command->type() == icLdc_i4_1 || command->type() == icLdc_i4_2 || command->type() == icLdc_i4_3
				|| command->type() == icLdc_i4_4 || command->type() == icLdc_i4_5 || command->type() == icLdc_i4_6 || command->type() == icLdc_i4_7
				|| command->type() == icLdc_i4_8 || command->type() == icLdc_i4_m1)) {
				switch (command->type()) {
				case icLdc_i4_0:
					old_value = 0;
					break;
				case icLdc_i4_1:
					old_value = 1;
					break;
				case icLdc_i4_2:
					old_value = 2;
					break;
				case icLdc_i4_3:
					old_value = 3;
					break;
				case icLdc_i4_4:
					old_value = 4;
					break;
				case icLdc_i4_5:
					old_value = 5;
					break;
				case icLdc_i4_6:
					old_value = 6;
					break;
				case icLdc_i4_7:
					old_value = 7;
					break;
				case icLdc_i4_8:
					old_value = 8;
					break;
				case icLdc_i4_m1:
					old_value = static_cast<uint32_t>(-1);
					break;
				case icLdc_i4_s:
					old_value = static_cast<int8_t>(command->operand_value());
					break;
				default:
					old_value = static_cast<uint32_t>(command->operand_value());
					break;
				}

				insert_command_list.push_back(AddCommand(icLdloc, index));
				switch (rand() & 3) {
				case 0:
					insert_command_list.push_back(AddCommand(icLdc_i4, old_value - value));
					insert_command_list.push_back(AddCommand(icAdd, 0));
					break;
				case 1:
					insert_command_list.push_back(AddCommand(icLdc_i4, value - old_value));
					insert_command_list.push_back(AddCommand(icSub, 0));
					break;
				default:
					insert_command_list.push_back(AddCommand(icLdc_i4, old_value ^ value));
					insert_command_list.push_back(AddCommand(icXor, 0));
					break;
				}

				command->clear();
				command->Init(icNop);
			}
			else if (rand() & 1) {
				stack = stack_map[command];
				if (stack < 1) 
				{
					ILCommand *random_command = NULL;
					for (j = 0; j < orig_count; j++) {
						insert_command = item(j);
						if (insert_command->type() == icComment || insert_command->type() == icCase || insert_command->is_data())
							continue;
						if (insert_command != command && insert_command->address_range() == address_range) {
							if (stack == stack_map[insert_command]) {
								random_command = insert_command;
								if (rand() & 1)
									break;
							}
						}
						while (insert_command->is_prefix() && j < orig_count) {
							insert_command = item(++j);
						}
					}

					if (random_command) {
						old_value = rand32();
						bit_mask = 0x1f;
						if (rand() & 1) {
							insert_command_list.push_back(AddCommand(icLdloc, index));
							insert_command_list.push_back(AddCommand(icLdc_i4, old_value));
						}
						else {
							insert_command_list.push_back(AddCommand(icLdc_i4, old_value));
							insert_command_list.push_back(AddCommand(icLdloc, index));
							std::swap(value, old_value);
						}

						ILCommandType branch_type;
						if (rand() & 1) {
							switch (rand() % 3) {
							case 0:
								branch_type = (value < old_value) ? icBge_un : icBlt_un;
								break;
							case 1:
								branch_type = (value > old_value) ? icBle_un : icBgt_un;
								break;
							default:
								branch_type = (value != old_value) ? icBeq : icBne_un;
								break;
							}
						}
						else {
							switch (rand() % (8 + (old_value != 0 ? 2 : 0))) {
							case 0:
								value += old_value;
								insert_command_list.push_back(AddCommand(icAdd, 0));
								break;
							case 1:
								value -= old_value;
								insert_command_list.push_back(AddCommand(icSub, 0));
								break;
							case 2:
								old_value &= bit_mask;
								if (insert_command_list.back()->type() == icLdc_i4)
									insert_command_list.back()->set_operand_value(0, old_value);
								else {
									insert_command_list.push_back(AddCommand(icLdc_i4, bit_mask));
									insert_command_list.push_back(AddCommand(icAnd, 0));
								}
								value >>= old_value;
								insert_command_list.push_back(AddCommand(icShr_un, 0));
								break;
							case 3:
								old_value &= bit_mask;
								if (insert_command_list.back()->type() == icLdc_i4)
									insert_command_list.back()->set_operand_value(0, old_value);
								else {
									insert_command_list.push_back(AddCommand(icLdc_i4, bit_mask));
									insert_command_list.push_back(AddCommand(icAnd, 0));
								}
								value <<= old_value;
								insert_command_list.push_back(AddCommand(icShl, 0));
								break;
							case 5:
								value &= old_value;
								insert_command_list.push_back(AddCommand(icAnd, 0));
								break;
							case 6:
								value *= old_value;
								insert_command_list.push_back(AddCommand(icMul, 0));
								break;
							case 8:
								value /= old_value;
								insert_command_list.push_back(AddCommand(icDiv_un, 0));
								break;
							case 9:
								value %= old_value;
								insert_command_list.push_back(AddCommand(icRem_un, 0));
								break;
							default:
								value ^= old_value;
								insert_command_list.push_back(AddCommand(icXor, 0));
								break;
							}
							branch_type = value ? icBrfalse : icBrtrue;
						}
						insert_command = AddCommand(branch_type, 0);
						insert_link_list.push_back(insert_command->AddLink(0, ltJmpWithFlag, random_command));
						insert_command_list.push_back(insert_command);
					}
				}
			}
		}

		if (command->type() == icSwitch) {
			size_t case_count = static_cast<uint32_t>(command->operand_value());
			std::set<ILCommand *> link_command_list;
			for (j = 0; j < case_count; j++) {
				ILCommand *case_command = item(i + 1 + j);
				link_command_list.insert(reinterpret_cast<ILCommand *>(case_command->link()->to_command()));
			}
			for (std::set<ILCommand *>::const_iterator l = link_command_list.begin(); l != link_command_list.end(); l++) {
				link_command = *l;
				if (link_command && (link_command->section_options() & rtLinkedToExt) == 0) {
					std::map<ILCommand *, std::map<size_t, uint32_t> >::const_iterator it = value_list_map.find(link_command);
					if (it == value_list_map.end())
						value_list_map[link_command] = value_list;
					else
						value_list_map.erase(it);
				}
			}
		} else if (link_command && (link_command->section_options() & rtLinkedToExt) == 0) {
			std::map<ILCommand *, std::map<size_t, uint32_t> >::const_iterator it = value_list_map.find(link_command);
			if (it == value_list_map.end())
				value_list_map[link_command] = value_list;
			else {
				// syncronize predicates
				std::map<size_t, uint32_t> dst_value_list = it->second;
				for (std::map<size_t, uint32_t>::const_iterator d = dst_value_list.begin(); d != dst_value_list.end(); d++) {
					index = d->first;
					value = d->second;
					std::map<size_t, uint32_t>::const_iterator v = value_list.find(index);
					if (v == value_list.end()) {
						insert_command_list.push_back(AddCommand(icLdc_i4, value));
					}
					else {
						insert_command_list.push_back(AddCommand(icLdloc, index));
						old_value = v->second;
						if (rand() & 1) {
							insert_command_list.push_back(AddCommand(icLdc_i4, value - old_value));
							insert_command_list.push_back(AddCommand(icAdd, 0));
						}
						else {
							insert_command_list.push_back(AddCommand(icLdc_i4, value ^ old_value));
							insert_command_list.push_back(AddCommand(icXor, 0));
						}
					}
					insert_command_list.push_back(AddCommand(icStloc, index));
					value_list[index] = value;
				}
			}
		}

		if (!insert_command_list.empty()) {
			if (command->type() == icNop) {
				new_command_list.push_back(command);
				command = NULL;
			} else if (command->section_options() & (rtLinkedToInt | rtLinkedToExt)) {
				insert_command = command->Clone(this);
				if (command->link())
					command->link()->set_from_command(insert_command);
				command->clear();
				command->Init(icNop);
				new_command_list.push_back(command);
				command = insert_command;
			}

			for (j = 0; j < insert_command_list.size(); j++) {
				insert_command = insert_command_list[j];
				insert_command->set_address_range(address_range);
				insert_command->CompileToNative();
				new_command_list.push_back(insert_command);
			}
		}
		if (command) {
			new_command_list.push_back(command);
			while (command->is_prefix() && i < orig_count) {
				command = item(++i);
				new_command_list.push_back(command);
			}
			if (command->is_end())
				value_list.clear();
		}
	}

	assign(new_command_list);

	for (i = 0; i < insert_link_list.size(); i++) {
		insert_link_list[i]->from_command()->PrepareLink(ctx);
	}
}

void ILFunction::CompileToNative(const CompileContext &ctx)
{
	size_t i, j, k;
	ILCommand *command;
	uint16_t variable_index;
	uint16_t add_stack = (compilation_type() == ctMutation) ? 20 : 0;

	for (i = 0; i < function_info_list()->count(); i++) {
		FunctionInfo *info = function_info_list()->item(i);
		ILCommand *entry = reinterpret_cast<ILCommand *>(info->entry());
		if (entry) {
			ILStandAloneSig *locals = reinterpret_cast<NETRuntimeFunction *>(info->source())->method()->locals();
			Data data;
			if (entry->dump_size() == 1) {
				data.PushByte(CorILMethod_InitLocals | CorILMethod_FatFormat);
				data.PushByte(0x30);
				data.PushWord(0x08 + add_stack);
			}
			else {
				data.PushByte(entry->dump(0));
				data.PushByte(entry->dump(1));
				data.PushWord(static_cast<uint16_t>(entry->dump_value(2, osWord)) + add_stack);
			}
			data.PushDWord(0x0);
			if (!locals)
				data.PushDWord(0x0);
			entry->set_dump(data.data(), data.size());
		}

		ILCommand *data_entry = reinterpret_cast<ILCommand *>(info->data_entry());
		if (!data_entry)
			continue;

		uint32_t flags = static_cast<uint32_t>(data_entry->operand_value());
		bool fat = (flags & CorILMethod_Sect_FatFormat) != 0;
		if (fat)
			continue;

		if (flags & CorILMethod_Sect_EHTable) {
			// convert EHTable to fat format
			uint32_t data_size = fat ? (flags >> 8) : static_cast<uint8_t>(flags >> 8);
			uint32_t clauses = (data_size - sizeof(uint32_t)) / (fat ? 24 : 12);
			k = IndexOf(data_entry) + 1;
			for (j = 0; j < clauses * 6; j++) {
				command = item(k + j);
				command->Init(icDword, command->operand_value(), command->token_reference());
				command->CompileToNative();
			}
			data_size = sizeof(uint32_t) + clauses * 6 * sizeof(uint32_t);
			data_entry->set_operand_value(0, (data_size << 8) | (flags & 0xff) | CorILMethod_Sect_FatFormat);
			data_entry->CompileToNative();
		}
	}

	for (i = 0; i < count(); i++) {
		command = item(i);

		switch (command->type()) {
		case icBr_s:
			command->Init(icBr, command->operand_value());
			command->CompileToNative();
			break;
		case icBrfalse_s:
			command->Init(icBrfalse, command->operand_value());
			command->CompileToNative();
			break;
		case icBrtrue_s:
			command->Init(icBrtrue, command->operand_value());
			command->CompileToNative();
			break;
		case icBeq_s:
			command->Init(icBeq, command->operand_value());
			command->CompileToNative();
			break;
		case icBge_s:
			command->Init(icBge, command->operand_value());
			command->CompileToNative();
			break;
		case icBgt_s:
			command->Init(icBgt, command->operand_value());
			command->CompileToNative();
			break;
		case icBle_s:
			command->Init(icBle, command->operand_value());
			command->CompileToNative();
			break;
		case icBlt_s:
			command->Init(icBlt, command->operand_value());
			command->CompileToNative();
			break;
		case icBne_un_s:
			command->Init(icBne_un, command->operand_value());
			command->CompileToNative();
			break;
		case icBge_un_s:
			command->Init(icBge_un, command->operand_value());
			command->CompileToNative();
			break;
		case icBgt_un_s:
			command->Init(icBgt_un, command->operand_value());
			command->CompileToNative();
			break;
		case icBle_un_s:
			command->Init(icBle_un, command->operand_value());
			command->CompileToNative();
			break;
		case icBlt_un_s:
			command->Init(icBlt_un, command->operand_value());
			command->CompileToNative();
			break;
		case icLeave_s:
			command->Init(icLeave, command->operand_value());
			command->CompileToNative();
			break;
		case icLdloc:
			variable_index = static_cast<uint16_t>(command->operand_value());
			switch (variable_index) {
			case 0:
				command->Init(icLdloc_0, 0);
				command->CompileToNative();
				break;
			case 1:
				command->Init(icLdloc_1, 0);
				command->CompileToNative();
				break;
			case 2:
				command->Init(icLdloc_2, 0);
				command->CompileToNative();
				break;
			case 3:
				command->Init(icLdloc_3, 0);
				command->CompileToNative();
				break;
			default:
				if (variable_index < 0x100) {
					command->Init(icLdloc_s, variable_index);
					command->CompileToNative();
				}
				break;
			}
			break;
		case icLdloca:
			variable_index = static_cast<uint16_t>(command->operand_value());
			if (variable_index < 0x100) {
				command->Init(icLdloca_s, variable_index);
				command->CompileToNative();
			}
			break;
		case icStloc:
			variable_index = static_cast<uint16_t>(command->operand_value());
			switch (variable_index) {
			case 0:
				command->Init(icStloc_0, 0);
				command->CompileToNative();
				break;
			case 1:
				command->Init(icStloc_1, 0);
				command->CompileToNative();
				break;
			case 2:
				command->Init(icStloc_2, 0);
				command->CompileToNative();
				break;
			case 3:
				command->Init(icStloc_3, 0);
				command->CompileToNative();
				break;
			default:
				if (variable_index < 0x100) {
					command->Init(icStloc_s, variable_index);
					command->CompileToNative();
				}
				break;
			}
			break;
		}
	}

	CreateBlocks();
}

bool IsBoxedElementType(CorElementType type)
{
	switch (type) {
	case ELEMENT_TYPE_BOOLEAN: case ELEMENT_TYPE_CHAR: case ELEMENT_TYPE_I1: case ELEMENT_TYPE_U1:
	case ELEMENT_TYPE_I2: case ELEMENT_TYPE_U2: case ELEMENT_TYPE_I4: case ELEMENT_TYPE_U4:
	case ELEMENT_TYPE_I8: case ELEMENT_TYPE_U8: case ELEMENT_TYPE_R4: case ELEMENT_TYPE_R8:
	case ELEMENT_TYPE_I: case ELEMENT_TYPE_U: case ELEMENT_TYPE_VALUETYPE:
		return true;
	}
	return false;
}

void ILFunction::CompileToVM(const CompileContext &ctx)
{
	if (function_info_list()->count() == 0)
		return;

	struct FunctionEntryInfo {
		ILMethodDef *method;
		ILToken *ret;
		std::vector<ILElement *> param_list;
		ILSignature *locals;
		ILCommand *data_entry;
		FunctionEntryInfo()
			: method(NULL), ret(NULL), locals(NULL), data_entry(NULL)
		{

		}
		FunctionEntryInfo(ILMethodDef *method_, ILToken *ret_, std::vector<ILElement *> param_list_, ILSignature *locals_, ILCommand *data_entry_)
		{
			method = method_;
			ret = ret_;
			param_list = param_list_;
			locals = locals_;
			data_entry = data_entry_;
		}
	};

	size_t i, j, c, k;
	ILCommand *command, *data_entry;
	CommandLink *link;
	CommandBlock *cur_block;
	std::string type_name;
	ILToken *type, *type_ret;
	ILSignature *locals;
	ILMetaData *meta;
	ILElement *element;
	ILMethodDef *method;
	std::map<ICommand *, FunctionEntryInfo> entry_map;

	// create internal links
	c = link_list()->count();
	for (i = 0; i < count(); i++) {
		command = item(i);
		if (command->block() || (command->options() & roNeedCompile) == 0)
			continue;

		switch (command->type()) {
		case icSwitch:
			command->AddLink(0, ltSwitch, item(i + 1));
			break;
		}
	}
	for (i = c; i < link_list()->count(); i++) {
		link_list()->item(i)->from_command()->PrepareLink(ctx);
	}

	ILVirtualMachine *virtual_machine = reinterpret_cast<ILVirtualMachine *>(BaseFunction::virtual_machine(ctx.file->virtual_machine_list(), NULL));

	for (k = 0; k < function_info_list()->count(); k++) {
		FunctionInfo *info = function_info_list()->item(k);
		if (!info->source() || info->entry()->block())
			continue;

		ICommand *entry = info->entry();
		data_entry = reinterpret_cast<ILCommand *>(info->data_entry());
		method = reinterpret_cast<NETRuntimeFunction *>(info->source())->method();
		ILSignature *signature = method->signature();
		if (!signature->ret())
			throw std::runtime_error("Invalid method signature");

		locals = method->locals() ? method->locals()->signature() : NULL;
		meta = method->meta();
		ILElement object_element(NULL, NULL);
		{
			ILData data;
			data.push_back(ELEMENT_TYPE_OBJECT);
			object_element.Parse(data);
		}

		ILToken *type_object = meta->ImportType(object_element);
		type_ret = (signature->ret()->type() == ELEMENT_TYPE_VOID) ? NULL : meta->ImportType(*signature->ret());

		bool need_try_finally = false;
		size_t try_start_index = 0;
		size_t try_end_index = 0;
		size_t finally_start_index = 0;
		size_t finally_end_index = 0;
		size_t newarr_index = 0;

		std::vector<ILElement *> param_list;
		for (i = 0; i < signature->count(); i++) {
			element = signature->item(i);
			if (element->is_ref())
				need_try_finally = true;
			param_list.push_back(element);
		}
		if ((method->flags() & mdStatic) == 0)
			param_list.insert(param_list.begin(), &object_element);

		if (!method->is_deleted()) {
			cur_block = AddBlock(count(), true);

			// header
			Data data;
			data.PushByte(CorILMethod_InitLocals | CorILMethod_FatFormat | (need_try_finally ? CorILMethod_MoreSects : 0));
			data.PushByte(0x30);
			data.PushWord(0x08);
			data.PushDWord(0x0);
			command = AddCommand(data);
			command->set_alignment(sizeof(uint32_t));

			info = function_info_list()->Add(info->begin(), info->begin(), btValue, command->dump_size() + sizeof(uint32_t), 0, 0, info->source(), command);

			// locals
			if (need_try_finally) {
				ILData locals_data;
				locals_data.push_back(stLocal);
				locals_data.push_back(1 + ((signature->ret()->type() == ELEMENT_TYPE_VOID) ? 0 : 1));
				locals_data.push_back(ELEMENT_TYPE_SZARRAY);
				locals_data.push_back(ELEMENT_TYPE_OBJECT);
				if (signature->ret()->type() != ELEMENT_TYPE_VOID)
					signature->ret()->WriteToData(locals_data);
				ILStandAloneSig *new_locals = meta->AddStandAloneSig(locals_data);
				command = AddCommand(icDword, new_locals->id());
				command->set_token_reference(new_locals->reference_list()->Add(0));
			}
			else {
				AddCommand(icDword, 0);
			}

			newarr_index = count();
			if (!param_list.empty()) {
				// load args
				AddCommand(icLdc_i4, param_list.size());
				command = AddCommand(icNewarr, type_object->id());
				command->set_token_reference(type_object->reference_list()->Add(0));

				for (i = 0; i < param_list.size(); i++) {
					AddCommand(icDup, 0);
					switch (i) {
					case 0: AddCommand(icLdc_i4_0, 0); break;
					case 1: AddCommand(icLdc_i4_1, 0); break;
					case 2: AddCommand(icLdc_i4_2, 0); break;
					case 3: AddCommand(icLdc_i4_3, 0); break;
					case 4: AddCommand(icLdc_i4_4, 0); break;
					case 5: AddCommand(icLdc_i4_5, 0); break;
					case 6: AddCommand(icLdc_i4_6, 0); break;
					case 7: AddCommand(icLdc_i4_7, 0); break;
					case 8: AddCommand(icLdc_i4_8, 0); break;
					default: AddCommand(icLdc_i4, i); break;
					}
					switch (i) {
					case 0: AddCommand(icLdarg_0, 0); break;
					case 1: AddCommand(icLdarg_1, 0); break;
					case 2: AddCommand(icLdarg_2, 0); break;
					case 3: AddCommand(icLdarg_3, 0); break;
					default: AddCommand(icLdarg_s, i); break;
					}

					element = param_list[i];
					if (element->is_ref()) {
						switch (element->type()) {
						case ELEMENT_TYPE_BOOLEAN:
						case ELEMENT_TYPE_I1:
							AddCommand(icLdind_i1, 0);
							break;
						case ELEMENT_TYPE_U1:
							AddCommand(icLdind_u1, 0);
							break;
						case ELEMENT_TYPE_I2:
							AddCommand(icLdind_i2, 0);
							break;
						case ELEMENT_TYPE_CHAR:
						case ELEMENT_TYPE_U2:
							AddCommand(icLdind_u2, 0);
							break;
						case ELEMENT_TYPE_I4:
							AddCommand(icLdind_i4, 0);
							break;
						case ELEMENT_TYPE_U4:
							AddCommand(icLdind_u4, 0);
							break;
						case ELEMENT_TYPE_I8:
							AddCommand(icLdind_i8, 0);
							break;
						case ELEMENT_TYPE_R4:
							AddCommand(icLdind_r4, 0);
							break;
						case ELEMENT_TYPE_R8:
							AddCommand(icLdind_r8, 0);
							break;
						case ELEMENT_TYPE_I:
						case ELEMENT_TYPE_U:
							AddCommand(icLdind_i, 0);
							break;
						default:
							AddCommand(icLdind_ref, 0);
							break;
						}
					}

					type = meta->ImportType(*element);
					if (element->type() == ELEMENT_TYPE_PTR) {
						uint64_t address = ctx.vm_runtime->export_list()->GetAddressByType(atBoxPointer);
						ILFunction *func = reinterpret_cast<ILFunction *>(ctx.vm_runtime->function_list()->GetFunctionByAddress(address));
						for (j = 0; j < func->count(); j++) {
							ILCommand *src_command = func->item(j);
							if (src_command->type() == icComment || src_command->type() == icLdarg_0)
								continue;
							if (src_command->type() == icRet)
								break;
							command = src_command->Clone(this);
							if (command->type() == icLdtoken) {
								command->set_operand_value(0, type->id());
								command->set_token_reference(type->reference_list()->Add(0));
							} else if (command->token_reference())
								command->set_token_reference(command->token_reference()->owner()->Add(0));
							AddObject(command);
						}
					} else if (IsBoxedElementType(element->type())) {
						command = AddCommand(icBox, type->id());
						command->set_token_reference(type->reference_list()->Add(0));
					}
					AddCommand(icStelem_ref, 0);
				}
				if (need_try_finally)
					AddCommand(icStloc_0, 0);
			}

			if (need_try_finally || param_list.empty()) {
				try_start_index = count();
				command = AddCommand(icNewobj, virtual_machine->ctor()->id());
				command->set_token_reference(virtual_machine->ctor()->reference_list()->Add(0));
				if (param_list.empty())
					AddCommand(icLdnull, 0);
				else
					AddCommand(icLdloc_0, 0);
			}
			else {
				command = new ILCommand(this, cpu_address_size(), icNewobj, virtual_machine->ctor()->id());
				command->set_token_reference(virtual_machine->ctor()->reference_list()->Add(0));
				InsertObject(newarr_index, command);
			}
			command = AddCommand(icLdc_i4, 0);
			link = command->AddLink(0, ltOffset, entry);
			entry->include_section_option(rtLinkedToExt);
			link->set_sub_value(ctx.file->image_base());

			command = AddCommand(icCall, virtual_machine->invoke()->id());
			command->set_token_reference(virtual_machine->invoke()->reference_list()->Add(0));

			if (!type_ret)
				AddCommand(icPop, 0);
			else {
				if (signature->ret()->type() == ELEMENT_TYPE_PTR) {
					uint64_t address = ctx.vm_runtime->export_list()->GetAddressByType(atUnboxPointer);
					ILFunction *func = reinterpret_cast<ILFunction *>(ctx.vm_runtime->function_list()->GetFunctionByAddress(address));
					for (j = 0; j < func->count(); j++) {
						ILCommand *src_command = func->item(j);
						if (src_command->type() == icComment || src_command->type() == icLdarg_0)
							continue;
						if (src_command->type() == icRet)
							break;
						command = src_command->Clone(this);
						if (command->token_reference())
							command->set_token_reference(command->token_reference()->owner()->Add(0));
						AddObject(command);
					}
				} else {
					command = AddCommand(IsBoxedElementType(signature->ret()->type()) ? icUnbox_any : icCastclass, type_ret->id());
					command->set_token_reference(type_ret->reference_list()->Add(0));
				}
				if (need_try_finally)
					AddCommand(icStloc_1, 0);
			}

			if (need_try_finally) {
				command = AddCommand(icLeave, 0);
				command->AddLink(0, ltJmp);
				try_end_index = count() - 1;

				if (!param_list.empty()) {
					// save out args
					finally_start_index = count();
					for (i = 0; i < param_list.size(); i++) {
						element = param_list[i];
						if (!element->is_ref())
							continue;

						switch (i) {
						case 0: AddCommand(icLdarg_0, 0); break;
						case 1: AddCommand(icLdarg_1, 0); break;
						case 2: AddCommand(icLdarg_2, 0); break;
						case 3: AddCommand(icLdarg_3, 0); break;
						default: AddCommand(icLdarg_s, i); break;
						}
						AddCommand(icLdloc_0, 0);
						switch (i) {
						case 0: AddCommand(icLdc_i4_0, 0); break;
						case 1: AddCommand(icLdc_i4_1, 0); break;
						case 2: AddCommand(icLdc_i4_2, 0); break;
						case 3: AddCommand(icLdc_i4_3, 0); break;
						case 4: AddCommand(icLdc_i4_4, 0); break;
						case 5: AddCommand(icLdc_i4_5, 0); break;
						case 6: AddCommand(icLdc_i4_6, 0); break;
						case 7: AddCommand(icLdc_i4_7, 0); break;
						case 8: AddCommand(icLdc_i4_8, 0); break;
						default: AddCommand(icLdc_i4, i); break;
						}
						AddCommand(icLdelem_ref, 0);
						if (IsBoxedElementType(element->type())) {
							type = meta->ImportType(*element);
							command = AddCommand(icUnbox_any, type->id());
							command->set_token_reference(type->reference_list()->Add(0));
						}
						switch (element->type()) {
						case ELEMENT_TYPE_BOOLEAN:
						case ELEMENT_TYPE_I1:
						case ELEMENT_TYPE_U1:
							AddCommand(icStind_i1, 0);
							break;
						case ELEMENT_TYPE_CHAR:
						case ELEMENT_TYPE_I2:
						case ELEMENT_TYPE_U2:
							AddCommand(icStind_i2, 0);
							break;
						case ELEMENT_TYPE_I4:
						case ELEMENT_TYPE_U4:
							AddCommand(icStind_i4, 0);
							break;
						case ELEMENT_TYPE_I8:
							AddCommand(icStind_i8, 0);
							break;
						case ELEMENT_TYPE_R4:
							AddCommand(icStind_r4, 0);
							break;
						case ELEMENT_TYPE_R8:
							AddCommand(icStind_r8, 0);
							break;
						case ELEMENT_TYPE_I:
						case ELEMENT_TYPE_U:
							AddCommand(icStind_i, 0);
							break;
						default:
							AddCommand(icStind_ref, 0);
							break;
						}
					}
					AddCommand(icEndfinally, 0);
					finally_end_index = count() - 1;

					if (type_ret)
						AddCommand(icLdloc_1, 0);
				}
			}
			AddCommand(icRet, 0);

			AddressRange *address_range = info->Add(0, 0, NULL, NULL, NULL);
			for (i = cur_block->start_index(); i < count(); i++) {
				command = item(i);
				command->set_block(cur_block);
				command->set_address_range(address_range);
				command->CompileToNative();
				cur_block->set_end_index(i);
			}

			if (need_try_finally) {
				item(try_end_index)->link()->set_to_command(item(finally_end_index + 1));
				address_range = function_info_list()->Add(info->begin() + 1, info->begin() + 1, btValue, 0, 0, 0, NULL, NULL)->Add(0, 0, NULL, NULL, NULL);

				cur_block = AddBlock(count(), true);
				command = AddCommand(icDword, (0x1c << 8) | CorILMethod_Sect_FatFormat | CorILMethod_Sect_EHTable);
				command->set_alignment(sizeof(uint32_t));
				AddCommand(icDword, COR_ILEXCEPTION_CLAUSE_FINALLY);
				ILCommand *try_entry = AddCommand(icDword, 0);
				ILCommand *try_length = AddCommand(icDword, 0);
				ILCommand *finally_entry = AddCommand(icDword, 0);
				link = finally_entry->AddLink(0, ltFinallyBlock, item(finally_start_index));
				link->set_base_function_info(info);
				ILCommand *finally_length = AddCommand(icDword, 0);
				AddCommand(icDword, 0);
				for (i = cur_block->start_index(); i < count(); i++) {
					command = item(i);
					command->set_block(cur_block);
					command->set_address_range(address_range);
					command->CompileToNative();
					cur_block->set_end_index(i);
				}

				address_range = range_list()->Add(0, 0, try_entry, NULL, try_length);
				for (i = try_start_index; i <= try_end_index; i++) {
					command = item(i);
					command->set_address_range(address_range);
				}
				address_range = range_list()->Add(0, 0, finally_entry, NULL, finally_length);
				for (i = finally_start_index; i <= finally_end_index; i++) {
					command = item(i);
					command->set_address_range(address_range);
				}
			}
		}

		entry_map[entry] = FunctionEntryInfo(method, type_ret, param_list, locals, data_entry);
	}

	cur_block = NULL;
	uint64_t image_base = ctx.file->image_base();
	std::set<uint64_t> try_list;
	std::map<uint32_t, uint32_t> variable_map, param_map;
	size_t temp_variable_index = NOT_ID;
	for (i = 0; i < count(); i++) {
		command = item(i);
		if ((command->options() & roNoProgress) == 0)
			ctx.file->StepProgress();

		if (command->block() || (command->options() & roNeedCompile) == 0) {
			cur_block = NULL;
			continue;
		}

		if (command->is_data()) {
			cur_block = NULL;
			continue;
		}

		if (!cur_block) {
			cur_block = AddBlock(i, false);
			cur_block->set_virtual_machine(virtual_machine);
		}

		cur_block->set_end_index(i);
		command->set_block(cur_block);

		if (command->type() == icComment) {
			if (command->token_reference())
				command->token_reference()->set_deleted(true);
			std::map<ICommand *, FunctionEntryInfo>::const_iterator it = entry_map.find(command);
			if (it != entry_map.end()) {
				try_list.clear();
				type_ret = it->second.ret;
				method = it->second.method;
				meta = method->meta();
				locals = it->second.locals;
				data_entry = it->second.data_entry;
				std::vector<ILElement *> param_list = it->second.param_list;
				variable_map.clear();
				param_map.clear();

				if (command->section_options() & rtLinkedToInt) {
					command->AddVMCommand(ctx, icWord, param_list.size(), 0);
					for (j = param_list.size(); j > 0; j--) {
						element = param_list[j - 1];
						type = meta->ImportType(*element);
						command->AddVMCommand(ctx, icDword, type->id(), 0, type->reference_list()->Add(0));
					}
					if (type_ret)
						command->AddVMCommand(ctx, icDword, type_ret->id(), 0, type_ret->reference_list()->Add(0));
					else
						command->AddVMCommand(ctx, icDword, 0, 0);

					if (command->section_options() & rtLinkedToExt)
						command->AddExtSection(ctx);
				}

				if (data_entry) {
					uint32_t flags = static_cast<uint32_t>(data_entry->operand_value());
					if (flags & CorILMethod_Sect_EHTable) {
						bool fat = (flags & CorILMethod_Sect_FatFormat) != 0;
						uint32_t data_size = fat ? (flags >> 8) : static_cast<uint8_t>(flags >> 8);
						uint32_t clauses = (data_size - sizeof(uint32_t)) / (fat ? 24 : 12);
						k = IndexOf(data_entry) + 1;
						for (j = 0; j < clauses; j++) {
							ILCommand *catch_flags = item(k++);
							ILCommand *catch_try_offset = item(k++);
							ILCommand *catch_try_length = item(k++);
							ILCommand *catch_handler_offset = item(k++);
							ILCommand *catch_handler_length = item(k++);
							ILCommand *catch_filter = item(k++);

							command->AddVMCommand(ctx, icInitcatchblock, 0);
							command->AddVMCommand(ctx, icByte, static_cast<uint8_t>(catch_flags->operand_value()));
							uint64_t try_address = method->address() + method->fat_size() + catch_try_offset->operand_value();
							try_list.insert(try_address);
							command->AddVMCommand(ctx, icDword, static_cast<uint32_t>(try_address - image_base));
							command->AddVMCommand(ctx, icDword, static_cast<uint32_t>(try_address - image_base + catch_try_length->operand_value()));
							command->AddVMCommand(ctx, icDword, 0, 0, NULL, catch_handler_offset->link() ? reinterpret_cast<ILCommand *>(catch_handler_offset->link()->to_command()) : NULL);
							if (catch_filter->link()) {
								command->AddVMCommand(ctx, icDword, 0, 0, NULL, reinterpret_cast<ILCommand *>(catch_filter->link()->to_command()));
							}
							else {
								command->AddVMCommand(ctx, icDword, catch_filter->operand_value(), 0, catch_filter->token_reference());
							}
						}
					}
				}

				// merge params and locals
				std::vector<ILElement *> arg_list;
				for (j = 0; j < param_list.size(); j++) {
					arg_list.push_back(param_list[j]);
				}
				if (locals) {
					for (j = 0; j < locals->count(); j++) {
						arg_list.push_back(locals->item(j));
					}
				}
				for (j = 0; j < arg_list.size(); j++) {
					std::swap(arg_list[j], arg_list[rand() % arg_list.size()]);
				}

				for (j = 0; j < arg_list.size(); j++) {
					element = arg_list[j];
					type = meta->ImportType(*element);
					uint32_t index;
					std::vector<ILElement *>::const_iterator it = std::find(param_list.begin(), param_list.end(), element);
					if (it != param_list.end()) {
						index = static_cast<uint32_t>(it - param_list.begin());
						command->AddVMCommand(ctx, icDup, 0);
						command->AddVMCommand(ctx, icLdc_i4, static_cast<uint32_t>(index));
						command->AddVMCommand(ctx, icLdelema, 0);
						command->AddVMCommand(ctx, element->is_ref() ? icDup : icLdc_i4, 0);
						command->AddVMCommand(ctx, icLdc_i4, type->id(), 0, type->reference_list()->Add(0));
						command->AddVMCommand(ctx, icInitarg, 0);
						param_map[index] = static_cast<uint32_t>(j);
					}
					else {
						index = static_cast<uint32_t>(locals->IndexOf(element));
						command->AddVMCommand(ctx, icLdnull, 0);
						command->AddVMCommand(ctx, icLdc_i4, element->is_ref() ? 1 : 0);
						command->AddVMCommand(ctx, icLdc_i4, type->id(), 0, type->reference_list()->Add(0));
						command->AddVMCommand(ctx, icInitarg, 0);
						variable_map[index] = static_cast<uint32_t>(j);
					}
				}
				if ((ctx.options.flags & cpMemoryProtection) && tag() != ftLoader) {
					temp_variable_index = arg_list.size();
					type = meta->ImportType(ELEMENT_TYPE_I4);
					command->AddVMCommand(ctx, icLdnull, 0);
					command->AddVMCommand(ctx, icDup, 0);
					command->AddVMCommand(ctx, icLdc_i4, type->id(), 0, type->reference_list()->Add(0));
					command->AddVMCommand(ctx, icInitarg, 0);
				}
				command->AddVMCommand(ctx, icPop, 0);
			}
			continue;
		}
		else {
			std::map<uint32_t, uint32_t>::const_iterator it;
			switch (command->type()) {
			case icRet:
				if (type_ret) {
					command->set_operand_value(0, type_ret->id());
					command->set_token_reference(type_ret->reference_list()->Add(0));
				}
				command->set_param(static_cast<uint32_t>(temp_variable_index));
				break;
			case icLdloc:
			case icLdloc_s:
				it = variable_map.find(static_cast<uint16_t>(command->operand_value()));
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icLdloca:
			case icLdloca_s:
				it = variable_map.find(static_cast<uint16_t>(command->operand_value()));
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icStloc:
			case icStloc_s:
				it = variable_map.find(static_cast<uint16_t>(command->operand_value()));
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icLdloc_0:
				it = variable_map.find(0);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icLdloc_1:
				it = variable_map.find(1);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icLdloc_2:
				it = variable_map.find(2);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icLdloc_3:
				it = variable_map.find(3);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icStloc_0:
				it = variable_map.find(0);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icStloc_1:
				it = variable_map.find(1);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icStloc_2:
				it = variable_map.find(2);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icStloc_3:
				it = variable_map.find(3);
				if (it == variable_map.end())
					throw std::runtime_error("Invalid variable index");
				command->set_param(it->second);
				break;
			case icLdarg:
			case icLdarg_s:
				it = param_map.find(static_cast<uint16_t>(command->operand_value()));
				if (it == param_map.end())
					throw std::runtime_error("Invalid param index");
				command->set_param(it->second);
				break;
			case icLdarga:
			case icLdarga_s:
				it = param_map.find(static_cast<uint16_t>(command->operand_value()));
				if (it == param_map.end())
					throw std::runtime_error("Invalid param index");
				command->set_param(it->second);
				break;
			case icStarg:
			case icStarg_s:
				it = param_map.find(static_cast<uint16_t>(command->operand_value()));
				if (it == param_map.end())
					throw std::runtime_error("Invalid param index");
				command->set_param(it->second);
				break;
			case icLdarg_0:
				it = param_map.find(0);
				if (it == param_map.end())
					throw std::runtime_error("Invalid param index");
				command->set_param(it->second);
				break;
			case icLdarg_1:
				it = param_map.find(1);
				if (it == param_map.end())
					throw std::runtime_error("Invalid param index");
				command->set_param(it->second);
				break;
			case icLdarg_2:
				it = param_map.find(2);
				if (it == param_map.end())
					throw std::runtime_error("Invalid param index");
				command->set_param(it->second);
				break;
			case icLdarg_3:
				it = param_map.find(3);
				if (it == param_map.end())
					throw std::runtime_error("Invalid param index");
				command->set_param(it->second);
				break;
			}
		}

		std::set<uint64_t>::const_iterator it = try_list.find(command->address());
		if (it != try_list.end()) {
			command->AddVMCommand(ctx, icLdc_i4, static_cast<uint32_t>(command->address() - image_base));
			command->AddVMCommand(ctx, icEntertry, 0);
		}

		command->CompileToVM(ctx);

		if (command->section_options() & rtCloseSection)
			cur_block = NULL;
	}
}

bool ILFunction::Compile(const CompileContext &ctx)
{
	if (compilation_type() != ctMutation) {
		for (size_t i = 0; i < function_info_list()->count(); i++) {
			FunctionInfo *info = function_info_list()->item(i);
			if (!info->source() || info->entry()->block())
				continue;

			ILSignature *signature = reinterpret_cast<NETRuntimeFunction *>(info->source())->method()->signature();
			bool is_generic = (signature->ret()->type() == ELEMENT_TYPE_VAR || signature->ret()->type() == ELEMENT_TYPE_MVAR);
			for (size_t j = 0; j < signature->count(); j++) {
				ILElement *element = signature->item(j);
				if (element->type() == ELEMENT_TYPE_VAR || element->type() == ELEMENT_TYPE_MVAR) {
					is_generic = true;
					break;
				}
			}
			if (is_generic) {
				ctx.file->Notify(mtError, this, "Method with generic parameters can't be virtualized");
				return false;
			}
		}
	}

	switch (compilation_type()) {
	case ctMutation:
		Mutate(ctx);
		CompileToNative(ctx);
		break;
	case ctVirtualization:
		CompileToVM(ctx);
		break;
	case ctUltra:
		Mutate(ctx);
		CompileToVM(ctx);
		break;
	default:
		return false;
	}

	return BaseFunction::Compile(ctx);
}

void ILFunction::AfterCompile(const CompileContext &ctx)
{
	for (size_t i = 0; i < count(); i++) {
		ILCommand *command = item(i);
		if (!command->block())
			continue;

		TokenReference *reference = command->token_reference();
		if (reference) {
			if (command->options() & roClearOriginalCode)
				reference->set_address(0);
		}
	}

	BaseFunction::AfterCompile(ctx);
}

ILCommand *ILFunction::ParseCommand(IArchitecture &file, uint64_t address, bool dump_mode /*= false*/)
{
	NETRuntimeFunction *runtime_func = reinterpret_cast<NETRuntimeFunction *>(file.runtime_function_list()->GetFunctionByAddress(address));

	ILCommand *command;
	if (dump_mode) {
		command = Add(address);
		if (!file.AddressSeek(address))
			command->InitUnknown();
		else if (!runtime_func || address < runtime_func->start() || (file.selected_segment()->memory_type() & mtExecutable) == 0)
			command->ReadValueFromFile(file, osByte);
		else {
			command->ReadFromFile(file);
			/*switch (command->type()) {
			case cmJmp:
			case cmCall:
			case cmJmpWithFlag: case cmJCXZ: case cmLoop: case cmLoope: case cmLoopne:
				if ((command->options() & roFar) == 0 && command->operand(0).type == otValue)
					command->AddLink(0, ltNone, command->operand(0).value);
				break;
			}
			*/
		}
		return command;
	} else {
		if (!runtime_func)
			return NULL;

		if (address == runtime_func->begin() && file.AddressSeek(address)) {
			size_t index = count();
			command = Add(address);
			command->set_alignment(sizeof(uint32_t));
			uint8_t format = static_cast<uint8_t>(command->ReadValueFromFile(file, osByte));

			FunctionInfo *info = function_info_list()->Add(runtime_func->begin(), runtime_func->end(), btValue, runtime_func->method()->fat_size(), 0, 0, runtime_func, command);

			switch (format & CorILMethod_FormatMask) {
			case CorILMethod_TinyFormat:
				command->InitComment(string_format(".maxstack %d", 8));
				break;
			case CorILMethod_FatFormat:
				{
					ILStandAloneSig *locals = runtime_func->method()->locals();

					command->ReadValueFromFile(file, osByte);
					uint16_t max_stack = static_cast<uint16_t>(command->ReadValueFromFile(file, osWord));
					command->ReadValueFromFile(file, osDWord);
					if (!locals)
						command->ReadValueFromFile(file, osDWord);
					command->InitComment(string_format(".maxstack %d", max_stack));
					address = command->next_address();

					if (locals) {
						command = Add(address);
						command->ReadValueFromFile(file, osDWord, true);
						command->InitComment(string_format(".locals init %s", locals ? locals->name(true).c_str() : "()"));
						address = command->next_address();
					}

					if (format & CorILMethod_MoreSects) {
						size_t old_count = count();
						uint64_t start_address = address;
						address = AlignValue(runtime_func->end(), sizeof(uint32_t));
						file.AddressSeek(address);
						uint64_t data_address = address;
						command = Add(address);
						command->set_alignment(sizeof(uint32_t));
						uint32_t flags = static_cast<uint32_t>(command->ReadValueFromFile(file, osDWord));
						bool fat = (flags & CorILMethod_Sect_FatFormat) != 0;
						uint32_t data_size = fat ? (flags >> 8) : static_cast<uint8_t>(flags >> 8);
						command->set_comment(CommentInfo(ttComment, string_format("Kind: %.2X; DataSize: %.8X", flags & CorILMethod_Sect_KindMask, data_size)));
						address = command->next_address();
						info->set_data_entry(command);
						if (flags & CorILMethod_Sect_EHTable) {
							CommandLink *link;
							uint64_t value;
							size_t clauses = (data_size - sizeof(uint32_t)) / (fat ? 24 : 12);
							for (size_t i = 0; i < clauses; i++) {
								command = Add(address);
								flags = static_cast<uint32_t>(command->ReadValueFromFile(file, fat ? osDWord : osWord));
								command->set_comment(CommentInfo(ttComment, "Flags"));
								address = command->next_address();

								ILCommand *try_entry = Add(address);
								value = try_entry->ReadValueFromFile(file, fat ? osDWord : osWord);
								try_entry->set_comment(CommentInfo(ttComment, "TryOffset"));
								address = try_entry->next_address();
								uint64_t try_begin = start_address + value;

								ILCommand *try_size = Add(address);
								value = try_size->ReadValueFromFile(file, fat ? osDWord : osByte);
								try_size->set_comment(CommentInfo(ttComment, "TryLength"));
								address = try_size->next_address();
								uint64_t try_end = try_begin + value;

								range_list()->Add(try_begin, try_end, try_entry, NULL, try_size);

								ILCommand *handler_entry = Add(address);
								value = handler_entry->ReadValueFromFile(file, fat ? osDWord : osWord);
								handler_entry->set_comment(CommentInfo(ttComment, "HandlerOffset"));
								address = handler_entry->next_address();
								link = handler_entry->AddLink(0, (flags & COR_ILEXCEPTION_CLAUSE_FINALLY) ? ltFinallyBlock : ltSEHBlock, start_address + value);
								link->set_base_function_info(info);
								uint64_t handler_begin = start_address + value;

								ILCommand *handler_size = Add(address);
								value = handler_size->ReadValueFromFile(file, fat ? osDWord : osByte);
								handler_size->set_comment(CommentInfo(ttComment, "HandlerLength"));
								address = handler_size->next_address();
								uint64_t handler_end = handler_begin + value;

								range_list()->Add(handler_begin, handler_end, handler_entry, NULL, handler_size);

								if (flags & COR_ILEXCEPTION_CLAUSE_FILTER) {
									command = Add(address);
									value = command->ReadValueFromFile(file, osDWord);
									command->set_comment(CommentInfo(ttComment, "FilterOffset"));
									address = command->next_address();
									link = command->AddLink(0, ltSEHBlock, start_address + value);
									link->set_base_function_info(info);
								} else {
									command = Add(address);
									command->ReadValueFromFile(file, osDWord, true);
									command->set_comment(CommentInfo(ttComment, "ClassToken"));
									address = command->next_address();
								}
							}
						}
						function_info_list()->Add(data_address, data_address + data_size, btValue, 0, 0, 0, NULL, NULL);

						for (size_t i = old_count; i < count(); i++) {
							command = item(i);
							//command->exclude_option(roNeedCompile);
							command->exclude_option(roClearOriginalCode);
						}
					}
				}
				break;
			}

			return item(index);
		}

		if (address < runtime_func->start())
			address = runtime_func->start();

		if (!file.AddressSeek(address))
			return NULL;
	}

	command = Add(address);
	command->ReadFromFile(file);

	switch (command->type()) {
	case icBrfalse_s: case icBrtrue_s: case icBeq_s: case icBge_s: case icBgt_s:
	case icBle_s: case icBlt_s: case icBne_un_s: case icBge_un_s: case icBgt_un_s: case icBle_un_s:
	case icBlt_un_s: case icBrfalse: case icBrtrue: case icBeq: case icBge: case icBgt:
	case icBle:	case icBlt: case icBne_un: case icBge_un: case icBgt_un: case icBle_un:	case icBlt_un:
		command->AddLink(0, ltJmpWithFlag, command->operand_value());
		break;
	case icBr: case icBr_s: case icJmp: case icLeave: case icLeave_s:
		command->AddLink(0, ltJmp, command->operand_value());
		break;
	case icSwitch: 
		{
			address = command->next_address();
			uint32_t case_count = static_cast<uint32_t>(command->operand_value());
			uint64_t add_value = address + case_count * sizeof(uint32_t);
			ILCommand *case_command = NULL;
			for (size_t i = 0; i < case_count; i++) {
				case_command = Add(address);
				case_command->ReadCaseCommand(file);
				case_command->set_comment(CommentInfo(ttComment, "Case"));
				CommandLink *link = case_command->AddLink(0, ltCase, case_command->operand_value() + add_value);
				link->set_parent_command(command);

				address = case_command->next_address();
			}
			if (case_command)
				command = case_command;
		}
		break;
	case icCall: case icCallvirt: case icLdftn: case icLdvirtftn: case icNewobj:
		{
			ILToken *token = reinterpret_cast<NETArchitecture&>(file).command_list()->token(static_cast<uint32_t>(command->operand_value()));
			uint64_t to_address = 0;
			LinkType link_type = ltNone;
			if (token && token->type() == ttMethodDef) {
				ILMethodDef *method = reinterpret_cast<ILMethodDef*>(token);
				if ((method->impl_flags() & miCodeTypeMask) == miIL && method->address()) {
					to_address = method->address();
					if (command->type() == icCall || (command->type() == icCallvirt && (method->flags() & mdVirtual) == 0))
						link_type = ltCall;
				}
			}
			
			if (to_address)
				command->AddLink(-1, link_type, to_address);
		}
		break;
	}

	return command;
}

ILCommand *ILFunction::AddCommand(const std::string &value)
{
	ILCommand *command = new ILCommand(this, cpu_address_size(), value);
	AddObject(command);
	return command;
}

ILCommand *ILFunction::AddCommand(const Data &value)
{
	ILCommand *command = new ILCommand(this, cpu_address_size(), value);
	AddObject(command);
	return command;
}

ILCommand *ILFunction::AddCommand(OperandSize value_size, uint64_t value)
{
	ILCommandType command_type;
	switch (value_size) {
	case osWord:
		command_type = icWord;
		break;
	case osDWord:
		command_type = icDword;
		break;
	default:
		return NULL;
	}

	return AddCommand(command_type, value);
}

ILCommand *ILFunction::CreateCommand()
{
	return new ILCommand(this, cpu_address_size());
}

ILCommand *ILFunction::AddCommand(ILCommandType type, uint64_t operand_value, IFixup *fixup)
{
	ILCommand *command = new ILCommand(this, cpu_address_size(), type, operand_value, fixup);
	AddObject(command);
	return command;
}

ILFunction *ILFunction::Clone(IFunctionList *owner) const
{
	ILFunction *func = new ILFunction(owner, *this);
	return func;
}

ILCommand *ILFunction::GetCommandByAddress(uint64_t address) const
{
	return reinterpret_cast<ILCommand *>(BaseFunction::GetCommandByAddress(address));
}

ILCommand *ILFunction::GetCommandByNearAddress(uint64_t address) const
{
	return reinterpret_cast<ILCommand *>(BaseFunction::GetCommandByNearAddress(address));
}

ILCommand *ILFunction::ParseString(IArchitecture &file, uint64_t address, size_t len)
{
	if (!file.AddressSeek(address))
		return NULL;

	ILCommand *command = Add(address);
	command->ReadString(file, len);
	command->exclude_option(roNeedCompile);
	command->exclude_option(roClearOriginalCode);
	return command;
}

ILCommand *ILFunction::Add(uint64_t address)
{
	ILCommand *command = new ILCommand(this, cpu_address_size(), address);
	AddObject(command);
	return command;
}

/**
 * ILFunctionList
 */

ILFunctionList::ILFunctionList(IArchitecture *owner) 
	: BaseFunctionList(owner), crc_table_(NULL), runtime_crc_table_(NULL), import_(NULL)
{
	crc_cryptor_ = new ValueCryptor();
}

ILFunctionList::ILFunctionList(IArchitecture *owner, const ILFunctionList &src) 
	: BaseFunctionList(owner, src), crc_table_(NULL), runtime_crc_table_(NULL), import_(NULL)
{
	crc_cryptor_ = new ValueCryptor();
}

ILFunctionList::~ILFunctionList()
{
	delete crc_cryptor_;
}

ILFunction *ILFunctionList::item(size_t index) const
{ 
	return reinterpret_cast<ILFunction *>(BaseFunctionList::item(index));
}

ILFunction *ILFunctionList::GetFunctionByAddress(uint64_t address) const
{ 
	return reinterpret_cast<ILFunction *>(BaseFunctionList::GetFunctionByAddress(address));
}

bool ILFunctionList::Prepare(const CompileContext &ctx)
{
	OperandSize cpu_address_size = ctx.file->cpu_address_size();

	crc_cryptor_->clear();
	crc_cryptor_->set_size(osDWord);
	crc_cryptor_->Add(ccXor, rand32());

	if ((ctx.options.flags | ctx.options.sdk_flags) & cpMemoryProtection) {
		crc_table_ = AddCRCTable(cpu_address_size);
	} else {
		crc_table_ = NULL;
	}

	if (ctx.runtime) {
		ILFunctionList *function_list = reinterpret_cast<ILFunctionList *>(ctx.runtime->function_list());
		ILFunction *func;
		size_t i, j;

		// remove DecryptString template
		uint64_t address = ctx.runtime->export_list()->GetAddressByType(atDecryptStringA);
		if (address) {
			func = function_list->GetFunctionByAddress(address);
			if (func)
				func->set_need_compile(false);
		}

		NETArchitecture *runtime = reinterpret_cast<NETArchitecture *>(ctx.runtime);
		NETRuntimeFunctionList *runtime_function_list = runtime->runtime_function_list();

		for (i = 0; i < function_list->count(); i++) {
			func = function_list->item(i);
			if (func->compilation_type() != ctMutation && func->entry_type() == etNone) {
				NETRuntimeFunction *runtime_func = runtime_function_list->GetFunctionByAddress(func->address());
				if (runtime_func) {
					std::vector<ILMethodDef *> method_list = runtime_func->method_list();
					for (j = 0; j < method_list.size(); j++) {
						ILMethodDef *method = method_list[j];
						if (method->flags() & mdSpecialName)
							return false;
						method->set_deleted(true);
					}
				}
			}
		}

		APIType crc_helpers[] = { atRandom, atCalcCRC };
		if (ctx.runtime->segment_list()->count() > 0) {
			// add runtime functions
			for (i = 0; i < function_list->count(); i++) {
				func = function_list->item(i);

				if (func->tag() == ftProcessor)
					continue;

				if (func->need_compile()) {
					func = func->Clone(this);
					AddObject(func);

					if (func->compilation_type() != ctMutation && func->entry_type() == etNone)
						func->entry()->include_section_option(rtLinkedToInt);

					for (j = 0; j < func->count(); j++) {
						ILCommand *command = func->item(j);
						if (command->type() == icLdstr) {
							ILData dump = runtime->command_list()->GetUserData(TOKEN_VALUE(command->operand_value()), &address);

							for (size_t k = 0; k < MESSAGE_COUNT; k++) {
								os::unicode_string unicode_message =
#ifdef VMP_GNU
										os::FromUTF8(default_message[k]);
#else
										default_message[k];
#endif

								if (dump.size() == unicode_message.size() * sizeof(os::unicode_char) && memcmp(dump.data(), unicode_message.c_str(), dump.size()) == 0) {
									MapFunction *map_function = runtime->map_function_list()->Add(address, address + dump.size(), otString, FunctionName());
									map_function->reference_list()->Add(command->address(), address);
									ILFunction *str_function = reinterpret_cast<ILFunction *>(function_list->AddByAddress(address, ctVirtualization, 0, true, NULL));
									if (str_function) {
										str_function->set_from_runtime(true);
										os::unicode_string str = os::FromUTF8(ctx.options.messages[k]);
										if (str.empty())
											str.push_back(0);
										str_function->item(0)->set_dump(reinterpret_cast<const uint8_t*>(str.c_str()), str.size() * sizeof(os::unicode_char));
									}
									break;
								}
							}
						}
					}

					for (j = 0; j < func->count(); j++) {
						func->item(j)->CompileToNative();
					}
				} else {
					if (func->tag() != ftLoader) {
						NETRuntimeFunction *runtime_func = runtime_function_list->GetFunctionByAddress(func->address());
						if (runtime_func) {
							std::vector<ILMethodDef *> method_list = runtime_func->method_list();
							for (j = 0; j < method_list.size(); j++) {
								method_list[j]->set_deleted(true);
							}
						}
					}

					if (!func->FreeByManager(ctx))
						return false;
				}
			}

			for (i = 0; i < _countof(crc_helpers); i++) {
				address = ctx.runtime->export_list()->GetAddressByType(crc_helpers[i]);
				func = reinterpret_cast<ILFunction *>(ctx.file->function_list()->GetFunctionByAddress(address));
				if (!func)
					return false;
				// exclude from memory protection
				func->entry()->include_section_option(rtLinkedToInt);
				func->set_tag(ftLoader);
			}

			AddRuntimeData(cpu_address_size);
		}
		else {
			// remove references to VMProtect.Core
			address = ctx.runtime->export_list()->GetAddressByType(atSetupImage);
			func = function_list->GetFunctionByAddress(address);
			if (!func)
				return false;

			for (i = 0; i < func->count(); i++) {
				ILCommand *command = func->item(i);
				if (command->token_reference()) {
					ILToken *token = command->token_reference()->owner()->owner();
					switch (token->type()) {
					case ttField:
						if (reinterpret_cast<ILField *>(token)->declaring_type()->full_name() == "VMProtect.Core")
							command->Init(icNop);
						break;
					case ttMethodDef:
						if (reinterpret_cast<ILMethodDef *>(token)->declaring_type()->full_name() == "VMProtect.Core")
							command->Init(icNop);
						break;
					}
				}
			}

			if (ctx.options.flags & cpMemoryProtection) {
				// add CRC helpers
				for (i = 0; i < _countof(crc_helpers); i++) {
					address = ctx.runtime->export_list()->GetAddressByType(crc_helpers[i]);
					func = function_list->GetFunctionByAddress(address);
					if (!func)
						return false;

					func = func->Clone(this);
					AddObject(func);
					for (j = 0; j < func->count(); j++) {
						ILCommand *command = func->item(j);
						command->exclude_option(roClearOriginalCode);
						command->CompileToNative();
					}

					// exclude from memory protection
					func->entry()->include_section_option(rtLinkedToInt);
					func->set_tag(ftLoader);
				}
			}
		}
	}

	if (ctx.options.flags & cpImportProtection) {
		import_ = AddImport(cpu_address_size);
	} else {
		import_ = NULL;
	}

	AddSDK(cpu_address_size);

	AddWatermark(cpu_address_size, ctx.options.watermark, ctx.runtime ? 8 : 10);

	return BaseFunctionList::Prepare(ctx);
}

void ILFunctionList::CompileLinks(const CompileContext &ctx)
{
	if (ctx.options.flags & cpMemoryProtection) {
		runtime_crc_table_ = AddRuntimeCRCTable(ctx.file->cpu_address_size());
		runtime_crc_table_->Compile(ctx);
	}
	else {
		runtime_crc_table_ = NULL;
	}

	return BaseFunctionList::CompileLinks(ctx);
}

void ILFunctionList::CompileInfo(const CompileContext &ctx)
{
	return BaseFunctionList::CompileInfo(ctx);
}

void ILFunctionList::ReadFromBuffer(Buffer &buffer, IArchitecture &file)
{
	BaseFunctionList::ReadFromBuffer(buffer, file);

	// add loader stubs
	size_t i, j;
	std::set<ILTypeDef*> class_list;
	for (i = 0; i < count(); i++) {
		ILFunction *func = item(i);
		if (func->tag() != ftLoader)
			continue;

		for (j = 0; j < func->count(); j++) {
			ILCommand *command = func->item(j);
			if (command->token_reference()) {
				ILToken *token = command->token_reference()->owner()->owner();
				if (token->type() == ttMethodDef)
					class_list.insert(reinterpret_cast<ILMethodDef*>(token)->declaring_type());
			}
		}
	}

	for (std::set<ILTypeDef*>::const_iterator it = class_list.begin(); it != class_list.end(); ) {
		ILTypeDef *type_def = *it;
		if (type_def->full_name() == "VMProtect.Core") {
			class_list.erase(it++);
			continue;
		}
		it++;
	}

	ILTable *method_table = reinterpret_cast<NETArchitecture&>(file).command_list()->table(ttMethodDef);
	for (i = 0; i < method_table->count(); i++) {
		ILMethodDef *method = reinterpret_cast<ILMethodDef*>(method_table->item(i));
		uint64_t address = method->address();
		if (!address || class_list.find(method->declaring_type()) == class_list.end() || GetFunctionByAddress(address))
			continue;

		ILFunction *new_func = reinterpret_cast<ILFunction *>(AddByAddress(address, ctMutation, 0, false, NULL));
		if (new_func) {
			new_func->set_tag(ftLoader);
			for (j = 0; j < new_func->count(); j++) {
				new_func->item(j)->exclude_option(roClearOriginalCode);
			}
		}
	}
}

ILFunction *ILFunctionList::Add(const std::string &name, CompilationType compilation_type, uint32_t compilation_options, bool need_compile, Folder *folder)
{
	ILFunction *func = new ILFunction(this, name, compilation_type, compilation_options, need_compile, folder);
	AddObject(func);
	return func;
}

ILFunctionList * ILFunctionList::Clone(IArchitecture *owner) const
{
	ILFunctionList *list = new ILFunctionList(owner, *this);
	return list;
}

IFunction * ILFunctionList::CreateFunction(OperandSize cpu_address_size)
{
	return new ILFunction(this, cpu_address_size);
}

bool ILFunctionList::Compile(const CompileContext &ctx)
{
	return BaseFunctionList::Compile(ctx);
}

ILSDK *ILFunctionList::AddSDK(OperandSize cpu_address_size)
{
	ILSDK *func = new ILSDK(this, cpu_address_size);
	AddObject(func);
	return func;
}

ILRuntimeData *ILFunctionList::AddRuntimeData(OperandSize cpu_address_size)
{
	ILRuntimeData *func = new ILRuntimeData(this, cpu_address_size);
	AddObject(func);
	return func;
}

ILCRCTable *ILFunctionList::AddCRCTable(OperandSize cpu_address_size)
{
	ILCRCTable *func = new ILCRCTable(this, cpu_address_size);
	AddObject(func);
	return func;
}

ILImport *ILFunctionList::AddImport(OperandSize cpu_address_size)
{
	ILImport *func = new ILImport(this, cpu_address_size);
	AddObject(func);
	return func;
}

ILFunction *ILFunctionList::AddWatermark(OperandSize cpu_address_size, Watermark *watermark, int copy_count)
{
	ILFunction *func = new ILFunction(this, cpu_address_size);
	func->set_compilation_type(ctMutation);
	func->set_memory_type(mtNone);
	func->AddWatermark(watermark, copy_count);
	AddObject(func);
	return func;
}

ILRuntimeCRCTable *ILFunctionList::AddRuntimeCRCTable(OperandSize cpu_address_size)
{
	ILRuntimeCRCTable *func = new ILRuntimeCRCTable(this, cpu_address_size);
	AddObject(func);
	return func;
}

ILVirtualMachineProcessor *ILFunctionList::AddProcessor(OperandSize cpu_address_size)
{
	ILVirtualMachineProcessor *func = new ILVirtualMachineProcessor(this, cpu_address_size);
	AddObject(func);
	return func;
}

/**
 * ILSDK
 */

ILSDK::ILSDK(IFunctionList *owner, OperandSize cpu_address_size)
	: ILFunction(owner, cpu_address_size)
{
	set_compilation_type(ctMutation);
}

bool ILSDK::Init(const CompileContext &ctx)
{
	MapFunctionList *map_function_list;
	MapFunction *map_function;
	IFunctionList *function_list;
	size_t i, c, j, n, f;
	uint64_t address;
	IArchitecture *file;
	IImportList *import_list;
	IImport *import;
	IImportFunction *import_function;
	ILCommand *command;
	CommandBlock *block;
	uint64_t api_address;

	f = (ctx.runtime && ctx.runtime->segment_list()->count() > 0) ? 2 : 1;
	for (n = 0; n < f; n++) {
		file = (n == 0) ? ctx.file : ctx.runtime;
		map_function_list = file->map_function_list();
		function_list = file->function_list();
		import_list = file->import_list();
		for (i = 0; i < import_list->count(); i++) {
			import = import_list->item(i);
			if (!import->is_sdk())
				continue;
			
			for (j = 0; j < import->count(); j++) {
				import_function = import->item(j);
				if (import_function->type() == atNone)
					continue;

				map_function = import_function->map_function();
				for (c = 0; c < map_function->reference_list()->count(); c++) {
					address = map_function->reference_list()->item(c)->address();

					command = reinterpret_cast<ILCommand *>(ctx.file->function_list()->GetCommandByNearAddress(address, true));
					if (command) {
						delete command->link();
					} else {
						if (!file->AddressSeek(address))
							return false;

						block = AddBlock(count(), true);
						block->set_address(address);

						command = Add(address);
						command->ReadFromFile(*file);
						command->set_block(block);
						command->include_option(roFillNop);
						command->exclude_option(roClearOriginalCode);
					}

					if (command->token_reference()) {
						command->token_reference()->set_deleted(true);
						command->set_token_reference(NULL);
					}

					switch (import_function->type()) {
						case atBegin:
							command->Init(icPop);
							break;

						case atEnd:
							command->Init(icNop);
							break;

						case atDecryptStringA:
						case atDecryptStringW:
							command->Init(icNop);
							break;

						case atFreeString:
							Notify(mtWarning, command, "VMProtect.SDK.FreeString is deprecated");
							// fall through
						case atIsDebuggerPresent:
						case atIsVirtualMachinePresent:
						case atIsValidImageCRC:
						case atActivateLicense:
						case atDeactivateLicense:
						case atGetOfflineActivationString:
						case atGetOfflineDeactivationString:
						case atSetSerialNumber:
						case atGetSerialNumberState:
						case atGetSerialNumberData:
						case atGetCurrentHWID:
						case atIsProtected:
							if (!ctx.runtime || ctx.runtime->segment_list()->count() == 0) {
								switch (import_function->type()) {
								case atFreeString:
									command->Init(icNop);
									break;
								case atIsProtected:
									command->Init(icLdc_i4_1);
									break;
								default:
									// other APIs can not work without runtime
									return false;
								}
							} else {
								api_address = ctx.runtime->export_list()->GetAddressByType(import_function->type());
								if (!api_address)
									return false;

								ILMethodDef *method = reinterpret_cast<ILMetaData *>(ctx.runtime->command_list())->GetMethod(api_address);
								if (!method)
									return false;

								command->set_operand_value(0, method->id());
								command->set_token_reference(method->reference_list()->Add(command->address() + 1));
							}
							break;

						default:
							throw std::runtime_error("Unknown API from SDK: " + import_function->name());
					}

					command->CompileToNative();
				}
			}
		}
	}

	for (i = 0; i < count(); i++) {
		item(i)->CompileToNative();
	}

	return true;
}

/**
 * ILCRCTable
 */

ILCRCTable::ILCRCTable(IFunctionList *owner, OperandSize cpu_address_size)
	: ILFunction(owner, cpu_address_size)
{
	set_compilation_type(ctMutation);
}

bool ILCRCTable::Init(const CompileContext &ctx)
{
	size_t i, c, n, f;

	c = 10;
	f = (ctx.runtime && ctx.runtime->segment_list()->count() > 0) ? 2 : 1;
	for (n = 0; n < f; n++) {
		IArchitecture *file = (n == 0) ? ctx.file : ctx.runtime;
		c += ctx.file->segment_list()->count();
	}

	for (i = 0; i < c; i++) {
		AddCommand(icDword, 0);
		AddCommand(icDword, 0);
		AddCommand(icDword, 0);
	}

	size_entry_ = AddCommand(icDword, 0);
	size_entry_->include_option(roCreateNewBlock);

	hash_entry_ = AddCommand(icDword, 0);
	hash_entry_->include_option(roCreateNewBlock);

	for (i = 0; i < count(); i++) {
		ILCommand *command = item(i);
		command->CompileToNative();
		command->include_option(roWritable);
	}

	return true;
}

/**
 * ILRuntimeData
 */

ILRuntimeData::ILRuntimeData(IFunctionList *owner, OperandSize cpu_address_size)
	: ILFunction(owner, cpu_address_size), strings_entry_(NULL), strings_size_(0), trial_hwid_entry_(NULL), trial_hwid_size_(0),
	resources_entry_(NULL), resources_size_(0)
#ifdef ULTIMATE
	, license_data_entry_(NULL), license_data_size_(0)
#endif
{
	set_compilation_type(ctMutation);
	rc5_key_.Create();
}

bool ILRuntimeData::CommandCompareHelper::operator()(const ILCommand *left, ILCommand *right) const
{
	return (left->address() < right->address());
}

bool ILRuntimeData::Init(const CompileContext &ctx)
{
	ILFunctionList *function_list;
	size_t i, j, index;
	std::vector<ILCommand *> string_command_list;
	ILCommand *command, *string_command;
	CommandLink *link;
	ILCommand *key_entry;
	Data key;
	uint32_t data_key;
	uint64_t image_base = ctx.file->image_base();

	key.PushBuff(rc5_key_.Value, sizeof(rc5_key_.Value));
	data_key = key.ReadDWord(0);

	resources_entry_ = NULL;
	resources_size_ = 0;
#ifdef ULTIMATE
	if (ctx.options.file_manager) {
		FileManager *file_manager = ctx.options.file_manager;
		if (!file_manager->OpenFiles())
			return false;

		InternalFile *internal_file;
		index = count();
		// write entry
		for (i = 0; i < file_manager->count(); i++) {
			internal_file = file_manager->item(i);
			AddCommand(osDWord, 0);
			AddCommand(osDWord, 0);
			AddCommand(osDWord, internal_file->stream()->Size());
			AddCommand(osDWord, 0);
		}

		// end entry
		AddCommand(osDWord, 0);
		AddCommand(osDWord, 0);
		AddCommand(osDWord, 0);
		AddCommand(osDWord, 0);

		resources_entry_ = item(index);
		resources_entry_->include_option(roCreateNewBlock);
		resources_size_ = static_cast<uint32_t>((count() - index) * OperandSizeToValue(osDWord));

		PEFile pe_file;
		for (i = 0; i < file_manager->count(); i++) {
			internal_file = file_manager->item(i);
			std::string file_name = internal_file->absolute_file_name();
			std::string assembly_name;
			try
			{
				if (pe_file.Open(file_name.c_str(), foRead | foHeaderOnly) == osSuccess && pe_file.count() > 1)
					assembly_name = reinterpret_cast<NETArchitecture *>(pe_file.item(1))->full_name();
			}
			catch (...)
			{

			}
			pe_file.Close();
			if (assembly_name.empty()) {
				Notify(mtError, internal_file, string_format(language[lsFileHasIncorrectFormat].c_str(), file_name.c_str(), ".NET"));
				file_manager->CloseFiles();
				return false;
			}

			// write name
			Data data;
			os::unicode_string name = os::FromUTF8(assembly_name.c_str());
			const os::unicode_char *p = name.c_str();
			for (j = 0; j < name.size() + 1; j++) {
				data.PushWord(static_cast<uint16_t>(p[j] ^ (_rotl32(data_key, static_cast<int>(j)) + j)));
			}
			command = AddCommand(data);
			command->include_option(roCreateNewBlock);
			link = item(index + i * 4)->AddLink(0, ltOffset, command);
			link->set_sub_value(image_base);

			// write data
			Notify(mtInformation, NULL, string_format("%s %s", language[lsLoading].c_str(), os::ExtractFileName(file_name.c_str()).c_str()));
			data.resize(static_cast<size_t>(internal_file->stream()->Size()));
			internal_file->stream()->Read(&data[0], data.size());
			for (j = 0; j < data.size(); j++) {
				data[j] = (data[j] ^ static_cast<uint8_t>(_rotl32(data_key, static_cast<int>(j)) + j));
			}
			command = AddCommand(data);
			command->include_option(roCreateNewBlock);
			link = item(index + i * 4 + 1)->AddLink(0, ltOffset, command);
			link->set_sub_value(image_base);
		}

		file_manager->CloseFiles();
	}
#endif

	if ((ctx.options.flags & cpResourceProtection) && ctx.file->resource_list()->count()) {
		std::string assembly_name;
		NETArchitecture *file = reinterpret_cast<NETArchitecture *>(ctx.file);
		while (true) {
			assembly_name = string_format("%.8X", rand32());
			if (!file->command_list()->GetAssemblyRef(assembly_name))
				break;
		}
		Data data;
		if (file->WriteResources(assembly_name, data)) {
			ILAssemblyRef *assembly_ref = file->command_list()->GetAssemblyRef(assembly_name);
			if (!assembly_ref)
				return false;

			if (resources_entry_) {
				index = IndexOf(resources_entry_) + (resources_size_ - 4 * sizeof(uint32_t)) / sizeof(uint32_t);

				// insert entry
				InsertObject(index + 0, new ILCommand(this, cpu_address_size(), icDword, 0));
				InsertObject(index + 1, new ILCommand(this, cpu_address_size(), icDword, 0));
				InsertObject(index + 2, new ILCommand(this, cpu_address_size(), icDword, data.size()));
				InsertObject(index + 3, new ILCommand(this, cpu_address_size(), icDword, 0));

				resources_size_ += 4 * sizeof(uint32_t);
			}
			else {
				// write entry
				index = count();
				AddCommand(osDWord, 0);
				AddCommand(osDWord, 0);
				AddCommand(osDWord, data.size());
				AddCommand(osDWord, 0); // action

				// end entry
				AddCommand(osDWord, 0);
				AddCommand(osDWord, 0);
				AddCommand(osDWord, 0);
				AddCommand(osDWord, 0);

				resources_entry_ = item(index);
				resources_entry_->include_option(roCreateNewBlock);
				resources_size_ = static_cast<uint32_t>((count() - index) * OperandSizeToValue(osDWord));
			}

			// write name
			Data str;
			os::unicode_string name = os::FromUTF8(assembly_ref->full_name().c_str());
			const os::unicode_char *p = name.c_str();
			for (i = 0; i < name.size() + 1; i++) {
				str.PushWord(static_cast<uint16_t>(p[i] ^ (_rotl32(data_key, static_cast<int>(i)) + i)));
			}
			command = AddCommand(str);
			command->include_option(roCreateNewBlock);
			link = item(index)->AddLink(0, ltOffset, command);
			link->set_sub_value(image_base);

			// write data
			for (i = 0; i < data.size(); i++) {
				data[i] = data[i] ^ static_cast<uint8_t>(_rotl32(data_key, static_cast<int>(i)) + i);
			}
			command = AddCommand(data);
			command->include_option(roCreateNewBlock);
			link = item(index + 1)->AddLink(0, ltOffset, command);
			link->set_sub_value(image_base);
		}
	}

	function_list = reinterpret_cast<ILFunctionList *>(ctx.file->function_list());
	for (i = 0; i < function_list->count(); i++) {
		ILFunction *func = function_list->item(i);
		if (func->need_compile() && func->type() == otString) {
			for (j = 0; j < func->count(); j++) {
				string_command_list.push_back(func->item(j));
			}
		}
	}

	key_entry = AddCommand(key);
	key_entry->include_option(roCreateNewBlock);

	strings_entry_ = NULL;
	strings_size_ = 0;
	if (string_command_list.size()) {
		std::sort(string_command_list.begin(), string_command_list.end(), CommandCompareHelper());
		index = count();

		// create directory
		AddCommand(osDWord, string_command_list.size());
		AddCommand(osDWord, 0);

		for (i = 0; i < string_command_list.size(); i++) {
			string_command = string_command_list[i];

			// create string entry
			AddCommand(osDWord, string_command->address() - ctx.file->image_base());
			command = AddCommand(osDWord, 0);
			link = command->AddLink(0, ltOffset);
			link->set_sub_value(ctx.file->image_base());
			AddCommand(osDWord, string_command->dump_size());
			AddCommand(osDWord, 0);
		}
		strings_entry_ = item(index);
		strings_entry_->include_option(roCreateNewBlock);
		strings_size_ = static_cast<uint32_t>((count() - index) * OperandSizeToValue(osDWord));
		i = AlignValue(strings_size_, 8);
		if (i > strings_size_) {
			Data tmp;
			tmp.resize(i - strings_size_, 0);
			AddCommand(tmp);
			strings_size_ = (uint32_t)i;
		}

		// create string values
		Data data;
		for (i = 0; i < string_command_list.size(); i++) {
			string_command = string_command_list[i];

			data.clear();
			for (j = 0; j < string_command->dump_size(); j++) {
				data.PushByte(string_command->dump(j) ^ static_cast<uint8_t>(_rotl32(data_key, static_cast<int>(j)) + j));
			}

			command = AddCommand(data);
			command->include_option(roCreateNewBlock);

			item(index + 2 + i * 4 + 1)->link()->set_to_command(command);
		}
	}

#ifdef ULTIMATE
	license_data_entry_ = NULL;
	license_data_size_ = 0;
	if (ctx.options.licensing_manager) {
		Data license_data;
		if (ctx.options.licensing_manager->GetLicenseData(license_data)) {
			license_data_entry_ = AddCommand(license_data);
			license_data_entry_->include_option(roCreateNewBlock);
			license_data_size_ = static_cast<uint32_t>(license_data.size());
		}
	}
#endif

	trial_hwid_entry_ = NULL;
	trial_hwid_size_ = 0;
#ifdef DEMO
	if (true)
#else
	if (ctx.options.flags & cpUnregisteredVersion)
#endif
	{
		size_t size = VMProtectGetCurrentHWID(NULL, 0);
		std::vector<char> hwid;
		hwid.resize(size);
		VMProtectGetCurrentHWID(hwid.data(), (int)hwid.size());

		std::vector<uint8_t> binary;
		binary.resize(size);
		Base64Decode(hwid.data(), hwid.size(), binary.data(), size);

		Data data;
		data.PushBuff(binary.data(), binary.size());
		data.resize(64);
		
		trial_hwid_size_ = static_cast<uint32_t>(std::min(size, data.size()));
		trial_hwid_entry_ = AddCommand(data);
		trial_hwid_entry_->include_option(roCreateNewBlock);
	}
#ifdef ULTIMATE
	else if (!ctx.options.hwid.empty()) {
		std::string hwid = ctx.options.hwid;
		size_t size = hwid.size();

		std::vector<uint8_t> binary;
		binary.resize(size);
		Base64Decode(hwid.data(), hwid.size(), binary.data(), size);
		if (size & 3) {
			Notify(mtError, NULL, "Invalid HWID");
			return false;
		}

		Data data;
		data.PushBuff(binary.data(), binary.size());
		data.resize(64);

		trial_hwid_size_ = static_cast<uint32_t>(std::min(size, data.size()));
		trial_hwid_entry_ = AddCommand(data);
		trial_hwid_entry_->include_option(roCreateNewBlock);
	}
#endif

	for (i = 0; i < count(); i++) {
		item(i)->CompileToNative();
	}

	// setup faces for common runtime functions
	ILCRCTable *il_crc = reinterpret_cast<ILFunctionList *>(ctx.file->function_list())->crc_table();
	for (i = 0; i < function_list->count(); i++) {
		ILFunction *func = function_list->item(i);
		if (!func->from_runtime())
			continue;

		for (j = 0; j < func->count(); j++) {
			ILCommand *command = func->item(j);
			if (command->type() != icLdc_i4)
				continue;

			uint32_t value = static_cast<uint32_t>(command->operand_value());
			if ((value & 0xFFFF0000) == 0xFACE0000) {
				switch (value) {
				case FACE_STRING_DECRYPT_KEY:
					command->set_operand_value(0, data_key);
					command->CompileToNative();
					break;
				case FACE_RC5_P:
					command->set_operand_value(0, rc5_key_.P);
					command->CompileToNative();
					break;
				case FACE_RC5_Q:
					command->set_operand_value(0, rc5_key_.Q);
					command->CompileToNative();
					break;
				case FACE_KEY_INFO:
					if (key_entry) {
						link = command->AddLink(0, ltOffset, key_entry);
						link->set_sub_value(image_base);
					} else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
				case FACE_STRING_INFO:
					if (strings_entry_) {
						link = command->AddLink(0, ltOffset, strings_entry_);
						link->set_sub_value(image_base);
					} else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
				case FACE_RESOURCE_INFO:
					if (resources_entry_) {
						link = command->AddLink(0, ltOffset, resources_entry_);
						link->set_sub_value(image_base);
					}
					else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
#ifdef ULTIMATE
				case FACE_LICENSE_INFO:
					if (license_data_entry_) {
						link = command->AddLink(0, ltOffset, license_data_entry_);
						link->set_sub_value(image_base);
					} else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
				case FACE_LICENSE_INFO_SIZE:
					command->set_operand_value(0, license_data_size_);
					command->CompileToNative();
					break;
#else
				case FACE_LICENSE_INFO:
				case FACE_LICENSE_INFO_SIZE:
					command->set_operand_value(0, 0);
					command->CompileToNative();
					break;
#endif
				case FACE_TRIAL_HWID:
					if (trial_hwid_entry_) {
						link = command->AddLink(0, ltOffset, trial_hwid_entry_);
						link->set_sub_value(image_base);
					} else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
				case FACE_TRIAL_HWID_SIZE:
					command->set_operand_value(0, trial_hwid_size_);
					command->CompileToNative();
					break;
				case FACE_CRC_TABLE_ENTRY:
					if (il_crc) {
						link = command->AddLink(0, ltOffset, il_crc->table_entry());
						link->set_sub_value(image_base);
					} else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
				case FACE_CRC_TABLE_SIZE:
					if (il_crc) {
						link = command->AddLink(0, ltOffset, il_crc->size_entry());
						link->set_sub_value(image_base);
					} else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
				case FACE_CRC_TABLE_HASH:
					if (il_crc) {
						link = command->AddLink(0, ltOffset, il_crc->hash_entry());
						link->set_sub_value(image_base);
					} else {
						command->set_operand_value(0, 0);
						command->CompileToNative();
					}
					break;
				case FACE_CORE_OPTIONS:
					{
						uint32_t options = 0;
						if (ctx.options.flags & cpInternalMemoryProtection)
							options |= CORE_OPTION_MEMORY_PROTECTION;
						if (ctx.options.flags & cpCheckDebugger)
							options |= CORE_OPTION_CHECK_DEBUGGER;
						command->set_operand_value(0, options);
					}
					command->CompileToNative();
					break;
				}
			}
		}
	}

	return true;
}

size_t ILRuntimeData::WriteToFile(IArchitecture &file)
{
	size_t res = ILFunction::WriteToFile(file);

	CipherRC5 cipher(rc5_key_);
	for (size_t i = 0; i < 3; i++) {
		ILCommand *command;
		size_t size;

		switch (i) {
		case 0:
			command = strings_entry_;
			size = strings_size_;
			break;
		case 1:
			command = trial_hwid_entry_;
			size = trial_hwid_entry_ ? trial_hwid_entry_->dump_size() : 0;
			break;
		case 2:
			command = resources_entry_;
			size = resources_size_;
			break;

#ifdef ULTIMATE
			/*
		case 2:
			command = license_data_entry_;
			size = license_data_size_;
			break;
			*/
#endif
		default:
			command = NULL;
			size = 0;
		}

		if (size) {
			std::vector<uint8_t> buff;
			buff.resize(size);
			file.AddressSeek(command->address());
			uint64_t pos = file.Tell();
			file.Read(buff.data(), buff.size());
			cipher.Encrypt(buff.data(), buff.size());
			file.Seek(pos);
			file.Write(buff.data(), buff.size());
		}
	}

	return res;
}

/**
* ILRuntimeCRCTable
*/

ILRuntimeCRCTable::ILRuntimeCRCTable(IFunctionList *owner, OperandSize cpu_address_size)
	: ILFunction(owner, cpu_address_size), cryptor_(NULL)
{
	set_compilation_type(ctMutation);
}

void ILRuntimeCRCTable::clear()
{
	region_info_list_.clear();
	ILFunction::clear();
}

bool ILRuntimeCRCTable::Compile(const CompileContext &ctx)
{
	ILFunctionList *function_list = reinterpret_cast<ILFunctionList *>(ctx.file->function_list());
	cryptor_ = function_list->crc_cryptor();

	size_t block_size, i, j, k;
	uint64_t block_address;
	MemoryManager manager(ctx.file);

	for (i = 0; i < function_list->count(); i++) {
		ILFunction *func = function_list->item(i);
		if (!func->need_compile())
			continue;

		for (j = 0; j < func->block_list()->count(); j++) {
			CommandBlock *block = func->block_list()->item(j);
			if (block->type() & mtExecutable) {
				// native block
				block_size = 0;
				block_address = 0;
				for (k = block->start_index(); k <= block->end_index(); k++) {
					ILCommand *command = func->item(k);
					if (command->options() & roWritable)
						continue;

					if (block_address && (block_address + block_size) != command->address()) {
						if (block_size)
							manager.Add(block_address, block_size, mtReadable);
						block_address = 0;
						block_size = 0;
					}
					if (!block_address)
						block_address = command->address();
					block_size += command->dump_size();
				}
				if (block_size)
					manager.Add(block_address, block_size, mtReadable);
			}
			else {
				// VM block
				block_size = 0;
				block_address = 0;
				for (k = block->start_index(); k <= block->end_index(); k++) {
					ILCommand *command = func->item(k);

					if (block_address && (block_address + block_size) != command->vm_address()) {
						if (block_size)
							manager.Add(block_address, block_size, mtReadable);
						block_address = 0;
						block_size = 0;
					}
					if (!block_address)
						block_address = command->vm_address();
					block_size += command->vm_dump_size();
				}
				if (block_size)
					manager.Add(block_address, block_size, mtReadable);
			}
		}
	}
	if (manager.count() == 0)
		return true;

	manager.Pack();

	for (i = 0; i < manager.count(); i++) {
		MemoryRegion *region = manager.item(i);
		uint64_t block_address = region->address();

		size_t region_size, block_size;
		for (region_size = region->size(); region_size != 0; region_size -= block_size, block_address += block_size) {
			block_size = 0x1000 - (rand() & 0xff);
			if (block_size > region_size)
				block_size = region_size;

			region_info_list_.push_back(RegionInfo(block_address, static_cast<uint32_t>(block_size), false));
		}
	}

	for (i = 0; i < region_info_list_.size(); i++) {
		std::swap(region_info_list_[i], region_info_list_[rand() % region_info_list_.size()]);
	}

	size_t self_crc_offset = 0;
	size_t self_crc_size = 0;
	for (i = 0; i < region_info_list_.size(); i++) {
		self_crc_size += sizeof(CRCInfo::POD);
		if (self_crc_size > 0x1000 && (rand() & 1)) {
			region_info_list_.insert(region_info_list_.begin() + i + 1, RegionInfo(self_crc_offset, (uint32_t)self_crc_size, true));
			self_crc_offset += self_crc_size;
			self_crc_size = 0;
		}
	}
	if (self_crc_size)
		region_info_list_.push_back(RegionInfo(self_crc_offset, (uint32_t)self_crc_size, true));

	for (i = 0; i < region_info_list_.size(); i++) {
		AddCommand(icDword, 0);
		AddCommand(icDword, 0);
		AddCommand(icDword, 0);
	}
	set_entry(item(0));
	for (i = 0; i < count(); i++) {
		item(i)->CompileToNative();
	}

	CreateBlocks();

	for (i = 0; i < block_list()->count(); i++) {
		block_list()->item(i)->Compile(*ctx.manager);
	}

	return true;
}

size_t ILRuntimeCRCTable::WriteToFile(IArchitecture &file)
{
	size_t res = ILFunction::WriteToFile(file);

	if (entry()) {
		uint64_t address = entry()->address();
		std::vector<CRCInfo> crc_info_list;
		std::vector<uint8_t> dump;
		for (size_t i = 0; i < region_info_list_.size(); i++) {
			RegionInfo region_info = region_info_list_[i];

			dump.resize(region_info.size);
			if (region_info.is_self_crc) {
				memcpy(&dump[0], reinterpret_cast<uint8_t *>(&crc_info_list[0]) + region_info.address, dump.size());
				region_info.address += address;
			}
			else {
				file.AddressSeek(region_info.address);
				file.Read(&dump[0], dump.size());
			}

			CRCInfo crc_info(static_cast<uint32_t>(region_info.address - file.image_base()), dump);
			if (cryptor_) {
				crc_info.pod.address = static_cast<uint32_t>(cryptor_->Encrypt(crc_info.pod.address));
				crc_info.pod.size = static_cast<uint32_t>(cryptor_->Encrypt(crc_info.pod.size));
			}
			crc_info.pod.hash = 0 - crc_info.pod.hash;
			crc_info_list.push_back(crc_info);
		}

		file.AddressSeek(address);
		file.Write(&crc_info_list[0], crc_info_list.size() * sizeof(CRCInfo::POD));
	}

	return res;
}

/**
 * ILFileHelper
 */

ILFileHelper::ILFileHelper() 
	: IObject(), marker_index_(0), marker_name_list_(NULL)
{
}

ILFileHelper::~ILFileHelper()
{

}

void ILFileHelper::Parse(NETArchitecture &file)
{
	size_t i, j;
	ILToken *token;
	ILFunction func(NULL, file.cpu_address_size());
	std::set<uint64_t> address_list;

	size_t c = file.map_function_list()->count();
	file.StartProgress(string_format("%s %s...", language[lsLoading].c_str(), os::ExtractFileName(file.owner()->file_name().c_str()).c_str()), c);
	for (i = 0; i < c; i++) {
		MapFunction *map_function = file.map_function_list()->item(i);
		file.StepProgress();

		if (map_function->type() != otCode && map_function->type() != otMarker && map_function->type() != otExport)
			continue;

		if (address_list.find(map_function->address()) != address_list.end())
			continue;

		address_list.insert(map_function->address());
		func.ReadFromFile(file, map_function->address());
		for (j = 0; j < func.count(); j++) {
			ILCommand *command = func.item(j);
			uint32_t value = static_cast<uint32_t>(command->operand_value());
			if (command->type() == icComment) {
				if (command->comment().value.substr(0, 7) == ".locals") {
					token = file.command_list()->token(value);
					if (token)
						token->reference_list()->Add(command->address());
				}
			} else if (command->type() == icDword) {
				if (command->comment().value == "ClassToken") {
					token = file.command_list()->token(value);
					if (token)
						token->reference_list()->Add(command->address());
				}
			} else switch (ILOpCodes[command->type()].operand_type) {
			case InlineString:
				token = file.command_list()->token(value);
				if (!token)
					token = file.command_list()->us_table()->Add(value);
				command->set_token_reference(token->reference_list()->Add(command->address() + command->operand_pos()));
				if (map_function->strings_protection())
					AddString(file, static_cast<uint32_t>(command->operand_value()), command->address());
				break;
			case InlineField: 
			case InlineMethod:
			case InlineSig:
			case InlineTok:
			case InlineType:
				token = file.command_list()->token(value);
				if (token) {
					command->set_token_reference(token->reference_list()->Add(command->address() + command->operand_pos(), command->type()));
					NETImportFunction *import_function = file.import_list()->GetFunctionByToken(value);
					if (import_function) {
						import_function->map_function()->reference_list()->Add(command->address(), 0);
						if (ILOpCodes[command->type()].flow_type == Call) {
							if (j > 0 && ILOpCodes[func.item(j - 1)->type()].opcode_type == Prefix)
								import_function->include_option(ioHasCallPrefix);
						}

						if (import_function->owner()->is_sdk()) {
							switch (import_function->type()) {
							case atDecryptStringA:
							case atDecryptStringW:
								if (command->type() == icCall && j > 0) {
									ILCommand *prev = func.item(j - 1);
									if (prev->type() == icLdstr)
										AddString(file, static_cast<uint32_t>(prev->operand_value()), prev->address());
								}
								break;
							}
						}
					}
				}
				break;
			}
		}

		ILCommandBlock block;
		if (block.Parse(func)) {
			for (j = 0; j < block.count(); j++) {
				ILCommandNode *node = block.item(j);
				ILCommand *command = node->command();
				switch (command->type()) {
				case icCall: case icNewobj:
					{
						std::string name = node->token_name();
						if (name == "instance void System.Runtime.InteropServices.ComAwareEventInfo::.ctor(class System.Type, string)") {
							auto stack = node->stack();
							token = block.GetTypeOf(stack[0]);
							if (token && token->type() == ttTypeDef) {
								ILTypeDef *type_def = reinterpret_cast<ILTypeDef*>(token);
								if (stack[1]->command()->type() == icLdstr) {
									if (ILEvent *event = type_def->GetEvent(reinterpret_cast<ILUserString*>(stack[1]->command()->token_reference()->owner()->owner())->name()))
										event->set_can_rename(false);
								}
								else {
									ILTable *ref_table = file.command_list()->table(ttEvent);
									for (size_t r = 0; r < ref_table->count(); r++) {
										ILEvent *event = reinterpret_cast<ILEvent *>(ref_table->item(r));
										if (event->declaring_type() == type_def)
											event->set_can_rename(false);
									}
								}
							}
						}
						else if (name.size() > 58 && name.substr(0, 58) == "string Newtonsoft.Json.JsonConvert::SerializeObject(object") {
							auto stack = node->stack();
							token = block.GetTypeFromStack(stack[0]);
							if (token && token->type() == ttTypeDef) {
								ILTypeDef *type_def = reinterpret_cast<ILTypeDef*>(token);
								ILTable *ref_table = file.command_list()->table(ttProperty);
								for (size_t r = 0; r < ref_table->count(); r++) {
									ILProperty *property = reinterpret_cast<ILProperty*>(ref_table->item(r));
									if (property->declaring_type() == type_def)
										property->set_can_rename(false);
								}
							}
						}
					}
					break;
				}
			}
		}
	}

	file.EndProgress();
}

void ILFileHelper::AddString(NETArchitecture &file, uint32_t token, uint64_t reference)
{
	if (TOKEN_TYPE(token) != ttUserString)
		return;

	uint64_t address;
	ILData dump = file.command_list()->GetUserData(TOKEN_VALUE(token), &address);
	if (dump.empty())
		return;

	std::string name = "string \"" + file.command_list()->GetUserString(TOKEN_VALUE(token)) + "\"";
	MapFunction *map_function = file.map_function_list()->Add(address, address + dump.size(), otString, name);
	map_function->reference_list()->Add(reference, address);
}

/**
* ILCommandNode
*/

ILCommandNode::ILCommandNode(ILCommandBlock *owner, ILCommand *command)
	: IObject(), owner_(owner), command_(command)
{

}

ILCommandNode::~ILCommandNode()
{
	if (owner_)
		owner_->RemoveObject(this);
}

std::string ILCommandNode::token_name() const
{
	if (command_->token_reference()) {
		ILToken *token = command_->token_reference()->owner()->owner();
		if (token->type() == ttMethodSpec)
			token = reinterpret_cast<ILMethodSpec*>(token)->parent();

		switch (token->type()) {
		case ttMemberRef:
			return reinterpret_cast<ILMemberRef*>(token)->full_name();
		}
	}

	return std::string();
}

/**
* ILCommandBlock
*/

ILCommandBlock::ILCommandBlock()
	: ObjectList<ILCommandNode>(), method_(NULL)
{

}

ILCommandNode *ILCommandBlock::Add(ILCommand *command)
{
	ILCommandNode *node = new ILCommandNode(this, command);
	AddObject(node);
	return node;
}

ILCommandNode *ILCommandBlock::GetNodeByCommand(ILCommand *command) const
{
	auto it = map_.find(command);
	return (it != map_.end()) ? it->second : NULL;
}

void ILCommandBlock::AddObject(ILCommandNode *node)
{
	ObjectList<ILCommandNode>::AddObject(node);
	if (node->command())
		map_[node->command()] = node;
}

ILToken *ILCommandBlock::GetTypeOf(ILCommandNode *node) const
{
	if (node->command()->type() == icCall) {
		if (node->token_name() == "class System.Type System.Type::GetTypeFromHandle(valuetype System.RuntimeTypeHandle)") {
			auto stack = node->stack();
			if (stack[0]->command()->type() == icLdtoken)
				return stack[0]->command()->token_reference()->owner()->owner();
		}
	}

	return NULL;
}

ILToken *ILCommandBlock::GetTypeFromStack(ILCommandNode *node) const
{
	switch (node->command()->type()) {
	case icLdarg_0:
		if (method_ && method_->signature()->has_this())
			return method_->declaring_type();
		break;
	case icDup:
		return GetTypeFromStack(node->stack()[0]);
	case icNewobj:
		{
			ILToken *token = node->command()->token_reference()->owner()->owner();
			switch (token->type()) {
			case ttMethodDef:
				return reinterpret_cast<ILMethodDef*>(token)->declaring_type();
			case ttMemberRef:
				return reinterpret_cast<ILMemberRef*>(token)->declaring_type();
			}
		}
		break;
	}
	return NULL;
}

bool ILCommandBlock::Parse(ILFunction &func)
{
	size_t i, j, c;
	ILCommand *command;
	std::set<ILCommand *> entry_list;
	std::vector<ILCommandNode *> command_stack;
	std::map<ILCommand *, std::vector<ILCommandNode *> > stack_map;

	for (i = 0; i < func.function_info_list()->count(); i++) {
		FunctionInfo *info = func.function_info_list()->item(i);
		if (!info->source())
			continue;

		command = reinterpret_cast<ILCommand *>(info->entry());
		entry_list.insert(command);
		method_ = reinterpret_cast<NETRuntimeFunction*>(info->source())->method();
	}

	command_stack.push_back(NULL);
	for (i = 0; i < func.link_list()->count(); i++) {
		CommandLink *link = func.link_list()->item(i);
		switch (link->type()) {
		case ltSEHBlock:
			command = func.GetCommandByAddress(link->to_address());
			if (command) {
				entry_list.insert(command);
				stack_map[command] = command_stack;
			}
			break;
		case ltFinallyBlock:
			command = func.GetCommandByAddress(link->to_address());
			if (command)
				entry_list.insert(command);
			break;
		}
	}

	while (!entry_list.empty()) {
		command = *entry_list.begin();
		j = func.IndexOf(command);

		auto it = stack_map.find(command);
		if (it != stack_map.end())
			command_stack = it->second;
		else
			command_stack.clear();

		for (i = j; i < func.count(); i++) {
			command = func.item(i);
			std::set<ILCommand *>::const_iterator it = entry_list.find(command);
			if (it != entry_list.end())
				entry_list.erase(it);

			ILCommandNode *node = Add(command);

			if (command->type() == icRet) {
				if (command_stack.size() > 1)
					return false;

				node->set_stack(command_stack);
				break;
			}

			size_t pop;
			int stack = command->GetStackLevel(&pop);

			if (pop) {
				if (pop > command_stack.size())
					return false;

				std::vector<ILCommandNode *> node_stack;
				node_stack.insert(node_stack.begin(), command_stack.end() - pop, command_stack.end());
				command_stack.erase(command_stack.end() - pop, command_stack.end());
				node->set_stack(node_stack);
			}

			for (c = 0; c < stack + pop; c++) {
				command_stack.push_back(node);
			}

			if (command->link() && (command->link()->type() == ltJmp || command->link()->type() == ltJmpWithFlag)) {
				if (ILCommand *link_command = func.GetCommandByAddress(command->link()->to_address())) {
					if (!GetNodeByCommand(link_command)) {
						entry_list.insert(link_command);
						stack_map[link_command] = command_stack;
					}
				}
			}

			if (command->is_end())
				break;
		}
	}

	return true;
}

/**
* ILImport
*/

ILImport::ILImport(IFunctionList *owner, OperandSize cpu_address_size)
	: ILFunction(owner, cpu_address_size)
{
	set_compilation_type(ctMutation);

}

bool ILImport::Init(const CompileContext &ctx)
{
	NETArchitecture *file = reinterpret_cast<NETArchitecture *>(ctx.file);
	NETArchitecture *runtime = reinterpret_cast<NETArchitecture *>(ctx.runtime);
	ILTypeDef *module = reinterpret_cast<ILTypeDef*>(file->command_list()->token(ttTypeDef | 1));
	ILTypeDef *runtime_loader = runtime->command_list()->GetTypeDef("VMProtect.Loader");
	if (!runtime_loader)
		return false;

	ILField *iat_field = runtime_loader->GetField("IAT");
	if (!iat_field)
		return false;

	size_t i, j, k, c;
	std::map<ILToken *, bool> value_type_map;
	ILTokenType table_types[] = {ttMemberRef, ttStandAloneSig, ttField, ttMethodDef};
	for (i = 0; i < _countof(table_types); i++) {
		ILTable *table = module->meta()->table(table_types[i]);
		for (k = 0; k < table->count(); k++) {
			ILToken *token = table->item(k);
			ILSignature *signature = NULL;
			switch (token->type()) {
			case ttMemberRef:
				signature = reinterpret_cast<ILMemberRef *>(token)->signature();
				break;
			case ttStandAloneSig:
				signature = reinterpret_cast<ILStandAloneSig *>(token)->signature();
				break;
			case ttField:
				signature = reinterpret_cast<ILField *>(token)->signature();
				break;
			case ttMethodDef:
				signature = reinterpret_cast<ILMethodDef *>(token)->signature();
				break;
			}

			if (!signature)
				continue;

			std::vector<ILElement *> element_stack;
			element_stack.push_back(signature->ret());
			for (c = 0; c < signature->count(); c++) {
				element_stack.push_back(signature->item(c));
			}
			for (c = 0; c < element_stack.size(); c++) {
				ILElement *element = element_stack[c];
				if (element->type() == ELEMENT_TYPE_CLASS)
					value_type_map[element->token()] = false;
				else if (element->type() == ELEMENT_TYPE_VALUETYPE)
					value_type_map[element->token()] = true;
				if (element->next())
					element_stack.push_back(element->next());
			}
		}
	}

	std::vector<NETImportFunction *> import_list;
	for (i = 0; i < file->import_list()->count(); i++) {
		NETImport *import = file->import_list()->item(i);

		// APIs processed by ILSDK
		if (import->is_sdk())
			continue;

		if (import->excluded_from_import_protection())
			continue;

		for (j = 0; j < import->count(); j++) {
			NETImportFunction *import_function = import->item(j);
			if (import_function->options() & ioHasCallPrefix)
				continue;
			
			import_list.push_back(import_function);
		}
	}
	for (i = 0; i < import_list.size(); i++) {
		std::swap(import_list[i], import_list[rand() % import_list.size()]);
	}

	size_t iat_index = 0;
	ILTypeRef *delegate_base_type = NULL;
	for (i = 0; i < import_list.size(); i++) {
		NETImportFunction *import_function = import_list[i];
		ILToken *import_token = file->command_list()->token(import_function->token());
		ILSignature *signature;
		ILToken *declaring_type;
		switch (import_token->type()) {
		case ttMemberRef:
			signature = reinterpret_cast<ILMemberRef*>(import_token)->signature();
			declaring_type = reinterpret_cast<ILMemberRef*>(import_token)->declaring_type();
			break;
		default:
			continue;
		}

		if (declaring_type->type() != ttTypeRef)
			continue;

		MapFunction *map_function = import_function->map_function();
		ReferenceList *reference_list = map_function->reference_list();

		std::map<ILCommandType, ILMethodDef *> proxy_map;
		for (j = 0; j < reference_list->count(); j++) {
			uint64_t address = reference_list->item(j)->address();

			if (!file->AddressSeek(address))
				return false;

			ILCommand *src_command = reinterpret_cast<ILCommand *>(ctx.file->function_list()->GetCommandByAddress(address, true));

			size_t index = count();
			ILCommand *ref_command = Add(address);
			ref_command->ReadFromFile(*file);
			ILCommandType ref_type = static_cast<ILCommandType>(ref_command->type());
			if (ref_type != icNewobj && ref_type != icCall && ref_type != icCallvirt && ref_type != icLdsfld && ref_type != icLdfld) {
				delete ref_command;
				continue;
			}

			ILMethodDef *proxy_method;
			std::map<ILCommandType, ILMethodDef *>::const_iterator it = proxy_map.find(ref_type);
			if (it == proxy_map.end()) {
				ILSignature *new_signature;
				if (signature->is_field()) {
					ILData data;
					data.push_back(stDefault);
					data.push_back(0);
					signature->ret()->WriteToData(data);
					new_signature = new ILSignature(module->meta());
					new_signature->Parse(data);
					if (ref_type == icLdfld)
						new_signature->set_has_this(true);
				}
				else {
					new_signature = signature->Clone(module->meta());
				}

				if (new_signature->has_this()) {
					std::map<ILToken *, bool>::const_iterator it = value_type_map.find(declaring_type);
					if (it == value_type_map.end()) {
						delete ref_command;
						continue;
					}
					bool is_value_type = it->second;

					ILData data;
					if (is_value_type) {
						if (ref_type != icNewobj && ref_type != icLdfld)
							data.push_back(ELEMENT_TYPE_BYREF);
						data.push_back(ELEMENT_TYPE_VALUETYPE);
					}
					else {
						data.push_back(ELEMENT_TYPE_CLASS);
					}
					uint32_t value = declaring_type->value() << 2;
					switch (declaring_type->type()) {
					case ttTypeDef:
						value |= 0;
						break;
					case ttTypeRef:
						value |= 1;
						break;
					case ttTypeSpec:
						value |= 2;
						break;
					}
					data.push_back(static_cast<uint8_t>(value >> 24) | 0xc0);
					data.push_back(static_cast<uint8_t>(value >> 16));
					data.push_back(static_cast<uint8_t>(value >> 8));
					data.push_back(static_cast<uint8_t>(value));

					if (ref_type == icNewobj)
						new_signature->ret()->Parse(data);
					else {
						ILElement *element = new ILElement(module->meta(), new_signature);
						element->Parse(data);
						new_signature->InsertObject(0, element);
					}
				}

				ILData proxy_signature;
				{
					ILData data;
					data.push_back(ELEMENT_TYPE_OBJECT);
					ILElement *element = new_signature->ret();
					switch (element->type()) {
					case ELEMENT_TYPE_CLASS:
					case ELEMENT_TYPE_SZARRAY:
						element->Parse(data);
						break;
					case ELEMENT_TYPE_GENERICINST:
						if (element->next()->type() == ELEMENT_TYPE_CLASS)
							element->Parse(data);
						break;
					}

					if (new_signature->has_this()) {
						new_signature->set_has_this(false);
						for (k = 0; k < new_signature->count(); k++) {
							element = new_signature->item(k);
							switch (element->type()) {
							case ELEMENT_TYPE_CLASS:
							case ELEMENT_TYPE_SZARRAY:
								element->Parse(data);
								break;
							case ELEMENT_TYPE_GENERICINST:
								if (element->next()->type() == ELEMENT_TYPE_CLASS)
									element->Parse(data);
								break;
							}
						}
					}
				}
				new_signature->WriteToData(proxy_signature);
				delete new_signature;

				if (!delegate_base_type) {
					ILAssemblyRef *assembly_ref = module->meta()->GetCoreLib();
					delegate_base_type = module->meta()->ImportTypeRef(assembly_ref, "System", "MulticastDelegate");
				}

				ILTypeDef *delegate_type = module->meta()->AddTypeDef(delegate_base_type, "", "", (CorTypeAttr)(tdNotPublic | tdSealed));
				file->RenameToken(delegate_type);

				ILData ctor_signature; // instance void (object, native int)
				ctor_signature.push_back(stDefault | stHasThis);
				ctor_signature.push_back(2);
				ctor_signature.push_back(ELEMENT_TYPE_VOID);
				ctor_signature.push_back(ELEMENT_TYPE_OBJECT);
				ctor_signature.push_back(ELEMENT_TYPE_I);

				ILMethodDef *delegate_ctor = module->meta()->AddMethod(delegate_type, ".ctor", ctor_signature, (CorMethodAttr)(mdAssem | mdHideBySig | mdRTSpecialName | mdSpecialName), miRuntime);
				ILMethodDef *delegate_invoke = module->meta()->AddMethod(delegate_type, "Invoke", proxy_signature, (CorMethodAttr)(mdAssem | mdHideBySig | mdVirtual | mdNewSlot), miRuntime);
				delegate_invoke->signature()->set_has_this(true);

				uint32_t call_type;
				switch (ref_type) {
				case icNewobj:
					call_type = 1;
					break;
				case icCallvirt:
					call_type = 2;
					break;
				case icLdsfld:
					call_type = 8;
					break;
				case icLdfld:
					call_type = 9;
					break;
				default:
					call_type = 0;
					break;
				}
				info_list_.push_back(ImportDelegateInfo(import_token, delegate_invoke, call_type));

				proxy_method = module->meta()->AddMethod(module, "", proxy_signature, (CorMethodAttr)(mdAssem | mdStatic), miIL);
				file->RenameToken(proxy_method);

				size_t entry_index = count();
				// header
				ILCommand *command = AddCommand(icComment, 0);
				command->include_section_option(rtLinkedToExt);
				Data data;
				data.PushByte(CorILMethod_TinyFormat);
				command->set_dump(data.data(), data.size());
				command->set_alignment(sizeof(uint32_t));
				// code
				command = AddCommand(icLdsfld, 0);
				command->set_token_reference(iat_field->reference_list()->Add(0));
				AddCommand(icLdc_i4, static_cast<uint32_t>(iat_index++));
				AddCommand(icLdelem_ref, 0);
				for (k = 0; k < proxy_method->signature()->count(); k++) {
					AddCommand(icLdarg, static_cast<uint32_t>(k));
				}
				command = AddCommand(icCall, 0);
				command->set_token_reference(delegate_invoke->reference_list()->Add(0));
				AddCommand(icRet, 0);

				FunctionInfo *info = function_info_list()->Add(0, 0, btValue, item(entry_index)->dump_size(), 0, 0, runtime->runtime_function_list()->Add(0, 0, 0, proxy_method), item(entry_index));
				AddressRange *range = info->Add(0, 0, NULL, NULL, NULL);
				for (k = entry_index; k < count(); k++) {
					item(k)->set_address_range(range);
				}

				proxy_map[ref_type] = proxy_method;
			}
			else {
				proxy_method = it->second;
			}
		
			if (src_command) {
				delete ref_command;

				src_command->Init(icCall, 0, src_command->token_reference());
				src_command->CompileToNative();
				src_command->token_reference()->set_owner(proxy_method->reference_list());
			}
			else {
				ref_command->exclude_option(roClearOriginalCode);
				ref_command->Init(icCall, 0, ref_command->token_reference());
				ref_command->CompileToNative();
				ref_command->token_reference()->set_owner(proxy_method->reference_list());

				CommandBlock *block = AddBlock(index, true);
				block->set_address(address);
				ref_command->set_block(block);
			}
		}
	}

	for (i = 0; i < count(); i++) {
		item(i)->CompileToNative();
	}

	return ILFunction::Init(ctx);
}

/**
* ILVirtualMachine
*/

ILVirtualMachine::ILVirtualMachine(ILVirtualMachineList *owner, uint8_t id, ILVirtualMachineProcessor *processor)
	: BaseVirtualMachine(owner, id), processor_(processor), ctor_(NULL), invoke_(NULL)
{

}

void ILVirtualMachine::Init(const CompileContext &ctx)
{
	processor_->InitCommands(ctx);
	ctor_ = processor_->function_info_list()->count() > 0 ? reinterpret_cast<NETRuntimeFunction *>(processor_->function_info_list()->item(0)->source())->method() : NULL;
	invoke_ = processor_->function_info_list()->count() > 1 ? reinterpret_cast<NETRuntimeFunction *>(processor_->function_info_list()->item(1)->source())->method() : NULL;

	size_t i, j, c;
	ILCommand *command;
	ILOpcodeInfo *opcode;
	size_t template_index, template_count;

	template_index = 0;
	template_count = 0;
	for (i = 0; i < processor_->count(); i++) {
		command = processor_->item(i);
		if (command->type() != icLdftn)
			continue;

		template_index = i - 4;
		for (j = i; j < processor_->count(); j++) {
			command = processor_->item(j);
			if (command->type() != icLdftn)
				continue;

			if (!template_count && j > i)
				template_count = j - i;
			opcode_list_.Add(static_cast<ILCommandType>(processor_->item(j - 2)->operand_value()), command->token_reference()->owner()->owner());
		}
		break;
	}

	for (i = template_index + template_count * opcode_list_.count(); i > template_index + template_count; i--) {
		command = processor_->item(i - 1);
		if (command->link())
			delete command->link();
		if (command->token_reference())
			delete command->token_reference();
		delete command;
	}

	// randomize opcodes
	c = opcode_list_.count();
	for (i = 0; i < opcode_list_.count(); i++) {
		opcode_list_.SwapObjects(i, rand() % c);
	}
	for (i = opcode_list_.count(); i < 0x100; i++) {
		opcode = opcode_list_.item(rand() % i);
		opcode_list_.Add(opcode->command_type(), opcode->entry());
	}

	opcode_stack_.clear();
	for (i = 0; i < opcode_list_.count(); i++) {
		opcode = opcode_list_.item(i);
		opcode->set_opcode(static_cast<uint8_t>(i));
		opcode_stack_[opcode->Key()].push_back(opcode);

		for (j = 0; j < template_count; j++) {
			command = processor_->item(template_index + j);
			if (i > 0) {
				command = command->Clone(processor_);
				processor_->InsertObject(template_index + i * template_count + j, command);
			}
			switch (command->type()) {
			case icLdc_i4: case icLdc_i4_s:
				command->Init(icLdc_i4, opcode->opcode());
				command->CompileToNative();
				break;
			case icLdftn:
				command->set_operand_value(0, opcode->entry()->id());
				command->set_token_reference(opcode->entry()->reference_list()->Add(0));
				command->CompileToNative();
				break;
			default:
				if (command->token_reference())
					command->set_token_reference(command->token_reference()->owner()->Add(0));
			}
		}
	}
}

ILOpcodeInfo *ILVirtualMachine::GetOpcode(ILCommandType command_type)
{
	ILOpcodeInfo *res = NULL;
	auto it = opcode_stack_.find(command_type);
	if (it != opcode_stack_.end())
		res = it->second.Next();
	return res;
}

static void EncryptBuffer(uint32_t *buffer, uint64_t key)
{
	uint32_t key0 = static_cast<uint32_t>(key >> 32);
	uint32_t key1 = static_cast<uint32_t>(key);
	buffer[0] = _rotr32(buffer[0] - key0, 7) ^ key1;
	buffer[1] = _rotr32(buffer[1] - key0, 11) ^ key1;
	buffer[2] = _rotr32(buffer[2] - key0, 17) ^ key1;
	buffer[3] = _rotr32(buffer[3] - key0, 23) ^ key1;
}

void ILVirtualMachine::CompileCommand(ILVMCommand &vm_command)
{
	Data dump;
	ILOpcodeInfo *opcode = NULL;
	uint64_t value = vm_command.value();
	if (vm_command.crypt_command() == icAdd) {
		uint32_t crypted_value[4];
		size_t i;
		for (i = 0; i < _countof(crypted_value); i++) {
			crypted_value[i] = rand32();
		}
		switch (vm_command.crypt_size()) {
		case osDWord:
			crypted_value[3] = static_cast<uint32_t>(value);
			break;
		case osQWord:
			*reinterpret_cast<uint64_t*>(&crypted_value[2]) = value;
			break;
		}
		uint32_t dw = 0;
		for (i = 1; i < 4; i++) {
			dw += crypted_value[i];
		}
		crypted_value[0] = 0 - dw;
		EncryptBuffer(crypted_value, vm_command.crypt_key());
		ILVMCommand *link_command = vm_command.link_command();
		for (i = 3; i > 0; i--) {
			link_command->set_value(crypted_value[i - 1]);
			link_command->Compile();
			link_command = link_command->link_command();
		}
		value = crypted_value[3];
	}

	switch (vm_command.command_type()) {
	case icLdarg: case icLdarga:
	case icStarg:
	case icLdloc: case icLdloca:
	case icStloc:
		opcode = GetOpcode(vm_command.command_type());
		dump.PushWord(static_cast<uint16_t>(vm_command.value()));
		break;
	case icLdc_i4:
		opcode = GetOpcode(vm_command.command_type());
		dump.PushDWord(static_cast<uint32_t>(value));
		break;
	case icLdc_i8:
		opcode = GetOpcode(vm_command.command_type());
		dump.PushQWord(value);
		break;
	case icLdc_r4:
		opcode = GetOpcode(vm_command.command_type());
		dump.PushDWord(static_cast<uint32_t>(value));
		break;
	case icLdc_r8:
		opcode = GetOpcode(vm_command.command_type());
		dump.PushQWord(value);
		break;
	case icByte:
		dump.PushByte(static_cast<uint8_t>(vm_command.value()));
		break;
	case icWord:
		dump.PushWord(static_cast<uint16_t>(vm_command.value()));
		break;
	case icDword:
		dump.PushDWord(static_cast<uint32_t>(vm_command.value()));
		break;
	case icQword:
		dump.PushQWord(vm_command.value());
		break;
	default:
		opcode = GetOpcode(vm_command.command_type());
		break;
	}

	if (opcode) {
		dump.InsertByte(0, opcode->opcode());
	}
	else if (!vm_command.is_data()) {
		throw std::runtime_error("Runtime error at CompileToVM: " + std::string(ILOpCodes[vm_command.command_type()].name));
	}

	vm_command.set_dump(dump);
}

/**
* ILOpcodeInfo
*/

ILOpcodeInfo::ILOpcodeInfo(ILOpcodeList *owner, ILCommandType command_type, ILToken *entry)
	: IObject(), owner_(owner), command_type_(command_type), entry_(entry), opcode_(0)
{

}

ILOpcodeInfo::~ILOpcodeInfo()
{
	if (owner_)
		owner_->RemoveObject(this);
}

ILOpcodeInfo *ILOpcodeInfo::circular_queue::Next()
{
	ILOpcodeInfo *res = NULL;
	if (size())
		res = this->operator[](position_++ % size());
	return res;
}

/**
* ILOpcodeList
*/

ILOpcodeList::ILOpcodeList()
	: ObjectList<ILOpcodeInfo>()
{

}

ILOpcodeInfo *ILOpcodeList::Add(ILCommandType command_type, ILToken *entry)
{
	ILOpcodeInfo *opcode = new ILOpcodeInfo(this, command_type, entry);
	AddObject(opcode);
	return opcode;
}

ILOpcodeInfo *ILOpcodeList::GetOpcodeInfo(ILCommandType command_type) const
{
	for (size_t i = 0; i < count(); i++) {
		ILOpcodeInfo *opcode = item(i);
		if (opcode->command_type() == command_type)
			return opcode;
	}
	return NULL;
}

/**
 * ILVirtualMachineList
 */

IVirtualMachineList * ILVirtualMachineList::Clone() const
{
	ILVirtualMachineList *list = new ILVirtualMachineList();
	return list;
}

void ILVirtualMachineList::Prepare(const CompileContext &ctx)
{
	ILFunctionList *function_list = reinterpret_cast<ILFunctionList *>(ctx.file->function_list());
	ILVirtualMachineProcessor *processor = function_list->AddProcessor(ctx.file->cpu_address_size());
	ILVirtualMachine *virtual_machine = new ILVirtualMachine(this, 1, processor);
	AddObject(virtual_machine);
	virtual_machine->Init(ctx);
}

/**
* ILVirtualMachineProcessor
*/

ILVirtualMachineProcessor::ILVirtualMachineProcessor(ILFunctionList *owner, OperandSize cpu_address_size)
	: ILFunction(owner, cpu_address_size)
{
	set_compilation_type(ctMutation);
	set_tag(ftProcessor);
}

void ILVirtualMachineProcessor::InitCommands(const CompileContext &ctx)
{
	size_t i, j, k;
	ILCommand *command, *src_command, *dst_command;
	CommandLink *src_link, *dst_link;

	ILFunctionList *runtime_function_list = reinterpret_cast<ILFunctionList *>(ctx.vm_runtime->function_list());
	for (i = 0; i < runtime_function_list->count(); i++) {
		ILFunction *func = runtime_function_list->item(i);
		if (func->tag() != ftProcessor)
			continue;

		//func->Init(ctx);

		size_t orig_function_info_count = function_info_list()->count();
		for (j = 0; j < func->function_info_list()->count(); j++) {
			FunctionInfo *info = func->function_info_list()->item(j);
			function_info_list()->AddObject(info->Clone(function_info_list()));
		}
		for (j = 0; j < func->range_list()->count(); j++) {
			AddressRange *range = func->range_list()->item(j);
			range_list()->AddObject(range->Clone(range_list()));
		}

		for (j = 0; j < func->count(); j++) {
			src_command = func->item(j);
			dst_command = src_command->Clone(this);
			AddressRange *address_range = src_command->address_range();
			if (address_range) {
				FunctionInfo *info = function_info_list()->item(orig_function_info_count + func->function_info_list()->IndexOf(address_range->owner()));
				dst_command->set_address_range(info->item(address_range->owner()->IndexOf(address_range)));
			}

			AddObject(dst_command);

			src_link = src_command->link();
			if (src_link) {
				dst_link = src_link->Clone(link_list());
				dst_link->set_from_command(dst_command);
				link_list()->AddObject(dst_link);

				if (src_link->parent_command())
					dst_link->set_parent_command(GetCommandByAddress(src_link->parent_command()->address()));
				if (src_link->base_function_info())
					dst_link->set_base_function_info(function_info_list()->GetItemByAddress(src_link->base_function_info()->begin()));
			}
		}
	}

	for (i = 0; i < function_info_list()->count(); i++) {
		FunctionInfo *info = function_info_list()->item(i);
		if (info->entry())
			info->set_entry(GetCommandByAddress(info->entry()->address()));
		if (info->data_entry())
			info->set_data_entry(GetCommandByAddress(info->data_entry()->address()));
		for (j = 0; j < info->count(); j++) {
			AddressRange *dest = info->item(j);
			for (k = 0; k < range_list()->count(); k++) {
				AddressRange *range = range_list()->item(k);
				if (range->begin() <= dest->begin() && range->end() > dest->begin())
					dest->AddLink(range);
			}
		}
	}
	for (i = 0; i < range_list()->count(); i++) {
		AddressRange *range = range_list()->item(i);
		if (range->begin_entry())
			range->set_begin_entry(GetCommandByAddress(range->begin_entry()->address()));
		if (range->end_entry())
			range->set_end_entry(GetCommandByAddress(range->end_entry()->address()));
		if (range->size_entry())
			range->set_size_entry(GetCommandByAddress(range->size_entry()->address()));
	}
	for (i = 0; i < count(); i++) {
		command = item(i);
		dst_link = command->link();
		if (dst_link) {
			if (dst_link->to_address())
				dst_link->set_to_command(GetCommandByAddress(dst_link->to_address()));
		}
		if (command->token_reference())
			command->token_reference()->set_address(0);
		command->exclude_option(roClearOriginalCode);
		command->CompileToNative();
	}
}

void ILVirtualMachineProcessor::AfterCompile(const CompileContext &ctx)
{
	ILFunction::AfterCompile(ctx);

	if (ctx.runtime)
		set_need_compile(false);
}

void ILVirtualMachineProcessor::CompileLinks(const CompileContext &ctx)
{
	if (!need_compile())
		return;

	ILFunction::CompileLinks(ctx);
}

void ILVirtualMachineProcessor::CompileInfo(const CompileContext &ctx)
{
	if (!need_compile())
		return;

	ILFunction::CompileInfo(ctx);
}

size_t ILVirtualMachineProcessor::WriteToFile(IArchitecture &file)
{
	if (!need_compile())
		return 0;

	return ILFunction::WriteToFile(file);
}

/**
 * NETLoader
 */

NETLoader::NETLoader(IFunctionList *owner, OperandSize cpu_address_size)
	: ILFunction(owner, cpu_address_size), file_crc_entry_(NULL), file_crc_size_(0),
	import_entry_(NULL), import_size_(0), iat_entry_(NULL), iat_size_(0), loader_crc_entry_(NULL), loader_crc_size_(0),
	file_crc_size_entry_(NULL), loader_crc_size_entry_(NULL), loader_crc_hash_entry_(NULL),
	pe_entry_(NULL), strong_name_signature_entry_(NULL), tls_entry_(NULL), tls_size_(0)
{
	set_compilation_type(ctVirtualization);
	set_tag(ftLoader);
}

void NETLoader::AddAVBuffer(const CompileContext &ctx)
{
	ILCommand *command;
	uint32_t sum = 0;
	CommandBlock *block = AddBlock(count(), true);
	for (size_t i = 0; i < 64; i++) {
		uint32_t value = (i == 0) ? 0 : rand32();
		sum += value;
		command = AddCommand(icDword, value);
		command->CompileToNative();
		command->set_block(block);
	}
	block->set_end_index(count() - 1);
	command = item(block->start_index());
	command->set_operand_value(0, 0xB7896EB5 - sum);
	command->CompileToNative();
	uint64_t address = ctx.manager->Alloc((block->end_index() - block->start_index() + 1) * sizeof(uint32_t), mtReadable);
	block->set_address(address);
}

Data EncryptString(const char *str, uint32_t key);
Data EncryptString(const os::unicode_char *str, uint32_t key);

bool NETLoader::Prepare(const CompileContext &ctx)
{
	size_t i, j, k, old_count, index, import_index, start_index;
	NETArchitecture *file, *runtime;
	ILFunctionList *runtime_function_list;
	ILFunction *func;
	CommandLink *link, *src_link, *dst_link;
	ILCommand *src_command, *dst_command, *command;
	ILCRCTable *il_crc;
	PEImportList new_import_list(NULL);
	std::vector<ImportInfo> import_info_list;
	std::vector<ImportFunctionInfo> import_function_info_list;
	PEImport *import;
	PEImportFunction *import_function;
	ImportInfo import_info;
	PESegment *segment;
	ILImport *il_import;
	uint64_t tls_index_address;
	
	file = reinterpret_cast<NETArchitecture *>(ctx.file);
	runtime = reinterpret_cast<NETArchitecture *>(ctx.runtime);
	il_crc = reinterpret_cast<ILFunctionList *>(file->function_list())->crc_table();
	il_import = reinterpret_cast<ILFunctionList *>(file->function_list())->import();
	PEArchitecture *pe_file = file->pe();
	uint32_t string_key = rand32();
	ILCommandType value_command_type = (cpu_address_size() == osDWord) ? icDword : icQword;

	// create AV signature buffer
	AddAVBuffer(ctx);
	start_index = count();

	import_size_ = 0;
	import_entry_ = 0;
	PEImportFunction *entry_point_import_function = NULL;
	if ((ctx.options.flags & cpPack) && pe_file->import_list()->count()) {
		for (i = 0; i < pe_file->import_list()->count(); i++) {
			import = pe_file->import_list()->item(i);

			new_import_list.AddObject(import->Clone(&new_import_list));
		}

		const std::string import_name = "mscoree.dll";
		const std::string import_function_name = (pe_file->image_type() == itExe) ? "_CorExeMain" : "_CorDllMain";

		import = reinterpret_cast<PEImport *>(new_import_list.GetImportByName(import_name));
		if (!import) {
			import = new PEImport(&new_import_list, import_name);
			new_import_list.AddObject(import);
		}
		entry_point_import_function = NULL;
		for (i = 0; i < import->count(); i++) {
			import_function = import->item(i);
			if (import_function->name() == import_function_name) {
				entry_point_import_function = import_function;
				break;
			}
		}
		if (!entry_point_import_function) {
			entry_point_import_function = new PEImportFunction(import, import_function_name);
			import->AddObject(entry_point_import_function);
		}

		// create import directory
		for (i = 0; i < new_import_list.count(); i++) {
			import = new_import_list.item(i);

			// IMAGE_IMPORT_DESCRIPTOR.OriginalFirstThunk
			command = AddCommand(icDword, 0); 
			command->AddLink(0, ltOffset);
			import_info.original_first_thunk = command;

			// IMAGE_IMPORT_DESCRIPTOR.TimeDateStamp
			AddCommand(icDword, 0); 

			// IMAGE_IMPORT_DESCRIPTOR.ForwarderChain
			AddCommand(icDword, 0); 

			// IMAGE_IMPORT_DESCRIPTOR.Name
			command = AddCommand(icDword, 0); 
			command->AddLink(0, ltOffset);
			import_info.name = command;

			// IMAGE_IMPORT_DESCRIPTOR.FirstThunk
			command = AddCommand(icDword, 0); 
			command->AddLink(0, ltOffset);
			import_info.first_thunk = command;

			import_info_list.push_back(import_info);
		}

		// end of import directory
		for (j = 0; j < 5; j++) {
			AddCommand(icDword, 0);
		}

		import_size_ = static_cast<uint32_t>((count() - start_index) * sizeof(uint32_t));
		import_entry_ = item(start_index);
		import_entry_->set_alignment(OperandSizeToValue(pe_file->cpu_address_size()));

		// create IAT
		uint64_t ordinal_mask = (cpu_address_size() == osDWord) ? IMAGE_ORDINAL_FLAG32 : IMAGE_ORDINAL_FLAG64;
		size_t name_index = count();
		for (i = 0; i < new_import_list.count(); i++) {
			import = new_import_list.item(i);

			index = count();
			for (j = 0; j < import->count(); j++) {
				import_function = import->item(j);

				if (import_function->is_ordinal()) {
					command = AddCommand(value_command_type, ordinal_mask | import_function->ordinal());
				} else {
					command = AddCommand(value_command_type, 0);
					command->AddLink(0, ltOffset);
				}

				ImportFunctionInfo import_function_info(import_function);
				import_function_info.name = command;

				import_function_info_list.push_back(import_function_info);
			}

			AddCommand(value_command_type, 0);

			import_info_list[i].original_first_thunk->link()->set_to_command(item(index));
		}
		command = item(name_index);
		command->set_alignment(OperandSizeToValue(cpu_address_size()));

		size_t iat_index = count();
		for (i = 0, import_index = 0; i < new_import_list.count(); i++) {
			import = new_import_list.item(i);

			bool is_delay_import = (import_info_list[i].name == NULL);

			index = count();
			for (j = 0; j < import->count(); j++, import_index++) {
				import_function = import->item(j);

				if (import_function->is_ordinal()) {
					command = AddCommand(value_command_type, ordinal_mask | import_function->ordinal());
				} else {
					command = AddCommand(value_command_type, 0);
					command->AddLink(0, ltOffset);
				}

				import_function_info_list[import_index].thunk = command;
			}

			if (is_delay_import)
				continue;

			AddCommand(value_command_type, 0);

			import_info_list[i].first_thunk->link()->set_to_command(item(index));
		}

		iat_entry_ = item(iat_index);
		iat_size_ = static_cast<uint32_t>((count() - iat_index) * OperandSizeToValue(cpu_address_size()));
		iat_entry_->set_alignment(file->segment_alignment());
		iat_entry_->include_option(roCreateNewBlock);

		// IAT size must be aligned by page size
		j = AlignValue(iat_size_, file->segment_alignment());
		if (j > iat_size_) {
			std::string buffer;
			buffer.resize(j - iat_size_, 0);
			AddCommand(buffer);
		}

		// create import DLL names
		uint32_t string_key = rand32();
		for (i = 0; i < new_import_list.count(); i++) {
			import = new_import_list.item(i);

			command = NULL;
			for (j = 0; j < i; j++) {
				if (new_import_list.item(j)->CompareName(import->name())) {
					command = reinterpret_cast<ILCommand *>(import_info_list[j].name->link()->to_command());
					break;
				}
			}
			if (command == NULL) {
				command = AddCommand(import->name());
				command->include_option(roCreateNewBlock);
				command->set_alignment(sizeof(uint16_t));
			}

			import_info_list[i].name->link()->set_to_command(command);
		}

		// create import function names
		for (i = 0, import_index = 0; i < new_import_list.count(); i++) {
			import = new_import_list.item(i);

			for (j = 0; j < import->count(); j++, import_index++) {
				import_function = import->item(j);
				if (import_function->is_ordinal())
					continue;

				command = AddCommand(icWord, 0);
				command->include_option(roCreateNewBlock);
				command->set_alignment(sizeof(uint16_t));

				AddCommand(import_function->name());

				import_function_info_list[import_index].name->link()->set_to_command(command);
				import_function_info_list[import_index].thunk->link()->set_to_command(command);
			}
		}

		// update links for PE structures
		for (i = 0; i < count(); i++) {
			link = item(i)->link();
			if (!link)
				continue;

			link->set_sub_value(file->image_base());
		}
	}

	pe_entry_ = NULL;
	strong_name_signature_entry_ = NULL;
	if ((ctx.options.flags & cpPack)) {
		if (pe_file->entry_point()) {
			// create native entry point
			segment = file->segment_list()->GetSectionByAddress(pe_file->entry_point());
			if (segment && !segment->excluded_from_packing()) {
				std::vector<ImportFunctionInfo>::const_iterator it = std::find(import_function_info_list.begin(), import_function_info_list.end(), entry_point_import_function);
				if (it == import_function_info_list.end())
					return false;

				index = count();
				CommandBlock *block = AddBlock(count(), true);
				AddCommand(icWord, 0x25ff);
				command = AddCommand(icDword, 0);
				command->AddLink(0, ltOffset, it->thunk);
				pe_entry_ = item(index);
				for (i = index; i < count(); i++) {
					command = item(i);
					command->set_block(block);
					block->set_end_index(i);
				}
			}
		}

		IMAGE_DATA_DIRECTORY strong_name_signature = file->header().StrongNameSignature;
		if (strong_name_signature.VirtualAddress) {
			// create blank StrongNameSignature
			segment = file->segment_list()->GetSectionByAddress(strong_name_signature.VirtualAddress + file->image_base());
			if (segment && !segment->excluded_from_packing()) {
				Data data;
				data.resize(strong_name_signature.Size ? strong_name_signature.Size : 0x80);
				strong_name_signature_entry_ = AddCommand(data);
				strong_name_signature_entry_->include_option(roCreateNewBlock);
				strong_name_signature_entry_->set_alignment(sizeof(uint32_t));
			}
		}
	}

	// create tls structure
	tls_entry_ = NULL;
	tls_size_ = 0;
	tls_call_back_entry_ = NULL;
	tls_index_address = 0;
	if (pe_file->tls_directory()->address() && (pe_file->tls_directory()->count() || (ctx.options.flags & cpPack))) {
		size_t tls_index = count();

		PETLSDirectory *tls_directory = pe_file->tls_directory();
		if (ctx.options.flags & cpPack) {
			if (file->AddressSeek(tls_directory->address_of_index()) && !file->selected_segment()->excluded_from_packing())
				tls_index_address = tls_directory->address_of_index();
		}

		ILCommand *start_address_entry = AddCommand(value_command_type, tls_directory->start_address_of_raw_data(), NEED_FIXUP);
		ILCommand *end_address_entry = AddCommand(value_command_type, tls_directory->end_address_of_raw_data(), NEED_FIXUP);
		AddCommand(value_command_type, tls_directory->address_of_index(), NEED_FIXUP);
		ILCommand *call_back_entry = AddCommand(value_command_type, 0);
		AddCommand(icDword, tls_directory->size_of_zero_fill());
		AddCommand(icDword, tls_directory->characteristics());

		if (tls_directory->end_address_of_raw_data() > tls_directory->start_address_of_raw_data()) {
			Data data;
			data.resize(static_cast<size_t>(tls_directory->end_address_of_raw_data() - tls_directory->start_address_of_raw_data()));
			if (file->AddressSeek(tls_directory->start_address_of_raw_data()))
				file->Read(&data[0], data.size());
			ILCommand *data_entry = AddCommand(data);
			start_address_entry->AddLink(0, ltOffset, data_entry);
			link = end_address_entry->AddLink(0, ltOffset, data_entry);
			link->set_sub_value(0 - (uint64_t)data.size());
		}

		index = count();
		if (pe_file->tls_directory()->count()) {
			tls_call_back_entry_ = AddCommand(value_command_type, 0, NEED_FIXUP);
			tls_call_back_entry_->AddLink(0, ltGateOffset);
		}
		for (i = 0; i < tls_directory->count(); i++) {
			AddCommand(icDword, tls_directory->item(i)->address(), NEED_FIXUP);
		}
		if (count() > index) {
			AddCommand(value_command_type, 0);
			call_back_entry->AddLink(0, ltOffset, item(index));
			call_back_entry->set_operand_fixup(NEED_FIXUP);
		}

		tls_entry_ = item(tls_index);
		tls_entry_->include_option(roCreateNewBlock);
		tls_entry_->set_alignment(OperandSizeToValue(cpu_address_size()));
		for (i = tls_index; i < count(); i++) {
			command = item(i);
			tls_size_ += OperandSizeToValue(command->type() == icDword ? osDWord : osQWord);
		}
	}

	// create watermarks
	AddWatermark(ctx.options.watermark, 2);

	// create section list for setting WRITABLE flag
	std::vector<PESegment *> writable_section_list;
	for (i = 0; i < new_import_list.count(); i++) {
		import = new_import_list.item(i);
		for (j = 0; j < import->count(); j++) {
			import_function = import->item(j);
			segment = file->segment_list()->GetSectionByAddress(import_function->address());
			if (!segment)
				continue;

			if (std::find(writable_section_list.begin(), writable_section_list.end(), segment) == writable_section_list.end())
				writable_section_list.push_back(segment);
		}
	}

	std::vector<PackerInfo> packer_info_list;
	PEFixupList loader_fixup_list;
	bool pack_resources = false;
	ILCommand *packer_props = NULL;
	vtable_fixups_entry_ = NULL;
	if (ctx.options.flags & cpPack) {
		std::set<PESegment *> skip_segment_list;
		if (file->header().VTableFixups.VirtualAddress) {
			uint64_t address = file->image_base() + file->header().VTableFixups.VirtualAddress;
			segment = file->segment_list()->GetSectionByAddress(address);
			if (segment && !segment->excluded_from_packing() && file->AddressSeek(address)) {
				index = count();

				j = file->header().VTableFixups.Size;
				for (i = 0; i < j; i += sizeof(IMAGE_COR_VTABLEFIXUP)) {
					IMAGE_COR_VTABLEFIXUP fixup;
					file->Read(&fixup, sizeof(fixup));

					AddCommand(icDword, fixup.RVA);
					AddCommand(icWord, fixup.Count);
					AddCommand(icWord, fixup.Type);

					if (PESegment *tmp = file->segment_list()->GetSectionByAddress(file->image_base() + fixup.RVA))
						skip_segment_list.insert(tmp);
				}

				if (index < count()) {
					vtable_fixups_entry_ = item(index);
					vtable_fixups_entry_->include_option(roCreateNewBlock);
					vtable_fixups_entry_->set_alignment(sizeof(uint64_t));
				}
			}
		}

		PackerInfo packer_info;
		for (i = 0; i < file->segment_list()->count(); i++) {
			segment = file->segment_list()->item(i);
			if (segment->excluded_from_packing())
				continue;

			bool can_be_packed = true;
			if ((segment->memory_type() & (mtWritable | mtShared)) == (mtWritable | mtShared)) {
				can_be_packed = false;
			} else if (skip_segment_list.find(segment) != skip_segment_list.end()) {
				can_be_packed = false;
			}

			if (!can_be_packed) {
				//file->Notify(mtWarning, NULL, string_format(language[lsSegmentCanNotBePacked].c_str(), segment->name().c_str()));
				continue;
			}

			if (segment->physical_size()) {
				packer_info.section = segment;
				packer_info.address = segment->address();
				packer_info.size = static_cast<size_t>(segment->physical_size());
				packer_info.data = NULL;
				packer_info_list.push_back(packer_info);

				// need add packed section into WRITABLE section list
				if (std::find(writable_section_list.begin(), writable_section_list.end(), segment) == writable_section_list.end())
					writable_section_list.push_back(segment);
			}
		}

		if ((ctx.options.flags & cpStripFixups) == 0) {
			PEFixupList *fixup_list = file->pe()->fixup_list();

			for (i = 0; i < fixup_list->count(); i++) {
				PEFixup *fixup = fixup_list->item(i);
				if (fixup->is_deleted())
					continue;

				segment = file->segment_list()->GetSectionByAddress(fixup->address());
				if (!segment || std::find(packer_info_list.begin(), packer_info_list.end(), segment) == packer_info_list.end())
					continue;

				loader_fixup_list.AddObject(fixup->Clone(&loader_fixup_list));
				fixup->set_deleted(true);

				// need add section into WRITABLE section list
				if (std::find(writable_section_list.begin(), writable_section_list.end(), segment) == writable_section_list.end())
					writable_section_list.push_back(segment);
			}
		}

		// packing sections
		j = 0;
		for (i = 0; i < packer_info_list.size(); i++) {
			j += packer_info_list[i].size;
		}
		/*
		if (file->resource_list()->size() > file->resource_list()->store_size())
			j += file->resource_list()->size() - file->resource_list()->store_size();
		*/
		file->StartProgress(string_format("%s...", language[lsPacking].c_str()), j);

		Data data;
		Packer packer;

		if (!packer.WriteProps(&data))
			throw std::runtime_error("Packer error");
		packer_props = AddCommand(data);
		packer_props->include_option(roCreateNewBlock);

		for (i = 0; i < packer_info_list.size(); i++) {
			packer_info = packer_info_list[i];
			if (!file->AddressSeek(packer_info.address))
				return false;

			if (!packer.Code(file, packer_info.size, &data))
				throw std::runtime_error("Packer error");

			command = AddCommand(data);
			command->include_option(roCreateNewBlock);
			packer_info_list[i].data = command;
		}

		// remove packed sections from file
		uint32_t physical_offset = 0;
		for (i = 0; i < file->segment_list()->count(); i++) {
			segment = file->segment_list()->item(i);
			if (segment->physical_offset() > 0 && segment->physical_size() > 0) {
				physical_offset = static_cast<uint32_t>(segment->physical_offset());
				break;
			}
		}

		for (i = 0; i < file->segment_list()->count(); i++) {
			segment = file->segment_list()->item(i);
			uint32_t physical_size = segment->physical_size();
			bool is_packed = false;
			std::vector<PackerInfo>::iterator it = std::find(packer_info_list.begin(), packer_info_list.end(), segment);
			if (it != packer_info_list.end()) {
				physical_size = static_cast<uint32_t>(it->address - segment->address());
				is_packed = true;
			}

			if (physical_size > 0 && segment->physical_offset() != physical_offset) {
				uint8_t *buff = new uint8_t[physical_size];
				file->Seek(segment->physical_offset());
				file->Read(buff, physical_size);
				file->Seek(physical_offset);
				file->Write(buff, physical_size);
				delete [] buff;
			}

			segment->set_physical_offset(physical_offset);
			segment->set_physical_size(physical_size);

			if (is_packed) {
				j = physical_offset + physical_size;
				file->Seek(j);
				physical_offset = (uint32_t)AlignValue(j, file->file_alignment());
				for (k = j; k < physical_offset; k++) {
					file->WriteByte(0);
				}
			} else {
				physical_offset += physical_size;
			}
		}
		file->Resize(physical_offset);
	}

	// create packer info for loader
	std::vector<LoaderInfo> loader_info_list;
	index = count();
	if (packer_props) {
		command = AddCommand(icDword, 0);
		link = command->AddLink(0, ltOffset, packer_props);
		link->set_sub_value(file->image_base());
		AddCommand(icDword, packer_props->dump_size());

		for (i = 0; i < packer_info_list.size(); i++) {
			command = AddCommand(icDword, 0);
			link = command->AddLink(0, ltOffset, packer_info_list[i].data);
			link->set_sub_value(file->image_base());

			AddCommand(icDword, packer_info_list[i].address - file->image_base());
		}
	}
	command = (count() == index) ? NULL : item(index);
	if (command)
		command->include_option(roCreateNewBlock);
	loader_info_list.push_back(LoaderInfo(command, (count() - index) * OperandSizeToValue(osDWord)));

	// create file CRC info for loader
	index = count();
	if (((ctx.options.flags | ctx.options.sdk_flags) & cpMemoryProtection)) {
		AddCommand(icDword, 0);
		for (i = 0; i < 10; i++) {
			AddCommand(icDword, 0);
			AddCommand(icDword, 0);
			AddCommand(icDword, 0);
		}
	}
	file_crc_entry_ = (count() == index) ? NULL : item(index);
	if (file_crc_entry_)
		file_crc_entry_->include_option(roCreateNewBlock);
	file_crc_size_ = static_cast<uint32_t>((count() - index) * OperandSizeToValue(osDWord));
	loader_info_list.push_back(LoaderInfo(file_crc_entry_, file_crc_size_));

	file_crc_size_entry_ = file_crc_entry_ ? AddCommand(icDword, 0) : NULL;
	if (file_crc_size_entry_)
		file_crc_size_entry_->include_option(roCreateNewBlock);

	// create header and loader CRC info for loader
	index = count();
	if (((ctx.options.flags | ctx.options.sdk_flags) & cpMemoryProtection) || (ctx.options.flags & cpLoaderCRC)) {
		// calc CRC blocks count
		k = 20 + new_import_list.count();
		if ((ctx.options.flags & cpStripFixups) == 0) {
			PEFixupList *fixup_list = file->pe()->fixup_list();
			for (i = 0; i < fixup_list->count(); i++) {
				PEFixup *fixup = fixup_list->item(i);
				if (!fixup->is_deleted())
					k++;
			}
		}
		for (i = 0; i < k; i++) {
			AddCommand(icDword, 0);
			AddCommand(icDword, 0);
			AddCommand(icDword, 0);
		}
	}
	loader_crc_entry_ = (count() == index) ? NULL : item(index);
	if (loader_crc_entry_)
		loader_crc_entry_->include_option(roCreateNewBlock);
	loader_crc_size_ = static_cast<uint32_t>((count() - index) * OperandSizeToValue(osDWord));
	loader_info_list.push_back(LoaderInfo(loader_crc_entry_, loader_crc_size_));

	loader_crc_size_entry_ = loader_crc_entry_ ? AddCommand(icDword, 0) : NULL;
	if (loader_crc_size_entry_)
		loader_crc_size_entry_->include_option(roCreateNewBlock);
	loader_crc_hash_entry_ = loader_crc_entry_ ? AddCommand(icDword, 0) : NULL;
	if (loader_crc_hash_entry_)
		loader_crc_hash_entry_->include_option(roCreateNewBlock);

	// create section info for loader
	index = count();
	for (i = 0; i < writable_section_list.size(); i++) {
		segment = writable_section_list[i];

		AddCommand(icDword, segment->address() - file->image_base());
		AddCommand(icDword, segment->size());
		AddCommand(icDword, segment->flags());
	}
	command = (count() == index) ? NULL : item(index);
	if (command)
		command->include_option(roCreateNewBlock);
	loader_info_list.push_back(LoaderInfo(command, (count() - index) * OperandSizeToValue(osDWord)));

	// create fixup info for loader
	if (loader_fixup_list.count() > 0) {
		Data data;
		loader_fixup_list.WriteToData(data, file->image_base());
		command = AddCommand(data);
	} else {
		command = NULL;
	}
	if (command)
		command->include_option(roCreateNewBlock);
	loader_info_list.push_back(LoaderInfo(command, (command) ? command->dump_size() : 0));

	// create relocation info for loader
	loader_info_list.push_back(LoaderInfo(NULL, 0));

	// create IAT info for loader
	index = count();
	for (i = 0, import_index = 0; i < new_import_list.count(); i++) {
		import = new_import_list.item(i);
		if (import->count() == 0)
			continue;

		import_function = import->item(0);
		command = AddCommand(icDword, 0);
		link = command->AddLink(0, ltOffset, import_function_info_list[import_index].thunk);
		link->set_sub_value(file->image_base());

		AddCommand(icDword, import_function->address() - file->image_base());
		AddCommand(icDword, import->count() * OperandSizeToValue(file->pe()->cpu_address_size()));

		import_index += import->count();
	}
	command = (count() == index) ? NULL : item(index);
	if (command)
		command->include_option(roCreateNewBlock);
	loader_info_list.push_back(LoaderInfo(command, (count() - index) * OperandSizeToValue(osDWord)));

	index = count();
	if (il_import) {
		std::vector<ImportDelegateInfo> info_list = il_import->info_list();
		for (i = 0; i < info_list.size(); i++) {
			ImportDelegateInfo info = info_list[i];
			AddCommand(icDword, info.method->id() ^ string_key);
			AddCommand(icDword, info.invoke->id() ^ string_key);
			AddCommand(icDword, info.call_type);
		}
	}
	command = (count() == index) ? NULL : item(index);
	if (command)
		command->include_option(roCreateNewBlock);
	loader_info_list.push_back(LoaderInfo(command, (count() - index) * OperandSizeToValue(osDWord)));

	loader_info_list.push_back(LoaderInfo(NULL, 0));

	// create memory CRC info for loader
	if (il_crc) {
		command = il_crc->table_entry();
		i = static_cast<size_t>(il_crc->size_entry()->operand_value());
	} else {
		command = NULL;
		i = 0;
	}
	loader_info_list.push_back(LoaderInfo(command, i));

	// create strings for loader
	std::map<uint32_t, ILCommand *> loader_string_list;
	loader_string_list[FACE_FILE_CORRUPTED] = AddCommand(EncryptString((ctx.options.flags & cpMemoryProtection) ? os::FromUTF8(ctx.options.messages[MESSAGE_FILE_CORRUPTED]).c_str() : os::unicode_string().c_str(), string_key));
	loader_string_list[FACE_DEBUGGER_FOUND] = AddCommand(EncryptString(os::FromUTF8(ctx.options.messages[MESSAGE_DEBUGGER_FOUND]).c_str(), string_key));
	loader_string_list[FACE_VIRTUAL_MACHINE_FOUND] = AddCommand(EncryptString(os::FromUTF8(ctx.options.messages[MESSAGE_VIRTUAL_MACHINE_FOUND]).c_str(), string_key));
	loader_string_list[FACE_INITIALIZATION_ERROR] = AddCommand(EncryptString(os::FromUTF8("Initialization error {0}").c_str(), string_key));
	VMProtectBeginVirtualization("Loader Strings");
	loader_string_list[FACE_UNREGISTERED_VERSION] = AddCommand(EncryptString(
#ifdef DEMO
		true
#else
		(ctx.options.flags & cpUnregisteredVersion) 
#endif
		? os::FromUTF8(VMProtectDecryptStringA("This application is protected with unregistered version of VMProtect.")).c_str() : os::unicode_string().c_str(), string_key));
	VMProtectEnd();
	loader_string_list[FACE_NTDLL_NAME] = AddCommand(EncryptString("ntdll.dll", string_key));
	loader_string_list[FACE_NT_SET_INFORMATION_THREAD_NAME] = AddCommand(EncryptString("NtSetInformationThread", string_key));
	loader_string_list[FACE_NT_QUERY_INFORMATION_PROCESS_NAME] = AddCommand(EncryptString("NtQueryInformationProcess", string_key));
	loader_string_list[FACE_NT_CLOSE] = AddCommand(EncryptString("NtClose", string_key));
	loader_string_list[FACE_NT_VIRTUAL_PROTECT_NAME] = AddCommand(EncryptString("NtProtectVirtualMemory", string_key));
	loader_string_list[FACE_NT_ALLOCATE_VIRTUAL_MEMORY_NAME] = AddCommand(EncryptString("NtAllocateVirtualMemory", string_key));
	loader_string_list[FACE_NT_FREE_VIRTUAL_MEMORY_NAME] = AddCommand(EncryptString("NtFreeVirtualMemory", string_key));
	loader_string_list[FACE_NT_UNMAP_VIEW_OF_SECTION] = AddCommand(EncryptString("NtUnmapViewOfSection", string_key));
	loader_string_list[FACE_NT_OPEN_FILE_NAME] = AddCommand(EncryptString("NtOpenFile", string_key));
	loader_string_list[FACE_NT_RAISE_HARD_ERROR_NAME] = AddCommand(EncryptString("NtRaiseHardError", string_key));
	loader_string_list[FACE_NT_CREATE_SECTION_NAME] = AddCommand(EncryptString("NtCreateSection", string_key));
	loader_string_list[FACE_KERNEL32_NAME] = AddCommand(EncryptString("kernel32.dll", string_key));
	loader_string_list[FACE_ENUM_SYSTEM_FIRMWARE_NAME] = AddCommand(EncryptString("EnumSystemFirmwareTables", string_key));
	loader_string_list[FACE_GET_SYSTEM_FIRMWARE_NAME] = AddCommand(EncryptString("GetSystemFirmwareTable", string_key));
	loader_string_list[FACE_NT_MAP_VIEW_OF_SECTION] = AddCommand(EncryptString("MapViewOfFile", string_key));

	for (std::map<uint32_t, ILCommand *>::const_iterator it = loader_string_list.begin(); it != loader_string_list.end(); it++) {
		it->second->include_option(roCreateNewBlock);
	}

	// append loader
	old_count = count();
	runtime_function_list = reinterpret_cast<ILFunctionList *>(runtime->function_list());
	for (size_t n = 0; n < 2; n++) {
		for (i = 0; i < runtime_function_list->count(); i++) {
			func = runtime_function_list->item(i);
			if (func->tag() != ftLoader)
				continue;

			if (func->compilation_type() == ctMutation) {
				if (n != 0)
					continue;
			} else {
				if (n != 1)
					continue;
			}

			func->Init(ctx);

			size_t orig_function_info_count = function_info_list()->count();
			for (j = 0; j < func->function_info_list()->count(); j++) {
				FunctionInfo *info = func->function_info_list()->item(j);
				function_info_list()->AddObject(info->Clone(function_info_list()));
			}
			for (j = 0; j < func->range_list()->count(); j++) {
				AddressRange *range = func->range_list()->item(j);
				range_list()->AddObject(range->Clone(range_list()));
			}

			for (j = 0; j < func->count(); j++) {
				src_command = func->item(j);
				dst_command = src_command->Clone(this);

				AddressRange *address_range = src_command->address_range();
				if (address_range) {
					FunctionInfo *info = function_info_list()->item(orig_function_info_count + func->function_info_list()->IndexOf(address_range->owner()));
					dst_command->set_address_range(info->item(address_range->owner()->IndexOf(address_range)));
				}

				AddObject(dst_command);

				src_link = src_command->link();
				if (src_link) {
					dst_link = src_link->Clone(link_list());
					dst_link->set_from_command(dst_command);
					link_list()->AddObject(dst_link);
				
					if (src_link->parent_command())
						dst_link->set_parent_command(GetCommandByAddress(src_link->parent_command()->address()));
					if (src_link->base_function_info())
						dst_link->set_base_function_info(function_info_list()->GetItemByAddress(src_link->base_function_info()->begin()));
				}

				command = dst_command;
				if (command->type() == icLdc_i4) {
					uint32_t value = static_cast<uint32_t>(command->operand_value());
					if ((value & 0xFFFF0000) == 0xFACE0000) {
						switch (value) {
						case FACE_LOADER_OPTIONS:
							value = 0;
							if (ctx.options.flags & cpMemoryProtection)
								value |= LOADER_OPTION_CHECK_PATCH;
							if (ctx.options.flags & cpCheckDebugger)
								value |= LOADER_OPTION_CHECK_DEBUGGER;
							if (ctx.options.flags & cpCheckKernelDebugger)
								value |= LOADER_OPTION_CHECK_KERNEL_DEBUGGER;
							if (ctx.options.flags & cpCheckVirtualMachine)
								value |= LOADER_OPTION_CHECK_VIRTUAL_MACHINE;
							command->set_operand_value(0, value);
							command->CompileToNative();
							break;
						case FACE_STRING_DECRYPT_KEY:
							command->set_operand_value(0, string_key);
							command->CompileToNative();
							break;
						case FACE_PACKER_INFO:
						case FACE_FILE_CRC_INFO:
						case FACE_LOADER_CRC_INFO:
						case FACE_SECTION_INFO:
						case FACE_FIXUP_INFO:
						case FACE_RELOCATION_INFO:
						case FACE_IAT_INFO:
						case FACE_IMPORT_INFO:
						case FACE_INTERNAL_IMPORT_INFO:
						case FACE_MEMORY_CRC_INFO:
						case FACE_DELAY_IMPORT_INFO:
							dst_command = loader_info_list[(value & 0xff) >> 1].data;
							if (dst_command) {
								link = command->AddLink(0, ltOffset, dst_command);
								link->set_sub_value(file->image_base());
							} else {
								command->set_operand_value(0, 0);
								command->CompileToNative();
							}
							break;
						case FACE_PACKER_INFO_SIZE:
						case FACE_SECTION_INFO_SIZE:
						case FACE_FIXUP_INFO_SIZE:
						case FACE_RELOCATION_INFO_SIZE:
						case FACE_IAT_INFO_SIZE:
						case FACE_IMPORT_INFO_SIZE:
						case FACE_INTERNAL_IMPORT_INFO_SIZE:
						case FACE_MEMORY_CRC_INFO_SIZE:
						case FACE_DELAY_IMPORT_INFO_SIZE:
							command->set_operand_value(0, loader_info_list[(value & 0xff) >> 1].size);
							command->CompileToNative();
							break;
						case FACE_LOADER_CRC_INFO_SIZE:
							if (loader_crc_size_entry_) {
								link = command->AddLink(0, ltOffset, loader_crc_size_entry_);
								link->set_sub_value(file->image_base());
							} else {
								command->set_operand_value(0, 0);
								command->CompileToNative();
							}
							break;
						case FACE_LOADER_CRC_INFO_HASH:
							if (loader_crc_hash_entry_) {
								link = command->AddLink(0, ltOffset, loader_crc_hash_entry_);
								link->set_sub_value(file->image_base());
							} else {
								command->set_operand_value(0, 0);
								command->CompileToNative();
							}
							break;
						case FACE_FILE_CRC_INFO_SIZE:
							if (file_crc_size_entry_) {
								link = command->AddLink(0, ltOffset, file_crc_size_entry_);
								link->set_sub_value(file->image_base());
							} else {
								command->set_operand_value(0, 0);
								command->CompileToNative();
							}
							break;
						case FACE_MEMORY_CRC_INFO_HASH:
							command->set_operand_value(0, il_crc ? il_crc->hash_entry()->operand_value() : 0);
							command->CompileToNative();
							break;
						case FACE_CRC_INFO_SALT:
							command->set_operand_value(0, file->function_list()->crc_cryptor()->item(0)->value());
							command->CompileToNative();
							break;
						case FACE_FILE_BASE:
							command->Init(icLdc_i8, file->image_base());
							command->CompileToNative();
							break;
						case FACE_TLS_INDEX_INFO:
							command->set_operand_value(0, tls_index_address ? tls_index_address - file->image_base() : 0);
							command->CompileToNative();
							break;
						case FACE_VAR_IS_PATCH_DETECTED:
						case FACE_VAR_IS_DEBUGGER_DETECTED:
						case FACE_VAR_LOADER_CRC_INFO:
						case FACE_VAR_LOADER_CRC_INFO_SIZE:
						case FACE_VAR_LOADER_CRC_INFO_HASH:
						case FACE_VAR_SESSION_KEY:
						case FACE_VAR_LOADER_STATUS:
						case FACE_VAR_SERVER_DATE:
							command->set_operand_value(j, ctx.runtime_var_index[(value & 0xff) >> 4]);
							command->CompileToNative();
							break;
						case FACE_VAR_IS_PATCH_DETECTED_SALT:
						case FACE_VAR_IS_DEBUGGER_DETECTED_SALT:
						case FACE_VAR_LOADER_CRC_INFO_SALT:
						case FACE_VAR_LOADER_CRC_INFO_SIZE_SALT:
						case FACE_VAR_LOADER_CRC_INFO_HASH_SALT:
						case FACE_VAR_SERVER_DATE_SALT:
							command->set_operand_value(j, ctx.runtime_var_salt[value & 0xff]);
							command->CompileToNative();
							break;
						default:
							std::map<uint32_t, ILCommand *>::const_iterator it = loader_string_list.find(value);
							if (it != loader_string_list.end()) {
								link = command->AddLink(0, ltOffset, it->second); 
								link->set_sub_value(file->image_base());
							} else {
								throw std::runtime_error(string_format("Unknown loader string: %X", value));
							}
						}
					}
				}
			}
		}
		if (n == 0) {
			// create native blocks
			for (j = 0; j < count(); j++) {
				item(j)->include_option(roNoProgress);
			}
			CompileToNative(ctx);
			for (j = 0; j < count(); j++) {
				item(j)->exclude_option(roNoProgress);
			}
		}
	}
	for (i = 0; i < function_info_list()->count(); i++) {
		FunctionInfo *info = function_info_list()->item(i);
		if (info->entry())
			info->set_entry(GetCommandByAddress(info->entry()->address()));
		if (info->data_entry())
			info->set_data_entry(GetCommandByAddress(info->data_entry()->address()));
		for (j = 0; j < info->count(); j++) {
			AddressRange *dest = info->item(j);
			for (k = 0; k < range_list()->count(); k++) {
				AddressRange *range = range_list()->item(k);
				if (range->begin() <= dest->begin() && range->end() > dest->begin())
					dest->AddLink(range);
			}
		}
	}
	for (i = 0; i < range_list()->count(); i++) {
		AddressRange *range = range_list()->item(i);
		if (range->begin_entry())
			range->set_begin_entry(GetCommandByAddress(range->begin_entry()->address()));
		if (range->end_entry())
			range->set_end_entry(GetCommandByAddress(range->end_entry()->address()));
		if (range->size_entry())
			range->set_size_entry(GetCommandByAddress(range->size_entry()->address()));
	}
	for (i = old_count; i < count(); i++) {
		command = item(i);
		dst_link = command->link();
		if (dst_link) {
			if (dst_link->to_address())
				dst_link->set_to_command(GetCommandByAddress(dst_link->to_address()));
		}
	}

	// create code for <Module>::.cctor()
	old_count = count();
	command = AddCommand(icComment, 0);
	Data data;
	data.PushByte(0);
	command->set_dump(data.data(), data.size());
	command->set_alignment(4);
	command = AddCommand(icCall, 0);
	command->AddLink(0, ltCall, GetCommandByAddress(runtime->export_list()->GetAddressByType(atSetupImage)));
	// search user <Module>::.cctor
	ILMethodDef *module_cctor = runtime->entry_point_method();
	ILTypeDef *module = module_cctor->declaring_type();
	ILTable *table = file->command_list()->table(ttMethodDef);
	for (i = table->IndexOf(module->method_list()); i < table->count(); i++) {
		ILMethodDef *method = reinterpret_cast<ILMethodDef *>(table->item(i));
		if (method->declaring_type() != module || method == module_cctor)
			break;

		if (method->name() == ".cctor") {
			method->set_flags((CorMethodAttr)(method->flags() & ~(mdSpecialName | mdRTSpecialName)));
			file->RenameToken(method);
			command = AddCommand(icCall, method->id());
			break;
		}
	}
	AddCommand(icRet, 0);
	FunctionInfo *info = function_info_list()->Add(0, 0, btValue, 1, 0, 0, file->runtime_function_list()->Add(0, 0, 0, runtime->entry_point_method()), item(old_count));
	AddressRange *range = info->Add(0, 0, NULL, NULL, NULL);
	for (i = old_count; i < count(); i++) {
		item(i)->set_address_range(range);
	}

	for (i = 0; i < count(); i++) {
		command = item(i);
		command->CompileToNative();
	}

	// prepare ranges
	std::vector<IFunction *> function_list = ctx.file->function_list()->processor_list();
	function_list.push_back(this);
	for (i = 0; i < function_list.size(); i++) {
		ILFunction *func = reinterpret_cast<ILFunction *>(function_list[i]);
		func->range_list()->Prepare();
		func->function_info_list()->Prepare();
	}

	PrepareExtCommands(ctx);
	for (i = 0; i < link_list()->count(); i++) {
		CommandLink *link = link_list()->item(i);
		link->from_command()->PrepareLink(ctx);
	}
	return true;
}

bool NETLoader::Compile(const CompileContext &ctx)
{
	size_t i, j;
	std::vector<CommandBlock*> block_list;
	ILCommand *command;

	std::vector<IFunction *> function_list = ctx.file->function_list()->processor_list();
	for (i = 0; i < function_list.size(); i++) {
		function_list[i]->set_need_compile(true);
	}
	function_list.push_back(this);

	// update tokens
	for (i = 0; i < function_list.size(); i++) {
		ILFunction *func = reinterpret_cast<ILFunction *>(function_list[i]);
		for (j = 0; j < func->count(); j++) {
			command = func->item(j);
			TokenReference *token_reference = command->token_reference();
			if (!token_reference)
				continue;

			uint32_t id = token_reference->owner()->owner()->id();
			if (command->type() == icComment) {
				Data data;
				data.PushDWord(id);
				command->set_dump(data.data(), data.size());
			}
			else {
				command->set_operand_value(0, id);
				command->CompileToNative();
			}
		}
	}

	if (!ILFunction::Compile(ctx))
		return false;
	ILFunction::AfterCompile(ctx);

	for (i = 0; i < count(); i++) {
		command = item(i);
		if (!command->block() || (command->block()->type() & mtExecutable) == 0 || !command->token_reference())
			continue;

		ILToken *token = command->token_reference()->owner()->owner();
		if (token->type() == ttStandAloneSig) {
			ILTable *table = token->owner();
			uint32_t id = token->type() | 1;
			for (j = 0; j < table->count(); j++) {
				ILToken *tmp = table->item(j);
				if (tmp == token)
					break;
				if (!tmp->is_deleted())
					id++;
			}
			if (token->id() != id) {
				token->set_value(TOKEN_VALUE(id));
				command->set_operand_value(0, id);
				command->CompileToNative();
			}
		}
	}

	for (i = 0; i < function_list.size(); i++) {
		IFunction *func = function_list[i];
		for (j = 0; j < func->block_list()->count(); j++) {
			block_list.push_back(func->block_list()->item(j));
		}
	}

	for (i = 0; i < block_list.size(); i++) {
		std::swap(block_list[i], block_list[rand() % block_list.size()]);
	}

	if (ctx.file->runtime_function_list() && ctx.file->runtime_function_list()->count()) {
		// sort blocks by address range
		std::sort(block_list.begin(), block_list.end(), CommandBlockListCompareHelper());
	}

	for (i = 0; i < block_list.size(); i++) {
		block_list[i]->Compile(*ctx.manager);
	}

	for (i = 0; i < function_list.size(); i++) {
		function_list[i]->CompileInfo(ctx);
	}

	for (i = 0; i < function_list.size(); i++) {
		function_list[i]->CompileLinks(ctx);
	}

	return true;
}

size_t NETLoader::WriteToFile(IArchitecture &file)
{
	size_t res = BaseFunction::WriteToFile(file);

	if (pe_entry_) {
		PEArchitecture *pe = reinterpret_cast<NETArchitecture &>(file).pe();
		if (pe->cpu_address_size() == osDWord) {
			IFixup *fixup = pe->fixup_list()->AddDefault(osDWord, true);
			fixup->set_address(pe_entry_->address() + pe_entry_->dump_size());
		}
	}

	return res;
}