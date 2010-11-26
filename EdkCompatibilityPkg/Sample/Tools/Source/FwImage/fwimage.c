/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  fwimage.c

Abstract:

  Converts a pe32/pe32+ image to an FW image type

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "TianoCommon.h"
#include "EfiImage.h"
#include "EfiUtilityMsgs.c"

#define UTILITY_NAME    "FwImage"
#define UTILITY_VERSION "v1.0"

typedef union {
  IMAGE_NT_HEADERS32 PeHeader32;
  IMAGE_NT_HEADERS64 PeHeader64;
} PE_HEADER;

VOID
Usage (
  VOID
  )
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Firmware Image Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",

#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif

    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]... FWTYPE SOURCE [DEST]",
    "Description:",
    "  Converts a pe32/pe32+ SOURCE to DEST with FWTYPE image type.",
    "Options:",
    "  FWTYPE        Can be one of APPLICATION, BS_DRIVER, RT_DRIVER, SAL_RT_DRIVER,",
    "                COMBINED_PEIM_DRIVER, SECURITY_CORE, PEI_CORE, PE32_PEIM and",
    "                RELOCATABLE_PEIM",
    "  -t time-date  Add Time Stamp for output image",
    "  -e            Not clear ExceptionTable for output image",
    "  -r            Not strip zero pending of .reloc for output image",
    NULL
  };

  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }  
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

VOID
ZeroExceptionTable (
  IN UINT8                 *FileBuffer,
  IN EFI_IMAGE_DOS_HEADER  *DosHdr,
  IN PE_HEADER             *PeHdr
  )
{
  UINT32                   PdataSize;
  UINT32                   PdataOffset;
  UINT32                   PdataRVASize;
  UINT32                   PdataRVA;
  UINT32                   SectionOffset;
  UINT16                   SectionNumber;
  UINT32                   SectionNameSize;
  EFI_IMAGE_SECTION_HEADER *Section;

  PdataSize     = 0;
  PdataOffset   = 0;
  PdataRVASize  = 0;
  PdataRVA      = 0;
  SectionOffset = 0;

  //
  // Search .pdata section
  //
  if (PeHdr->PeHeader32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    if ((PeHdr->PeHeader32.OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXCEPTION) &&
        (PeHdr->PeHeader32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress != 0) &&
        (PeHdr->PeHeader32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size != 0)) {

      PdataRVA     = PeHdr->PeHeader32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
      PdataRVASize = PeHdr->PeHeader32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

      PeHdr->PeHeader32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress = 0;
      PeHdr->PeHeader32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size = 0;

      SectionOffset = sizeof(PeHdr->PeHeader32);
    }
  } else {
    if ((PeHdr->PeHeader64.OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXCEPTION) &&
        (PeHdr->PeHeader64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress != 0) &&
        (PeHdr->PeHeader64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size != 0)) {

      PdataRVA     = PeHdr->PeHeader64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
      PdataRVASize = PeHdr->PeHeader64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

      PeHdr->PeHeader64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress = 0;
      PeHdr->PeHeader64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size = 0;

      SectionOffset = sizeof(PeHdr->PeHeader64);
    }
  }

  if ((PdataRVASize != 0) && (PdataRVA != 0)) {

    SectionNumber = PeHdr->PeHeader32.FileHeader.NumberOfSections;
    SectionNameSize = sizeof(Section->Name);
    while (SectionNumber > 0) {
      Section = (EFI_IMAGE_SECTION_HEADER *) &FileBuffer[DosHdr->e_lfanew + SectionOffset];
      if (strcmp (Section->Name, ".pdata") == 0) {
        //
        // Zero .pdata Section Header Name
        //
        memset (
          FileBuffer + DosHdr->e_lfanew + SectionOffset,
          0,
          SectionNameSize);

        //
        // Zero .pdata Secton raw data
        //
        PdataOffset = Section->PointerToRawData;
        PdataSize   = Section->SizeOfRawData;
        memset (FileBuffer + PdataOffset, 0, PdataSize);
        break;
      }
      SectionNumber--;
      SectionOffset += sizeof(EFI_IMAGE_SECTION_HEADER);
    }
  }
  
  return ;
}

VOID
StripZeroPendingReloc (
  IN UINT8                 *FileBuffer,
  IN OUT UINTN             *FileLength,
  IN EFI_IMAGE_DOS_HEADER  *DosHdr,
  IN PE_HEADER             *PeHdr
  )
{
  EFI_IMAGE_OPTIONAL_HEADER32  *Optional32;
  EFI_IMAGE_OPTIONAL_HEADER64  *Optional64;
  EFI_IMAGE_SECTION_HEADER     *SectionHeader;
  UINTN                        AllignedRelocSize;
  UINTN                        Index;

  if (PeHdr->PeHeader32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->PeHeader32.OptionalHeader;
    if ((Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) &&
        (Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size != 0)) {
      SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->PeHeader32.FileHeader.SizeOfOptionalHeader);
      for (Index = 0; Index < PeHdr->PeHeader32.FileHeader.NumberOfSections; Index++, SectionHeader++) {
        //
        // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
        //
        if (strcmp (SectionHeader->Name, ".reloc") == 0) {
          SectionHeader->Misc.VirtualSize = Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

          AllignedRelocSize = (Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size +
                               Optional32->FileAlignment - 1) & (~(Optional32->FileAlignment - 1));
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
              Optional32->SizeOfImage           -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
              Optional32->SizeOfInitializedData -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
              SectionHeader->SizeOfRawData       = AllignedRelocSize;
              *FileLength                        = Optional32->SizeOfImage;
            }
          }
        }
      }
    }
  } else {
    Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->PeHeader64.OptionalHeader;
    if ((Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) &&
        (Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size != 0)) {
      SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof(UINT32) + sizeof (EFI_IMAGE_FILE_HEADER) + PeHdr->PeHeader64.FileHeader.SizeOfOptionalHeader);
      for (Index = 0; Index < PeHdr->PeHeader64.FileHeader.NumberOfSections; Index++, SectionHeader++) {
        //
        // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
        //
        if (strcmp (SectionHeader->Name, ".reloc") == 0) {
          SectionHeader->Misc.VirtualSize = Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

          AllignedRelocSize = (Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size +
                               Optional64->FileAlignment - 1) & (~(Optional64->FileAlignment - 1));
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
              Optional64->SizeOfImage           -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
              Optional64->SizeOfInitializedData -= (SectionHeader->SizeOfRawData - AllignedRelocSize);
              SectionHeader->SizeOfRawData       = AllignedRelocSize;
              *FileLength                        = Optional64->SizeOfImage;
            }
          }
        }
      }
    }
  }
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
  ULONG                        Type;
  PUCHAR                       Ext;
  PUCHAR                       p;
  PUCHAR                       pe;
  PUCHAR                       OutImageName;
  UCHAR                        outname[500];
  FILE                         *fpIn;
  FILE                         *fpOut;
  EFI_IMAGE_DOS_HEADER         *DosHdr;
  PE_HEADER                    *PeHdr;
  time_t                       TimeStamp;
  struct tm                    TimeStruct;
  EFI_IMAGE_DOS_HEADER         BackupDosHdr;
  ULONG                        Index;
  BOOLEAN                      TimeStampPresent;
  BOOLEAN                      NeedClearExceptionTable;
  BOOLEAN                      NeedStripZeroPendingReloc;
  UINT8                        *FileBuffer;
  UINTN                        FileLength;
  EFI_IMAGE_OPTIONAL_HEADER32  *Optional32;
  EFI_IMAGE_OPTIONAL_HEADER64  *Optional64;

  SetUtilityName (UTILITY_NAME);
  //
  // Assign to fix compile warning
  //
  OutImageName      = NULL;
  Type              = 0;
  Ext               = 0;
  TimeStamp         = 0;
  TimeStampPresent  = FALSE;

  NeedClearExceptionTable   = TRUE;
  NeedStripZeroPendingReloc = TRUE;

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
  // Look for -e option.
  //
  if ((argc > 1) && !strcmp (argv[1], "-e")) {
    NeedClearExceptionTable = FALSE;
    //
    // Skip over the args
    //
    argc -= 1;
    argv += 1;
  }

  //
  // Look for -r option
  //
  if ((argc > 1) && !strcmp (argv[1], "-r")) {
    NeedStripZeroPendingReloc = FALSE;
    //
    // Skip over the args
    //
    argc -= 1;
    argv += 1;
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

  if (_stricmp (p, "app") == 0 || _stricmp (p, "APPLICATION") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION;
    Ext   = ".efi";

  } else if (_stricmp (p, "bsdrv") == 0 || _stricmp (p, "BS_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".efi";

  } else if (_stricmp (p, "rtdrv") == 0 || _stricmp (p, "RT_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
    Ext   = ".efi";

  } else if (_stricmp (p, "rtdrv") == 0 || _stricmp (p, "SAL_RT_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER;
    Ext   = ".efi";
  } else if (_stricmp (p, "SECURITY_CORE") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".sec";
  } else if (_stricmp (p, "peim") == 0 ||
           _stricmp (p, "PEI_CORE") == 0 ||
           _stricmp (p, "PE32_PEIM") == 0 ||
           _stricmp (p, "RELOCATABLE_PEIM") == 0 ||
           _stricmp (p, "combined_peim_driver") == 0
          ) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".pei";
  } else {
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
  DosHdr = (EFI_IMAGE_DOS_HEADER *) FileBuffer;
  if (DosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
    Error (NULL, 0, 0, argv[2], "DOS header signature not found in source image");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  PeHdr = (PE_HEADER *)(FileBuffer + DosHdr->e_lfanew);
  if (PeHdr->PeHeader32.Signature != IMAGE_NT_SIGNATURE) {
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
    FileBuffer[Index] = (UINT8) DosHdr->e_cp;
  }
  
  //
  // Modify some fields in the PE header
  //

  //
  // TimeDateStamp's offset is fixed for PE32/32+
  //
  if (TimeStampPresent) {
    PeHdr->PeHeader32.FileHeader.TimeDateStamp = (UINT32) TimeStamp;
  }

  //
  // PE32/32+ has different optional header layout
  // Determine format is PE32 or PE32+ before modification
  //
  if (PeHdr->PeHeader32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // PE32 image
    //
    Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->PeHeader32.OptionalHeader;

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
    Optional32->SizeOfStackReserve          = 0;
    Optional32->SizeOfStackCommit           = 0;
    Optional32->SizeOfHeapReserve           = 0;
    Optional32->SizeOfHeapCommit            = 0;
    Optional32->Subsystem                   = (USHORT) Type;

    //
    // Strip zero padding at the end of the .reloc section 
    //
    if (NeedStripZeroPendingReloc) {
      StripZeroPendingReloc (FileBuffer, &FileLength, DosHdr, PeHdr);
    }
  } else if (PeHdr->PeHeader32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    //
    // PE32+ image
    //
    Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->PeHeader64.OptionalHeader;

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
    Optional64->SizeOfStackReserve          = 0;
    Optional64->SizeOfStackCommit           = 0;
    Optional64->SizeOfHeapReserve           = 0;
    Optional64->SizeOfHeapCommit            = 0;
    Optional64->Subsystem                   = (USHORT) Type;

    //
    // Strip zero padding at the end of the .reloc section 
    //
    if (NeedStripZeroPendingReloc) {
      StripZeroPendingReloc (FileBuffer, &FileLength, DosHdr, PeHdr);
    }
  } else {
    Error (NULL, 0, 0, argv[2], "Unsupported PE image");
    fclose (fpIn);
    fclose (fpOut);
    return STATUS_ERROR;
  }

  //
  // Zero PDATA section for smaller binary size after compression
  //
  if (NeedClearExceptionTable) {
    ZeroExceptionTable (FileBuffer, DosHdr, PeHdr);
  }

  FWriteFile (fpOut, FileBuffer, FileLength);

  //
  // Done
  //
  fclose (fpIn);
  fclose (fpOut);

  return STATUS_SUCCESS;
}
