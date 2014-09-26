/** @file
Reading/writing MBR/DBR.
  NOTE:
    If we write MBR to disk, we just update the MBR code and the partition table wouldn't be over written.
    If we process DBR, we will patch MBR to set first partition active if no active partition exists.
    
Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <Common/UefiBaseTypes.h>

#include "ParseInf.h"
#include "EfiUtilityMsgs.h"
#include "CommonLib.h"

//
// Utility Name
//
#define UTILITY_NAME  "GenBootSector"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 2

#define MAX_DRIVE                             26
#define PARTITION_TABLE_OFFSET                0x1BE

#define SIZE_OF_PARTITION_ENTRY               0x10

#define PARTITION_ENTRY_STARTLBA_OFFSET       8

#define PARTITION_ENTRY_NUM                   4

INT
GetDrvNumOffset (
  IN VOID *BootSector
  );

typedef enum {
  PatchTypeUnknown,
  PatchTypeFloppy,
  PatchTypeIde,
  PatchTypeUsb,
  PatchTypeFileImage   // input and output are all file image, patching action is same as PatchTypeFloppy
} PATCH_TYPE;

typedef enum {
  PathUnknown,
  PathFile,
  PathFloppy,
  PathUsb,
  PathIde
} PATH_TYPE;

typedef enum {
  ErrorSuccess,
  ErrorFileCreate,
  ErrorFileReadWrite,
  ErrorNoMbr,
  ErrorFatType,
  ErrorPath,
} ERROR_STATUS;

CHAR *ErrorStatusDesc[] = {
  "Success",
  "Failed to create files",
  "Failed to read/write files",
  "No MBR exists",
  "Failed to detect Fat type",
  "Inavlid path"
};

typedef struct _DRIVE_TYPE_DESC {
  UINT  Type;
  CHAR  *Description;
} DRIVE_TYPE_DESC;

#define DRIVE_TYPE_ITEM(x) {x, #x}
DRIVE_TYPE_DESC DriveTypeDesc[] = {
  DRIVE_TYPE_ITEM (DRIVE_UNKNOWN),
  DRIVE_TYPE_ITEM (DRIVE_NO_ROOT_DIR),
  DRIVE_TYPE_ITEM (DRIVE_REMOVABLE),
  DRIVE_TYPE_ITEM (DRIVE_FIXED),
  DRIVE_TYPE_ITEM (DRIVE_REMOTE),
  DRIVE_TYPE_ITEM (DRIVE_CDROM),
  DRIVE_TYPE_ITEM (DRIVE_RAMDISK),
  (UINT) -1, NULL
};

typedef struct _DRIVE_INFO {
  CHAR              VolumeLetter;
  DRIVE_TYPE_DESC   *DriveType;
  UINT              DiskNumber;
} DRIVE_INFO;

typedef struct _PATH_INFO {
  CHAR             *Path;
  CHAR             PhysicalPath[260];
  PATH_TYPE        Type;
  BOOL             Input;
} PATH_INFO;

#define BOOT_SECTOR_LBA_OFFSET 0x1FA

#define IsLetter(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z'))

BOOL
GetDriveInfo (
  CHAR       VolumeLetter,
  DRIVE_INFO *DriveInfo
  )
/*++
Routine Description:
  Get drive information including disk number and drive type,
  where disknumber is useful for reading/writing disk raw data.
  NOTE: Floppy disk doesn't have disk number but it doesn't matter because
        we can reading/writing floppy disk without disk number.

Arguments:
  VolumeLetter : volume letter, e.g.: C for C:, A for A:
  DriveInfo    : pointer to DRIVE_INFO structure receiving drive information.

Return:
  TRUE  : successful
  FALSE : failed
--*/
{
  HANDLE                  VolumeHandle;
  STORAGE_DEVICE_NUMBER   StorageDeviceNumber;
  DWORD                   BytesReturned;
  BOOL                    Success;
  UINT                    DriveType;
  UINT                    Index;

  CHAR RootPath[]         = "X:\\";       // "X:\"  -> for GetDriveType
  CHAR VolumeAccessPath[] = "\\\\.\\X:";  // "\\.\X:"  -> to open the volume

  RootPath[0] = VolumeAccessPath[4] = VolumeLetter;
  DriveType = GetDriveType(RootPath);
  if (DriveType != DRIVE_REMOVABLE && DriveType != DRIVE_FIXED) {
    return FALSE;
  }

  DriveInfo->VolumeLetter = VolumeLetter;
  VolumeHandle = CreateFile (
                   VolumeAccessPath,
                   0,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL,
                   OPEN_EXISTING,
                   0,
                   NULL
                   );
  if (VolumeHandle == INVALID_HANDLE_VALUE) {
    fprintf (
      stderr, 
      "error E0005: CreateFile failed: Volume = %s, LastError = 0x%x\n", 
      VolumeAccessPath, 
      GetLastError ()
      );
    return FALSE;
  }

  //
  // Get Disk Number. It should fail when operating on floppy. That's ok 
  //  because Disk Number is only needed when operating on Hard or USB disk.
  //
  // To direct write to disk:
  //   for USB and HD: use path = \\.\PHYSICALDRIVEx, where x is Disk Number
  //   for floppy:     use path = \\.\X:, where X can be A or B
  //
  Success = DeviceIoControl(
              VolumeHandle, 
              IOCTL_STORAGE_GET_DEVICE_NUMBER,
              NULL, 
              0, 
              &StorageDeviceNumber, 
              sizeof(StorageDeviceNumber),
              &BytesReturned, 
              NULL
              );
  //
  // DeviceIoControl should fail if Volume is floppy or network drive.
  //
  if (!Success) {
    DriveInfo->DiskNumber = (UINT) -1;
  } else if (StorageDeviceNumber.DeviceType != FILE_DEVICE_DISK) {
    //
    // Only care about the disk.
    //
    return FALSE;
  } else{
    DriveInfo->DiskNumber = StorageDeviceNumber.DeviceNumber;
  }
  CloseHandle(VolumeHandle);
  
  //
  // Fill in the type string
  //
  DriveInfo->DriveType = NULL;
  for (Index = 0; DriveTypeDesc[Index].Description != NULL; Index ++) {
    if (DriveType == DriveTypeDesc[Index].Type) {
      DriveInfo->DriveType = &DriveTypeDesc[Index];
      break;
    }
  }

  if (DriveInfo->DriveType == NULL) {
    //
    // Should have a type.
    //
    fprintf (stderr, "error E3005: Fatal Error!!!\n");
    return FALSE;
  }
  return TRUE;
}

VOID
ListDrive (
  VOID
  )
/*++
Routine Description:
  List every drive in current system and their information.

--*/
{
  UINT       Index;
  DRIVE_INFO DriveInfo;
  
  UINT Mask =  GetLogicalDrives();

  for (Index = 0; Index < MAX_DRIVE; Index++) {
    if (((Mask >> Index) & 0x1) == 1) {
      if (GetDriveInfo ('A' + (CHAR) Index, &DriveInfo)) {
        if (Index < 2) {
          // Floppy will occupy 'A' and 'B'
          fprintf (
            stdout,
            "%c: - Type: %s\n",
            DriveInfo.VolumeLetter,
            DriveInfo.DriveType->Description
            );
        } else {
          fprintf (
            stdout,
            "%c: - DiskNum: %u, Type: %s\n", 
            DriveInfo.VolumeLetter,
            (unsigned) DriveInfo.DiskNumber, 
            DriveInfo.DriveType->Description
            );
        }
      }
    }
  }

}

INT
GetBootSectorOffset (
  HANDLE     DiskHandle,
  PATH_INFO  *PathInfo
  )
/*++
Description:
  Get the offset of boot sector.
  For non-MBR disk, offset is just 0
  for disk with MBR, offset needs to be caculated by parsing MBR

  NOTE: if no one is active, we will patch MBR to select first partition as active.

Arguments:
  DiskHandle  : HANDLE of disk
  PathInfo    : PATH_INFO structure.
  WriteToDisk : TRUE indicates writing

Return:
  -1   : failed
  o.w. : Offset to boot sector
--*/
{
  BYTE    DiskPartition[0x200];
  DWORD   BytesReturn;
  DWORD   DbrOffset;
  DWORD   Index;
  BOOL    HasMbr;

  DbrOffset = 0;
  HasMbr    = FALSE;
  
  SetFilePointer(DiskHandle, 0, NULL, FILE_BEGIN);
  if (!ReadFile (DiskHandle, DiskPartition, 0x200, &BytesReturn, NULL)) {
    return -1;
  }

  //
  // Check Signature, Jmp, and Boot Indicator.
  // if all pass, we assume MBR found.
  //

  // Check Signature: 55AA
  if ((DiskPartition[0x1FE] == 0x55) && (DiskPartition[0x1FF] == 0xAA)) {
    // Check Jmp: (EB ?? 90) or (E9 ?? ??)
    if (((DiskPartition[0] != 0xEB) || (DiskPartition[2] != 0x90)) &&
        (DiskPartition[0] != 0xE9)) {
      // Check Boot Indicator: 0x00 or 0x80
      // Boot Indicator is the first byte of Partition Entry
      HasMbr = TRUE;
      for (Index = 0; Index < PARTITION_ENTRY_NUM; ++Index) {
        if ((DiskPartition[PARTITION_TABLE_OFFSET + Index * SIZE_OF_PARTITION_ENTRY] & 0x7F) != 0) {
          HasMbr = FALSE;
          break;
        }
      }
    }
  }

  if (HasMbr) {
    //
    // Skip MBR
    //
    for (Index = 0; Index < PARTITION_ENTRY_NUM; Index++) {
      //
      // Found Boot Indicator.
      //
      if (DiskPartition[PARTITION_TABLE_OFFSET + (Index * SIZE_OF_PARTITION_ENTRY)] == 0x80) {
        DbrOffset = *(DWORD *)&DiskPartition[PARTITION_TABLE_OFFSET + (Index * SIZE_OF_PARTITION_ENTRY) + PARTITION_ENTRY_STARTLBA_OFFSET];
        break;
      }
    }
    //
    // If no boot indicator, we manually select 1st partition, and patch MBR.
    //
    if (Index == PARTITION_ENTRY_NUM) {
      DbrOffset = *(DWORD *)&DiskPartition[PARTITION_TABLE_OFFSET + PARTITION_ENTRY_STARTLBA_OFFSET];
      if (!PathInfo->Input && (PathInfo->Type == PathUsb)) {
        SetFilePointer(DiskHandle, 0, NULL, FILE_BEGIN);
        DiskPartition[PARTITION_TABLE_OFFSET] = 0x80;
        WriteFile (DiskHandle, DiskPartition, 0x200, &BytesReturn, NULL);
      }
    }
  }

  return DbrOffset;
}

/**
 * Get window file handle for input/ouput disk/file. 
 *  
 * @param PathInfo
 * @param ProcessMbr
 * @param FileHandle
 * 
 * @return ERROR_STATUS
 */
ERROR_STATUS
GetFileHandle (
  PATH_INFO  *PathInfo,
  BOOL       ProcessMbr,
  HANDLE     *FileHandle,
  DWORD      *DbrOffset
  )
{
  DWORD  OpenFlag;

  OpenFlag = OPEN_ALWAYS;
  if (PathInfo->Input || PathInfo->Type != PathFile) {
    OpenFlag = OPEN_EXISTING;
  }

  *FileHandle = CreateFile(
                   PathInfo->PhysicalPath,
                   GENERIC_READ | GENERIC_WRITE, 
                   FILE_SHARE_READ, 
                   NULL, 
                   OpenFlag, 
                   FILE_ATTRIBUTE_NORMAL, 
                   NULL
                   );
  if (*FileHandle == INVALID_HANDLE_VALUE) {
    return ErrorFileCreate;
  }

  if ((PathInfo->Type == PathIde) || (PathInfo->Type == PathUsb)){
    *DbrOffset = GetBootSectorOffset (*FileHandle, PathInfo);
    if (!ProcessMbr) {
      //
      // 1. Process boot sector, set file pointer to the beginning of boot sector
      //
      SetFilePointer (*FileHandle, *DbrOffset * 0x200, NULL, FILE_BEGIN);
    } else if(*DbrOffset == 0) {
      //
      // If user want to process Mbr, but no Mbr exists, simply return FALSE
      //
      return ErrorNoMbr;
    } else {
      //
      // 2. Process MBR, set file pointer to 0
      //
      SetFilePointer (*FileHandle, 0, NULL, FILE_BEGIN);
    }
  }

  return ErrorSuccess;
}

/**
  Writing or reading boot sector or MBR according to the argument. 
   
  @param InputInfo PATH_INFO instance for input path
  @param OutputInfo PATH_INFO instance for output path
  @param ProcessMbr TRUE is to process MBR, otherwise, processing boot sector
  
  @return ERROR_STATUS
 **/
ERROR_STATUS
ProcessBsOrMbr (
  PATH_INFO     *InputInfo,
  PATH_INFO     *OutputInfo,
  BOOL        	ProcessMbr
  )
{
  BYTE              DiskPartition[0x200] = {0};
  BYTE              DiskPartitionBackup[0x200] = {0};
  DWORD             BytesReturn;
  INT               DrvNumOffset;
  HANDLE            InputHandle;
  HANDLE            OutputHandle;
  ERROR_STATUS      Status;
  DWORD             InputDbrOffset;
  DWORD             OutputDbrOffset;

  //
  // Create file Handle and move file Pointer is pointed to beginning of Mbr or Dbr
  //
  Status =  GetFileHandle(InputInfo, ProcessMbr, &InputHandle, &InputDbrOffset);
  if (Status != ErrorSuccess) {
    return Status;
  }

  //
  // Create file Handle and move file Pointer is pointed to beginning of Mbr or Dbr
  //
  Status = GetFileHandle(OutputInfo, ProcessMbr, &OutputHandle, &OutputDbrOffset);
  if (Status != ErrorSuccess) {
    return Status;
  }

  //
  // Read boot sector from source disk/file
  // 
  if (!ReadFile (InputHandle, DiskPartition, 0x200, &BytesReturn, NULL)) {
    return ErrorFileReadWrite;
  }

  if (InputInfo->Type == PathUsb) {
      // Manually set BS_DrvNum to 0x80 as window's format.exe has a bug which will clear this field discarding USB disk's MBR. 
      // offset of BS_DrvNum is 0x24 for FAT12/16
      //                        0x40 for FAT32
      //
      DrvNumOffset = GetDrvNumOffset (DiskPartition);
      if (DrvNumOffset == -1) {
        return ErrorFatType;
      }
      //
      // Some legacy BIOS require 0x80 discarding MBR.
      // Question left here: is it needed to check Mbr before set 0x80?
      //
      DiskPartition[DrvNumOffset] = ((InputDbrOffset > 0) ? 0x80 : 0);
  }

  if (InputInfo->Type == PathIde) {
      //
      // Patch LBAOffsetForBootSector
      //
      *(DWORD *)&DiskPartition [BOOT_SECTOR_LBA_OFFSET] = InputDbrOffset;
  }

  if (OutputInfo->Type != PathFile) {
    if (ProcessMbr) {
      //
      // Use original partition table
      //
      if (!ReadFile (OutputHandle, DiskPartitionBackup, 0x200, &BytesReturn, NULL)) {
        return ErrorFileReadWrite;
      }
      memcpy (DiskPartition + 0x1BE, DiskPartitionBackup + 0x1BE, 0x40);
      SetFilePointer (OutputHandle, 0, NULL, FILE_BEGIN);

    }
  }

  //
  // Write boot sector to taget disk/file
  // 
  if (!WriteFile (OutputHandle, DiskPartition, 0x200, &BytesReturn, NULL)) {
    return ErrorFileReadWrite;
  }

  CloseHandle (InputHandle);
  CloseHandle (OutputHandle);

  return ErrorSuccess;
}

void
Version (
  void
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  printf ("%s Version %d.%d %s\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

VOID
PrintUsage (
  void
  )
{
  printf ("Usage: GenBootSector [options] --cfg-file CFG_FILE\n\n\
Copyright (c) 2009 - 2014, Intel Corporation.  All rights reserved.\n\n\
  Utility to retrieve and update the boot sector or MBR.\n\n\
optional arguments:\n\
  -h, --help            Show this help message and exit\n\
  --version             Show program's version number and exit\n\
  -d [DEBUG], --debug [DEBUG]\n\
                        Output DEBUG statements, where DEBUG_LEVEL is 0 (min)\n\
                        - 9 (max)\n\
  -v, --verbose         Print informational statements\n\
  -q, --quiet           Returns the exit code, error messages will be\n\
                        displayed\n\
  -s, --silent          Returns only the exit code; informational and error\n\
                        messages are not displayed\n\
  -l, --list            List disk drives\n\
  -i INPUT_FILENAME, --input INPUT_FILENAME\n\
                        Input file name\n\
  -o OUTPUT_FILENAME, --output OUTPUT_FILENAME\n\
                        Output file name\n\
  -m, --mbr             Also process the MBR\n\
  --sfo                 Reserved for future use\n");

}

/**
  Get path information, including physical path for windows platform.

  @param PathInfo   Point to PATH_INFO structure.

  @return whether path is valid.
**/
ERROR_STATUS
GetPathInfo (
  PATH_INFO   *PathInfo
  )
{
  DRIVE_INFO  DriveInfo;
  CHAR        VolumeLetter;
  CHAR        DiskPathTemplate[]   = "\\\\.\\PHYSICALDRIVE%u";
  CHAR        FloppyPathTemplate[] = "\\\\.\\%c:";
  FILE        *f;

  //
  // If path is disk path
  //
  if (IsLetter(PathInfo->Path[0]) && (PathInfo->Path[1] == ':') && (PathInfo->Path[2] == '\0')) {
    VolumeLetter = PathInfo->Path[0];
    if ((VolumeLetter == 'A') || (VolumeLetter == 'a') || 
        (VolumeLetter == 'B') || (VolumeLetter == 'b')) {
      PathInfo->Type = PathFloppy;
      sprintf (PathInfo->PhysicalPath, FloppyPathTemplate, VolumeLetter);
      return ErrorSuccess;
    }

    if (!GetDriveInfo(VolumeLetter, &DriveInfo)) {
      fprintf (stderr, "ERROR: GetDriveInfo - 0x%x\n", GetLastError ());
      return ErrorPath;
    }

    if (!PathInfo->Input && (DriveInfo.DriveType->Type == DRIVE_FIXED)) {
      fprintf (stderr, "ERROR: Could patch own IDE disk!\n");
      return ErrorPath;
    }

    sprintf(PathInfo->PhysicalPath, DiskPathTemplate, DriveInfo.DiskNumber);
    if (DriveInfo.DriveType->Type == DRIVE_REMOVABLE) {
      PathInfo->Type = PathUsb;
    } else if (DriveInfo.DriveType->Type == DRIVE_FIXED) {
      PathInfo->Type = PathIde;
    } else {
      fprintf (stderr, "ERROR, Invalid disk path - %s", PathInfo->Path);
      return ErrorPath;
    }

	return ErrorSuccess;
  } 

  PathInfo->Type = PathFile;
  if (PathInfo->Input) {
    //
    // If path is file path, check whether file is valid.
    //
    f = fopen (LongFilePath (PathInfo->Path), "r");
    if (f == NULL) {
      fprintf (stderr, "error E2003: File was not provided!\n");
      return ErrorPath;
    }  
  }
  PathInfo->Type = PathFile;
  strcpy(PathInfo->PhysicalPath, PathInfo->Path);

  return ErrorSuccess;
}    

INT
main (
  INT  argc,
  CHAR *argv[]
  )
{
  CHAR8         *AppName;
  INTN          Index;
  BOOLEAN       ProcessMbr;
  ERROR_STATUS  Status;
  EFI_STATUS    EfiStatus;
  PATH_INFO     InputPathInfo = {0};
  PATH_INFO     OutputPathInfo = {0};
  UINT64        LogLevel;

  SetUtilityName (UTILITY_NAME);

  AppName = *argv;
  argv ++;
  argc --;
  
  ProcessMbr    = FALSE;

  if (argc == 0) {
    PrintUsage();
    return 0;
  }
   
  //
  // Parse command line
  //
  for (Index = 0; Index < argc; Index ++) {
    if ((stricmp (argv[Index], "-l") == 0) || (stricmp (argv[Index], "--list") == 0)) {
      ListDrive ();
      return 0;
    } 
    
    if ((stricmp (argv[Index], "-m") == 0) || (stricmp (argv[Index], "--mbr") == 0)) {
      ProcessMbr = TRUE;
      continue;
    } 
    
    if ((stricmp (argv[Index], "-i") == 0) || (stricmp (argv[Index], "--input") == 0)) {
      InputPathInfo.Path  = argv[Index + 1];
      InputPathInfo.Input = TRUE;
      if (InputPathInfo.Path == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Input file name can't be NULL");
        return 1;
      } 
      if (InputPathInfo.Path[0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Input file is missing");
        return 1;       
      }
      ++Index;
      continue;
    }

    if ((stricmp (argv[Index], "-o") == 0) || (stricmp (argv[Index], "--output") == 0)) {
      OutputPathInfo.Path  = argv[Index + 1];
      OutputPathInfo.Input = FALSE;
      if (OutputPathInfo.Path == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Output file name can't be NULL");
        return 1;
      } 
      if (OutputPathInfo.Path[0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output file is missing");
        return 1;       
      }
      ++Index;
      continue;
    }
    
    if ((stricmp (argv[Index], "-h") == 0) || (stricmp (argv[Index], "--help") == 0)) {
      PrintUsage ();
      return 0;
    } 
    
    if (stricmp (argv[Index], "--version") == 0) {
      Version ();
      return 0;
    } 
    
    if ((stricmp (argv[Index], "-v") == 0) || (stricmp (argv[Index], "--verbose") == 0)) {
      continue;
    } 
    
    if ((stricmp (argv[Index], "-q") == 0) || (stricmp (argv[Index], "--quiet") == 0)) {
      continue;
    } 
    
    if ((stricmp (argv[Index], "-d") == 0) || (stricmp (argv[Index], "--debug") == 0)) {
      EfiStatus = AsciiStringToUint64 (argv[Index + 1], FALSE, &LogLevel);
      if (EFI_ERROR (EfiStatus)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[Index], argv[Index + 1]);
        return 1;
      }
      if (LogLevel > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, currnt input level is %d", (int) LogLevel);
        return 1;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", argv[Index + 1]);
      ++Index;
      continue;
    }

    //
    // Don't recognize the parameter.
    //
    Error (NULL, 0, 1000, "Unknown option", "%s", argv[Index]);
    return 1;
  }
  
  if (InputPathInfo.Path == NULL) {
    Error (NULL, 0, 1001, "Missing options", "Input file is missing");
    return 1;
  }

  if (OutputPathInfo.Path == NULL) {
    Error (NULL, 0, 1001, "Missing options", "Output file is missing");
    return 1;
  }
  
  if (GetPathInfo(&InputPathInfo) != ErrorSuccess) {
    Error (NULL, 0, 1003, "Invalid option value", "Input file can't be found.");
    return 1;
  }

  if (GetPathInfo(&OutputPathInfo) != ErrorSuccess) {
    Error (NULL, 0, 1003, "Invalid option value", "Output file can't be found.");
    return 1;
  }
  
  //
  // Process DBR (Patch or Read)
  //
  Status = ProcessBsOrMbr (&InputPathInfo, &OutputPathInfo, ProcessMbr);

  if (Status == ErrorSuccess) {
    fprintf (
      stdout, 
      "%s %s: successful!\n", 
      (OutputPathInfo.Type != PathFile) ? "Write" : "Read", 
      ProcessMbr ? "MBR" : "DBR"
      );
    return 0;
  } else {
    fprintf (
      stderr, 
      "%s: %s %s: failed - %s (LastError: 0x%x)!\n",
      (Status == ErrorNoMbr) ? "WARNING" : "ERROR",
      (OutputPathInfo.Type != PathFile) ? "Write" : "Read", 
      ProcessMbr ? "MBR" : "DBR", 
      ErrorStatusDesc[Status],
      GetLastError ()
      );
    return 1;
  }
}
