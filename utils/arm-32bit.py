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
#	if prev_mnem != mnem:
		# Opcode changed, purge cache set
#		set.clear()
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
	if n % 10000 == 0:
		print '%d opcodes processed' % n
	return (prev_mnem, n)

def generate_arm32(filename):
	set = Set()
	ea = GetEntryPoint(GetEntryOrdinal(0))
	for i in range(0, 20):
		PatchByte(ea + i, i)
	f = open(filename, 'wt')
	n = 0
	data = 0L
	mnem = ''
	# Do not enumerate highest 4 bits.
	# Use only 0b1110 (AL) and 0b1111 (extended opcode encoding)
	while data <= 0x1fffffff:
		data2 = (data & 0xfffffff)
		if (data & 0x10000000) == 0:
			data2 |= 0xe0000000
		else:
			data2 |= 0xf0000000
		(mnem, n) = iteration(f, set, ea, n, data2, mnem)
		data += 1
	f.close()
	print 'Finished'

def main():
	generate_arm32("./arm32-opcodes.txt")

if __name__ == "__main__":
	main()


