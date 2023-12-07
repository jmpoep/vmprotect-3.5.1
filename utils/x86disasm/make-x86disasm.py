#!/usr/bin/python
import os
import os.path
import sys
sys.path.append("../..")
import fileop
import utils

name = "x86disasm"

def do_clean():
	fileop.clean_dir(".", ".obj")
	fileop.clean_dir(".", ".pdb")
	fileop.clean_dir(".", ".ilk")
	fileop.clean_dir(".", ".exe")
	fileop.remove_file(os.path.join(bin_dir, name))
	fileop.remove_file(os.path.join(bin_dir, name + '.exe'))
	fileop.remove_file(os.path.join(bin_dir, name + '.pdb'))
	fileop.remove_file(os.path.join(bin_dir, name + '.ilk'))
	return 0

def make(clean, release, x64):
	if clean:
		do_clean()
	result = os.system("scons -f Sconstruct-%s release=%d clean=%d amd64=%d" % (name, release, clean, x64))
	if 0 == result:
    	# Scons does not put file in the required directory. Do it ourselves.
		if utils.get_platform() == 'windows':
			os.system("move " + name + ".exe " + bin_dir)
			os.system("move " + name + ".pdb " + bin_dir)
			os.system("move " + name + ".ilk " + bin_dir)
		else:
			os.system("mv " + name + " " + bin_dir)
	return result

def title(clean, release, x64):
	print "*** %s making ... ***" % name
	print "clean = ", clean
	print "release = ", release
	print "x64 = ", x64

def print_result(result):
	if 0 == result:
		print "*** %s make: OK ***" % name
	else:
		print "*** %s make: error %d" % (name, result)

clean = 'clean' in sys.argv
release = 'release' in sys.argv
x64 = 'x64' in sys.argv

bin_dir = fileop.get_bin_dir('../..', x64, release)
assert os.path.isdir(bin_dir)

title(clean, release, x64)
result = make(clean, release, x64)
print_result(result)
sys.exit(result)

