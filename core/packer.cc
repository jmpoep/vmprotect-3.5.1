#include "../third-party/lzma/Alloc.h"
#include "objects.h"
#include "files.h"
#include "packer.h"

/**
 * PackerInStream
 */

static SRes ReadFromStream(void *object, void *data, size_t *size)
{
	PackerInputStream *p = reinterpret_cast<PackerInputStream *>(object);
	size_t read_size = (*size < p->size - p->pos) ? *size : p->size - p->pos;
	if (read_size) {
		read_size = p->file->Read(data, read_size);
		p->pos += read_size;
	}
	*size = read_size;
	return SZ_OK;
}

PackerInputStream::PackerInputStream(IArchitecture *file_, size_t size_)
	: file(file_), data(NULL), size(size_), pos(0)
{
	p.Read = ReadFromStream;
}

static SRes ReadFromData(void *object, void *data, size_t *size)
{
	PackerInputStream *p = reinterpret_cast<PackerInputStream *>(object);
	size_t read_size = (*size < p->size - p->pos) ? *size : p->size - p->pos;
	if (read_size) {
		memcpy(data, p->data->data() + p->pos, read_size);
		p->pos += read_size;
	}
	*size = read_size;
	return SZ_OK;
}

PackerInputStream::PackerInputStream(Data *data_)
	: file(NULL), data(data_), size(data_->size()), pos(0)
{
	p.Read = ReadFromData;
}

/**
 * PackerOutStream
 */

static size_t WriteToStream(void *object, const void *data, size_t size)
{
	PackerOutputStream *p = reinterpret_cast<PackerOutputStream *>(object);
	p->data->PushBuff(data, size);
	return size;
}

PackerOutputStream::PackerOutputStream(Data *data_)
	: data(data_) 
{
	p.Write = WriteToStream;
}

/**
 * PackerProgress
 */

SRes PackProgress(void *object, UInt64 inSize, UInt64 /*outSize*/)
{
	PackerProgress *p = reinterpret_cast<PackerProgress *>(object);
	if (p->file)
		p->file->StepProgress(inSize - p->last_pos);
	p->last_pos = inSize;
	return SZ_OK;
}

PackerProgress::PackerProgress(IArchitecture *file_)
	: file(file_), last_pos(0)
{
	p.Progress = PackProgress;
}

/**
 * Packer
 */

Packer::Packer()
{
	encoder_ = LzmaEnc_Create(&g_Alloc);
	if (encoder_ == 0)
		throw 1;

	LzmaEncProps_Init(&props_);
	props_.level = 9;
	props_.writeEndMark = true;
	props_.dictSize = 1 << 24;
	if (LzmaEnc_SetProps(encoder_, &props_) != SZ_OK) 
		throw 1;
}

Packer::~Packer()
{
	if (encoder_ != 0)
		LzmaEnc_Destroy(encoder_, &g_Alloc, &g_BigAlloc);
}

bool Packer::WriteProps(Data *data)
{
	data->clear();

	Byte props_buff[LZMA_PROPS_SIZE];
	size_t props_size = sizeof(props_buff);
	if (LzmaEnc_WriteProperties(encoder_, props_buff, &props_size) != SZ_OK)
		return false;
	data->PushBuff(props_buff, props_size);
	return true;
}

bool Packer::Code(IArchitecture *file, PackerInputStream &in, PackerOutputStream &out)
{
	out.data->clear();

	PackerProgress progress(file);
	if (LzmaEnc_Encode(encoder_, &out.p, &in.p, &progress.p, &g_Alloc, &g_BigAlloc) != SZ_OK)
		return false;

	// LzmaEnc_Encode never calls last progress
	file->StepProgress(in.size - progress.last_pos);
	return true;
}

bool Packer::Code(IArchitecture *file, Data *in_data, Data *out_data)
{
	PackerInputStream in(in_data);
	PackerOutputStream out(out_data);

	return Code(file, in, out);
}

bool Packer::Code(IArchitecture *file, size_t size, Data *data)
{
	PackerInputStream in(file, size);
	PackerOutputStream out(data);

	return Code(file, in, out);
}