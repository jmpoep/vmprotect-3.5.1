/* Arrays of asctime-date day and month strs, rfc1123-date day and month strs, and rfc850-date day and month strs. */
static const char* kDayStrs[] = {
    "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday",
	"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

static const char* kMonthStrs[] = {
	"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December",
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* NOTE that these are ordered this way on purpose. */
static const char* kUSTimeZones[] = {"PST", "PDT", "MST", "MDT", "CST", "CDT", "EST", "EDT"};


/* extern */ const UInt8*
_CFGregorianDateCreateWithBytes(CFAllocatorRef alloc, const UInt8* bytes, CFIndex length, CFGregorianDate* date, CFTimeZoneRef* tz) {

	UInt8 buffer[256];					/* Any dates longer than this are not understood. */

	length = (length == 256) ? 255 : length;
	memmove(buffer, bytes, length);
	buffer[length] = '\0';				/* Guarantees every compare will fail if trying to index off the end. */
	
	memset(date, 0, sizeof(date[0]));
	if (tz) *tz = NULL;
	
	do {
		size_t i;
		CFIndex scan = 0;
		UInt8 c = buffer[scan];
			
		/* Skip leading whitespace */
		while (isspace(c))
			c = buffer[++scan];
		
		/* Check to see if there is a weekday up front. */
		if (!isdigit(c)) {
			
			for (i = 0; i < (sizeof(kDayStrs) / sizeof(kDayStrs[0])); i++) {
				if (!memcmp(kDayStrs[i], &buffer[scan], strlen(kDayStrs[i])))
					break;
			}

			if (i >=(sizeof(kDayStrs) / sizeof(kDayStrs[0])))
				break;
			
			scan += strlen(kDayStrs[i]);
			c = buffer[scan];
			
			while (isspace(c) || c == ',')
				c = buffer[++scan];
		}
		
		/* check for asctime where month comes first */
		if (!isdigit(c)) {
			
			for (i = 0; i < (sizeof(kMonthStrs) / sizeof(kMonthStrs[0])); i++) {
				if (!memcmp(kMonthStrs[i], &buffer[scan], strlen(kMonthStrs[i])))
					break;
			}
			
			if (i >= (sizeof(kMonthStrs) / sizeof(kMonthStrs[0])))
				break;
			
			date->month = (i % 12) + 1;
			
			scan += strlen(kMonthStrs[i]);
			c = buffer[scan];
			
			while (isspace(c))
				c = buffer[++scan];
			
			if (!isdigit(c))
				break;
		}
		
		/* Read the day of month */
		for (i = 0; isdigit(c) && (i < 2); i++) {
			date->day *= 10;
			date->day += c - '0';
			c = buffer[++scan];
		}		
		
		while (isspace(c) || c == '-')
			c = buffer[++scan];
		
		/* Not asctime so now comes the month. */
		if (date->month == 0) {
			
			if (isdigit(c)) {
				for (i = 0; isdigit(c) && (i < 2); i++) {
					date->month *= 10;
					date->month += c - '0';
					c = buffer[++scan];
				}		
			}
			else {
				for (i = 0; i < (sizeof(kMonthStrs) / sizeof(kMonthStrs[0])); i++) {
					if (!memcmp(kMonthStrs[i], &buffer[scan], strlen(kMonthStrs[i])))
						break;
				}
				
				if (i >= (sizeof(kMonthStrs) / sizeof(kMonthStrs[0])))
					break;
				
				date->month = (i % 12) + 1;
				
				scan += strlen(kMonthStrs[i]);
				c = buffer[scan];
			}
			
			while (isspace(c) || c == '-')
				c = buffer[++scan];
			
			/* Read the year */
			for (i = 0; isdigit(c) && (i < 4); i++) {
				date->year *= 10;
				date->year += c - '0';
				c = buffer[++scan];
			}
			
			while (isspace(c))
				c = buffer[++scan];
		}
		
		/* Read the hours */
		for (i = 0; isdigit(c) && (i < 2); i++) {
			date->hour *= 10;
			date->hour += c - '0';
			c = buffer[++scan];
		}		
		
		if (c != ':')
			break;
		c = buffer[++scan];
		
		/* Read the minutes */
		for (i = 0; isdigit(c) && (i < 2); i++) {
			date->minute *= 10;
			date->minute += c - '0';
			c = buffer[++scan];
		}		
		
		if (c == ':') {
			
			c = buffer[++scan];
			
			/* Read the seconds */
			for (i = 0; isdigit(c) && (i < 2); i++) {
				date->second *= 10;
				date->second += c - '0';
				c = buffer[++scan];
			}		
			c = buffer[++scan];
		}
		
		/* If haven't read the year yet, now is the time. */
		if (date->year == 0) {
			
			while (isspace(c))
				c = buffer[++scan];
			
			/* Read the year */
			for (i = 0; isdigit(c) && (i < 4); i++) {
				date->year *= 10;
				date->year += c - '0';
				c = buffer[++scan];
			}
		}
		
		if (date->year && date->year < 100) {
			
			if (date->year < 70)
				date->year += 2000;		/* My CC is still using 2-digit years! */
			else
				date->year += 1900;		/* Bad 2 byte clients */
		}
		
		while (isspace(c))
			c = buffer[++scan];

		if (c && tz) {
			
			/* If it has absolute offset, read the hours and minutes. */
			if ((c == '+') || (c == '-')) {
				
				char sign = c;
				CFTimeInterval minutes = 0, offset = 0;
				
				c = buffer[++scan];
				
				/* Read the hours */
				for (i = 0; isdigit(c) && (i < 2); i++) {
					offset *= 10;
					offset += c - '0';
					c = buffer[++scan];
				}
				
				/* Read the minutes */
				for (i = 0; isdigit(c) && (i < 2); i++) {
					minutes *= 10;
					minutes += c - '0';
					c = buffer[++scan];
				}
				
				offset *= 60;
				offset += minutes;

				if (sign == '-') offset *= -60;
				else offset *= 60;
				
				*tz = CFTimeZoneCreateWithTimeIntervalFromGMT(alloc, offset);
			}
			
			/* If it's not GMT/UT time, need to parse the alpha offset. */
			else if (!strncmp((const char*)(&buffer[scan]), "UT", 2)) {
				*tz = CFTimeZoneCreateWithTimeIntervalFromGMT(alloc, 0);
				scan += 2;
			}
				
			else if (!strncmp((const char*)(&buffer[scan]), "GMT", 3)) {
				*tz = CFTimeZoneCreateWithTimeIntervalFromGMT(alloc, 0);
				scan += 3;
			}
			
			else if (isalpha(c)) {
				
				UInt8 next = buffer[scan + 1];
				
				/* Check for military time. */
				if ((c != 'J') && (!next || isspace(next) || (next == '*'))) {
					
					if (c == 'Z')
						*tz = CFTimeZoneCreateWithTimeIntervalFromGMT(alloc, 0);
					
					else {

						CFTimeInterval offset = (c < 'N') ? (c - 'A' + 1) : ('M' - c);
					
						offset *= 60;
						
						if (next == '*') {
							scan++;
							offset = (offset < 0) ? offset - 30 : offset + 30;
						}
						
						offset *= 60;
						
						*tz = CFTimeZoneCreateWithTimeIntervalFromGMT(alloc, 0);
					}
				}
					
				else {
					
					for (i = 0; i < (sizeof(kUSTimeZones) / sizeof(kUSTimeZones[0])); i++) {
						
						if (!memcmp(kUSTimeZones[i], &buffer[scan], strlen(kUSTimeZones[i]))) {
							
							*tz = CFTimeZoneCreateWithTimeIntervalFromGMT(alloc, (-8 + (i >> 2) + (i & 0x1)) * 3600);
							
							scan += strlen(kUSTimeZones[i]);
							
							break;
						}
					}
				}
			}				
		}
		
		if (!CFGregorianDateIsValid(*date, kCFGregorianAllUnits))
			break;
		
		return bytes + scan;
			
	} while (1);
	
	memset(date, 0, sizeof(date[0]));
	if (tz) {
		if (*tz) CFRelease(*tz);
		*tz = NULL;
	}
	
	return bytes;
}

/* extern */ CFIndex
_CFGregorianDateCreateWithString(CFAllocatorRef alloc, CFStringRef str, CFGregorianDate* date, CFTimeZoneRef* tz) {
	
	UInt8 buffer[256];					/* Any dates longer than this are not understood. */
	CFIndex length = CFStringGetLength(str);
	CFIndex result = 0;
	
	CFStringGetBytes(str, CFRangeMake(0, length), kCFStringEncodingASCII, 0, FALSE, buffer, sizeof(buffer), &length);
	
	if (length)
		result = _CFGregorianDateCreateWithBytes(alloc, buffer, length, date, tz) - buffer;
	
	else {
		memset(date, 0, sizeof(date[0]));
		if (tz) *tz = NULL;
	}
	
	return result;
}
