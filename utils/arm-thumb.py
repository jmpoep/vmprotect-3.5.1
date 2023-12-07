from idc import *
import sys
import binascii
from sets import Set
import sys
import re

p_spaces = re.compile(r'(\s+)')

# Apply fixes to IDA opcode
def ida_disasm_fix(insn_binary, insn_str):
	# Remove extra spaces and tabs. Replace tabs with spaces
	insn_str = p_spaces.sub(r' ', insn_str)
	return insn_str

def get_insn(ea, sz):
	s = ''
	for i in range(0, sz):
		s += chr(Byte(ea + i))
	return s

def insn_write(f, insn_binary, insn_str, header):
	assert len(insn_binary) != 0
	s = ''
	sz = len(insn_binary)
	if header:
		s += '{%d, "' % sz
		for i in range(0, sz):
			s += '\\x%02x' % ord(insn_binary[i])
		s += '", "' + insn_str + '"},\n'
	else:
		s += binascii.hexlify(insn_binary)
		s += (' %s\n' % insn_str)
	f.write(s)
	f.flush()

# Normalize operand (replace numeric operands with -1)
def normalize_operand(op_type, op_str):
	if op_type in [o_mem, o_displ, o_imm, o_near, o_far]:
		return "-1"
	else:
		return op_str

def is_unique(set, ea):
	ot1 = GetOpType(ea, 0)
	ot2 = GetOpType(ea, 1)
	ot3 = GetOpType(ea, 2)
	v1 = GetOpnd(ea, 0)
	v2 = GetOpnd(ea, 1)
	v3 = GetOpnd(ea, 2)
	mnem = GetMnem(ea)
	hashstr = "%s|%s|%s|%s" % (mnem, 
			normalize_operand(ot1, v1),
			normalize_operand(ot2, v2),
			normalize_operand(ot3, v3))
	if hashstr in set:
		return False
	else:
		set.add(hashstr)
		return True

def iteration(f, set, ea, n, data, prev_mnem):
	PatchDword(ea, data)
	sz = MakeCode(ea)
	if sz == 0:
		return (prev_mnem, n)
	str = GetDisasm(ea)
	mnem = GetMnem(ea)
	if prev_mnem != mnem:
		# Opcode changed, purge cache set
		set.clear()
	prev_mnem = mnem
	# Now we got disasm
	# Remove comments
	pos = str.find(';')
	if pos != -1:
		str = str[0:pos]
	# Remove spaces at start and end
	str = str.strip(' ')
	if str == '':
		return (prev_mnem, n)
	if not is_unique(set, ea):
		return (prev_mnem, n)
	insn_binary = get_insn(ea, sz)
	# Add unique disasms to file
	str = ida_disasm_fix(insn_binary, str)
	insn_write(f, insn_binary, str, False)
	n += 1
	if n % 1000 == 0:
		print '%d opcodes processed' % n
	return (prev_mnem, n)

def generate_arm_thumb(filename):
	set = Set()
	ea = GetEntryPoint(GetEntryOrdinal(0))
	for i in range(0, 20):
		PatchByte(ea + i, i)
	f = open(filename, 'wt')
	data = 0L
	mnem = ''
	data = 0
	n = 0
	while data <= 0xffff:
		prefix = data >> (32 - 5)
		if prefix in [0x1d, 0x1e, 0x1f]:
			data2 = 0
			while data2 <= 0xffff:
				(mnem, n) = iteration(f, set, ea, n, data | (data2 << 16), mnem)
				data2 += 1
		else:
			mnem = iteration(f, set, ea, n, data | 0xffff0000, mnem)
		data += 1
	f.close()
	print 'Finished'

def main():
	generate_arm_thumb("./thumb-opcodes.txt")

if __name__ == "__main__":
	main()


