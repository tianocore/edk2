/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    fwimage.c

Abstract:

    Converts a pe32+ image to an FW image type

--*/

#include "WinNtInclude.h"

#ifndef __GNUC__
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h>

#include "CommonLib.h"
#include "EfiUtilityMsgs.c"

#define UTILITY_NAME  "FwImage"

#ifdef __GNUC__
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef unsigned char *PUCHAR;
typedef unsigned short USHORT;
#endif

VOID
Usage (
  VOID
  )
{
  printf ("Usage: " UTILITY_NAME "  {-t time-date} [BASE|SEC|PEI_CORE|PEIM|DXE_CORE|DXE_DRIVER|DXE_RUNTIME_DRIVER|DXE_SAL_DRIVER|DXE_SMM_DRIVER|TOOL|UEFI_DRIVER|UEFI_APPLICATION|USER_DEFINED] peimage [outimage]");
}

static
STATUS
FCopyFile (
  FILE    *in,
  FILE    *out
  )
{
  ULONG filesize;
  ULONG offset;
  ULONG length;
  UCHAR Buffer[8 * 1024];

  fseek (in, 0, SEEK_END);
  filesize = ftell (in);

  fseek (in, 0, SEEK_SET);
  fseek (out, 0, SEEK_SET);

  offset = 0;
  while (offset < filesize) {
    length = sizeof (Buffer);
    if (filesize - offset < length) {
      length = filesize - offset;
    }

    fread (Buffer, length, 1, in);
    fwrite (Buffer, length, 1, out);
    offset += length;
  }

  if ((ULONG) ftell (out) != filesize) {
    Error (NULL, 0, 0, "write error", NULL);
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

static
STATUS
FReadFile (
  FILE    *in,
  VOID    **Buffer,
  UINTN   *Length
  )
{
  fseek (in, 0, SEEK_END);
  *Length = ftell (in);
  *Buffer = malloc (*Length);
  fseek (in, 0, SEEK_SET);
  fread (*Buffer, *Length, 1, in);
  return STATUS_SUCCESS;
}

static
STATUS
FWriteFile (
  FILE    *out,
  VOID    *Buffer,
  UINTN   Length
  )
{
  fseek (out, 0, SEEK_SET);
  fwrite (Buffer, Length, 1, out);
  if ((ULONG) ftell (out) != Length) {
    Error (NULL, 0, 0, "write error", NULL);
    return STATUS_ERROR;
  }
  free (Buffer);
  return STATUS_SUCCESS;
}

int
main (
  int  argc,
  char *argv[]
  )
/*++

Routine Description:

  Main function.

Arguments:

  argc - Number of command line parameters.
  argv - Array of pointers to command line parameter strings.

Returns:
  STATUS_SUCCESS - Utility exits successfully.
  STATUS_ERROR   - Some error occurred during execution.

--*/
{
  ULONG             Type;
  PUCHAR            Ext;
  PUCHAR            p;
  PUCHAR            pe;
  PUCHAR            OutImageName;
  UCHAR             outname[500];
  FILE              *fpIn;
  FILE              *fpOut;
  VOID              *ZeroBuffer;
  EFI_IMAGE_DOS_HEADER  *DosHdr;
  EFI_IMAGE_NT_HEADERS  *PeHdr;
  EFI_IMAGE_OPTIONAL_HEADER32  *Optional32;
  EFI_IMAGE_OPTIONAL_HEADER64  *Optional64;
  time_t            TimeStamp;
  struct tm         TimeStruct;
  EFI_IMAGE_DOS_HEADER  BackupDosHdr;
  ULONG             Index;
  ULONG             Index1;
  ULONG             Index2;
  ULONG             Index3;
  BOOLEAN           TimeStampPresent;
  UINTN                                 AllignedRelocSize;
  UINTN                                 Delta;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;
  UINT8      *FileBuffer;
  UINTN      FileLength;
  RUNTIME_FUNCTION  *RuntimeFunction;
  UNWIND_INFO       *UnwindInfo;

  SetUtilityName (UTILITY_NAME);
  //
  // Assign to fix compile warning
  //
  OutImageName      = NULL;
  Type              = 0;
  Ext               = 0;
  TimeStamp         = 0;
  TimeStampPresent  = FALSE;

  //
  // Look for -t time-date option first. If the time is "0", then
  // skip it.
  //
  if ((argc > 2) && !strcmp (argv[1], "-t")) {
    TimeStampPresent = TRUE;
    if (strcmp (argv[2], "0") != 0) {
      //
      // Convert the string to a value
      //
      memset ((char *) &TimeStruct, 0, sizeof (TimeStruct));
      if (sscanf(
          argv[2], "%d/%d/%d,%d:%d:%d",
          &TimeStruct.tm_mon,   /* months since January - [0,11] */
          &TimeStruct.tm_mday,  /* day of the month - [1,31] */
          &TimeStruct.tm_year,  /* years since 1900 */
          &TimeStruct.tm_hour,  /* hours since midnight - [0,23] */
          &TimeStruct.tm_min,   /* minutes after the hour - [0,59] */
          &TimeStruct.tm_sec    /* seconds after the minute - [0,59] */
            ) != 6) {
        Error (NULL, 0, 0, argv[2], "failed to convert to mm/dd/yyyy,hh:mm:ss format");
        return STATUS_ERROR;
      }
      //
      // Now fixup some of the fields
      //
      TimeStruct.tm_mon--;
      TimeStruct.tm_year -= 1900;
      //
      // Sanity-check values?
      // Convert
      //
      TimeStamp = mktime (&TimeStruct);
      if (TimeStamp == (time_t) - 1) {
        Error (NULL, 0, 0, argv[2], "failed to convert time");
        return STATUS_ERROR;
      }
    }
    //
    // Skip over the args
    //
    argc -= 2;
    argv += 2;
  }
  //
  // Check for enough args
  //
  if (argc < 3) {
    Usage ();
    return STATUS_ERROR;
  }

  if (argc == 4) {
    OutImageName = argv[3];
  }
  //
  // Get new image type
  //
  p = argv[1];
  if (*p == '/' || *p == '\\') {
    p += 1;
  }

  if (stricmp (p, "app") == 0 || stricmp (p, "UEFI_APPLICATION") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION;
    Ext   = ".efi";

  } else if (stricmp (p, "bsdrv") == 0 || stricmp (p, "DXE_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".efi";

  } else if (stricmp (p, "rtdrv") == 0 || stricmp (p, "DXE_RUNTIME_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
    Ext   = ".efi";

  } else if (stricmp (p, "rtdrv") == 0 || stricmp (p, "DXE_SAL_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER;
    Ext   = ".efi";
  } else if (stricmp (p, "SEC") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".sec";
  } else if (stricmp (p, "peim") == 0 ||
           stricmp (p, "BASE") == 0 ||
           stricmp (p, "PEI_CORE") == 0 ||
           stricmp (p, "PEIM") == 0 ||
           stricmp (p, "DXE_SMM_DRIVER") == 0 ||
           stricmp (p, "TOOL") == 0 ||
           stricmp (p, "UEFI_APPLICATION") == 0 ||
           stricmp (p, "USER_DEFINED") == 0 ||
           stricmp (p, "UEFI_DRIVER") == 0 ||
           stricmp (p, "DXE_CORE") == 0
          ) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".pei";
  } else {
  	printf ("%s", p);
    Usage ();
    return STATUS_ERROR;
  }
  //
  // open source file
  //
  fpIn = fopen (argv[2], "rb");
  if (!fpIn) {
    Error (NULL, 0, 0, argv[2], "failed to open input file for reading");
    return STATUS_ERROR;
  }

  FReadFile (fpIn, (VOID **)&FileBuffer, &FileLength);

  //
  // Read the dos & pe hdrs of the image
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)FileBuffer;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    Error (NULL, 0, 0, argv[2], "DOS header signature not found in source image");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  PeHdr = (EFI_IMAGE_NT_HEADERS *)(FileBuffer + DosHdr->e_lfanew);
  if (PeHdr->Signature != EFI_IMAGE_NT_SIGNATURE) {
    Error (NULL, 0, 0, argv[2], "PE header signature not found in source image");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  //
  // open output file
  //
  strcpy (outname, argv[2]);
  pe = NULL;
  for (p = outname; *p; p++) {
    if (*p == '.') {
      pe = p;
    }
  }

  if (!pe) {
    pe = p;
  }

  strcpy (pe, Ext);

  if (!OutImageName) {
    OutImageName = outname;
  }

  fpOut = fopen (OutImageName, "w+b");
  if (!fpOut) {
    Error (NULL, 0, 0, OutImageName, "could not open output file for writing");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  //
  // Zero all unused fields of the DOS header
  //
  memcpy (&BackupDosHdr, DosHdr, sizeof (EFI_IMAGE_DOS_HEADER));
  memset (DosHdr, 0, sizeof (EFI_IMAGE_DOS_HEADER));
  DosHdr->e_magic  = BackupDosHdr.e_magic;
  DosHdr->e_lfanew = BackupDosHdr.e_lfanew;

  for (Index = sizeof (EFI_IMAGE_DOS_HEADER); Index < (ULONG) DosHdr->e_lfanew; Index++) {
    FileBuffer[Index] = DosHdr->e_cp;
  }

  //
  // Path the PE header
  //
  PeHdr->OptionalHeader.Subsystem = (USHORT) Type;
  if (TimeStampPresent) {
    PeHdr->FileHeader.TimeDateStamp = (UINT32) TimeStamp;
  }

  if (PeHdr->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->OptionalHeader;
    Optional32->MajorLinkerVersion          = 0;
    Optional32->MinorLinkerVersion          = 0;
    Optional32->MajorOperatingSystemVersion = 0;
    Optional32->MinorOperatingSystemVersion = 0;
    Optional32->MajorImageVersion           = 0;
    Optional32->MinorImageVersion           = 0;
    Optional32->MajorSubsystemVersion       = 0;
    Optional32->MinorSubsystemVersion       = 0;
    Optional32->Win32VersionValue           = 0;
    Optional32->CheckSum                    = 0;
    Optional32->SizeOfStackReserve = 0;
    Optional32->SizeOfStackCommit  = 0;
    Optional32->SizeOfHeapReserve  = 0;
    Optional32->SizeOfHeapCommit   = 0;

    //
    // Strip zero padding at the end of the .reloc section 
    //
    if (Optional32->NumberOfRvaAndSizes >= 6) {
      if (Optional32->DataDirectory[5].Size != 0) {
        SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
        for (Index = 0; Index < PeHdr->FileHeader.NumberOfSections; Index++, SectionHeader++) {
          //
          // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
          //
          if (SectionHeader->VirtualAddress == Optional32->DataDirectory[5].VirtualAddress) {
            SectionHeader->Misc.VirtualSize = Optional32->DataDirectory[5].Size;
            AllignedRelocSize = (Optional32->DataDirectory[5].Size + Optional32->FileAlignment - 1) & (~(Optional32->FileAlignment - 1));
            //
            // Check to see if there is zero padding at the end of the base relocations
            //
            if (AllignedRelocSize < SectionHeader->SizeOfRawData) {
              //
              // Check to see if the base relocations are at the end of the file
              //
              if (SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData == Optional32->SizeOfImage) {
                //
                // All the required conditions are met to strip the zero padding of the end of the base relocations section
                //
                Optional32->SizeOfImage -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                Optional32->SizeOfInitializedData -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                SectionHeader->SizeOfRawData = AllignedRelocSize;
                FileLength = Optional32->SizeOfImage;
              }
            }
          }
        }
      }
    }
  } 
  if (PeHdr->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->OptionalHeader;
    Optional64->MajorLinkerVersion          = 0;
    Optional64->MinorLinkerVersion          = 0;
    Optional64->MajorOperatingSystemVersion = 0;
    Optional64->MinorOperatingSystemVersion = 0;
    Optional64->MajorImageVersion           = 0;
    Optional64->MinorImageVersion           = 0;
    Optional64->MajorSubsystemVersion       = 0;
    Optional64->MinorSubsystemVersion       = 0;
    Optional64->Win32VersionValue           = 0;
    Optional64->CheckSum                    = 0;
    Optional64->SizeOfStackReserve = 0;
    Optional64->SizeOfStackCommit  = 0;
    Optional64->SizeOfHeapReserve  = 0;
    Optional64->SizeOfHeapCommit   = 0;

    //
    // Zero the .pdata section if the machine type is X64 and the Debug Directory is empty
    //
    if (PeHdr->FileHeader.Machine == 0x8664) { // X64
      if (Optional64->NumberOfRvaAndSizes >= 4) {
        if (Optional64->NumberOfRvaAndSizes < 7 || (Optional64->NumberOfRvaAndSizes >= 7 && Optional64->DataDirectory[6].Size == 0)) {
          SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
          for (Index = 0; Index < PeHdr->FileHeader.NumberOfSections; Index++, SectionHeader++) {
            if (SectionHeader->VirtualAddress == Optional64->DataDirectory[3].VirtualAddress) {
              RuntimeFunction = (RUNTIME_FUNCTION *)(FileBuffer + SectionHeader->PointerToRawData);
              for (Index1 = 0; Index1 < Optional64->DataDirectory[3].Size / sizeof (RUNTIME_FUNCTION); Index1++, RuntimeFunction++) {
                SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
                for (Index2 = 0; Index2 < PeHdr->FileHeader.NumberOfSections; Index2++, SectionHeader++) {
                  if (RuntimeFunction->UnwindInfoAddress > SectionHeader->VirtualAddress && RuntimeFunction->UnwindInfoAddress < (SectionHeader->VirtualAddress + SectionHeader->SizeOfRawData)) {
                    UnwindInfo = (UNWIND_INFO *)(FileBuffer + SectionHeader->PointerToRawData + (RuntimeFunction->UnwindInfoAddress - SectionHeader->VirtualAddress));
                    if (UnwindInfo->Version == 1) {
                      memset (UnwindInfo + 1, 0, UnwindInfo->CountOfUnwindCodes * sizeof (UINT16));
                      memset (UnwindInfo, 0, sizeof (UNWIND_INFO));
                    }
                  }
                }
                memset (RuntimeFunction, 0, sizeof (RUNTIME_FUNCTION));
              }
            }
          }
          Optional64->DataDirectory[3].Size = 0;
          Optional64->DataDirectory[3].VirtualAddress = 0;
        }
      }
    }

    //
    // Strip zero padding at the end of the .reloc section 
    //
    if (Optional64->NumberOfRvaAndSizes >= 6) {
      if (Optional64->DataDirectory[5].Size != 0) {
        SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->FileHeader.SizeOfOptionalHeader);
        for (Index = 0; Index < PeHdr->FileHeader.NumberOfSections; Index++, SectionHeader++) {
          //
          // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
          //
          if (SectionHeader->VirtualAddress == Optional64->DataDirectory[5].VirtualAddress) {
            SectionHeader->Misc.VirtualSize = Optional64->DataDirectory[5].Size;
            AllignedRelocSize = (Optional64->DataDirectory[5].Size + Optional64->FileAlignment - 1) & (~(Optional64->FileAlignment - 1));
            //
            // Check to see if there is zero padding at the end of the base relocations
            //
            if (AllignedRelocSize < SectionHeader->SizeOfRawData) {
              //
              // Check to see if the base relocations are at the end of the file
              //
              if (SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData == Optional64->SizeOfImage) {
                //
                // All the required conditions are met to strip the zero padding of the end of the base relocations section
                //
                Optional64->SizeOfImage -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                Optional64->SizeOfInitializedData -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
                SectionHeader->SizeOfRawData = AllignedRelocSize;
                FileLength = Optional64->SizeOfImage;
              }
            }
          }
        }
      }
    }
  }

  FWriteFile (fpOut, FileBuffer, FileLength);

  //
  // Done
  //
  fclose (fpIn);
  fclose (fpOut);
  //
  // printf ("Created %s\n", OutImageName);
  //
  return STATUS_SUCCESS;
}
