/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GnuGenBootSector.c
  
Abstract:
  Reading/writing MBR/DBR.
  NOTE:
    If we write MBR to disk, we just update the MBR code and the partition table wouldn't be over written.
    If we process DBR, we will patch MBR to set first partition active if no active partition exists.

**/

#include "CommonLib.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <Common/UefiBaseTypes.h>

#include "ParseInf.h"
#include "EfiUtilityMsgs.h"

//
// Utility Name
//
#define UTILITY_NAME  "GnuGenBootSector"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

#define MAX_DRIVE                             26
#define PARTITION_TABLE_OFFSET                0x1BE

#define SIZE_OF_PARTITION_ENTRY               0x10

#define PARTITION_ENTRY_STARTLBA_OFFSET       8

#define PARTITION_ENTRY_NUM                   4

#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6

typedef struct _DRIVE_TYPE_DESC {
  UINTN  Type;
  CHAR8  *Description;
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
  {(UINTN) -1, NULL}
};

typedef struct _DRIVE_INFO {
  CHAR8             VolumeLetter;
  DRIVE_TYPE_DESC   *DriveType;
  UINTN             DiskNumber;
} DRIVE_INFO;

typedef enum {
  PathUnknown,
  PathFile,
  PathFloppy,
  PathUsb,
  PathIde
} PATH_TYPE;

typedef struct _PATH_INFO {
  CHAR8            *Path;
  CHAR8            PhysicalPath[260];
  PATH_TYPE        Type;
  BOOLEAN          Input;
} PATH_INFO;

typedef enum {
  ErrorSuccess,
  ErrorFileCreate,
  ErrorFileReadWrite,
  ErrorNoMbr,
  ErrorFatType,
  ErrorPath,
} ERROR_STATUS;

CHAR8 *ErrorStatusDesc[] = {
  "Success",
  "Failed to create files",
  "Failed to read/write files",
  "No MBR exists",
  "Failed to detect Fat type",
  "Inavlid path"
};


//UnSupported Windows API functions.
UINTN GetLogicalDrives(void) { return 1; }



/**
  Get path information, including physical path for Linux platform.

  @param PathInfo   Point to PATH_INFO structure.

  @return whether path is valid.
**/
ERROR_STATUS
GetPathInfo (
  PATH_INFO   *PathInfo
  )
{
  FILE        *f;

  if (strncmp(PathInfo->Path, "/dev/", 5) == 0) {
    //
    // Process disk path here.
    // 
    
    // Process floppy disk
    if (PathInfo->Path[5] == 'f' && PathInfo->Path[6] == 'd' && PathInfo->Path[8] == '\0') {
      PathInfo->Type = PathFloppy;
      strcpy (PathInfo->PhysicalPath, PathInfo->Path);
      
      return ErrorSuccess;
    } else {
    // Other disk types is not supported yet.
    fprintf (stderr, "ERROR: It's not a floppy disk!\n");
    return ErrorPath;
    }  
     
    // Try to open the device.   
    f = fopen(PathInfo->Path,"r");
    if (f == NULL) {
      printf ("error :open device failed!\n");
      return ErrorPath;
    }
    fclose (f);
    return ErrorSuccess;
  }
 
  // Process file path here.
  PathInfo->Type = PathFile;
  if (PathInfo->Input) {
    // If path is file path, check whether file is valid.
    printf("Path = %s\n",PathInfo->Path);
    f = fopen (PathInfo->Path, "r");
    if (f == NULL) {
      fprintf (stderr, "Test error E2003: File was not provided!\n");
      return ErrorPath;
    }
    fclose (f);
  }

  strcpy(PathInfo->PhysicalPath, PathInfo->Path);
  return ErrorSuccess;

}

VOID
ListDrive (
  VOID
  )
{
  printf("-l or -list not supported!\n");
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
  BOOLEAN       ProcessMbr
  )
{
  CHAR8 FirstSector[0x200] = {0};
  CHAR8 FirstSectorBackup[0x200] = {0};
  
  FILE *InputFile;
  FILE *OutputFile;
  
  
  InputFile = fopen(InputInfo->PhysicalPath, "r");
  if (InputFile == NULL) {
    return ErrorFileReadWrite;
  }
   
  if (0x200 != fread(FirstSector, 1, 0x200, InputFile)) {
    fclose(InputFile);
    return ErrorFileReadWrite;
  }
  
  fclose(InputFile);
  
  //Not support USB and IDE.
  if (InputInfo->Type == PathUsb) {
    printf("USB has not been supported yet!");
    return ErrorSuccess;
  }
  
  if (InputInfo->Type == PathIde) {
    printf("IDE has not been supported yet!");
    return ErrorSuccess;
  } 
  
  //Process Floppy Disk
  OutputFile = fopen(OutputInfo->PhysicalPath, "r+");
  if (OutputFile == NULL) {
    OutputFile = fopen(OutputInfo->PhysicalPath, "w");
    if (OutputFile == NULL) {
      return ErrorFileReadWrite;
    }
  }
  
  if (OutputInfo->Type != PathFile) {
    if (ProcessMbr) {
      //
      // Use original partition table
      //
      if (0x200 != fread (FirstSectorBackup, 1, 0x200, OutputFile)) {
        fclose(OutputFile);
        return ErrorFileReadWrite; 
        }
      memcpy (FirstSector + 0x1BE, FirstSectorBackup + 0x1BE, 0x40);  
    }
  }
  if(0x200 != fwrite(FirstSector, 1, 0x200, OutputFile)) {
    fclose(OutputFile);
    return ErrorFileReadWrite;
  }
  
  fclose(OutputFile);
  return ErrorSuccess;
}


/**

  Displays the standard utility information to SDTOUT

**/
VOID
Version (
  VOID
  )
{
  printf ("%s v%d.%d %s-Utility to retrieve and update the boot sector or MBR.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
  printf ("Copyright (c) 2007-2010 Intel Corporation. All rights reserved.\n");
}


VOID
PrintUsage (
  VOID
    )
{
  Version();
  printf ("\nUsage: \n\
   GenBootSector\n\
     [-l, --list list disks]\n\
     [-i, --input Filename]\n\
     [-o, --output Filename]\n\
     [-m, --mbr process the MBR also]\n\
     [-v, --verbose]\n\
     [--version]\n\
     [-q, --quiet disable all messages except fatal errors]\n\
     [-d, --debug[#]\n\
     [-h, --help]\n");
}

int
main (
  int  argc,
  char *argv[]
  )
{
  INTN           Index;
  BOOLEAN        ProcessMbr;
  ERROR_STATUS   Status;
  EFI_STATUS     EfiStatus;
  PATH_INFO      InputPathInfo;
  PATH_INFO      OutputPathInfo;
  UINT64         LogLevel;

  SetUtilityName (UTILITY_NAME);
  
  ZeroMem(&InputPathInfo, sizeof(PATH_INFO));
  ZeroMem(&OutputPathInfo, sizeof(PATH_INFO));
  
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
      errno 
      );
    return 1;
  }
}
