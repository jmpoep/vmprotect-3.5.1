#ifndef PACKER_H
#define PACKER_H

#include "../third-party/lzma/LzmaEnc.h"

class IArchitecture;

struct PackerInputStream
{
	ISeqInStream p;
	IArchitecture *file;
	Data *data;
	size_t size;
	size_t pos;
	PackerInputStream(IArchitecture *file_, size_t size_);
	PackerInputStream(Data *data_);
};

struct PackerOutputStream
{
	ISeqOutStream p;
	Data *data;
	PackerOutputStream(Data *data_);
};

struct PackerProgress
{
	ICompressProgress p;
	IArchitecture *file;
	uint64_t last_pos;
	PackerProgress(IArchitecture *file_);
};

class Packer
{
public:
	Packer();
	~Packer();
	bool Code(IArchitecture *file, size_t size, Data *data);
	bool Code(IArchitecture *file, Data *in_data, Data *out_data);
	bool WriteProps(Data *data);
private:
	bool Code(IArchitecture *file, PackerInputStream &in, PackerOutputStream &out);

	CLzmaEncHandle encoder_;
	CLzmaEncProps props_;
};

#endif
