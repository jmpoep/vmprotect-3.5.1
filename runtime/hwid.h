#ifndef HWID_H
#define HWID_H

class CryptoContainer;

class HardwareID
{
public:
	HardwareID();
	~HardwareID();

	size_t Copy(void *dest, size_t size) const;
	int GetCurrent(char *buffer, int size);
	bool IsCorrect(uint8_t *p, size_t s) const;
	bool IsCorrect(CryptoContainer &cont, size_t offset, size_t size) const;
private:
	enum {
		MAX_BLOCKS = 16,
		TYPE_MASK = 3
	};
	enum BlockType {
		BLOCK_CPU,
		BLOCK_HOST,
		BLOCK_MAC,
		BLOCK_HDD,
	};
	uint32_t block_count_;
	uint32_t start_block_;
	CryptoContainer	*blocks_;

	void AddBlock(const void *p, size_t size, BlockType type);
	void ProcessMAC(const uint8_t *p, size_t size);
	void ProcessCPU(uint8_t method);

	void GetCPU(uint8_t method);
	void GetMacAddresses();
	void GetMachineName();
	void GetHDD();

	// no copy ctr or assignment op
	HardwareID(const HardwareID &);
	HardwareID &operator =(const HardwareID &);
};

#endif