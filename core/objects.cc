/**
 * Support of object hierarchy.
 */

#include "objects.h"

#ifdef __APPLE__
#define _vsnprintf_s(dest, dest_sz, cnt, fmt, args) _vsnprintf((dest), (dest_sz), (fmt), (args))
#endif // __APPLE__

std::string string_format(const char *format, ...) 
{
	va_list args;
	va_start(args, format);
	int size = 100;
	std::string res;
	for (;;) {
		res.resize(size);
        va_start(args, format);
        int n = _vsnprintf_s(&res[0], size, _TRUNCATE, format, args);
        if (n > -1 && n < size) {
            res.resize(n);
			break;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
	}
	va_end(args);
	return res;
}

int AddressableObject::CompareWith(const AddressableObject &other) const
{
	if (address_ > other.address_) return 1;
	if (address_ < other.address_) return -1;
	return 0;
}
