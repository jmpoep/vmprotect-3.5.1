//  diskid.cpp
//  for displaying the details of hard drives in a command window

//  06/11/00  Lynn McGuire  written with many contributions from others,
//                            IDE drives only under Windows NT/2K and 9X,
//                            maybe SCSI drives later
//  11/20/03  Lynn McGuire  added ReadPhysicalDriveInNTWithZeroRights
//  10/26/05  Lynn McGuire  fix the flipAndCodeBytes function
//  01/22/08  Lynn McGuire  incorporate changes from Gonzalo Diethelm,
//                             remove media serial number code since does 
//                             not work on USB hard drives or thumb drives
//  01/29/08  Lynn McGuire  add ReadPhysicalDriveInNTUsingSmart
//  10/01/13  Lynn Mcguire  fixed reference of buffer address per Torsten Eschner email

#pragma warning (disable:4996)

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <windows.h>
#include <winioctl.h>

char HardDriveSerialNumber [1024];

#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

#define  FILE_DEVICE_SCSI              0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT 0x0004D008  //  see NTDDSCSI.H for definition

#pragma pack(1)

//  Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

// The following struct defines the interesting part of the IDENTIFY
// buffer:
typedef struct _IDSECTOR
{
   USHORT  wGenConfig;
   USHORT  wNumCyls;
   USHORT  wReserved;
   USHORT  wNumHeads;
   USHORT  wBytesPerTrack;
   USHORT  wBytesPerSector;
   USHORT  wSectorsPerTrack;
   USHORT  wVendorUnique[3];
   CHAR    sSerialNumber[20];
   USHORT  wBufferType;
   USHORT  wBufferSize;
   USHORT  wECCSize;
   CHAR    sFirmwareRev[8];
   CHAR    sModelNumber[40];
   USHORT  wMoreVendorUnique;
   USHORT  wDoubleWordIO;
   USHORT  wCapabilities;
   USHORT  wReserved1;
   USHORT  wPIOTiming;
   USHORT  wDMATiming;
   USHORT  wBS;
   USHORT  wNumCurrentCyls;
   USHORT  wNumCurrentHeads;
   USHORT  wNumCurrentSectorsPerTrack;
   ULONG   ulCurrentSectorCapacity;
   USHORT  wMultSectorStuff;
   ULONG   ulTotalAddressableSectors;
   USHORT  wSingleWordDMA;
   USHORT  wMultiWordDMA;
   BYTE    bReserved[128];
} IDSECTOR, *PIDSECTOR;

typedef struct _SRB_IO_CONTROL
{
   ULONG HeaderLength;
   UCHAR Signature[8];
   ULONG Timeout;
   ULONG ControlCode;
   ULONG ReturnCode;
   ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

char *ConvertToString (DWORD diskdata [256],
		       int firstIndex,
		       int lastIndex,
		       char* buf);
void PrintIdeInfo (int drive, DWORD diskdata [256]);

//
// IDENTIFY data (from ATAPI driver source)
//

typedef struct _IDENTIFY_DATA {
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A  53
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT MajorRevision;                   //     73
    USHORT MinorRevision;                   //     74
    USHORT Reserved6[50];                   //     75-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved7[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack()

int ReadPhysicalDriveInNTUsingSmart (int drive)
{
    int done = FALSE;
    HANDLE hPhysicalDriveIOCTL = 0;

    //  Try to get a handle to PhysicalDrive IOCTL, report failure
    //  and exit if can't.
    char driveName [256];

    sprintf (driveName, "\\\\.\\PhysicalDrive%d", drive);

    //  Windows NT, Windows 2000, Windows Server 2003, Vista
    hPhysicalDriveIOCTL = CreateFile (driveName,
                            GENERIC_READ | GENERIC_WRITE, 
                            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, 0, NULL);
    if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
    {
        printf ("\n%d ReadPhysicalDriveInNTUsingSmart ERROR"
					"\nCreateFile(%s) returned INVALID_HANDLE_VALUE\n"
					"Error Code %d\n",
		 			__LINE__, driveName, GetLastError ());
    }
    else
    {
        GETVERSIONINPARAMS GetVersionParams;
        DWORD cbBytesReturned = 0;

        // Get the version, etc of PhysicalDrive IOCTL
        memset ((void*) & GetVersionParams, 0, sizeof(GetVersionParams));

        if ( ! DeviceIoControl (hPhysicalDriveIOCTL, SMART_GET_VERSION,
                NULL, 
                0,
     			&GetVersionParams, sizeof (GETVERSIONINPARAMS),
				&cbBytesReturned, NULL) )
        {         
        DWORD err = GetLastError ();
	    printf ("\n%d ReadPhysicalDriveInNTUsingSmart ERROR"
		            "\nDeviceIoControl(%d, SMART_GET_VERSION) returned 0, error is %d\n",
		            __LINE__, (int) hPhysicalDriveIOCTL, (int) err);
        }
        else
        {
			// Print the SMART version
        // PrintVersion (& GetVersionParams);
	        // Allocate the command buffer
		ULONG CommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
        PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS) malloc (CommandSize);
	        // Retrieve the IDENTIFY data
	        // Prepare the command
#define ID_CMD          0xEC            // Returns ID sector for ATA
		Command -> irDriveRegs.bCommandReg = ID_CMD;
		DWORD BytesReturned = 0;
	    if ( ! DeviceIoControl (hPhysicalDriveIOCTL, 
				                SMART_RCV_DRIVE_DATA, Command, sizeof(SENDCMDINPARAMS),
								Command, CommandSize,
								&BytesReturned, NULL) )
        {
		        // Print the error
		    //PrintError ("SMART_RCV_DRIVE_DATA IOCTL", GetLastError());
	    } 
		else
		{
        	    // Print the IDENTIFY data
            DWORD diskdata [256];
            USHORT *pIdSector = (USHORT *)
                            (PIDENTIFY_DATA) ((PSENDCMDOUTPARAMS) Command) -> bBuffer;

            for (int ijk = 0; ijk < 256; ijk++)
                diskdata [ijk] = pIdSector [ijk];

            PrintIdeInfo (drive, diskdata);
            done = TRUE;
		}
	        // Done
        CloseHandle (hPhysicalDriveIOCTL);
		free (Command);
		}
    }
   return done;
}

//  function to decode the serial numbers of IDE hard drives
//  using the IOCTL_STORAGE_QUERY_PROPERTY command 
char * flipAndCodeBytes (const char * str,
			 int pos,
			 int flip,
			 char * buf)
{
   int i;
   int j = 0;
   int k = 0;

   buf [0] = '\0';
   if (pos <= 0)
      return buf;

   if ( ! j)
   {
      char p = 0;

      // First try to gather all characters representing hex digits only.
      j = 1;
      k = 0;
      buf[k] = 0;
      for (i = pos; j && str[i] != '\0'; ++i)
      {
	 char c = tolower(str[i]);

	 if (isspace(c))
	    c = '0';

	 ++p;
	 buf[k] <<= 4;

	 if (c >= '0' && c <= '9')
	    buf[k] |= (unsigned char) (c - '0');
	 else if (c >= 'a' && c <= 'f')
	    buf[k] |= (unsigned char) (c - 'a' + 10);
	 else
	 {
	    j = 0;
	    break;
	 }

	 if (p == 2)
	 {
	    if (buf[k] != '\0' && ! isprint(buf[k]))
	    {
	       j = 0;
	       break;
	    }
	    ++k;
	    p = 0;
	    buf[k] = 0;
	 }

      }
   }

   if ( ! j)
   {
      // There are non-digit characters, gather them as is.
      j = 1;
      k = 0;
      for (i = pos; j && str[i] != '\0'; ++i)
      {
	     char c = str[i];

	     if ( ! isprint(c))
	     {
	        j = 0;
	        break;
	     }

	     buf[k++] = c;
      }
   }

   if ( ! j)
   {
      // The characters are not there or are not printable.
      k = 0;
   }

   buf[k] = '\0';

   if (flip)
      // Flip adjacent characters
      for (j = 0; j < k; j += 2)
      {
	     char t = buf[j];
	     buf[j] = buf[j + 1];
	     buf[j + 1] = t;
      }

   // Trim any beginning and end space
   i = j = -1;
   for (k = 0; buf[k] != '\0'; ++k)
   {
      if (! isspace(buf[k]))
      {
	     if (i < 0)
	        i = k;
	     j = k;
      }
   }

   if ((i >= 0) && (j >= 0))
   {
      for (k = i; (k <= j) && (buf[k] != '\0'); ++k)
         buf[k - i] = buf[k];
      buf[k - i] = '\0';
   }

   return buf;
}

int ReadPhysicalDriveInNTWithZeroRights (int drive)
{
    int done = FALSE;
    HANDLE hPhysicalDriveIOCTL = 0;

    //  Try to get a handle to PhysicalDrive IOCTL, report failure
    //  and exit if can't.
    char driveName [256];

    sprintf (driveName, "\\\\.\\PhysicalDrive%d", drive);

        //  Windows NT, Windows 2000, Windows XP - admin rights not required
    hPhysicalDriveIOCTL = CreateFile (driveName, 0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL);
    if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
    {
		STORAGE_PROPERTY_QUERY query;
        DWORD cbBytesReturned = 0;
		char local_buffer [10000];

        memset ((void *) & query, 0, sizeof (query));
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;

		memset (local_buffer, 0, sizeof (local_buffer));

        if ( DeviceIoControl (hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY,
                & query,
                sizeof (query),
				& local_buffer [0],
				sizeof (local_buffer),
                & cbBytesReturned, NULL) )
        {         
			STORAGE_DEVICE_DESCRIPTOR * descrip = (STORAGE_DEVICE_DESCRIPTOR *) & local_buffer;

	        flipAndCodeBytes (local_buffer,
			                descrip -> SerialNumberOffset,
			                1, HardDriveSerialNumber );

			done = TRUE;
            printf ("\n**** STORAGE_DEVICE_DESCRIPTOR for drive %d ****\n"
		            "Serial Number = [%s]\n",
		            drive,
		            HardDriveSerialNumber);
        }
		else
		{
			DWORD err = GetLastError ();
			printf ("\nDeviceIOControl IOCTL_STORAGE_QUERY_PROPERTY error = %d\n", err);
		}

        CloseHandle (hPhysicalDriveIOCTL);
    }
    return done;
}

#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE

int ReadIdeDriveAsScsiDriveInNT(int driveno)
{
    int done = FALSE;
    int controller = driveno / 2;

    HANDLE hScsiDriveIOCTL = 0;
    char   driveName [256];

    //  Try to get a handle to PhysicalDrive IOCTL, report failure
    //  and exit if can't.
    sprintf (driveName, "\\\\.\\Scsi%d:", controller);

    //  Windows NT, Windows 2000, any rights should do
    hScsiDriveIOCTL = CreateFile (driveName,
                            GENERIC_READ | GENERIC_WRITE, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL);

    if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
    {
        int drive = driveno % 2;

        char buffer [sizeof (SRB_IO_CONTROL) + SENDIDLENGTH];
        SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
        SENDCMDINPARAMS *pin =
                (SENDCMDINPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
        DWORD dummy;
   
        memset (buffer, 0, sizeof (buffer));
        p -> HeaderLength = sizeof (SRB_IO_CONTROL);
        p -> Timeout = 10000;
        p -> Length = SENDIDLENGTH;
        p -> ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
        strncpy ((char *) p -> Signature, "SCSIDISK", 8);
  
        pin -> irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
        pin -> bDriveNumber = drive;

        if (DeviceIoControl (hScsiDriveIOCTL, IOCTL_SCSI_MINIPORT, 
                                buffer,
                                sizeof (SRB_IO_CONTROL) +
                                        sizeof (SENDCMDINPARAMS) - 1,
                                buffer,
                                sizeof (SRB_IO_CONTROL) + SENDIDLENGTH,
                                &dummy, NULL))
        {
            SENDCMDOUTPARAMS *pOut =
                (SENDCMDOUTPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
            IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
            if (pId -> sModelNumber [0])
            {
                DWORD diskdata [256];
                int ijk = 0;
                USHORT *pIdSector = (USHORT *) pId;
          
                for (ijk = 0; ijk < 256; ijk++)
                    diskdata [ijk] = pIdSector [ijk];

                PrintIdeInfo (controller * 2 + drive, diskdata);

                done = TRUE;
            }
        }
        CloseHandle (hScsiDriveIOCTL);
    }

   return done;
}

void PrintIdeInfo (int drive, DWORD diskdata [256])
{
   ConvertToString (diskdata, 10, 19, HardDriveSerialNumber);

   printf ("\nDrive %d - ", drive);

   switch (drive / 2)
   {
      case 0: printf ("Primary Controller - ");
              break;
      case 1: printf ("Secondary Controller - ");
              break;
      case 2: printf ("Tertiary Controller - ");
              break;
      case 3: printf ("Quaternary Controller - ");
              break;
   }

   switch (drive % 2)
   {
      case 0: printf ("Master drive\n\n");
              break;
      case 1: printf ("Slave drive\n\n");
              break;
   }

   printf ("Drive Serial Number_______________: [%s]\n",
           HardDriveSerialNumber);

   printf ("Drive Type________________________: ");
   if (diskdata [0] & 0x0080)
      printf ("Removable\n");
   else if (diskdata [0] & 0x0040)
      printf ("Fixed\n");
   else printf ("Unknown\n");
}

char *ConvertToString (DWORD diskdata [256],
		       int firstIndex,
		       int lastIndex,
		       char* buf)
{
   int index = 0;
   int position = 0;

      //  each integer has two characters stored in it backwards
   for (index = firstIndex; index <= lastIndex; index++)
   {
         //  get high byte for 1st character
      buf [position++] = (char) (diskdata [index] / 256);

         //  get low byte for 2nd character
      buf [position++] = (char) (diskdata [index] % 256);
   }

      //  end the string 
   buf[position] = '\0';

      //  cut off the trailing blanks
   for (index = position - 1; index > 0 && isspace(buf [index]); index--)
      buf [index] = '\0';

   return buf;
}

int SystemDrive()
{
	int drive = 0;
	HANDLE                hPart;
	DWORD                 dwTmp, dwErr;
	STORAGE_DEVICE_NUMBER Info;

	wchar_t buf[MAX_PATH + 10] = L"\\\\.\\";
	GetSystemDirectoryW(buf + 4, MAX_PATH);
	buf[6] = 0;
	hPart = CreateFileW( buf, 0,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING,
		NULL );
	if ( INVALID_HANDLE_VALUE == hPart )
	{
		dwErr = GetLastError();
		printf ("\nCreateFileW ERROR %d\n", (int) dwErr);
	}
	else 
	{
		if ( !DeviceIoControl( hPart, IOCTL_STORAGE_GET_DEVICE_NUMBER,
			NULL, 0, 
			&Info, sizeof(Info),
			&dwTmp, NULL ) )
		{
			dwErr = GetLastError();
			printf ("\nDeviceIoControl ERROR %d\n", (int) dwErr);
		}
		else if ( dwTmp == sizeof(Info) && Info.DeviceType == FILE_DEVICE_DISK)
		{
			drive = Info.DeviceNumber;
			printf ("\nSystem drive: %d\n", drive);
		} else
		{
			printf ("\nInfo missized or bad DeviceType\n");
		}
		CloseHandle(hPart);
	}
	return drive;
}
int main ()
{
	int done;
	*HardDriveSerialNumber = 0;

	int drive = SystemDrive();
	//  this should work in WinNT or Win2K
	//  this is kind of a backdoor via the SCSI mini port driver into
	//     the IDE drives
	printf ("\nTrying to read the drive IDs using the SCSI back door\n");
	done = ReadIdeDriveAsScsiDriveInNT (drive);

	//if ( ! done) 
	{
		//  this works under WinNT4 or Win2K or WinXP if you have any rights
		printf ("\nTrying to read the drive IDs using physical access with zero rights\n");
		done = ReadPhysicalDriveInNTWithZeroRights (drive);
	}

	//if ( ! done)
	{
		//  this works under WinNT4 or Win2K or WinXP or Windows Server 2003 or Vista if you have any rights
		printf ("\nTrying to read the drive IDs using Smart\n");
		done = ReadPhysicalDriveInNTUsingSmart (drive);
	}

    printf ("\nHard Drive Serial Number__________: %s\n", 
           HardDriveSerialNumber);

   return 0;
}
