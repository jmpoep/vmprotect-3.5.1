from idc import *
import sys
import binascii
from sets import Set
import sys
import re

def replace_farptr(matchobj):
	s1 = matchobj.group()
	return s1.replace('far ptr ', '')

def replace_ptr(matchobj):
	s1 = matchobj.group()
	pos = s1.rfind(' ')
	assert pos != -1
	return s1[0:pos] + ' [' + s1[pos+1:] + ']'

def replace_ptr2(matchobj):
	s1 = matchobj.group()
	pos = s1.rfind(':')
	assert pos != -1
	return s1[0:pos+1] + '[' + s1[pos+1:] + ']'

def replace_hex(matchobj):
	nstr = matchobj.group()
	assert not (nstr in ['ah', 'bh', 'ch', 'dh'])
	nstr = nstr.replace('h', '')
	return nstr

def repl(matchobj):
	s1 = matchobj.group()
	s2 = s1.replace(' ', '')
	pos1 = s2.find('[')
	pos2 = s2.find(']')
	if pos1 == -1 or pos2 == -1:
		return s1
	nstr = s2[0:pos1]
	nstr = nstr.replace('h', '')
	num = int(nstr, 16)
	hex = "%08x" % num
	s3 = s2[pos1:pos2] + '+' + hex + ']'
	return s3

p_seg  = re.compile(r'(es:|ds:|cs:|fs:|gs:|ss:)')
p_seg_abs = re.compile(r'(es:|ds:|cs:|fs:|gs:|ss:)[0-9][0-9a-fA-F]*h')
p_farptr = re.compile(r'far\sptr\s[0-9a-fA-F]+:[0-9a-fA-F]+')
p_ptr = re.compile(r'(byte|word|dword|qword|oword|fword)\sptr\s[0-9][0-9a-fA-F]*')
p_ptr2 = re.compile(r'(byte|word|dword|qword|oword|fword)\sptr\s(es:|ds:|cs:|fs:|gs:|ss:)[0-9][0-9a-fA-F]*')
p_hex = re.compile(r'([0-9][0-9a-fA-F]*)(h|H)')
p_spaces = re.compile(r'(\s+)')
p_repl = re.compile(r'[0-9][0-9a-fA-F]*(h|H)\s*\[[^\]]+\]')

replacements = [('retn', 'ret'), ('retnw', 'ret'), ('iretw', 'iret'), ('retfw', 'retf'),
				('pushfw', 'pushf'), ('popfw', 'popf'), ('pushaw', 'pusha'),
				('popaw', 'popa'), ('enterw', 'enter'), ('enterw', 'enter'),
				('cmova', 'cmovnbe'), ('cmovg', 'cmovnle'), ('cmovge', 'cmovnl'),
				('leavew', 'leave'),
				('int 3', 'int 03')]

def is_invalid_insn(insn_binary):
	k = 0
	while True:
		b = ord(insn_binary[k])
		if not(b == 0x26 or b == 0x2e or b == 0x36 or b == 0x3e or b == 0x64 or b == 0x65):
			break
		k += 1
	if k >= len(insn_binary) - 1:
		return True
	b = ord(insn_binary[k])
	b2 = ord(insn_binary[k + 1])
	if b == 0x0f and (b2 == 0x19 or b2 == 0x24 or b2 == 0x26 or b2 == 0xa6 or b2 == 0xa7):
		return True
	if b == 0xcd and b2 == 0x20: #vxdcall
		return True
	if b == 0xd6: #setalc
		return True
	if b == 0x0f and b2 == 0x0d:
		if k > len(insn_binary) - 2:
			return True
		if ord(insn_binary[k + 2]) == 0x13:
			return True
	return False

# Miscellaneous replacements
def misc_replacements(opcode_str):
	for r_from, r_to in replacements:
		if opcode_str == r_from:
			opcode_str = r_to
	return opcode_str

def remove_ds_prefix(insn_binary, rest_str):
	pos = insn_binary.find('\x3e')
	# No prefix - remove ds:
	if -1 == pos:
		return -1 != rest_str.find('ds:')
	if pos == 0:
		return True
	ds_prefix = True
	for i in range(0, pos):
		if not (ord(insn_binary[i]) in [0x66, 0x67, 0xF0, 0xF2, 0xF3]):
			ds_prefix = False
			break
	if ds_prefix:
		return True
	else:
		# No prefix - remove ds
		return -1 != rest_str.find('ds:')

def replace_ds_seg(matchobj):
	nstr = matchobj.group()
	if nstr[0:2].lower() == 'ds':
		return '[' + nstr[3:] + ']'
	else:
		return nstr[0:3] + '[' + nstr[3:] + ']'

def replace_seg(matchobj):
	nstr = matchobj.group()
	if nstr[0:2].lower() == 'ds':
		return '[' + nstr[3:] + ']'
	else:
		return nstr[0:3] + '[' + nstr[3:] + ']'

def replace_lea_seg(matchobj):
	nstr = matchobj.group()
	return '[' + nstr[3:] + ']'

def replace_segments(insn_binary, opcode_str, rest_str):
	# Remove segments from LEA (for absolute and relative offsets)
	if opcode_str.lower() == 'lea':
		tmp = p_seg_abs.sub(replace_lea_seg, rest_str)
		if tmp == rest_str:
			return p_seg.sub('', rest_str)
		else:
			return tmp
	# Now search for ?s:01020304, replace to ?s:[01020304] except of ds: -> [01020304]
	if remove_ds_prefix(insn_binary, rest_str):
		return p_seg_abs.sub(replace_ds_seg, rest_str)
	else:
		return p_seg_abs.sub(replace_seg, rest_str)

# Apply fixes to IDA opcode
def ida_disasm_fix(insn_binary, insn_str):
	# Remove extra spaces and tabs. Replace tabs with spaces
	insn_str = p_spaces.sub(r' ', insn_str)
	# Avoid opcode changing
	pos = insn_str.find(' ')
	if pos == -1:
		return misc_replacements(insn_str) # This is opcode like 'cli'
	opcode_str = insn_str[0:pos]
	rest_str = insn_str[pos+1:]
	# remove 'small'
	rest_str = rest_str.replace('small ', '')
	# Transform '6050403[eax], al' to '[eax+6050403], al'
	rest_str = p_repl.sub(repl, rest_str)
	rest_str = replace_segments(insn_binary, opcode_str, rest_str)
	# Remove 'ds:' if no 3Eh prefix found
	if remove_ds_prefix(insn_binary, rest_str):
		rest_str = rest_str.replace('ds:', '')
	# Replace 'xmmword' to 'oword'
	rest_str = rest_str.replace('xmmword', 'oword')
	# Remove 'h' after hex constants
	rest_str = p_hex.sub(replace_hex, rest_str)
	# Transform 'ptr 012345' -> 'ptr [012345]'
	rest_str = p_ptr.sub(replace_ptr, rest_str)
	rest_str = p_ptr2.sub(replace_ptr2, rest_str)
	# Transform 'call far ptr 1817:16151413' -> 'call 1817:16151413'
	rest_str = p_farptr.sub(replace_farptr, rest_str)
	opcode_str = misc_replacements(opcode_str)
	return opcode_str + ' ' + rest_str

def get_insn(ea, len):
	s = ''
	for i in range(0, len):
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

def is_unique(set, ea, insn_str, mnem):
	# Consider undisassemblable opcodes as unique
	if insn_str[0:2].lower() == 'db':
		return True
	ot1 = GetOpType(ea, 0)
	ot2 = GetOpType(ea, 1)
	ot3 = GetOpType(ea, 2)
	v1 = GetOpnd(ea, 0)
	v2 = GetOpnd(ea, 1)
	v3 = GetOpnd(ea, 2)
	hashstr = "%s|%s|%s|%s" % (mnem, 
			normalize_operand(ot1, v1),
			normalize_operand(ot2, v2),
			normalize_operand(ot3, v3))
	if hashstr in set:
		return False
	else:
		set.add(hashstr)
		return True

def generate_x86(filename):
	set = Set()
	ea = GetEntryPoint(GetEntryOrdinal(0))
	for i in range(0, 20):
		PatchByte(ea + i, i + 0x10)
	flog = open(filename + '.log', 'wt')
	f = open(filename, 'wt')
	n = 0
	len = 0
	for p0 in range(0x10, 0x110):
		q0 = p0 & 0xff
		set.clear() # Clear cached of opcodes (they become unrelevant)
		PatchByte(ea, q0)
		for p1 in range(0x10, 0x110):
			q1 = p1 & 0xff
			PatchByte(ea + 1, q1)
			for p2 in range(0x10, 0x110):
				q2 = p2 & 0xff
				PatchByte(ea + 2, q2)
				len = MakeCode(ea)
				str = GetDisasm(ea)
				mnem = GetMnem(ea)
				# Now we got disasm
				# Remove comments
				pos = str.find(';')
				if pos != -1:
					str = str[0:pos]
				# Remove spaces at start and end
				str = str.strip(' ')
				if str[0:2] == 'db' or str == '' or len == 0:
					insn_binary = get_insn(ea, 10)
					flog.write('INPUT hex: %20s; disasm: "%s"\n' % (binascii.hexlify(insn_binary), str))
					insn_write(f, insn_binary, 'db', False)
					flog.write('\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tOUTPUT disasm: "db"\n')
					continue
				if not is_unique(set, ea, str, mnem):
					continue
				insn_binary = get_insn(ea, len)
				flog.write('INPUT hex: %20s; disasm: "%s"\n' % (binascii.hexlify(insn_binary), str))
				if is_invalid_insn(insn_binary):
					flog.write('*** Skipping invalid opcode ***')
					continue
				# Add unique disasms to file
				str = ida_disasm_fix(insn_binary, str)
				flog.write('\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tOUTPUT disasm: "%s"\n' % str)
				flog.flush()
				insn_write(f, insn_binary, str, False)
				n += 1
				if n % 1000 == 0:
					print '%d opcodes processed' % n
				if len == 2 or len == 1:
					break # Optimization: break if third byte does not matter
			if len == 1:
				break # Optimization: break if second byte does not matter
	f.close()
	flog.write('Finished\n')
	flog.close()

def main():
	generate_x86("./intel-x86-opcodes.txt")

if __name__ == "__main__":
	main()


