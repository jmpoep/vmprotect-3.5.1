#ifdef WIN_DRIVER
#else
#include "common.h"
#include "objects.h"
#include "utils.h"
#include "hook_manager.h"

/**
 * HookManager
 */

HookManager::HookManager() 
	: update_count_(0)
{

}

void *HookManager::HookAPI(HMODULE dll, const char *api_name, void *handler, bool show_error, void **result)
{
	void *res = NULL;
	void *api_address = InternalGetProcAddress(dll, api_name);
	if (api_address) {
		Begin();
		HookedAPI *hooked_api = new HookedAPI();
		if (hooked_api->Hook(api_address, handler, result)) {
			api_list_.push_back(hooked_api);
			res = hooked_api->old_handler();
		} else {
			delete hooked_api;
		}
		End();
	}

	if (!res && show_error) {
		wchar_t error[512] = {};
		{
			// string "Error at hooking API \"%S\"\n"
			wchar_t str[] = { L'E', L'r', L'r', L'o', L'r', L' ', L'a', L't', L' ', L'h', L'o', L'o', L'k', L'i', L'n', L'g', L' ', L'A', L'P', L'I', L' ', L'\"', L'%', L'S', L'\"', L'\n', 0 };
			swprintf_s(error, str, api_name);
		}
		if (api_address){
			int n = 32;
			wchar_t line[100] = {};
			{
				// string "Dumping first %d bytes:\n"
				wchar_t str[] = { L'D', L'u', L'm', L'p', L'i', L'n', L'g', L' ', L'f', L'i', L'r', L's', L't', L' ', L'%', L'd', L' ', L'b', L'y', L't', L'e', L's', L':', L'\n', 0 };
				swprintf_s(line, str, n);
			}
			wcscat_s(error, line);
			for (int i = 0; i < n; i++) {
				wchar_t str[] = { L'%', L'0', L'2', L'X', (i % 16 == 15) ? L'\n' : L' ', 0};
				swprintf_s(line, str, reinterpret_cast<uint8_t *>(api_address)[i]);
				wcscat_s(error, line);
			}
		}
		ShowMessage(error);
		::TerminateProcess(::GetCurrentProcess(), 0xDEADC0DE);
	}

	return res;
}

bool HookManager::UnhookAPI(void *handler)
{
	if (!handler)
		return false;

	bool res = false;
	Begin();
	for (size_t i = 0; i < api_list_.size(); i++) {
		HookedAPI *hooked_api = api_list_[i];
		if (hooked_api->old_handler() == handler) {
			api_list_.erase(i);
			hooked_api->Unhook();
			delete hooked_api;
			res = true;
			break;
		}
	}
	End();
	return res;
}

void HookManager::GetThreads()
{
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h == INVALID_HANDLE_VALUE) 
		return;

	THREADENTRY32 thread_entry;
	thread_entry.dwSize = sizeof(thread_entry);
	if (Thread32First(h, &thread_entry)) {
		DWORD process_id = GetCurrentProcessId();
		DWORD thread_id = GetCurrentThreadId();

		do {
			if (thread_entry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(thread_entry.th32OwnerProcessID)) 
			{
				if (thread_entry.th32OwnerProcessID == process_id && thread_entry.th32ThreadID != thread_id) {
					HANDLE thread = ::OpenThread(THREAD_SUSPEND_RESUME, FALSE, thread_entry.th32ThreadID);
					if (thread) 
						thread_list_.push_back(thread);
				}
			}
			thread_entry.dwSize = sizeof(thread_entry);
		} while (Thread32Next(h, &thread_entry));
	}

	::CloseHandle(h);
}

void HookManager::SuspendThreads()
{
	for (size_t i = 0; i < thread_list_.size(); i++) {
		::SuspendThread(thread_list_[i]);
	}
}

void HookManager::ResumeThreads()
{
	for (size_t i = 0; i < thread_list_.size(); i++) {
		::ResumeThread(thread_list_[i]);
	}
}

void HookManager::FreeThreads()
{
	for (size_t i = 0; i < thread_list_.size(); i++) {
		::CloseHandle(thread_list_[i]);
	}
	thread_list_.clear();
}

void HookManager::Begin()
{
	if (!update_count_) {
		GetThreads();
		SuspendThreads();
	}
	update_count_++;
}

void HookManager::End()
{
	update_count_--;
	if (!update_count_) {
		ResumeThreads();
		FreeThreads();
	}
}

/**
 * HookedAPI
 */

static bool PutJump(uint8_t *address, const uint8_t *jump_dest)
{
	INT64 offset = reinterpret_cast<uint64_t>(jump_dest) - reinterpret_cast<uint64_t>(address) - 5;
	if (offset < INT_MIN || offset > INT_MAX) 
		return false;

	uint8_t buff[5];
	buff[0] = 0xE9;
	*(reinterpret_cast<uint32_t *>(buff + 1)) = static_cast<uint32_t>(offset);

	return FALSE != WriteProcessMemory(GetCurrentProcess(), address, buff, sizeof(buff), NULL);
}

static bool PutJumpMem(uint8_t *address, const uint8_t *memory)
{
#ifdef _WIN64
	// offset is relative
	INT64 offset = reinterpret_cast<uint64_t>(memory) - reinterpret_cast<uint64_t>(address) - 6;
	if (offset < INT_MIN || offset > INT_MAX) 
		return false;
#else
	// offset is absolute
	uint32_t offset = reinterpret_cast<uint32_t>(memory);
#endif

	uint8_t buff[6];
	buff[0] = 0xFF;
	buff[1] = 0x25;
	*(reinterpret_cast<uint32_t *>(buff + 2)) = static_cast<uint32_t>(offset);

	return FALSE != WriteProcessMemory(GetCurrentProcess(), address, buff, sizeof(buff), NULL);
}

enum eCommandType
{
	CT_UNKNOWN = 0,
	CT_JMP,
	CT_RET
};

struct sCommandInfo
{
	DWORD dwSize;
	DWORD dwRelOffset;
	eCommandType cmdType;
};

#define C_66            0x00000001            // 66-prefix
#define C_67            0x00000002            // 67-prefix
#define C_LOCK            0x00000004            // lock
#define C_REP            0x00000008            // repz/repnz
#define C_SEG            0x00000010            // seg-prefix
#define C_OPCODE2        0x00000020            // 2nd opcode present (1st == 0F)
#define C_MODRM        0x00000040            // modrm present
#define C_SIB            0x00000080            // sib present      

bool GetInstructionSize(const byte *pOpCode, sCommandInfo &info)
{
	const byte *opcode = pOpCode;

//	BYTE    disasm_seg = 0;                    // CS DS ES SS FS GS
//	BYTE    disasm_rep = 0;                    // REPZ/REPNZ
	BYTE    disasm_opcode = 0;                // opcode
	BYTE    disasm_opcode2 = 0;                // used when opcode == 0F
	BYTE    disasm_modrm = 0;                // modxxxrm
	BYTE    disasm_sib = 0;                    // scale-index-base
#ifdef _WIN64
	BYTE	disasm_preffix = {0};
#endif

//	DWORD    disasm_len = 0;                    // 0 if error
	DWORD    disasm_flag = 0;                // C_xxx
	DWORD    disasm_memsize = 0;                // value = disasm_mem
	DWORD    disasm_datasize = 0;            // value = disasm_data
	DWORD    disasm_defdata = 4;                // == C_66 ? 2 : 4
	DWORD    disasm_defmem = 4;                // == C_67 ? 2 : 4
	DWORD    disasm_offset = 0;                 

	BYTE    mod = 0;
	BYTE    rm = 0;

	if (! pOpCode) return false;

	info.dwSize = 0;
	info.dwRelOffset = 0;
	info.cmdType = CT_UNKNOWN;

RETRY:

	disasm_opcode = *opcode ++;

	switch (disasm_opcode)
	{
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x08: case 0x09: case 0x0A: case 0x0B:
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x18: case 0x19: case 0x1A: case 0x1B:
		case 0x20: case 0x21: case 0x22: case 0x23:
		case 0x28: case 0x29: case 0x2A: case 0x2B:
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x38: case 0x39: case 0x3A: case 0x3B:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8A: case 0x8B:
		case 0x8C: case 0x8D: case 0x8E:
		case 0xD0: case 0xD1: case 0xD2: case 0xD3:
		case 0xD8: case 0xD9: case 0xDA: case 0xDB:
		case 0xDC: case 0xDD: case 0xDE: case 0xDF:
			disasm_flag |= C_MODRM;
			break;

		case 0x04: case 0x05: case 0x0C: case 0x0D:
		case 0x14: case 0x15: case 0x1C: case 0x1D:
		case 0x24: case 0x25: case 0x2C: case 0x2D:
		case 0x34: case 0x35: case 0x3C: case 0x3D:
		case 0xA8: case 0xA9:

			if (disasm_opcode & 1)
			{
				disasm_datasize += disasm_defdata;
			}
			else
			{
				disasm_datasize ++;
			}
			break;

		case 0xA0: case 0xA1: case 0xA2: case 0xA3:

			if (disasm_opcode & 1)
			{
				#ifdef _WIN64
				if (!(disasm_flag & C_66) && (disasm_preffix & 8))
				{
					disasm_datasize += 8;
				}
				else
				#endif
				{
					disasm_datasize += disasm_defdata;
				}
			}
			else
			{
				disasm_datasize ++;
			}
			break;

		case 0x06: case 0x07: case 0x0E:
		case 0x16: case 0x17: case 0x1E: case 0x1F:
		case 0x27: case 0x2F:
		case 0x37: case 0x3F:
		case 0x60: case 0x61:
		case 0xCE:
			#ifdef _WIN64
			return false;
			#else
			break;
			#endif

		case 0x26: case 0x2E:
		case 0x36: case 0x3E:
		case 0x64: case 0x65:
			disasm_flag |= C_SEG;
			goto RETRY;

		case 0x40: case 0x41: case 0x42: case 0x43:
		case 0x44: case 0x45: case 0x46: case 0x47:
		case 0x48: case 0x49: case 0x4A: case 0x4B:
		case 0x4C: case 0x4D: case 0x4E: case 0x4F:
			#ifdef _WIN64
			disasm_preffix = disasm_opcode;
			goto RETRY;
			#else
			break;
			#endif

		case 0x50: case 0x51: case 0x52: case 0x53:
		case 0x54: case 0x55: case 0x56: case 0x57:
		case 0x58: case 0x59: case 0x5A: case 0x5B:
		case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		case 0x6C: case 0x6D: case 0x6E: case 0x6F:
		case 0x90: case 0x91: case 0x92: case 0x93:
		case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9B: case 0x9C:
		case 0x9D: case 0x9E: case 0x9F:
		case 0xA4: case 0xA5: case 0xA6: case 0xA7:
		case 0xAA: case 0xAB: case 0xAC: case 0xAD:
		case 0xAE: case 0xAF:
		case 0xC3: case 0xC9: case 0xCB: case 0xCC:
		case 0xCF:
		case 0xD7:
		case 0xEC: case 0xED: case 0xEE: case 0xEF:
		case 0xF4: case 0xF5: case 0xF8: case 0xF9:
		case 0xFA: case 0xFB: case 0xFC: case 0xFD:
			break;

		case 0x62: 
		case 0xC4: case 0xC5:
			#ifdef _WIN64
			return false;
			#else
			disasm_flag |= C_MODRM;
			break;
			#endif

		case 0x66:
			disasm_flag |= C_66;
			disasm_defdata = 2;
			goto RETRY;

		case 0x67:
			disasm_flag |= C_67;
			disasm_defmem = 2;
			goto RETRY;

		case 0x69:
		case 0x81:
		case 0xC1:
			disasm_flag |= C_MODRM;
			disasm_datasize += disasm_defdata;
			break;

		case 0x6A:
		case 0x70: case 0x71: case 0x72: case 0x73:
		case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7A: case 0x7B:
		case 0x7C: case 0x7D: case 0x7E: case 0x7F:
		case 0xB0: case 0xB1: case 0xB2: case 0xB3:
		case 0xB4: case 0xB5: case 0xB6: case 0xB7:
		case 0xCD:
		case 0xE0: case 0xE1: case 0xE2: case 0xE3:
		case 0xE4: case 0xE5: case 0xE6: case 0xE7:
			disasm_datasize ++;
			break;

		case 0xEB:
			info.cmdType = CT_JMP;
			disasm_offset = (DWORD) (opcode - pOpCode);
			disasm_datasize ++;
			break;

		case 0x8F:
			if ((*opcode >> 3) & 7)
				return false;
			disasm_flag |= C_MODRM;
			break;

		case 0x9A: case 0xEA:
			#ifdef _WIN64
			return false;
			#else
			disasm_datasize += (disasm_defdata + 2);
			break;
			#endif

		case 0x68:
			disasm_datasize += disasm_defdata;
			break;

		case 0xB8: case 0xB9: case 0xBA: case 0xBB:
		case 0xBC: case 0xBD: case 0xBE: case 0xBF:
#ifdef _WIN64
			if (!(disasm_flag & C_66) && (disasm_preffix & 8))
				disasm_datasize += 8;
			else
#endif
				disasm_datasize += disasm_defdata;
			break;


		//case 0x68:
		//case 0xB8: case 0xB9: case 0xBA: case 0xBB:
		//case 0xBC: case 0xBD: case 0xBE: case 0xBF:
		//	disasm_datasize += disasm_defdata;
		//	break;

		case 0xE8: case 0xE9:
			if (disasm_defdata == 2)
				return false;
			disasm_offset = (DWORD) (opcode - pOpCode);
			disasm_datasize += disasm_defdata;
			break;

		case 0x6B:
		case 0x80: case 0x82: case 0x83:
		case 0xC0:
			disasm_flag |= C_MODRM;
			disasm_datasize ++;
			break;

		case 0xC2:
			disasm_datasize += 2;
			break;

		case 0xC6: case 0xC7:
			if ((*opcode >> 3) & 7)
				return false;
			disasm_flag |= C_MODRM;
			if (disasm_opcode & 1)
			{
				disasm_datasize += disasm_defdata;
			}
			else
			{
				disasm_datasize ++;
			}
			break;

		case 0xC8:
			disasm_datasize += 3;
			break;

		case 0xCA:
			disasm_datasize += 2;
			break;

		case 0xD4: case 0xD5:
			#ifdef _WIN64
			return false;
			#else
			disasm_datasize ++;
			break;
			#endif

		case 0xF0:
			disasm_flag |= C_LOCK;
			goto RETRY;

		case 0xF2: case 0xF3:
			disasm_flag |= C_REP;
			goto RETRY;

		case 0xF6: case 0xF7:
			disasm_flag |= C_MODRM;
			if (((*opcode >> 3) & 7) < 2)
			{
				if (disasm_opcode & 1)
				{
					disasm_datasize += disasm_defdata;
				}
				else
				{
					disasm_datasize ++;
				}
			}
			break;

		case 0xFE:
			if (((*opcode >> 3) & 7) > 1)
				return false;
			disasm_flag |= C_MODRM;
			break;

		case 0xFF:
			if (((*opcode >> 3) & 7) == 7)
				return false;
			disasm_flag |= C_MODRM;
			break;

        case 0x0F:

            disasm_flag |= C_OPCODE2;
            disasm_opcode2 = *opcode ++;

            switch (disasm_opcode2)
            {
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x90: case 0x91: case 0x92: case 0x93:
                case 0x94: case 0x95: case 0x96: case 0x97:
                case 0x98: case 0x99: case 0x9A: case 0x9B:
                case 0x9C: case 0x9D: case 0x9E: case 0x9F:
                case 0xA3: case 0xA5: case 0xAB: case 0xAD:
                case 0xAF:
                case 0xB0: case 0xB1: case 0xB2: case 0xB3:
                case 0xB4: case 0xB5: case 0xB6: case 0xB7:
                case 0xBB:
                case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                case 0xC0: case 0xC1:
                        disasm_flag |= C_MODRM;
                        break;

                case 0x06:
                case 0x08: case 0x09: case 0x0A: case 0x0B:
                case 0xA0: case 0xA1: case 0xA2: case 0xA8:
                case 0xA9: case 0xAA:
                case 0xC8: case 0xC9: case 0xCA: case 0xCB:
                case 0xCC: case 0xCD: case 0xCE: case 0xCF:
                        break;

                case 0x80: case 0x81: case 0x82: case 0x83:
                case 0x84: case 0x85: case 0x86: case 0x87:
                case 0x88: case 0x89: case 0x8A: case 0x8B:
                case 0x8C: case 0x8D: case 0x8E: case 0x8F:
						if (disasm_defdata == 2)
							return false;
						disasm_offset = (DWORD) (opcode - pOpCode);
                        disasm_datasize += disasm_defdata;
                        break;

                case 0xA4: case 0xAC:
                case 0xBA:
                        disasm_datasize ++;
                        disasm_flag |= C_MODRM;
                        break;

			default:
				return false;
			}

	default:
		return false;

	}

	if (disasm_flag & C_MODRM)
	{
		disasm_modrm = *opcode ++;
		mod = disasm_modrm & 0xC0;
		rm  = disasm_modrm & 0x07;

		if (mod != 0xC0)
		{
			if (mod == 0x40) disasm_memsize ++;
			if (mod == 0x80) disasm_memsize += disasm_defmem;

			if (disasm_defmem == 2)           // modrm16
			{
				if ((mod == 0x00) && (rm == 0x06))
				{
					disasm_memsize += 2;
				}
			}
			else                              // modrm32
			{
				if (rm == 0x04)
				{
					disasm_flag |= C_SIB;
					disasm_sib = *opcode ++;
					rm = disasm_sib & 0x07;
				}

				if ((mod == 0x00) && (rm == 0x05))
				{

					#ifdef _WIN64
					if (!(disasm_flag & C_SIB))
						disasm_offset = (DWORD) (opcode - pOpCode);
					#endif
					disasm_memsize += 4;
				}
			}
		}
	}

	info.dwSize =(DWORD) (opcode - pOpCode) + disasm_memsize + disasm_datasize;
	info.dwRelOffset = disasm_offset; 
	
	return true;
}

size_t MoveCode(byte *&pSrcOriginal, byte *pDst, size_t nNeedBytes)
{
	size_t nBytes = 0;
	byte *pSrc = pSrcOriginal;
	sCommandInfo ii;
	while (nBytes < nNeedBytes)
	{
		if (!GetInstructionSize(pSrc, ii)) 
			return 0; // error
		if (nBytes == 0 && ii.cmdType == CT_JMP && ii.dwSize - ii.dwRelOffset == 1)
		{ // this is a short jump and we need to follow it
			ptrdiff_t offset = static_cast<char>(pSrc[ii.dwSize - 1]);
			pSrcOriginal += offset + ii.dwSize; // skip JMP XX
			pSrc = pSrcOriginal;
			continue;
		}
		memcpy(pDst, pSrc, ii.dwSize); // copy original bytes
		if (ii.dwRelOffset)
			*reinterpret_cast<int32_t*>(pDst + ii.dwRelOffset) = static_cast<int32_t>(pSrc + *reinterpret_cast<int32_t*>(pSrc + ii.dwRelOffset) - pDst);
		pSrc += ii.dwSize;
		pDst += ii.dwSize;
		nBytes += ii.dwSize;
	}
	return nBytes; // return number of bytes copied
}

size_t HookedAPI::page_size_ = 0;

HookedAPI::HookedAPI()
	: size_(0), api_(NULL), old_handler_(NULL), crc_(0)
{

}

bool HookedAPI::Hook(void *api, void *handler, void **result)
{
	if (!page_size_) {
		SYSTEM_INFO system_info;
		GetSystemInfo(&system_info);
		page_size_ = (system_info.dwPageSize > 2048) ? system_info.dwPageSize : 2048;
	}
	uint8_t *p = static_cast<uint8_t *>(api);

	// alloc virtual memory
#ifdef _WIN64
	for (size_t i = 0; i < 0x70000000; i += page_size_) {
		old_handler_ = ::VirtualAlloc(p + i, page_size_, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (old_handler_)
			break;
	}
#else
	old_handler_ = ::VirtualAlloc(NULL, page_size_, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#endif

	if (!old_handler_)
		return false;

	uint8_t *memory = static_cast<uint8_t *>(old_handler_);
	if (result)
		*result = memory;

	// structure of the block:
	// some api commands
	// jmp api + 5
	// original commands
	// jmp [handler]
	// handler

	// copy api code to the new place
	size_ = MoveCode(p, memory, 5);
	if (!size_) 
		return false;

	// add JMP command
	if (!PutJump(memory + size_, p + size_)) 
		return false;

	// copy original commands
	memcpy(memory + size_ + 5, p, size_);

	// jmp [handler]
	uint8_t *jump = memory + size_ * 2 + 5;
	if (!PutJumpMem(jump, jump + 6)) 
		return false;

	// put handler to [handler]
	*(reinterpret_cast<void **>(jump + 6)) = handler;

	// put JMP at the beginning of the API
	if (!PutJump(p, jump)) 
		return false;

	api_ = p; // this line should be below MoveCode, because pAPI can be corrected by it
	crc_ = *reinterpret_cast<uint32_t *>(p + 1);

	DWORD old_protect;
	VirtualProtect(old_handler_, page_size_, PAGE_EXECUTE_READ, &old_protect);

	return true;
}

void HookedAPI::Unhook()
{
	if (!api_) 
		return;

	// check CRC
	uint8_t *p = static_cast<uint8_t *>(api_);
	if (*p == 0xE9 && *reinterpret_cast<uint32_t *>(p + 1) == crc_) {
		// put original bytes back
		WriteProcessMemory(GetCurrentProcess(), api_, reinterpret_cast<uint8_t *>(old_handler_) + size_ + 5, size_, NULL);
		VirtualFree(old_handler_, 0, MEM_RELEASE);
	} else {
		// API hooked by another code
		WriteProcessMemory(GetCurrentProcess(), static_cast<uint8_t *>(old_handler_) + size_ * 2 + 5 + 6, &old_handler_, sizeof(old_handler_), NULL);
	}

	// cleanup
	size_ = 0;
	api_ = old_handler_ = NULL;
}

#endif