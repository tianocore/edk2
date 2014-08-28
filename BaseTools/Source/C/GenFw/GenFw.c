/** @file
Converts a pe32+ image to an FW, Te image type, or other specific image.

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "WinNtInclude.h"

#ifndef __GNUC__
#include <windows.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <Common/UefiBaseTypes.h>
#include <IndustryStandard/PeImage.h>
#include <Common/UefiInternalFormRepresentation.h>

//
// Acpi Table definition
//
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Acpi1_0.h>
#include <IndustryStandard/Acpi2_0.h>
#include <IndustryStandard/Acpi3_0.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>

#include "CommonLib.h"
#include "PeCoffLib.h"
#include "ParseInf.h"
#include "EfiUtilityMsgs.h"

#include "GenFw.h"

//
// Version of this utility
//
#define UTILITY_NAME "GenFw"
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 2

#define HII_RESOURCE_SECTION_INDEX  1
#define HII_RESOURCE_SECTION_NAME   "HII"

#define DEFAULT_MC_PAD_BYTE_VALUE  0xFF
#define DEFAULT_MC_ALIGNMENT       16

#define STATUS_IGNORE 0xA
//
// Structure definition for a microcode header
//
typedef struct {
  UINT32  HeaderVersion;
  UINT32  PatchId;
  UINT32  Date;
  UINT32  CpuId;
  UINT32  Checksum;
  UINT32  LoaderVersion;
  UINT32  PlatformId;
  UINT32  DataSize;   // if 0, then TotalSize = 2048, and TotalSize field is invalid
  UINT32  TotalSize;  // number of bytes
  UINT32  Reserved[3];
} MICROCODE_IMAGE_HEADER;

static EFI_GUID mZeroGuid = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};

static const char *gHiiPackageRCFileHeader[] = {
  "//",
  "//  DO NOT EDIT -- auto-generated file",
  "//",
  NULL
};

//
// Module image information
//
CHAR8  *mInImageName;
UINT32 mImageTimeStamp = 0;
UINT32 mImageSize = 0;
UINT32 mOutImageType = FW_DUMMY_IMAGE;


STATIC
EFI_STATUS
ZeroDebugData (
  IN OUT UINT8   *FileBuffer,
  BOOLEAN        ZeroDebug
  );

STATIC
EFI_STATUS
SetStamp (
  IN OUT UINT8  *FileBuffer,
  IN     CHAR8  *TimeStamp
  );

STATIC
STATUS
MicrocodeReadData (
  FILE          *InFptr,
  UINT32        *Data
  );

STATIC
VOID
Version (
  VOID
  )
/*++

Routine Description:

  Print out version information for this utility.

Arguments:

  None

Returns:

  None

--*/
{
  fprintf (stdout, "%s Version %d.%d %s \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

STATIC
VOID
Usage (
  VOID
  )
/*++

Routine Description:

  Print Help message.

Arguments:

  VOID

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "\nUsage: %s [options] <input_file>\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -o FileName, --outputfile FileName\n\
                        File will be created to store the ouput content.\n");
  fprintf (stdout, "  -e EFI_FILETYPE, --efiImage EFI_FILETYPE\n\
                        Create Efi Image. EFI_FILETYPE is one of BASE,SMM_CORE,\n\
                        PEI_CORE, PEIM, DXE_CORE, DXE_DRIVER, UEFI_APPLICATION,\n\
                        SEC, DXE_SAL_DRIVER, UEFI_DRIVER, DXE_RUNTIME_DRIVER,\n\
                        DXE_SMM_DRIVER, SECURITY_CORE, COMBINED_PEIM_DRIVER,\n\
                        PIC_PEIM, RELOCATABLE_PEIM, BS_DRIVER, RT_DRIVER,\n\
                        APPLICATION, SAL_RT_DRIVER to support all module types\n\
                        It can only be used together with --keepexceptiontable,\n\
                        --keepzeropending, -r, -o option.It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -c, --acpi            Create Acpi table.\n\
                        It can't be combined with other action options\n\
                        except for -o, -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -t, --terse           Create Te Image.\n\
                        It can only be used together with --keepexceptiontable,\n\
                        --keepzeropending, -r, -o option.It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -u, --dump            Dump TeImage Header.\n\
                        It can't be combined with other action options\n\
                        except for -o, -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -z, --zero            Zero the Debug Data Fields in the PE input image file.\n\
                        It also zeros the time stamp fields.\n\
                        This option can be used to compare the binary efi image.\n\
                        It can't be combined with other action options\n\
                        except for -o, -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -b, --exe2bin         Convert the input EXE to the output BIN file.\n\
                        It can't be combined with other action options\n\
                        except for -o, -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");;
  fprintf (stdout, "  -l, --stripped        Strip off the relocation info from PE or TE image.\n\
                        It can't be combined with other action options\n\
                        except for -o, -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -s timedate, --stamp timedate\n\
                        timedate format is \"yyyy-mm-dd 00:00:00\". if timedata \n\
                        is set to NOW, current system time is used. The support\n\
                        date scope is 1970-01-01 00+timezone:00:00\n\
                        ~ 2038-01-19 03+timezone:14:07\n\
                        The scope is adjusted according to the different zones.\n\
                        It can't be combined with other action options\n\
                        except for -o, -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -m, --mcifile         Convert input microcode txt file to microcode bin file.\n\
                        It can't be combined with other action options\n\
                        except for -o option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -j, --join            Combine multi microcode bin files to one file.\n\
                        It can be specified with -a, -p, -o option.\n\
                        No other options can be combined with it.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -a NUM, --align NUM   NUM is one HEX or DEC format alignment value.\n\
                        This option is only used together with -j option.\n");  
  fprintf (stdout, "  -p NUM, --pad NUM     NUM is one HEX or DEC format padding value.\n\
                        This option is only used together with -j option.\n");
  fprintf (stdout, "  --keepexceptiontable  Don't clear exception table.\n\
                        This option can be used together with -e or -t.\n\
                        It doesn't work for other options.\n");
  fprintf (stdout, "  --keepzeropending     Don't strip zero pending of .reloc.\n\
                        This option can be used together with -e or -t.\n\
                        It doesn't work for other options.\n");
  fprintf (stdout, "  -r, --replace         Overwrite the input file with the output content.\n\
                        If more input files are specified,\n\
                        the last input file will be as the output file.\n");
  fprintf (stdout, "  -g HiiPackageListGuid, --hiiguid HiiPackageListGuid\n\
                        Guid is used to specify hii package list guid.\n\
                        Its format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\n\
                        If not specified, the first Form FormSet guid is used.\n");
  fprintf (stdout, "  --hiipackage          Combine all input binary hii pacakges into \n\
                        a single package list as the text resource data(RC).\n\
                        It can't be combined with other action options\n\
                        except for -o option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  --hiibinpackage       Combine all input binary hii pacakges into \n\
                        a single package list as the binary resource section.\n\
                        It can't be combined with other action options\n\
                        except for -o option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  --rebase NewAddress   Rebase image to new base address. New address \n\
                        is also set to the first none code section header.\n\
                        It can't be combined with other action options\n\
                        except for -o or -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  --address NewAddress  Set new address into the first none code \n\
                        section header of the input image.\n\
                        It can't be combined with other action options\n\
                        except for -o or -r option. It is a action option.\n\
                        If it is combined with other action options, the later\n\
                        input action option will override the previous one.\n");
  fprintf (stdout, "  -v, --verbose         Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  -d, --debug level     Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version             Show program's version number and exit\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit\n");
}

STATIC
STATUS
CheckAcpiTable (
  VOID      *AcpiTable,
  UINT32    Length
  )
/*++

Routine Description:

  Check Acpi Table

Arguments:

  AcpiTable     Buffer for AcpiSection
  Length        AcpiSection Length

Returns:

  0             success
  non-zero      otherwise

--*/
{
  EFI_ACPI_DESCRIPTION_HEADER                   *AcpiHeader;
  EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  UINT32                                        ExpectedLength;

  AcpiHeader = (EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable;

  //
  // Generic check for AcpiTable length.
  //
  if (AcpiHeader->Length > Length) {
    Error (NULL, 0, 3000, "Invalid", "AcpiTable length check failed.", NULL);
    return STATUS_ERROR;
  }

  //
  // Currently, we only check must-have tables: FADT, FACS, DSDT,
  // and some important tables: MADT, MCFG.
  //
  switch (AcpiHeader->Signature) {

  //
  // "FACP" Fixed ACPI Description Table
  //
  case EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:
    switch (AcpiHeader->Revision) {
    case EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION:
      ExpectedLength = sizeof(EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE);
      break;
    case EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION:
      ExpectedLength = sizeof(EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE);
      break;
    case EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION:
      ExpectedLength = sizeof(EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE);
      break;
    default:
      if (AcpiHeader->Revision > EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION) {
        ExpectedLength = AcpiHeader->Length;
        break;
      }
      Error (NULL, 0, 3000, "Invalid", "FACP revision check failed.");
      return STATUS_ERROR;
    }
    if (ExpectedLength != AcpiHeader->Length) {
      Error (NULL, 0, 3000, "Invalid", "FACP length check failed.");
      return STATUS_ERROR;
    }
    break;

  //
  // "FACS" Firmware ACPI Control Structure
  //
  case EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE:
    Facs = (EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)AcpiTable;
    if (Facs->Version > EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) {
      break;
    }
    if ((Facs->Version != EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) &&
        (Facs->Version != EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) &&
        (Facs->Version != EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION)){
      Error (NULL, 0, 3000, "Invalid", "FACS version check failed.");
      return STATUS_ERROR;
    }
    if ((Facs->Length != sizeof(EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE)) &&
        (Facs->Length != sizeof(EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE)) &&
        (Facs->Length != sizeof(EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE))) {
      Error (NULL, 0, 3000, "Invalid", "FACS length check failed.");
      return STATUS_ERROR;
    }
    break;

  //
  // "DSDT" Differentiated System Description Table
  //
  case EFI_ACPI_3_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
    if (AcpiHeader->Revision > EFI_ACPI_3_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_REVISION) {
      break;
    }
    if (AcpiHeader->Length <= sizeof(EFI_ACPI_DESCRIPTION_HEADER)) {
      Error (NULL, 0, 3000, "Invalid", "DSDT length check failed.");
      return STATUS_ERROR;
    }
    break;

  //
  // "APIC" Multiple APIC Description Table
  //
  case EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:
    if (AcpiHeader->Revision > EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION) {
      break;
    }
    if ((AcpiHeader->Revision != EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION) &&
        (AcpiHeader->Revision != EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION) &&
        (AcpiHeader->Revision != EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION)) {
      Error (NULL, 0, 3000, "Invalid", "APIC revision check failed.");
      return STATUS_ERROR;
    }
    if (AcpiHeader->Length <= sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(UINT32) + sizeof(UINT32)) {
      Error (NULL, 0, 3000, "Invalid", "APIC length check failed.");
      return STATUS_ERROR;
    }
    break;

  //
  // "MCFG" PCI Express Memory Mapped Configuration Space Base Address Description Table
  //
  case EFI_ACPI_3_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE:
    if (AcpiHeader->Revision > EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION) {
      break;
    }
    if (AcpiHeader->Revision != EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION) {
      Error (NULL, 0, 3000, "Invalid", "MCFG revision check failed.");
      return STATUS_ERROR;
    }
    if (AcpiHeader->Length <= sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(UINT64)) {
      Error (NULL, 0, 3000, "Invalid", "MCFG length check failed.");
      return STATUS_ERROR;
    }
    break;

  //
  // Other table pass check
  //
  default:
    break;
  }

  return STATUS_SUCCESS;
}

VOID
SetHiiResourceHeader (
  UINT8   *HiiBinData,
  UINT32  OffsetToFile
  )
{
  UINT32  Index;
  EFI_IMAGE_RESOURCE_DIRECTORY        *ResourceDirectory;
  EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY  *ResourceDirectoryEntry;
  EFI_IMAGE_RESOURCE_DIRECTORY_STRING *ResourceDirectoryString;
  EFI_IMAGE_RESOURCE_DATA_ENTRY       *ResourceDataEntry;

  //
  // Fill Resource section entry
  //
  ResourceDirectory      = (EFI_IMAGE_RESOURCE_DIRECTORY *) (HiiBinData);
  ResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (ResourceDirectory + 1);
  for (Index = 0; Index < ResourceDirectory->NumberOfNamedEntries; Index ++) {
    if (ResourceDirectoryEntry->u1.s.NameIsString) {
      ResourceDirectoryString = (EFI_IMAGE_RESOURCE_DIRECTORY_STRING *) (HiiBinData + ResourceDirectoryEntry->u1.s.NameOffset);

      if (ResourceDirectoryString->Length == 3 &&
          ResourceDirectoryString->String[0] == L'H' &&
          ResourceDirectoryString->String[1] == L'I' &&
          ResourceDirectoryString->String[2] == L'I') {
        //
        // Resource Type "HII" found
        //
        if (ResourceDirectoryEntry->u2.s.DataIsDirectory) {
          //
          // Move to next level - resource Name
          //
          ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) (HiiBinData + ResourceDirectoryEntry->u2.s.OffsetToDirectory);
          ResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (ResourceDirectory + 1);

          if (ResourceDirectoryEntry->u2.s.DataIsDirectory) {
            //
            // Move to next level - resource Language
            //
            ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) (HiiBinData + ResourceDirectoryEntry->u2.s.OffsetToDirectory);
            ResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (ResourceDirectory + 1);
          }
        }

        //
        // Now it ought to be resource Data and update its OffsetToData value 
        //
        if (!ResourceDirectoryEntry->u2.s.DataIsDirectory) {
          ResourceDataEntry = (EFI_IMAGE_RESOURCE_DATA_ENTRY *) (HiiBinData + ResourceDirectoryEntry->u2.OffsetToData);
          ResourceDataEntry->OffsetToData = ResourceDataEntry->OffsetToData + OffsetToFile;
          break;
        }
      }
    }
    ResourceDirectoryEntry++;
  }
  
  return;
}

EFI_IMAGE_OPTIONAL_HEADER_UNION *
GetPeCoffHeader (
  void *Data
  )
{
  EFI_IMAGE_DOS_HEADER             *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr;

  //
  // Read the dos & pe hdrs of the image
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)Data;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    // NO DOS header, check for PE/COFF header
    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(Data);
    if (PeHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
      return NULL;
    }
  } else {

    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(((UINT8 *)Data) + DosHdr->e_lfanew);
    if (PeHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
      return NULL;
    }
  }
  
  return PeHdr;
}

void
PeCoffConvertImageToXip (
  UINT8  **FileBuffer,
  UINT32 *FileLength
  )
{
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *NewPeHdr;
  EFI_IMAGE_SECTION_HEADER         *SectionHeader;
  UINTN                            TotalNecessaryFileSize;
  UINTN                            SectionSize;
  UINT8                            *XipFile;
  UINT32                           XipLength;
  UINTN                            Index;
  UINTN                            FirstSectionOffset;
  BOOLEAN                          ConversionNeeded;

  PeHdr = GetPeCoffHeader ((void *) *FileBuffer);
  if (PeHdr == NULL) {
    return;
  }
  
  if (PeHdr->Pe32.OptionalHeader.SectionAlignment != PeHdr->Pe32.OptionalHeader.FileAlignment) {
    //
    // The only reason to expand zero fill sections is to make them compatible with XIP images.
    // If SectionAlignment is not equal to FileAlginment then it is not an XIP type image.
    //
    return;
  }

  //
  // Calculate size of XIP file, and determine if the conversion is needed.
  //
  ConversionNeeded = FALSE;
  XipLength = 0;
  FirstSectionOffset = *FileLength;
  TotalNecessaryFileSize = 0;
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
  for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
    SectionSize = MAX (SectionHeader->Misc.VirtualSize, SectionHeader->SizeOfRawData);
    TotalNecessaryFileSize += SectionSize;
    if (SectionSize > 0) {
      FirstSectionOffset = MIN (FirstSectionOffset, SectionHeader->VirtualAddress);
      XipLength = MAX (XipLength, SectionHeader->VirtualAddress + SectionSize);
      if (SectionHeader->VirtualAddress != SectionHeader->PointerToRawData) {
        ConversionNeeded = TRUE;
      }
    }
    if (SectionHeader->Misc.VirtualSize > SectionHeader->SizeOfRawData) {
      ConversionNeeded = TRUE;
    }
  }

  if (FirstSectionOffset < PeHdr->Pe32.OptionalHeader.SizeOfHeaders) {
    //
    // If one of the sections should be loaded to an offset overlapping with
    // the executable header, then it cannot be made into an XIP image.
    //
    VerboseMsg ("PE/COFF conversion to XIP is impossible due to overlap");
    VerboseMsg ("of section data with the executable header.");
    return;
  }

  if (FirstSectionOffset == *FileLength) {
    //
    // If we never found a section with a non-zero size, then we
    // skip the conversion.
    //
    return;
  }

  TotalNecessaryFileSize += FirstSectionOffset;

  if (!ConversionNeeded) {
    return;
  }

  if (XipLength > (2 * TotalNecessaryFileSize)) {
    VerboseMsg ("PE/COFF conversion to XIP appears to be larger than necessary.");
    VerboseMsg ("The image linking process may have left unused memory ranges.");
  }

  if (PeHdr->Pe32.FileHeader.PointerToSymbolTable != 0) {
    //
    // This field is obsolete and should be zero
    //
    PeHdr->Pe32.FileHeader.PointerToSymbolTable = 0;
  }

  //
  // Allocate the extra space that we need to grow the image
  //
  XipFile = malloc (XipLength);
  memset (XipFile, 0, XipLength);

  //
  // Copy the file headers
  //
  memcpy (XipFile, *FileBuffer, PeHdr->Pe32.OptionalHeader.SizeOfHeaders);

  NewPeHdr = GetPeCoffHeader ((void *)XipFile);
  if (NewPeHdr == NULL) {
    free (XipFile);
    return;
  }

  //
  // Copy the section data over to the appropriate XIP offsets
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(NewPeHdr->Pe32.OptionalHeader) + NewPeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
  for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
    if (SectionHeader->SizeOfRawData > 0) {
      memcpy (
        XipFile + SectionHeader->VirtualAddress,
        *FileBuffer + SectionHeader->PointerToRawData,
        SectionHeader->SizeOfRawData
        );
    }
    //
    // Make the size of raw data in section header alignment.
    //
    SectionHeader->SizeOfRawData = (SectionHeader->Misc.VirtualSize + PeHdr->Pe32.OptionalHeader.FileAlignment - 1) & (~(PeHdr->Pe32.OptionalHeader.FileAlignment - 1));
    SectionHeader->PointerToRawData = SectionHeader->VirtualAddress;
  }

  free (*FileBuffer);
  *FileLength = XipLength;
  *FileBuffer = XipFile;
}

UINT8 *
CreateHiiResouceSectionHeader (
  UINT32 *pSectionHeaderSize, 
  UINT32 HiiDataSize
  )
/*++

Routine Description:

  Create COFF resource section header

Arguments:

  pSectionHeaderSize - Pointer to section header size.
  HiiDataSize        - Size of the total HII data in section.

Returns:
  The created section header buffer.

--*/
{
  UINT32  HiiSectionHeaderSize;
  UINT32  HiiSectionOffset;
  UINT8   *HiiSectionHeader;
  EFI_IMAGE_RESOURCE_DIRECTORY        *ResourceDirectory;
  EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY  *TypeResourceDirectoryEntry;
  EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY  *NameResourceDirectoryEntry;
  EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY  *LanguageResourceDirectoryEntry;
  EFI_IMAGE_RESOURCE_DIRECTORY_STRING *ResourceDirectoryString;
  EFI_IMAGE_RESOURCE_DATA_ENTRY       *ResourceDataEntry;

  //
  // Calculate the total size for the resource header (include Type, Name and Language)
  // then allocate memory for the resource header.
  //
  HiiSectionHeaderSize = 3 * (sizeof (EFI_IMAGE_RESOURCE_DIRECTORY) + sizeof (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY)) 
                          + 3 * (sizeof (UINT16) + 3 * sizeof (CHAR16)) 
                          + sizeof (EFI_IMAGE_RESOURCE_DATA_ENTRY);
  HiiSectionHeader = malloc (HiiSectionHeaderSize);
  memset (HiiSectionHeader, 0, HiiSectionHeaderSize);

  HiiSectionOffset = 0;
  //
  // Create Type entry 
  //
  ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) (HiiSectionHeader + HiiSectionOffset);
  HiiSectionOffset += sizeof (EFI_IMAGE_RESOURCE_DIRECTORY);
  ResourceDirectory->NumberOfNamedEntries = 1;
  TypeResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (HiiSectionHeader + HiiSectionOffset);
  HiiSectionOffset += sizeof (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY);
  TypeResourceDirectoryEntry->u1.s.NameIsString      = 1;
  TypeResourceDirectoryEntry->u2.s.DataIsDirectory   = 1;
  TypeResourceDirectoryEntry->u2.s.OffsetToDirectory = HiiSectionOffset;
  //
  // Create Name entry
  //
  ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) (HiiSectionHeader + HiiSectionOffset);
  HiiSectionOffset += sizeof (EFI_IMAGE_RESOURCE_DIRECTORY);
  ResourceDirectory->NumberOfNamedEntries = 1;
  NameResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (HiiSectionHeader + HiiSectionOffset);
  HiiSectionOffset += sizeof (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY);
  NameResourceDirectoryEntry->u1.s.NameIsString      = 1;
  NameResourceDirectoryEntry->u2.s.DataIsDirectory   = 1;
  NameResourceDirectoryEntry->u2.s.OffsetToDirectory = HiiSectionOffset;
  //
  // Create Language entry
  //
  ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) (HiiSectionHeader + HiiSectionOffset);
  HiiSectionOffset += sizeof (EFI_IMAGE_RESOURCE_DIRECTORY);
  ResourceDirectory->NumberOfNamedEntries = 1;
  LanguageResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (HiiSectionHeader + HiiSectionOffset);
  HiiSectionOffset += sizeof (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY);
  LanguageResourceDirectoryEntry->u1.s.NameIsString = 1;
  //
  // Create string entry for Type
  //
  TypeResourceDirectoryEntry->u1.s.NameOffset = HiiSectionOffset;
  ResourceDirectoryString = (EFI_IMAGE_RESOURCE_DIRECTORY_STRING *) (HiiSectionHeader + HiiSectionOffset);
  ResourceDirectoryString->Length = 3;
  ResourceDirectoryString->String[0] = L'H';
  ResourceDirectoryString->String[1] = L'I';
  ResourceDirectoryString->String[2] = L'I';
  HiiSectionOffset = HiiSectionOffset + sizeof (ResourceDirectoryString->Length) + ResourceDirectoryString->Length * sizeof (ResourceDirectoryString->String[0]);
  //
  // Create string entry for Name
  //
  NameResourceDirectoryEntry->u1.s.NameOffset = HiiSectionOffset;
  ResourceDirectoryString = (EFI_IMAGE_RESOURCE_DIRECTORY_STRING *) (HiiSectionHeader + HiiSectionOffset);
  ResourceDirectoryString->Length = 3;
  ResourceDirectoryString->String[0] = L'E';
  ResourceDirectoryString->String[1] = L'F';
  ResourceDirectoryString->String[2] = L'I';
  HiiSectionOffset = HiiSectionOffset + sizeof (ResourceDirectoryString->Length) + ResourceDirectoryString->Length * sizeof (ResourceDirectoryString->String[0]);
  //
  // Create string entry for Language
  //
  LanguageResourceDirectoryEntry->u1.s.NameOffset = HiiSectionOffset;
  ResourceDirectoryString = (EFI_IMAGE_RESOURCE_DIRECTORY_STRING *) (HiiSectionHeader + HiiSectionOffset);
  ResourceDirectoryString->Length = 3;
  ResourceDirectoryString->String[0] = L'B';
  ResourceDirectoryString->String[1] = L'I';
  ResourceDirectoryString->String[2] = L'N';
  HiiSectionOffset = HiiSectionOffset + sizeof (ResourceDirectoryString->Length) + ResourceDirectoryString->Length * sizeof (ResourceDirectoryString->String[0]);
  //
  // Create Leaf data
  //
  LanguageResourceDirectoryEntry->u2.OffsetToData = HiiSectionOffset;
  ResourceDataEntry = (EFI_IMAGE_RESOURCE_DATA_ENTRY *) (HiiSectionHeader + HiiSectionOffset);
  HiiSectionOffset += sizeof (EFI_IMAGE_RESOURCE_DATA_ENTRY);
  ResourceDataEntry->OffsetToData = HiiSectionOffset;
  ResourceDataEntry->Size = HiiDataSize;

  *pSectionHeaderSize = HiiSectionHeaderSize;
  return HiiSectionHeader;
}

EFI_STATUS
RebaseImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINT32  *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file

  FileOffset - The offset, in bytes, into the file to read

  ReadSize   - The number of bytes to read from the file starting at FileOffset

  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8   *Destination8;
  CHAR8   *Source8;
  UINT32  Length;

  Destination8  = Buffer;
  Source8       = (CHAR8 *) ((UINTN) FileHandle + FileOffset);
  Length        = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
SetAddressToSectionHeader (
  IN     CHAR8   *FileName,
  IN OUT UINT8   *FileBuffer,
  IN     UINT64  NewPe32BaseAddress
  )
/*++

Routine Description:

  Set new base address into the section header of PeImage

Arguments:

  FileName           - Name of file
  FileBuffer         - Pointer to PeImage.
  NewPe32BaseAddress - New Base Address for PE image.

Returns:

  EFI_SUCCESS          Set new base address into this image successfully.

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  UINTN                                 Index;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *ImgHdr;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;

  //
  // Initialize context
  //
  memset (&ImageContext, 0, sizeof (ImageContext));
  ImageContext.Handle     = (VOID *) FileBuffer;
  ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) RebaseImageRead;
  Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s is not valid", FileName);
    return Status;
  }

  if (ImageContext.RelocationsStripped) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s has no relocation to be fixed up", FileName);
    return Status;    
  }

  //
  // Get PeHeader pointer
  //
  ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer + ImageContext.PeCoffHeaderOffset);

  //
  // Get section header list
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
    (UINTN) ImgHdr +
    sizeof (UINT32) + 
    sizeof (EFI_IMAGE_FILE_HEADER) +  
    ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
    );

  //
  // Set base address into the first section header that doesn't point to code section.
  //
  for (Index = 0; Index < ImgHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
    if ((SectionHeader->Characteristics & EFI_IMAGE_SCN_CNT_CODE) == 0) {
      *(UINT64 *) &SectionHeader->PointerToRelocations = NewPe32BaseAddress;
      break;
    }
  }

  //
  // No available section header is found.
  //
  if (Index == ImgHdr->Pe32.FileHeader.NumberOfSections) {
    return EFI_NOT_FOUND;
  }
  
  //
  // BaseAddress is set to section header.
  //
  return EFI_SUCCESS;
}

EFI_STATUS
RebaseImage (
  IN     CHAR8   *FileName,
  IN OUT UINT8   *FileBuffer,
  IN     UINT64  NewPe32BaseAddress
  )
/*++

Routine Description:

  Set new base address into PeImage, and fix up PeImage based on new address.

Arguments:

  FileName           - Name of file
  FileBuffer         - Pointer to PeImage.
  NewPe32BaseAddress - New Base Address for PE image.

Returns:

  EFI_INVALID_PARAMETER   - BaseAddress is not valid.
  EFI_SUCCESS             - Update PeImage is correctly.

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  UINTN                                 Index;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *ImgHdr;
  UINT8                                 *MemoryImagePointer;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;

  //
  // Initialize context
  //
  memset (&ImageContext, 0, sizeof (ImageContext));
  ImageContext.Handle     = (VOID *) FileBuffer;
  ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) RebaseImageRead;
  Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s is not valid", FileName);
    return Status;
  }

  if (ImageContext.RelocationsStripped) {
    Error (NULL, 0, 3000, "Invalid", "The input PeImage %s has no relocation to be fixed up", FileName);
    return Status;    
  }

  //
  // Get PeHeader pointer
  //
  ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer + ImageContext.PeCoffHeaderOffset);

  //
  // Load and Relocate Image Data
  //
  MemoryImagePointer = (UINT8 *) malloc ((UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
  if (MemoryImagePointer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
    return EFI_OUT_OF_RESOURCES;
  }
  memset ((VOID *) MemoryImagePointer, 0, (UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
  ImageContext.ImageAddress = ((UINTN) MemoryImagePointer + ImageContext.SectionAlignment - 1) & (~((INT64)ImageContext.SectionAlignment - 1));

  Status =  PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "LocateImage() call failed on rebase of %s", FileName);
    free ((VOID *) MemoryImagePointer);
    return Status;
  }

  ImageContext.DestinationAddress = NewPe32BaseAddress;
  Status                          = PeCoffLoaderRelocateImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "RelocateImage() call failed on rebase of %s", FileName);
    free ((VOID *) MemoryImagePointer);
    return Status;
  }

  //
  // Copy Relocated data to raw image file.
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
    (UINTN) ImgHdr +
    sizeof (UINT32) + 
    sizeof (EFI_IMAGE_FILE_HEADER) +  
    ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
    );

  for (Index = 0; Index < ImgHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
    CopyMem (
      FileBuffer + SectionHeader->PointerToRawData, 
      (VOID*) (UINTN) (ImageContext.ImageAddress + SectionHeader->VirtualAddress), 
      SectionHeader->SizeOfRawData
      );
  }

  free ((VOID *) MemoryImagePointer);

  //
  // Update Image Base Address
  //
  if ((ImgHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) && (ImgHdr->Pe32.FileHeader.Machine != IMAGE_FILE_MACHINE_IA64)) {
    ImgHdr->Pe32.OptionalHeader.ImageBase = (UINT32) NewPe32BaseAddress;
  } else if (ImgHdr->Pe32Plus.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    ImgHdr->Pe32Plus.OptionalHeader.ImageBase = NewPe32BaseAddress;
  } else {
    Error (NULL, 0, 3000, "Invalid", "unknown PE magic signature %X in PE32 image %s",
      ImgHdr->Pe32.OptionalHeader.Magic,
      FileName
      );
    return EFI_ABORTED;
  }

  //
  // Set new base address into section header
  //
  Status = SetAddressToSectionHeader (FileName, FileBuffer, NewPe32BaseAddress);

  return Status;
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
  UINT32                           Type;
  UINT32                           InputFileNum;
  CHAR8                            **InputFileName;
  char                             *OutImageName;
  char                             *ModuleType;
  CHAR8                            *TimeStamp;
  FILE                             *fpIn;
  FILE                             *fpOut;
  FILE                             *fpInOut;
  UINT32                           Data;
  UINT32                           *DataPointer;
  UINT32                           *OldDataPointer;
  UINT32                           CheckSum;
  UINT32                           Index;
  UINT32                           Index1;
  UINT32                           Index2;
  UINT64                           Temp64;
  UINT32                           MciAlignment;
  UINT8                            MciPadValue;
  UINT32                           AllignedRelocSize;
  UINT8                            *FileBuffer;
  UINT32                           FileLength;
  UINT8                            *OutputFileBuffer;
  UINT32                           OutputFileLength;
  UINT8                            *InputFileBuffer;
  UINT32                           InputFileLength;
  RUNTIME_FUNCTION                 *RuntimeFunction;
  UNWIND_INFO                      *UnwindInfo;
  STATUS                           Status;
  BOOLEAN                          ReplaceFlag;
  BOOLEAN                          KeepExceptionTableFlag;
  BOOLEAN                          KeepZeroPendingFlag;
  UINT64                           LogLevel;
  EFI_TE_IMAGE_HEADER              TEImageHeader;
  EFI_TE_IMAGE_HEADER              *TeHdr;
  EFI_IMAGE_SECTION_HEADER         *SectionHeader;
  EFI_IMAGE_DOS_HEADER             *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr;
  EFI_IMAGE_OPTIONAL_HEADER32      *Optional32;
  EFI_IMAGE_OPTIONAL_HEADER64      *Optional64;
  EFI_IMAGE_DOS_HEADER             BackupDosHdr;
  MICROCODE_IMAGE_HEADER           *MciHeader;
  UINT8                            *HiiPackageListBuffer;
  UINT8                            *HiiPackageDataPointer;
  EFI_GUID                         HiiPackageListGuid;
  EFI_HII_PACKAGE_LIST_HEADER      HiiPackageListHeader;
  EFI_HII_PACKAGE_HEADER           HiiPackageHeader;
  EFI_IFR_FORM_SET                 IfrFormSet;
  UINT8                            NumberOfFormPacakge;
  EFI_HII_PACKAGE_HEADER           EndPackage;
  UINT32                           HiiSectionHeaderSize;
  UINT8                            *HiiSectionHeader;
  UINT64                           NewBaseAddress;
  BOOLEAN                          NegativeAddr;
  FILE                             *ReportFile;
  CHAR8                            *ReportFileName;
  UINTN                            FileLen;
  time_t                           InputFileTime;
  time_t                           OutputFileTime;
  struct stat                      Stat_Buf;

  SetUtilityName (UTILITY_NAME);

  //
  // Assign to fix compile warning
  //
  FileLen           = 0;
  InputFileNum      = 0;
  InputFileName     = NULL;
  mInImageName      = NULL;
  OutImageName      = NULL;
  ModuleType        = NULL;
  Type              = 0;
  Status            = STATUS_SUCCESS;
  FileBuffer        = NULL;
  fpIn              = NULL;
  fpOut             = NULL;
  fpInOut           = NULL;
  TimeStamp         = NULL;
  MciAlignment      = DEFAULT_MC_ALIGNMENT;
  MciPadValue       = DEFAULT_MC_PAD_BYTE_VALUE;
  FileLength        = 0;
  MciHeader         = NULL;
  CheckSum          = 0;
  ReplaceFlag       = FALSE;
  LogLevel          = 0;
  OutputFileBuffer  = NULL;
  OutputFileLength  = 0;
  InputFileBuffer   = NULL;
  InputFileLength   = 0;
  Optional32        = NULL;
  Optional64        = NULL;
  KeepExceptionTableFlag = FALSE;
  KeepZeroPendingFlag    = FALSE;
  NumberOfFormPacakge    = 0;
  HiiPackageListBuffer   = NULL;
  HiiPackageDataPointer  = NULL;
  EndPackage.Length      = sizeof (EFI_HII_PACKAGE_HEADER);
  EndPackage.Type        = EFI_HII_PACKAGE_END;
  memset (&HiiPackageListGuid, 0, sizeof (HiiPackageListGuid));
  HiiSectionHeaderSize   = 0;
  HiiSectionHeader       = NULL;
  NewBaseAddress         = 0;
  NegativeAddr           = FALSE;
  InputFileTime          = 0;
  OutputFileTime         = 0;

  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "No input options.");
    Usage ();
    return STATUS_ERROR;
  }

  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Version ();
    Usage ();
    return STATUS_SUCCESS;
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version ();
    return STATUS_SUCCESS;
  }

  while (argc > 0) {
    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--outputfile") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output file name is missing for -o option");
        goto Finish;
      }
      OutImageName = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-e") == 0) || (stricmp (argv[0], "--efiImage") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Module Type is missing for -o option");
        goto Finish;
      }
      ModuleType = argv[1];
      if (mOutImageType != FW_TE_IMAGE) {
        mOutImageType = FW_EFI_IMAGE;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-l") == 0) || (stricmp (argv[0], "--stripped") == 0)) {
      mOutImageType = FW_RELOC_STRIPEED_IMAGE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-c") == 0) || (stricmp (argv[0], "--acpi") == 0)) {
      mOutImageType = FW_ACPI_IMAGE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-t") == 0) || (stricmp (argv[0], "--terse") == 0)) {
      mOutImageType = FW_TE_IMAGE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-u") == 0) || (stricmp (argv[0], "--dump") == 0)) {
      mOutImageType = DUMP_TE_HEADER;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-b") == 0) || (stricmp (argv[0], "--exe2bin") == 0)) {
      mOutImageType = FW_BIN_IMAGE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-z") == 0) || (stricmp (argv[0], "--zero") == 0)) {
      mOutImageType = FW_ZERO_DEBUG_IMAGE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-s") == 0) || (stricmp (argv[0], "--stamp") == 0)) {
      mOutImageType = FW_SET_STAMP_IMAGE;
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "time stamp is missing for -s option");
        goto Finish;
      }
      TimeStamp = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-r") == 0) || (stricmp (argv[0], "--replace") == 0)) {
      ReplaceFlag = TRUE;
      argc --;
      argv ++;
      continue;
    }

    if (stricmp (argv[0], "--keepexceptiontable") == 0) {
      KeepExceptionTableFlag = TRUE;
      argc --;
      argv ++;
      continue;
    }

    if (stricmp (argv[0], "--keepzeropending") == 0) {
      KeepZeroPendingFlag = TRUE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-m") == 0) || (stricmp (argv[0], "--mcifile") == 0)) {
      mOutImageType = FW_MCI_IMAGE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-j") == 0) || (stricmp (argv[0], "--join") == 0)) {
      mOutImageType = FW_MERGE_IMAGE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-a") == 0) || (stricmp (argv[0], "--align") == 0)) {
      if (AsciiStringToUint64 (argv[1], FALSE, &Temp64) != EFI_SUCCESS) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      MciAlignment = (UINT32) Temp64;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "--rebase") == 0)) {
      if (argv[1][0] == '-') {
        NegativeAddr = TRUE;
        Status = AsciiStringToUint64 (argv[1] + 1, FALSE, &Temp64);
      } else {
        NegativeAddr = FALSE;
        Status = AsciiStringToUint64 (argv[1], FALSE, &Temp64);
      }
      if (Status != EFI_SUCCESS) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      mOutImageType = FW_REBASE_IMAGE;
      NewBaseAddress = (UINT64) Temp64;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "--address") == 0)) {
      if (argv[1][0] == '-') {
        NegativeAddr = TRUE;
        Status = AsciiStringToUint64 (argv[1] + 1, FALSE, &Temp64);
      } else {
        NegativeAddr = FALSE;
        Status = AsciiStringToUint64 (argv[1], FALSE, &Temp64);
      }
      if (Status != EFI_SUCCESS) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      mOutImageType = FW_SET_ADDRESS_IMAGE;
      NewBaseAddress = (UINT64) Temp64;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-p") == 0) || (stricmp (argv[0], "--pad") == 0)) {
      if (AsciiStringToUint64 (argv[1], FALSE, &Temp64) != EFI_SUCCESS) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      MciPadValue = (UINT8) Temp64;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-v") == 0) || (stricmp (argv[0], "--verbose") == 0)) {
      SetPrintLevel (VERBOSE_LOG_LEVEL);
      VerboseMsg ("Verbose output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      SetPrintLevel (KEY_LOG_LEVEL);
      KeyMsg ("Quiet output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &LogLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      if (LogLevel > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, currnt input level is %d", (int) LogLevel);
        goto Finish;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-g") == 0) || (stricmp (argv[0], "--hiiguid") == 0)) {
      Status = StringToGuid (argv[1], &HiiPackageListGuid);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if (stricmp (argv[0], "--hiipackage") == 0) {
      mOutImageType = FW_HII_PACKAGE_LIST_RCIMAGE;
      argc --;
      argv ++;
      continue;
    }

    if (stricmp (argv[0], "--hiibinpackage") == 0) {
      mOutImageType = FW_HII_PACKAGE_LIST_BINIMAGE;
      argc --;
      argv ++;
      continue;
    }

    if (argv[0][0] == '-') {
      Error (NULL, 0, 1000, "Unknown option", argv[0]);
      goto Finish;
    }
    //
    // Get Input file name
    //
    if ((InputFileNum == 0) && (InputFileName == NULL)) {
      InputFileName = (CHAR8 **) malloc (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *));
      if (InputFileName == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
        goto Finish;
      }

      memset (InputFileName, 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *)));
    } else if (InputFileNum % MAXIMUM_INPUT_FILE_NUM == 0) {
      //
      // InputFileName buffer too small, need to realloc
      //
      InputFileName = (CHAR8 **) realloc (
        InputFileName,
        (InputFileNum + MAXIMUM_INPUT_FILE_NUM) * sizeof (CHAR8 *)
        );

      if (InputFileName == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
        goto Finish;
      }

      memset (&(InputFileName[InputFileNum]), 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *)));
    }

    InputFileName [InputFileNum ++] = argv[0];
    argc --;
    argv ++;
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);

  if (mOutImageType == FW_DUMMY_IMAGE) {
    Error (NULL, 0, 1001, "Missing option", "No create file action specified; pls specify -e, -c or -t option to create efi image, or acpi table or TeImage!");
    if (ReplaceFlag) {
      Error (NULL, 0, 1001, "Missing option", "-r option is not supported as the independent option. It can be used together with other create file option specified at the above.");
    }
    goto Finish;
  }

  //
  // check input files
  //
  if (InputFileNum == 0) {
    Error (NULL, 0, 1001, "Missing option", "Input files");
    goto Finish;
  }

  //
  // Combine MciBinary files to one file
  //
  if ((mOutImageType == FW_MERGE_IMAGE) && ReplaceFlag) {
    Error (NULL, 0, 1002, "Conflicting option", "-r replace option cannot be used with -j merge files option.");
    goto Finish;
  }

  //
  // Combine HiiBinary packages to a single package list
  //
  if ((mOutImageType == FW_HII_PACKAGE_LIST_RCIMAGE) && ReplaceFlag) {
    Error (NULL, 0, 1002, "Conflicting option", "-r replace option cannot be used with --hiipackage merge files option.");
    goto Finish;
  }

  if ((mOutImageType == FW_HII_PACKAGE_LIST_BINIMAGE) && ReplaceFlag) {
    Error (NULL, 0, 1002, "Conflicting option", "-r replace option cannot be used with --hiibinpackage merge files option.");
    goto Finish;
  }

  //
  // Input image file
  //
  mInImageName = InputFileName [InputFileNum - 1];
  VerboseMsg ("the input file name is %s", mInImageName);

  //
  // Action will be taken for the input file.
  //
  switch (mOutImageType) {
  case FW_EFI_IMAGE:
    VerboseMsg ("Create efi image on module type %s based on the input PE image.", ModuleType);
    break;
  case FW_TE_IMAGE:
    VerboseMsg ("Create Te Image based on the input PE image.");
    break;
  case FW_ACPI_IMAGE:
    VerboseMsg ("Get acpi table data from the input PE image.");
    break;
  case FW_RELOC_STRIPEED_IMAGE:
    VerboseMsg ("Remove relocation section from Pe or Te image.");
    break;
  case FW_BIN_IMAGE:
    VerboseMsg ("Convert the input EXE to the output BIN file.");
    break;
  case FW_ZERO_DEBUG_IMAGE:
    VerboseMsg ("Zero the Debug Data Fields and Time Stamp in input PE image.");
    break;
  case FW_SET_STAMP_IMAGE:
    VerboseMsg ("Set new time stamp %s in the input PE image.", TimeStamp);
    break;
  case DUMP_TE_HEADER:
    VerboseMsg ("Dump the TE header information of the input TE image.");
    break;
  case FW_MCI_IMAGE:
    VerboseMsg ("Conver input MicroCode.txt file to MicroCode.bin file.");
    break;
  case FW_MERGE_IMAGE:
    VerboseMsg ("Combine the input multi microcode bin files to one bin file.");
    break;
  case FW_HII_PACKAGE_LIST_RCIMAGE:
    VerboseMsg ("Combine the input multi hii bin packages to one text pacakge list RC file.");
    break;
  case FW_HII_PACKAGE_LIST_BINIMAGE:
    VerboseMsg ("Combine the input multi hii bin packages to one binary pacakge list file.");
    break;
  case FW_REBASE_IMAGE:
    VerboseMsg ("Rebase the input image to new base address.");
    break;
  case FW_SET_ADDRESS_IMAGE:
    VerboseMsg ("Set the preferred address into the section header of the input image");
    break;
  default:
    break;
  }

  if (ReplaceFlag) {
    VerboseMsg ("Overwrite the input file with the output content.");
  }

  //
  // Open output file and Write image into the output file.
  //
  if (OutImageName != NULL) {
    fpOut = fopen (LongFilePath (OutImageName), "rb");
    if (fpOut != NULL) {
      //
      // Get Output file time stamp
      //
      fstat(fileno (fpOut), &Stat_Buf);
      OutputFileTime = Stat_Buf.st_mtime;
      //
      // Get Output file data
      //
      OutputFileLength = _filelength (fileno (fpOut));
      OutputFileBuffer = malloc (OutputFileLength);
      if (OutputFileBuffer == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
        fclose (fpOut);
        fpOut = NULL;
        goto Finish;
      }
      fread (OutputFileBuffer, 1, OutputFileLength, fpOut);
      fclose (fpOut);
      fpOut = NULL;
    }
    VerboseMsg ("Output file name is %s", OutImageName);
  } else if (!ReplaceFlag && mOutImageType != DUMP_TE_HEADER) {
    Error (NULL, 0, 1001, "Missing option", "output file");
    goto Finish;
  }

  //
  // Open input file and read file data into file buffer.
  //
  fpIn = fopen (LongFilePath (mInImageName), "rb");
  if (fpIn == NULL) {
    Error (NULL, 0, 0001, "Error opening file", mInImageName);
    goto Finish;
  }
  //
  // Get Iutput file time stamp
  //
  fstat(fileno (fpIn), &Stat_Buf);
  InputFileTime = Stat_Buf.st_mtime;
  //
  // Get Input file data
  //
  InputFileLength = _filelength (fileno (fpIn));
  InputFileBuffer = malloc (InputFileLength);
  if (InputFileBuffer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    fclose (fpIn);
    goto Finish;
  }
  fread (InputFileBuffer, 1, InputFileLength, fpIn);
  fclose (fpIn);
  DebugMsg (NULL, 0, 9, "input file info", "the input file size is %u bytes", (unsigned) InputFileLength);

  //
  // Combine multi binary HII package files.
  //
  if (mOutImageType == FW_HII_PACKAGE_LIST_RCIMAGE || mOutImageType == FW_HII_PACKAGE_LIST_BINIMAGE) {
    //
    // Open output file handle.
    //
    fpOut = fopen (LongFilePath (OutImageName), "wb");
    if (!fpOut) {
      Error (NULL, 0, 0001, "Error opening output file", OutImageName);
      goto Finish;
    }
    //
    // Get hii package list lenght
    //
    HiiPackageListHeader.PackageLength = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
    for (Index = 0; Index < InputFileNum; Index ++) {
      fpIn = fopen (LongFilePath (InputFileName [Index]), "rb");
      if (fpIn == NULL) {
        Error (NULL, 0, 0001, "Error opening file", InputFileName [Index]);
        goto Finish;
      }
      FileLength = _filelength (fileno (fpIn));
      fread (&HiiPackageHeader, 1, sizeof (HiiPackageHeader), fpIn);
      if (HiiPackageHeader.Type == EFI_HII_PACKAGE_FORM) {
        if (HiiPackageHeader.Length != FileLength) {
          Error (NULL, 0, 3000, "Invalid", "The wrong package size is in HII package file %s", InputFileName [Index]);
          fclose (fpIn);
          goto Finish;
        }
        if (memcmp (&HiiPackageListGuid, &mZeroGuid, sizeof (EFI_GUID)) == 0) {
          fread (&IfrFormSet, 1, sizeof (IfrFormSet), fpIn);
          memcpy (&HiiPackageListGuid, &IfrFormSet.Guid, sizeof (EFI_GUID));
        }
        NumberOfFormPacakge ++;
      }
      HiiPackageListHeader.PackageLength += FileLength;
      fclose (fpIn);
    }
    HiiPackageListHeader.PackageLength += sizeof (EndPackage);
    //
    // Check whether hii packages are valid
    //
    if (NumberOfFormPacakge > 1) {
      Error (NULL, 0, 3000, "Invalid", "The input hii packages contains more than one hii form package");
      goto Finish;
    }
    if (memcmp (&HiiPackageListGuid, &mZeroGuid, sizeof (EFI_GUID)) == 0) {
      Error (NULL, 0, 3000, "Invalid", "HII pacakge list guid is not specified!");
      goto Finish;
    }
    memcpy (&HiiPackageListHeader.PackageListGuid, &HiiPackageListGuid, sizeof (EFI_GUID));
    //
    // read hii packages
    //
    HiiPackageListBuffer = malloc (HiiPackageListHeader.PackageLength);
    if (HiiPackageListBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
      goto Finish;
    }
    memcpy (HiiPackageListBuffer, &HiiPackageListHeader, sizeof (HiiPackageListHeader));
    HiiPackageDataPointer = HiiPackageListBuffer + sizeof (HiiPackageListHeader);
    for (Index = 0; Index < InputFileNum; Index ++) {
      fpIn = fopen (LongFilePath (InputFileName [Index]), "rb");
      if (fpIn == NULL) {
        Error (NULL, 0, 0001, "Error opening file", InputFileName [Index]);
        free (HiiPackageListBuffer);
        goto Finish;
      }

      FileLength = _filelength (fileno (fpIn));
      fread (HiiPackageDataPointer, 1, FileLength, fpIn);
      fclose (fpIn);
      HiiPackageDataPointer = HiiPackageDataPointer + FileLength;
    }
    memcpy (HiiPackageDataPointer, &EndPackage, sizeof (EndPackage));

    //
    // write the hii package into the binary package list file with the resource section header
    //
    if (mOutImageType == FW_HII_PACKAGE_LIST_BINIMAGE) {
      //
      // Create the resource section header
      //
      HiiSectionHeader = CreateHiiResouceSectionHeader (&HiiSectionHeaderSize, HiiPackageListHeader.PackageLength);
      //
      // Wrtie section header and HiiData into File.
      //
      fwrite (HiiSectionHeader, 1, HiiSectionHeaderSize, fpOut);
      fwrite (HiiPackageListBuffer, 1, HiiPackageListHeader.PackageLength, fpOut);
      //
      // Free allocated resources.
      //
      free (HiiSectionHeader);
      free (HiiPackageListBuffer);
      //
      // Done successfully
      //
      goto Finish;
    }

    //
    // write the hii package into the text package list rc file.
    //
    if (mOutImageType == FW_HII_PACKAGE_LIST_RCIMAGE) {
      for (Index = 0; gHiiPackageRCFileHeader[Index] != NULL; Index++) {
        fprintf (fpOut, "%s\n", gHiiPackageRCFileHeader[Index]);
      }
      fprintf (fpOut, "\n%d %s\n{", HII_RESOURCE_SECTION_INDEX, HII_RESOURCE_SECTION_NAME);

      HiiPackageDataPointer = HiiPackageListBuffer;
      for (Index = 0; Index + 2 < HiiPackageListHeader.PackageLength; Index += 2) {
        if (Index % 16 == 0) {
          fprintf (fpOut, "\n ");
        }
        fprintf (fpOut, " 0x%04X,", *(UINT16 *) HiiPackageDataPointer);
        HiiPackageDataPointer += 2;
      }

      if (Index % 16 == 0) {
        fprintf (fpOut, "\n ");
      }
      if ((Index + 2) == HiiPackageListHeader.PackageLength) {
        fprintf (fpOut, " 0x%04X\n}\n", *(UINT16 *) HiiPackageDataPointer);
      }
      if ((Index + 1) == HiiPackageListHeader.PackageLength) {
        fprintf (fpOut, " 0x%04X\n}\n", *(UINT8 *) HiiPackageDataPointer);
      }
      free (HiiPackageListBuffer);
      //
      // Done successfully
      //
      goto Finish;
    }
  }

  //
  // Combine MciBinary files to one file
  //
  if (mOutImageType == FW_MERGE_IMAGE) {
    //
    // Open output file handle.
    //
    fpOut = fopen (LongFilePath (OutImageName), "wb");
    if (!fpOut) {
      Error (NULL, 0, 0001, "Error opening output file", OutImageName);
      goto Finish;
    }
    for (Index = 0; Index < InputFileNum; Index ++) {
      fpIn = fopen (LongFilePath (InputFileName [Index]), "rb");
      if (!fpIn) {
        Error (NULL, 0, 0001, "Error opening file", InputFileName [Index]);
        goto Finish;
      }

      FileLength = _filelength (fileno (fpIn));
      FileBuffer = malloc (FileLength);
      if (FileBuffer == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
        fclose (fpIn);
        goto Finish;
      }

      fread (FileBuffer, 1, FileLength, fpIn);
      fclose (fpIn);
      //
      // write input file to out file
      //
      fwrite (FileBuffer, 1, FileLength, fpOut);
      //
      // write pad value to out file.
      //
      while (FileLength ++ % MciAlignment != 0) {
        fwrite (&MciPadValue, 1, 1, fpOut);
      }
      //
      // free allocated memory space
      //
      free (FileBuffer);
      FileBuffer = NULL;
    }
    //
    // Done successfully
    //
    goto Finish;
  }

  //
  // Convert MicroCode.txt file to MicroCode.bin file
  //
  if (mOutImageType == FW_MCI_IMAGE) {
    fpIn = fopen (LongFilePath (mInImageName), "r");
    if (fpIn == NULL) {
      Error (NULL, 0, 0001, "Error opening file", mInImageName);
      goto Finish;
    }

    //
    // The first pass is to determine
    // how much data is in the file so we can allocate a working buffer.
    //
    FileLength = 0;
    do {
      Status = MicrocodeReadData (fpIn, &Data);
      if (Status == STATUS_SUCCESS) {
        FileLength += sizeof (Data);
      }
      if (Status == STATUS_IGNORE) {
        Status = STATUS_SUCCESS;
      }
    } while (Status == STATUS_SUCCESS);
    //
    // Error if no data.
    //
    if (FileLength == 0) {
      Error (NULL, 0, 3000, "Invalid", "no parseable data found in file %s", mInImageName);
      goto Finish;
    }
    if (FileLength < sizeof (MICROCODE_IMAGE_HEADER)) {
      Error (NULL, 0, 3000, "Invalid", "amount of parseable data in %s is insufficient to contain a microcode header", mInImageName);
      goto Finish;
    }

    //
    // Allocate a buffer for the data
    //
    FileBuffer = malloc (FileLength);
    if (FileBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
      goto Finish;
    }
    //
    // Re-read the file, storing the data into our buffer
    //
    fseek (fpIn, 0, SEEK_SET);
    DataPointer = (UINT32 *) FileBuffer;
    OldDataPointer = DataPointer;
    do {
      OldDataPointer = DataPointer;
      Status = MicrocodeReadData (fpIn, DataPointer++);
      if (Status == STATUS_IGNORE) {
        DataPointer = OldDataPointer;
        Status = STATUS_SUCCESS;
      }
    } while (Status == STATUS_SUCCESS);
    //
    // close input file after read data
    //
    fclose (fpIn);

    //
    // Can't do much checking on the header because, per the spec, the
    // DataSize field may be 0, which means DataSize = 2000 and TotalSize = 2K,
    // and the TotalSize field is invalid (actually missing). Thus we can't
    // even verify the Reserved fields are 0.
    //
    MciHeader = (MICROCODE_IMAGE_HEADER *) FileBuffer;
    if (MciHeader->DataSize == 0) {
      Index = 2048;
    } else {
      Index = MciHeader->TotalSize;
    }

    if (Index != FileLength) {
      Error (NULL, 0, 3000, "Invalid", "file length of %s (0x%x) does not equal expected TotalSize: 0x%04X.", mInImageName, (unsigned) FileLength, (unsigned) Index);
      goto Finish;
    }

    //
    // Checksum the contents
    //
    DataPointer = (UINT32 *) FileBuffer;
    CheckSum  = 0;
    Index     = 0;
    while (Index < FileLength) {
      CheckSum    += *DataPointer;
      DataPointer ++;
      Index       += sizeof (*DataPointer);
    }
    if (CheckSum != 0) {
      Error (NULL, 0, 3000, "Invalid", "checksum (0x%x) failed on file %s.", (unsigned) CheckSum, mInImageName);
      goto Finish;
    }
    //
    // Open the output file and write the buffer contents
    //
    VerboseMsg ("the size of output file is %u bytes", (unsigned) FileLength);
    goto WriteFile;
  }

  //
  // Open input file and read file data into file buffer.
  //
  FileLength = InputFileLength;
  FileBuffer = malloc (FileLength);
  if (FileBuffer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    goto Finish;
  }
  memcpy (FileBuffer, InputFileBuffer, InputFileLength);

  //
  // Dump TeImage Header into output file.
  //
  if (mOutImageType == DUMP_TE_HEADER) {
    memcpy (&TEImageHeader, FileBuffer, sizeof (TEImageHeader));
    if (TEImageHeader.Signature != EFI_TE_IMAGE_HEADER_SIGNATURE) {
      Error (NULL, 0, 3000, "Invalid", "TE header signature of file %s is not correct.", mInImageName);
      goto Finish;
    }
    //
    // Open the output file handle.
    //
    if (ReplaceFlag) {
      fpInOut = fopen (LongFilePath (mInImageName), "wb");
      if (fpInOut == NULL) {
        Error (NULL, 0, 0001, "Error opening file", mInImageName);
        goto Finish;
      }
    } else {
      if (OutImageName != NULL) {
        fpOut = fopen (LongFilePath (OutImageName), "wb");
      } else {
        fpOut = stdout;
      }
      if (fpOut == NULL) {
        Error (NULL, 0, 0001, "Error opening output file", OutImageName);
        goto Finish;
      }
    }
    if (fpInOut != NULL) {
      fprintf (fpInOut, "Dump of file %s\n\n", mInImageName);
      fprintf (fpInOut, "TE IMAGE HEADER VALUES\n");
      fprintf (fpInOut, "%17X machine\n", TEImageHeader.Machine);
      fprintf (fpInOut, "%17X number of sections\n", TEImageHeader.NumberOfSections);
      fprintf (fpInOut, "%17X subsystems\n", TEImageHeader.Subsystem);
      fprintf (fpInOut, "%17X stripped size\n", TEImageHeader.StrippedSize);
      fprintf (fpInOut, "%17X entry point\n", (unsigned) TEImageHeader.AddressOfEntryPoint);
      fprintf (fpInOut, "%17X base of code\n", (unsigned) TEImageHeader.BaseOfCode);
      fprintf (fpInOut, "%17llX image base\n", (unsigned long long)TEImageHeader.ImageBase);
      fprintf (fpInOut, "%17X [%8X] RVA [size] of Base Relocation Directory\n", (unsigned) TEImageHeader.DataDirectory[0].VirtualAddress, (unsigned) TEImageHeader.DataDirectory[0].Size);
      fprintf (fpInOut, "%17X [%8X] RVA [size] of Debug Directory\n", (unsigned) TEImageHeader.DataDirectory[1].VirtualAddress, (unsigned) TEImageHeader.DataDirectory[1].Size);
    }
    if (fpOut != NULL) {
      fprintf (fpOut, "Dump of file %s\n\n", mInImageName);
      fprintf (fpOut, "TE IMAGE HEADER VALUES\n");
      fprintf (fpOut, "%17X machine\n", TEImageHeader.Machine);
      fprintf (fpOut, "%17X number of sections\n", TEImageHeader.NumberOfSections);
      fprintf (fpOut, "%17X subsystems\n", TEImageHeader.Subsystem);
      fprintf (fpOut, "%17X stripped size\n", TEImageHeader.StrippedSize);
      fprintf (fpOut, "%17X entry point\n", (unsigned) TEImageHeader.AddressOfEntryPoint);
      fprintf (fpOut, "%17X base of code\n", (unsigned) TEImageHeader.BaseOfCode);
      fprintf (fpOut, "%17llX image base\n", (unsigned long long)TEImageHeader.ImageBase);
      fprintf (fpOut, "%17X [%8X] RVA [size] of Base Relocation Directory\n", (unsigned) TEImageHeader.DataDirectory[0].VirtualAddress, (unsigned) TEImageHeader.DataDirectory[0].Size);
      fprintf (fpOut, "%17X [%8X] RVA [size] of Debug Directory\n", (unsigned) TEImageHeader.DataDirectory[1].VirtualAddress, (unsigned) TEImageHeader.DataDirectory[1].Size);
    }
    goto Finish;
  }

  //
  // Following code to convert dll to efi image or te image.
  // Get new image type
  //
  if ((mOutImageType == FW_EFI_IMAGE) || (mOutImageType == FW_TE_IMAGE)) {
    if (ModuleType == NULL) {
      if (mOutImageType == FW_EFI_IMAGE) {
        Error (NULL, 0, 1001, "Missing option", "EFI_FILETYPE");
        goto Finish;
      } else if (mOutImageType == FW_TE_IMAGE) {
        //
        // Default TE Image Type is Boot service driver
        //
        Type = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
        VerboseMsg ("Efi Image subsystem type is efi boot service driver.");
      }
    } else {
      if (stricmp (ModuleType, "BASE") == 0 ||
        stricmp (ModuleType, "SEC") == 0 ||
        stricmp (ModuleType, "SECURITY_CORE") == 0 ||
        stricmp (ModuleType, "PEI_CORE") == 0 ||
        stricmp (ModuleType, "PEIM") == 0 ||
        stricmp (ModuleType, "COMBINED_PEIM_DRIVER") == 0 ||
        stricmp (ModuleType, "PIC_PEIM") == 0 ||
        stricmp (ModuleType, "RELOCATABLE_PEIM") == 0 ||
        stricmp (ModuleType, "DXE_CORE") == 0 ||
        stricmp (ModuleType, "BS_DRIVER") == 0  ||
        stricmp (ModuleType, "DXE_DRIVER") == 0 ||
        stricmp (ModuleType, "DXE_SMM_DRIVER") == 0  ||
        stricmp (ModuleType, "UEFI_DRIVER") == 0 ||
        stricmp (ModuleType, "SMM_CORE") == 0) {
          Type = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
          VerboseMsg ("Efi Image subsystem type is efi boot service driver.");

      } else if (stricmp (ModuleType, "UEFI_APPLICATION") == 0 ||
        stricmp (ModuleType, "APPLICATION") == 0) {
          Type = EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION;
          VerboseMsg ("Efi Image subsystem type is efi application.");

      } else if (stricmp (ModuleType, "DXE_RUNTIME_DRIVER") == 0 ||
        stricmp (ModuleType, "RT_DRIVER") == 0) {
          Type = EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
          VerboseMsg ("Efi Image subsystem type is efi runtime driver.");

      } else if (stricmp (ModuleType, "DXE_SAL_DRIVER") == 0 ||
        stricmp (ModuleType, "SAL_RT_DRIVER") == 0) {
          Type = EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER;
          VerboseMsg ("Efi Image subsystem type is efi sal runtime driver.");

      } else {
        Error (NULL, 0, 1003, "Invalid option value", "EFI_FILETYPE = %s", ModuleType);
        goto Finish;
      }
    }
  }

  //
  // Convert ELF image to PeImage
  //
  if (IsElfHeader(FileBuffer)) {
    VerboseMsg ("Convert %s from ELF to PE/COFF.", mInImageName);
    if (!ConvertElf(&FileBuffer, &FileLength)) {
      Error (NULL, 0, 3000, "Invalid", "Unable to convert %s from ELF to PE/COFF.", mInImageName);
      goto Finish;
    }
  }

  //
  // Make sure File Offsets and Virtual Offsets are the same in the image so it is XIP
  // XIP == eXecute In Place
  //
  PeCoffConvertImageToXip (&FileBuffer, &FileLength);

  //
  // Remove reloc section from PE or TE image
  //
  if (mOutImageType == FW_RELOC_STRIPEED_IMAGE) {
    //
    // Check TeImage
    //
    TeHdr = (EFI_TE_IMAGE_HEADER *) FileBuffer;
    if (TeHdr->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
      SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (TeHdr + 1);
      for (Index = 0; Index < TeHdr->NumberOfSections; Index ++, SectionHeader ++) {
        if (strcmp ((char *)SectionHeader->Name, ".reloc") == 0) {
          //
          // Check the reloc section is in the end of image.
          //
          if ((SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData) ==
            (FileLength + TeHdr->StrippedSize - sizeof (EFI_TE_IMAGE_HEADER))) {
              //
              // Remove .reloc section and update TeImage Header
              //
              FileLength = FileLength - SectionHeader->SizeOfRawData;
              SectionHeader->SizeOfRawData = 0;
              SectionHeader->Misc.VirtualSize = 0;
              TeHdr->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
              TeHdr->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size           = 0;
              break;
          }
        }
      }
    } else {
      //
      // Check PE Image
      //
      DosHdr = (EFI_IMAGE_DOS_HEADER *) FileBuffer;
      if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
        PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer);
        if (PeHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
          Error (NULL, 0, 3000, "Invalid", "TE and DOS header signatures were not found in %s image.", mInImageName);
          goto Finish;
        }
        DosHdr = NULL;
      } else {
        PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer + DosHdr->e_lfanew);
        if (PeHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
          Error (NULL, 0, 3000, "Invalid", "PE header signature was not found in %s image.", mInImageName);
          goto Finish;
        }
      }
      SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
      for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
        if (strcmp ((char *)SectionHeader->Name, ".reloc") == 0) {
          //
          // Check the reloc section is in the end of image.
          //
          if ((SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData) == FileLength) {
            //
            // Remove .reloc section and update PeImage Header
            //
            FileLength = FileLength - SectionHeader->SizeOfRawData;

            PeHdr->Pe32.FileHeader.Characteristics |= EFI_IMAGE_FILE_RELOCS_STRIPPED;
            if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
              Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->Pe32.OptionalHeader;
              Optional32->SizeOfImage -= SectionHeader->SizeOfRawData;
              Optional32->SizeOfInitializedData -= SectionHeader->SizeOfRawData;
              if (Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
                Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
                Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;
              }
            }
            if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
              Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->Pe32.OptionalHeader;
              Optional64->SizeOfImage -= SectionHeader->SizeOfRawData;
              Optional64->SizeOfInitializedData -= SectionHeader->SizeOfRawData;
              if (Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
                Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
                Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;
              }
            }
            SectionHeader->Misc.VirtualSize = 0;
            SectionHeader->SizeOfRawData = 0;
            break;
          }
        }
      }
    }
    //
    // Write file
    //
    goto WriteFile;
  }
  //
  // Read the dos & pe hdrs of the image
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)FileBuffer;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    // NO DOS header, check for PE/COFF header
    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer);
    if (PeHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
      Error (NULL, 0, 3000, "Invalid", "DOS header signature was not found in %s image.", mInImageName);
      goto Finish;
    }
    DosHdr = NULL;
  } else {

    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)(FileBuffer + DosHdr->e_lfanew);
    if (PeHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
      Error (NULL, 0, 3000, "Invalid", "PE header signature was not found in %s image.", mInImageName);
      goto Finish;
    }
  }

  if (PeHdr->Pe32.FileHeader.Machine == IMAGE_FILE_MACHINE_ARM) {
    // Some tools kick out IMAGE_FILE_MACHINE_ARM (0x1c0) vs IMAGE_FILE_MACHINE_ARMT (0x1c2)
    // so patch back to the offical UEFI value.
    PeHdr->Pe32.FileHeader.Machine = IMAGE_FILE_MACHINE_ARMT;
  }

  //
  // Set new base address into image
  //
  if (mOutImageType == FW_REBASE_IMAGE || mOutImageType == FW_SET_ADDRESS_IMAGE) {
    if ((PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) && (PeHdr->Pe32.FileHeader.Machine != IMAGE_FILE_MACHINE_IA64)) {
      if (NewBaseAddress >= 0x100000000ULL) {
        Error (NULL, 0, 3000, "Invalid", "New base address is larger than 4G for 32bit PE image");
        goto Finish;
      }
    }
    
    if (NegativeAddr) {
      //
      // Set Base Address to a negative value.
      //
      NewBaseAddress = (UINT64) (0 - NewBaseAddress);
    }
    if (mOutImageType == FW_REBASE_IMAGE) {
      Status = RebaseImage (mInImageName, FileBuffer, NewBaseAddress);
    } else {
      Status = SetAddressToSectionHeader (mInImageName, FileBuffer, NewBaseAddress);
    }
    if (EFI_ERROR (Status)) {
      if (NegativeAddr) {
        Error (NULL, 0, 3000, "Invalid", "Rebase/Set Image %s to Base address -0x%llx can't success", mInImageName, 0 - NewBaseAddress);
      } else {
        Error (NULL, 0, 3000, "Invalid", "Rebase/Set Image %s to Base address 0x%llx can't success", mInImageName, NewBaseAddress);
      }
      goto Finish;
    }

    //
    // Write file
    //
    goto WriteFile;
  }

  //
  // Extract bin data from Pe image.
  //
  if (mOutImageType == FW_BIN_IMAGE) {
    if (FileLength < PeHdr->Pe32.OptionalHeader.SizeOfHeaders) {
      Error (NULL, 0, 3000, "Invalid", "FileSize of %s is not a legal size.", mInImageName);
      goto Finish;
    }
    //
    // Output bin data from exe file
    //
    FileLength = FileLength - PeHdr->Pe32.OptionalHeader.SizeOfHeaders;
    memmove (FileBuffer, FileBuffer + PeHdr->Pe32.OptionalHeader.SizeOfHeaders, FileLength);
    VerboseMsg ("the size of output file is %u bytes", (unsigned) FileLength);
    goto WriteFile;
  }

  //
  // Zero Debug Information of Pe Image
  //
  if (mOutImageType == FW_ZERO_DEBUG_IMAGE) {
    Status = ZeroDebugData (FileBuffer, TRUE);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "Zero DebugData Error status is 0x%x", (int) Status);
      goto Finish;
    }

    //
    // Write the updated Image
    //
    VerboseMsg ("the size of output file is %u bytes", (unsigned) FileLength);
    goto WriteFile;
  }

  //
  // Set Time Stamp of Pe Image
  //
  if (mOutImageType == FW_SET_STAMP_IMAGE) {
    Status = SetStamp (FileBuffer, TimeStamp);
    if (EFI_ERROR (Status)) {
      goto Finish;
    }

    //
    // Write the updated Image
    //
    VerboseMsg ("the size of output file is %u bytes", (unsigned) FileLength);
    goto WriteFile;
  }

  //
  // Extract acpi data from pe image.
  //
  if (mOutImageType == FW_ACPI_IMAGE) {
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
    for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
      if (strcmp ((char *)SectionHeader->Name, ".data") == 0 || strcmp ((char *)SectionHeader->Name, ".sdata") == 0) {
        //
        // Check Acpi Table
        //
        if (SectionHeader->Misc.VirtualSize < SectionHeader->SizeOfRawData) {
          FileLength = SectionHeader->Misc.VirtualSize;
        } else {
          FileLength = SectionHeader->SizeOfRawData;
        }

        if (CheckAcpiTable (FileBuffer + SectionHeader->PointerToRawData, FileLength) != STATUS_SUCCESS) {
          Error (NULL, 0, 3000, "Invalid", "ACPI table check failed in %s.", mInImageName);
          goto Finish;
        }

        //
        // Output Apci data to file
        //
        memmove (FileBuffer, FileBuffer + SectionHeader->PointerToRawData, FileLength);
        VerboseMsg ("the size of output file is %u bytes", (unsigned) FileLength);
        goto WriteFile;
      }
    }
    Error (NULL, 0, 3000, "Invalid", "failed to get ACPI table from %s.", mInImageName);
    goto Finish;
  }
  //
  // Zero all unused fields of the DOS header
  //
  if (DosHdr != NULL) {
    memcpy (&BackupDosHdr, DosHdr, sizeof (EFI_IMAGE_DOS_HEADER));
    memset (DosHdr, 0, sizeof (EFI_IMAGE_DOS_HEADER));
    DosHdr->e_magic  = BackupDosHdr.e_magic;
    DosHdr->e_lfanew = BackupDosHdr.e_lfanew;

    for (Index = sizeof (EFI_IMAGE_DOS_HEADER); Index < (UINT32 ) DosHdr->e_lfanew; Index++) {
      FileBuffer[Index] = (UINT8) DosHdr->e_cp;
    }
  }

  //
  // Initialize TeImage Header
  //
  memset (&TEImageHeader, 0, sizeof (EFI_TE_IMAGE_HEADER));
  TEImageHeader.Signature        = EFI_TE_IMAGE_HEADER_SIGNATURE;
  TEImageHeader.Machine          = PeHdr->Pe32.FileHeader.Machine;
  TEImageHeader.NumberOfSections = (UINT8) PeHdr->Pe32.FileHeader.NumberOfSections;
  TEImageHeader.StrippedSize     = (UINT16) ((UINTN) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader) - (UINTN) FileBuffer);
  TEImageHeader.Subsystem        = (UINT8) Type;

  //
  // Patch the PE header
  //
  PeHdr->Pe32.OptionalHeader.Subsystem = (UINT16) Type;

  if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->Pe32.OptionalHeader;
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

    TEImageHeader.AddressOfEntryPoint = Optional32->AddressOfEntryPoint;
    TEImageHeader.BaseOfCode          = Optional32->BaseOfCode;
    TEImageHeader.ImageBase           = (UINT64) (Optional32->ImageBase);

    if (Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    }

    if (Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].Size = Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }

    //
    // Zero .pdata section data.
    //
    if (!KeepExceptionTableFlag && Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION &&
      Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress != 0 &&
      Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size != 0) {
        SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
        for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index++, SectionHeader++) {
          if (SectionHeader->VirtualAddress == Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress) {
            //
            // Zero .pdata Section data
            //
            memset (FileBuffer + SectionHeader->PointerToRawData, 0, SectionHeader->SizeOfRawData);
            //
            // Zero .pdata Section header name
            //
            memset (SectionHeader->Name, 0, sizeof (SectionHeader->Name));
            //
            // Zero Execption Table
            //
            Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress = 0;
            Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size           = 0;
            DebugMsg (NULL, 0, 9, "Zero the .pdata section for PE image", NULL);
            break;
          }
        }
    }

    //
    // Strip zero padding at the end of the .reloc section
    //
    if (!KeepZeroPendingFlag && Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
      if (Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size != 0) {
        SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
        for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index++, SectionHeader++) {
          //
          // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
          //
          if (SectionHeader->VirtualAddress == Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) {
            SectionHeader->Misc.VirtualSize = Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            AllignedRelocSize = (Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size + Optional32->FileAlignment - 1) & (~(Optional32->FileAlignment - 1));
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
                DebugMsg (NULL, 0, 9, "Remove the zero padding bytes at the end of the base relocations", "The size of padding bytes is %u", (unsigned) (SectionHeader->SizeOfRawData - AllignedRelocSize));
              }
            }
          }
        }
      }
    }
  } else if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->Pe32.OptionalHeader;
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

    TEImageHeader.AddressOfEntryPoint = Optional64->AddressOfEntryPoint;
    TEImageHeader.BaseOfCode          = Optional64->BaseOfCode;
    TEImageHeader.ImageBase           = (UINT64) (Optional64->ImageBase);

    if (Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    }

    if (Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].Size = Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }

    //
    // Zero the .pdata section for X64 machine and don't check the Debug Directory is empty
    // For Itaninum and X64 Image, remove .pdata section.
    //
    if ((!KeepExceptionTableFlag && PeHdr->Pe32.FileHeader.Machine == IMAGE_FILE_MACHINE_X64) || PeHdr->Pe32.FileHeader.Machine == IMAGE_FILE_MACHINE_IA64) {
      if (Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION &&
        Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress != 0 &&
        Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size != 0) {
          SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
          for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index++, SectionHeader++) {
            if (SectionHeader->VirtualAddress == Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress) {
              //
              // Zero .pdata Section header name
              //
              memset (SectionHeader->Name, 0, sizeof (SectionHeader->Name));

              RuntimeFunction = (RUNTIME_FUNCTION *)(FileBuffer + SectionHeader->PointerToRawData);
              for (Index1 = 0; Index1 < Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size / sizeof (RUNTIME_FUNCTION); Index1++, RuntimeFunction++) {
                SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
                for (Index2 = 0; Index2 < PeHdr->Pe32.FileHeader.NumberOfSections; Index2++, SectionHeader++) {
                  if (RuntimeFunction->UnwindInfoAddress >= SectionHeader->VirtualAddress && RuntimeFunction->UnwindInfoAddress < (SectionHeader->VirtualAddress + SectionHeader->SizeOfRawData)) {
                    UnwindInfo = (UNWIND_INFO *)(FileBuffer + SectionHeader->PointerToRawData + (RuntimeFunction->UnwindInfoAddress - SectionHeader->VirtualAddress));
                    if (UnwindInfo->Version == 1) {
                      memset (UnwindInfo + 1, 0, UnwindInfo->CountOfUnwindCodes * sizeof (UINT16));
                      memset (UnwindInfo, 0, sizeof (UNWIND_INFO));
                    }
                    break;
                  }
                }
                memset (RuntimeFunction, 0, sizeof (RUNTIME_FUNCTION));
              }
              //
              // Zero Execption Table
              //
              Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size = 0;
              Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress = 0;
              DebugMsg (NULL, 0, 9, "Zero the .pdata section if the machine type is X64 for PE32+ image", NULL);
              break;
            }
          }
      }
    }

    //
    // Strip zero padding at the end of the .reloc section
    //
    if (!KeepZeroPendingFlag && Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
      if (Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size != 0) {
        SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
        for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index++, SectionHeader++) {
          //
          // Look for the Section Header that starts as the same virtual address as the Base Relocation Data Directory
          //
          if (SectionHeader->VirtualAddress == Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) {
            SectionHeader->Misc.VirtualSize = Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            AllignedRelocSize = (Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size + Optional64->FileAlignment - 1) & (~(Optional64->FileAlignment - 1));
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
                DebugMsg (NULL, 0, 9, "Remove the zero padding bytes at the end of the base relocations", "The size of padding bytes is %u", (unsigned) (SectionHeader->SizeOfRawData - AllignedRelocSize));
              }
            }
          }
        }
      }
    }
  } else {
    Error (NULL, 0, 3000, "Invalid", "Magic 0x%x of PeImage %s is unknown.", PeHdr->Pe32.OptionalHeader.Magic, mInImageName);
    goto Finish;
  }

  if (((PeHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) == 0) && \
    (TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress == 0) && \
    (TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size == 0)) {
      //
      // PeImage can be loaded into memory, but it has no relocation section. 
      // Fix TeImage Header to set VA of relocation data directory to not zero, the size is still zero.
      //
      if (Optional32 != NULL) {
        TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = Optional32->SizeOfImage - sizeof (EFI_IMAGE_BASE_RELOCATION);
      } else if (Optional64 != NULL) {
        TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = Optional64->SizeOfImage - sizeof (EFI_IMAGE_BASE_RELOCATION);
      }
  }

  //
  // Fill HII section data
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
  for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index++) {
    if (stricmp ((char *)SectionHeader[Index].Name, ".hii") == 0) {
      //
      // Update resource section header offset
      //
      SetHiiResourceHeader ((UINT8*) FileBuffer + SectionHeader[Index].PointerToRawData, SectionHeader[Index].VirtualAddress);
      //
      // Update resource section name
      //
      strcpy((char *) SectionHeader[Index].Name, ".rsrc");
      //
      // Update resource data directory.
      //
      if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->Pe32.OptionalHeader;
        Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = SectionHeader[Index].VirtualAddress;
        Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = SectionHeader[Index].Misc.VirtualSize;
      } else if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->Pe32.OptionalHeader;
        Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = SectionHeader[Index].VirtualAddress;
        Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = SectionHeader[Index].Misc.VirtualSize;
      }
      break;
    }
  }

  //
  // Zero ExceptionTable Xdata
  //
  if (!KeepExceptionTableFlag) {
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) &(PeHdr->Pe32.OptionalHeader) + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader);
    for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index++) {
      if (stricmp ((char *)SectionHeader[Index].Name, ".xdata") == 0) {
        //
        // zero .xdata section
        //
        memset (FileBuffer + SectionHeader[Index].PointerToRawData, 0, SectionHeader[Index].SizeOfRawData);
        DebugMsg (NULL, 0, 9, NULL, "Zero the .xdata section for PE image at Offset 0x%x and Length 0x%x", (unsigned) SectionHeader[Index].PointerToRawData, (unsigned) SectionHeader[Index].SizeOfRawData);
        break;
      }
    }
  }

  //
  // Zero Time/Data field
  //
  ZeroDebugData (FileBuffer, FALSE);

  if (mOutImageType == FW_TE_IMAGE) {
    if ((PeHdr->Pe32.FileHeader.NumberOfSections &~0xFF) || (Type &~0xFF)) {
      //
      // Pack the subsystem and NumberOfSections into 1 byte. Make sure they fit both.
      //
      Error (NULL, 0, 3000, "Invalid", "Image's subsystem or NumberOfSections of PeImage %s cannot be packed into 1 byte.", mInImageName);
      goto Finish;
    }

    if ((PeHdr->Pe32.OptionalHeader.SectionAlignment != PeHdr->Pe32.OptionalHeader.FileAlignment)) {
      //
      // TeImage has the same section alignment and file alignment.
      //
      Error (NULL, 0, 3000, "Invalid", "Section-Alignment and File-Alignment of PeImage %s do not match, they must be equal for a TeImage.", mInImageName);
      goto Finish;
    }

    DebugMsg (NULL, 0, 9, "TeImage Header Info", "Machine type is %X, Number of sections is %X, Stripped size is %X, EntryPoint is %X, BaseOfCode is %X, ImageBase is %llX",
      TEImageHeader.Machine, TEImageHeader.NumberOfSections, TEImageHeader.StrippedSize, (unsigned) TEImageHeader.AddressOfEntryPoint, (unsigned) TEImageHeader.BaseOfCode, (unsigned long long) TEImageHeader.ImageBase);
    //
    // Update Image to TeImage
    //
    FileLength = FileLength - TEImageHeader.StrippedSize;
    memmove (FileBuffer + sizeof (EFI_TE_IMAGE_HEADER), FileBuffer + TEImageHeader.StrippedSize, FileLength);
    FileLength = FileLength + sizeof (EFI_TE_IMAGE_HEADER);
    memcpy (FileBuffer, &TEImageHeader, sizeof (EFI_TE_IMAGE_HEADER));
    VerboseMsg ("the size of output file is %u bytes", (unsigned) (FileLength));
  } else {

    //
    // Following codes are to fix the objcopy's issue:
    // objcopy in binutil 2.50.18 will set PE image's charactices to "RELOC_STRIPPED" if image has no ".reloc" section
    // It cause issue for EFI image which has no ".reloc" sections.
    // Following codes will be removed when objcopy in binutil fix this problem for PE image.
    //
    if ((PeHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) != 0) {
      if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        Optional32 = (EFI_IMAGE_OPTIONAL_HEADER32 *)&PeHdr->Pe32.OptionalHeader;
        if (Optional32->ImageBase == 0) {
          PeHdr->Pe32.FileHeader.Characteristics &= ~EFI_IMAGE_FILE_RELOCS_STRIPPED;
        }
      } else if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        Optional64 = (EFI_IMAGE_OPTIONAL_HEADER64 *)&PeHdr->Pe32.OptionalHeader;
        if (Optional64->ImageBase == 0) {
          PeHdr->Pe32.FileHeader.Characteristics &= ~EFI_IMAGE_FILE_RELOCS_STRIPPED;
        }
      }
    }
  }

WriteFile:
  //
  // Update Image to EfiImage or TE image
  //
  if (ReplaceFlag) {
    if ((FileLength != InputFileLength) || (memcmp (FileBuffer, InputFileBuffer, FileLength) != 0)) {
      //
      // Update File when File is changed.
      //
      fpInOut = fopen (LongFilePath (mInImageName), "wb");
      if (fpInOut == NULL) {
        Error (NULL, 0, 0001, "Error opening file", mInImageName);
        goto Finish;
      }
      fwrite (FileBuffer, 1, FileLength, fpInOut);
      VerboseMsg ("the size of output file is %u bytes", (unsigned) FileLength);
    }
  } else {
    if ((OutputFileTime < InputFileTime) || (FileLength != OutputFileLength) || (memcmp (FileBuffer, OutputFileBuffer, FileLength) != 0)) {
      //
      // Update File when File is changed or File is old.
      //
      fpOut = fopen (LongFilePath (OutImageName), "wb");
      if (fpOut == NULL) {
        Error (NULL, 0, 0001, "Error opening output file", OutImageName);
        goto Finish;
      }
      fwrite (FileBuffer, 1, FileLength, fpOut);
      VerboseMsg ("the size of output file is %u bytes", (unsigned) FileLength);
    }
  }
  mImageSize = FileLength;

Finish:
  if (fpInOut != NULL) {
    if (GetUtilityStatus () != STATUS_SUCCESS) {
      //
      // when file updates failed, original file is still recovered.
      //
      fwrite (InputFileBuffer, 1, InputFileLength, fpInOut);
    }
    //
    // Write converted data into fpInOut file and close input file.
    //
    fclose (fpInOut);
  }

  if (FileBuffer != NULL) {
    free (FileBuffer);
  }

  if (InputFileName != NULL) {
    free (InputFileName);
  }

  if (fpOut != NULL) {
    //
    // Write converted data into fpOut file and close output file.
    //
    fclose (fpOut);
    if (GetUtilityStatus () != STATUS_SUCCESS) {
      if (OutputFileBuffer == NULL) {
        remove (OutImageName);
      } else {
        fpOut = fopen (LongFilePath (OutImageName), "wb");
        fwrite (OutputFileBuffer, 1, OutputFileLength, fpOut);
        fclose (fpOut);
      }
    }
  }
  
  if (InputFileBuffer != NULL) {
    free (InputFileBuffer);
  }

  if (OutputFileBuffer != NULL) {
    free (OutputFileBuffer);
  }

  //
  // Write module size and time stamp to report file.
  //
  if (OutImageName != NULL) {
    FileLen = strlen (OutImageName);
  }
  if (FileLen >= 4 && strcmp (OutImageName + (FileLen - 4), ".efi") == 0) {
    ReportFileName = (CHAR8 *) malloc (FileLen + 1);
    if (ReportFileName != NULL) {
      strcpy (ReportFileName, OutImageName);
      strcpy (ReportFileName + (FileLen - 4), ".txt"); 
      ReportFile = fopen (LongFilePath (ReportFileName), "w+");
      if (ReportFile != NULL) {
        fprintf (ReportFile, "MODULE_SIZE = %u\n", (unsigned) mImageSize);
        fprintf (ReportFile, "TIME_STAMP = %u\n", (unsigned) mImageTimeStamp);
        fclose(ReportFile);
      }
      free (ReportFileName);
    }
  }
  VerboseMsg ("%s tool done with return code is 0x%x.", UTILITY_NAME, GetUtilityStatus ());

  return GetUtilityStatus ();
}

STATIC
EFI_STATUS
ZeroDebugData (
  IN OUT UINT8   *FileBuffer,
  BOOLEAN        ZeroDebugFlag
  )
/*++

Routine Description:

  Zero debug information in PeImage.

Arguments:

  FileBuffer    - Pointer to PeImage.
  ZeroDebugFlag - TRUE to zero Debug information, FALSE to only zero time/stamp

Returns:

  EFI_ABORTED   - PeImage is invalid.
  EFI_SUCCESS   - Zero debug data successfully.

--*/
{
  UINT32                           Index;
  UINT32                           DebugDirectoryEntryRva;
  UINT32                           DebugDirectoryEntryFileOffset;
  UINT32                           ExportDirectoryEntryRva;
  UINT32                           ExportDirectoryEntryFileOffset;
  UINT32                           ResourceDirectoryEntryRva;
  UINT32                           ResourceDirectoryEntryFileOffset;
  EFI_IMAGE_DOS_HEADER            *DosHdr;
  EFI_IMAGE_FILE_HEADER           *FileHdr;
  EFI_IMAGE_OPTIONAL_HEADER32     *Optional32Hdr;
  EFI_IMAGE_OPTIONAL_HEADER64     *Optional64Hdr;
  EFI_IMAGE_SECTION_HEADER        *SectionHeader;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *DebugEntry;
  UINT32                          *NewTimeStamp;  

  //
  // Init variable.
  //
  DebugDirectoryEntryRva           = 0;
  ExportDirectoryEntryRva          = 0;
  ResourceDirectoryEntryRva        = 0;
  DebugDirectoryEntryFileOffset    = 0;
  ExportDirectoryEntryFileOffset   = 0;
  ResourceDirectoryEntryFileOffset = 0;
  DosHdr   = (EFI_IMAGE_DOS_HEADER *)  FileBuffer;
  FileHdr  = (EFI_IMAGE_FILE_HEADER *) (FileBuffer + DosHdr->e_lfanew + sizeof (UINT32));


  DosHdr = (EFI_IMAGE_DOS_HEADER *)FileBuffer;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    // NO DOS header, must start with PE/COFF header
    FileHdr  = (EFI_IMAGE_FILE_HEADER *)(FileBuffer + sizeof (UINT32));
  } else {
    FileHdr  = (EFI_IMAGE_FILE_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof (UINT32));
  }

  //
  // Get Debug, Export and Resource EntryTable RVA address.
  // Resource Directory entry need to review.
  //
  if (FileHdr->Machine == EFI_IMAGE_MACHINE_IA32) {
    Optional32Hdr = (EFI_IMAGE_OPTIONAL_HEADER32 *) ((UINT8*) FileHdr + sizeof (EFI_IMAGE_FILE_HEADER));
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) Optional32Hdr +  FileHdr->SizeOfOptionalHeader);
    if (Optional32Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_EXPORT && \
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].Size != 0) {
      ExportDirectoryEntryRva = Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    }
    if (Optional32Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE && \
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size != 0) {
      ResourceDirectoryEntryRva = Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
    }
    if (Optional32Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG && \
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size != 0) {
      DebugDirectoryEntryRva = Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
      if (ZeroDebugFlag) {
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size = 0;
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = 0;
      }
    }
  } else {
    Optional64Hdr = (EFI_IMAGE_OPTIONAL_HEADER64 *) ((UINT8*) FileHdr + sizeof (EFI_IMAGE_FILE_HEADER));
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) Optional64Hdr +  FileHdr->SizeOfOptionalHeader);
    if (Optional64Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_EXPORT && \
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].Size != 0) {
      ExportDirectoryEntryRva = Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    }
    if (Optional64Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE && \
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size != 0) {
      ResourceDirectoryEntryRva = Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
    }
    if (Optional64Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG && \
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size != 0) {
      DebugDirectoryEntryRva = Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
      if (ZeroDebugFlag) {
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size = 0;
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = 0;
      }
    }
  }

  //
  // Get DirectoryEntryTable file offset.
  //
  for (Index = 0; Index < FileHdr->NumberOfSections; Index ++, SectionHeader ++) {
    if (DebugDirectoryEntryRva >= SectionHeader->VirtualAddress &&
        DebugDirectoryEntryRva < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize) {
        DebugDirectoryEntryFileOffset =
        DebugDirectoryEntryRva - SectionHeader->VirtualAddress + SectionHeader->PointerToRawData;
    }
    if (ExportDirectoryEntryRva >= SectionHeader->VirtualAddress &&
        ExportDirectoryEntryRva < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize) {
        ExportDirectoryEntryFileOffset =
        ExportDirectoryEntryRva - SectionHeader->VirtualAddress + SectionHeader->PointerToRawData;
    }
    if (ResourceDirectoryEntryRva >= SectionHeader->VirtualAddress &&
        ResourceDirectoryEntryRva < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize) {
        ResourceDirectoryEntryFileOffset =
        ResourceDirectoryEntryRva - SectionHeader->VirtualAddress + SectionHeader->PointerToRawData;
    }
  }

  //
  //Zero Debug Data and TimeStamp
  //
  FileHdr->TimeDateStamp = 0;
  mImageTimeStamp = 0;
  if (ExportDirectoryEntryFileOffset != 0) {
    NewTimeStamp  = (UINT32 *) (FileBuffer + ExportDirectoryEntryFileOffset + sizeof (UINT32));
    *NewTimeStamp = 0;
  }

  if (ResourceDirectoryEntryFileOffset != 0) {
    NewTimeStamp  = (UINT32 *) (FileBuffer + ResourceDirectoryEntryFileOffset + sizeof (UINT32));
    *NewTimeStamp = 0;
  }

  if (DebugDirectoryEntryFileOffset != 0) {
    DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) (FileBuffer + DebugDirectoryEntryFileOffset);
    DebugEntry->TimeDateStamp = 0;
    mImageTimeStamp = 0;
    if (ZeroDebugFlag) {
      memset (FileBuffer + DebugEntry->FileOffset, 0, DebugEntry->SizeOfData);
      memset (DebugEntry, 0, sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY));
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetStamp (
  IN OUT UINT8  *FileBuffer,
  IN     CHAR8  *TimeStamp
  )
/*++

Routine Description:

  Set new time stamp into PeImage FileHdr and Directory table:
  Debug, Export and Resource.

Arguments:

  FileBuffer    - Pointer to PeImage.
  TimeStamp     - Time stamp string.

Returns:

  EFI_INVALID_PARAMETER   - TimeStamp format is not recognized.
  EFI_SUCCESS             - Set new time stamp in this image successfully.

--*/
{
  struct tm                       stime;
  struct tm                       *ptime;
  time_t                          newtime;
  UINT32                          Index;
  UINT32                          DebugDirectoryEntryRva;
  UINT32                          DebugDirectoryEntryFileOffset;
  UINT32                          ExportDirectoryEntryRva;
  UINT32                          ExportDirectoryEntryFileOffset;
  UINT32                          ResourceDirectoryEntryRva;
  UINT32                          ResourceDirectoryEntryFileOffset;
  EFI_IMAGE_DOS_HEADER            *DosHdr;
  EFI_IMAGE_FILE_HEADER           *FileHdr;
  EFI_IMAGE_OPTIONAL_HEADER32     *Optional32Hdr;
  EFI_IMAGE_OPTIONAL_HEADER64     *Optional64Hdr;
  EFI_IMAGE_SECTION_HEADER        *SectionHeader;
  UINT32                          *NewTimeStamp;
  
  //
  // Init variable.
  //
  DebugDirectoryEntryRva           = 0;
  DebugDirectoryEntryFileOffset    = 0;
  ExportDirectoryEntryRva          = 0;
  ExportDirectoryEntryFileOffset   = 0;
  ResourceDirectoryEntryRva        = 0;
  ResourceDirectoryEntryFileOffset = 0;
  //
  // Get time and date that will be set.
  //
  if (TimeStamp == NULL) {
    Error (NULL, 0, 3000, "Invalid", "TimeStamp cannot be NULL.");
    return EFI_INVALID_PARAMETER;
  }
  //
  // compare the value with "NOW", if yes, current system time is set.
  //
  if (stricmp (TimeStamp, "NOW") == 0) {
    //
    // get system current time and date
    //
    time (&newtime);
  } else {
    //
    // Check Time Format strictly yyyy-mm-dd 00:00:00
    //
    for (Index = 0; TimeStamp[Index] != '\0' && Index < 20; Index ++) {
      if (Index == 4 || Index == 7) {
        if (TimeStamp[Index] == '-') {
          continue;
        }
      } else if (Index == 13 || Index == 16) {
        if (TimeStamp[Index] == ':') {
          continue;
        }
      } else if (Index == 10 && TimeStamp[Index] == ' ') {
        continue;
      } else if ((TimeStamp[Index] < '0') || (TimeStamp[Index] > '9')) {
        break;
      }
    }

    if (Index < 19 || TimeStamp[19] != '\0') {
      Error (NULL, 0, 1003, "Invalid option value", "Incorrect Time \"%s\"\n  Correct Format \"yyyy-mm-dd 00:00:00\"", TimeStamp);
      return EFI_INVALID_PARAMETER;
    }

    //
    // get the date and time from TimeStamp
    //
    if (sscanf (TimeStamp, "%d-%d-%d %d:%d:%d",
            &stime.tm_year,
            &stime.tm_mon,
            &stime.tm_mday,
            &stime.tm_hour,
            &stime.tm_min,
            &stime.tm_sec
            ) != 6) {
      Error (NULL, 0, 1003, "Invalid option value", "Incorrect Tiem \"%s\"\n  Correct Format \"yyyy-mm-dd 00:00:00\"", TimeStamp);
      return EFI_INVALID_PARAMETER;
    }

    //
    // in struct, Month (0 - 11; Jan = 0). So decrease 1 from it
    //
    if (stime.tm_mon <= 0 || stime.tm_mday <=0) {
      Error (NULL, 0, 3000, "Invalid", "%s Invalid date!", TimeStamp);
      return EFI_INVALID_PARAMETER;
    }
    stime.tm_mon -= 1;

    //
    // in struct, Year (current year minus 1900)
    // and only the dates can be handled from Jan 1, 1970 to Jan 18, 2038
    //
    //
    // convert 0 -> 100 (2000), 1 -> 101 (2001), ..., 38 -> 138 (2038)
    //
    if (stime.tm_year >= 1970 && stime.tm_year <= 2038) {
      //
      // convert 1970 -> 70, 2000 -> 100, ...
      //
      stime.tm_year -= 1900;
    } else {
      Error (NULL, 0, 3000, "Invalid", "%s Invalid or unsupported datetime!", TimeStamp);
      return EFI_INVALID_PARAMETER;
    }

    //
    // convert the date and time to time_t format
    //
    newtime = mktime (&stime);
    if (newtime == (time_t) - 1) {
      Error (NULL, 0, 3000, "Invalid", "%s Invalid or unsupported datetime!", TimeStamp);
      return EFI_INVALID_PARAMETER;
    }
  }

  ptime = localtime (&newtime);
  DebugMsg (NULL, 0, 9, "New Image Time Stamp", "%04d-%02d-%02d %02d:%02d:%02d",
            ptime->tm_year + 1900, ptime->tm_mon + 1, ptime->tm_mday, ptime->tm_hour, ptime->tm_min, ptime->tm_sec);
  //
  // Set new time and data into PeImage.
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *)FileBuffer;
  if (DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    // NO DOS header, must start with PE/COFF header
    FileHdr  = (EFI_IMAGE_FILE_HEADER *)(FileBuffer + sizeof (UINT32));
  } else {
    FileHdr  = (EFI_IMAGE_FILE_HEADER *)(FileBuffer + DosHdr->e_lfanew + sizeof (UINT32));
  }

  //
  // Get Debug, Export and Resource EntryTable RVA address.
  // Resource Directory entry need to review.
  //
  if (FileHdr->Machine == EFI_IMAGE_MACHINE_IA32) {
    Optional32Hdr = (EFI_IMAGE_OPTIONAL_HEADER32 *) ((UINT8*) FileHdr + sizeof (EFI_IMAGE_FILE_HEADER));
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) Optional32Hdr +  FileHdr->SizeOfOptionalHeader);
    if (Optional32Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_EXPORT && \
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].Size != 0) {
      ExportDirectoryEntryRva = Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    }
    if (Optional32Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE && \
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size != 0) {
      ResourceDirectoryEntryRva = Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
    }
    if (Optional32Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG && \
        Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size != 0) {
      DebugDirectoryEntryRva = Optional32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    }
  } else {
    Optional64Hdr = (EFI_IMAGE_OPTIONAL_HEADER64 *) ((UINT8*) FileHdr + sizeof (EFI_IMAGE_FILE_HEADER));
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) Optional64Hdr +  FileHdr->SizeOfOptionalHeader);
    if (Optional64Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_EXPORT && \
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].Size != 0) {
      ExportDirectoryEntryRva = Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    }
    if (Optional64Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE && \
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].Size != 0) {
      ResourceDirectoryEntryRva = Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
    }
    if (Optional64Hdr->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG && \
        Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size != 0) {
      DebugDirectoryEntryRva = Optional64Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    }
  }

  //
  // Get DirectoryEntryTable file offset.
  //
  for (Index = 0; Index < FileHdr->NumberOfSections; Index ++, SectionHeader ++) {
    if (DebugDirectoryEntryRva >= SectionHeader->VirtualAddress &&
        DebugDirectoryEntryRva < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize) {
        DebugDirectoryEntryFileOffset =
        DebugDirectoryEntryRva - SectionHeader->VirtualAddress + SectionHeader->PointerToRawData;
    }
    if (ExportDirectoryEntryRva >= SectionHeader->VirtualAddress &&
        ExportDirectoryEntryRva < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize) {
        ExportDirectoryEntryFileOffset =
        ExportDirectoryEntryRva - SectionHeader->VirtualAddress + SectionHeader->PointerToRawData;
    }
    if (ResourceDirectoryEntryRva >= SectionHeader->VirtualAddress &&
        ResourceDirectoryEntryRva < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize) {
        ResourceDirectoryEntryFileOffset =
        ResourceDirectoryEntryRva - SectionHeader->VirtualAddress + SectionHeader->PointerToRawData;
    }
  }

  //
  // Set new stamp
  //
  FileHdr->TimeDateStamp = (UINT32) newtime;
  mImageTimeStamp = (UINT32) newtime;
  if (ExportDirectoryEntryRva != 0) {
    NewTimeStamp  = (UINT32 *) (FileBuffer + ExportDirectoryEntryFileOffset + sizeof (UINT32));
    *NewTimeStamp = (UINT32) newtime;
  }

  if (ResourceDirectoryEntryRva != 0) {
    NewTimeStamp  = (UINT32 *) (FileBuffer + ResourceDirectoryEntryFileOffset + sizeof (UINT32));
    *NewTimeStamp = (UINT32) newtime;
  }

  if (DebugDirectoryEntryRva != 0) {
    NewTimeStamp  = (UINT32 *) (FileBuffer + DebugDirectoryEntryFileOffset + sizeof (UINT32));
    *NewTimeStamp = (UINT32) newtime;
  }

  return EFI_SUCCESS;
}

STATIC
STATUS
MicrocodeReadData (
  FILE          *InFptr,
  UINT32        *Data
  )
/*++

Routine Description:
  Read a 32-bit microcode data value from a text file and convert to raw binary form.

Arguments:
  InFptr    - file pointer to input text file
  Data      - pointer to where to return the data parsed

Returns:
  STATUS_SUCCESS    - no errors or warnings, Data contains valid information
  STATUS_ERROR      - errors were encountered

--*/
{
  CHAR8  Line[MAX_LINE_LEN];
  CHAR8  *cptr;
  unsigned ScannedData = 0;

  Line[MAX_LINE_LEN - 1]  = 0;
  while (1) {
    if (fgets (Line, MAX_LINE_LEN, InFptr) == NULL) {
      return STATUS_ERROR;
    }
    //
    // If it was a binary file, then it may have overwritten our null terminator
    //
    if (Line[MAX_LINE_LEN - 1] != 0) {
      return STATUS_ERROR;
    }

    //
    // strip space
    //
    for (cptr = Line; *cptr && isspace((int)*cptr); cptr++) {
    }

    // Skip Blank Lines and Comment Lines
    if ((strlen(cptr) != 0) && (*cptr != ';')) {
      break;
    }
  }

  // Look for
  // dd 000000001h ; comment
  // dd XXXXXXXX
  // DD  XXXXXXXXX
  //  DD XXXXXXXXX
  //
  if ((tolower((int)cptr[0]) == 'd') && (tolower((int)cptr[1]) == 'd') && isspace ((int)cptr[2])) {
    //
    // Skip blanks and look for a hex digit
    //
    cptr += 3;
    for (; *cptr && isspace((int)*cptr); cptr++) {
    }
    if (isxdigit ((int)*cptr)) {
      if (sscanf (cptr, "%X", &ScannedData) != 1) {
        return STATUS_ERROR;
      }
    }
    *Data = (UINT32) ScannedData;
    return STATUS_SUCCESS;
  }

  return STATUS_ERROR;
}
