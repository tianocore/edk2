/*++

Copyright (c) 2004 - 2007 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  GenAcpiTable.c
  
Abstract:

  A utility that extracts the .DATA section from a PE/COFF image.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h> // for PE32 structure definitions

#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

//
// Acpi Table definition
//
#include "Acpi.h"
#include "Acpi1_0.h"
#include "Acpi2_0.h"
#include "Acpi3_0.h"
#include "MemoryMappedConfigurationSpaceAccessTable.h"

//
// Version of this utility
//
#define UTILITY_NAME  "GenAcpiTable"
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 11

//
// Define the max length of a filename
//
#define MAX_PATH                  256
#define DEFAULT_OUTPUT_EXTENSION  ".acpi"

//
// Use this to track our command-line options and globals
//
struct {
  INT8  OutFileName[MAX_PATH];
  INT8  InFileName[MAX_PATH];
} mOptions;

//
// Use these to convert from machine type value to a named type
//
typedef struct {
  UINT16  Value;
  INT8    *Name;
} STRING_LOOKUP;

static STRING_LOOKUP  mMachineTypes[] = {
  EFI_IMAGE_MACHINE_IA32,
  "IA32",
  EFI_IMAGE_MACHINE_IA64,
  "IA64",
  EFI_IMAGE_MACHINE_EBC,
  "EBC",
  0,
  NULL
};

static STRING_LOOKUP  mSubsystemTypes[] = {
  EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION,
  "EFI application",
  EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER,
  "EFI boot service driver",
  EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER,
  "EFI runtime driver",
  0,
  NULL
};
//
//  Function prototypes
//
static
void
Version (
  VOID
  );

static
void
Usage (
  VOID
  );

static
STATUS
ParseCommandLine (
  int       Argc,
  char      *Argv[]
  );

static
STATUS
CheckAcpiTable (
  VOID      *AcpiTable,
  UINT32    Length
  );

static
STATUS
CheckPE32File (
  INT8      *FileName,
  FILE      *Fptr,
  UINT16    *MachineType,
  UINT16    *SubSystem
  );

static
STATUS
ProcessFile (
  INT8      *InFileName,
  INT8      *OutFileName
  );

static
void
DumpImage (
  INT8      *FileName
  );

main (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:
  

Arguments:

  Argc            - standard C main() argument count

  Argv            - standard C main() argument list

Returns:

  0             success
  non-zero      otherwise

--*/
// GC_TODO:    ] - add argument and description to function comment
{
  UINT32  Status;

  SetUtilityName (UTILITY_NAME);
  //
  // Parse the command line arguments
  //
  if (ParseCommandLine (Argc, Argv)) {
    return STATUS_ERROR;
  }
  //
  // Make sure we don't have the same filename for input and output files
  //
  if (stricmp (mOptions.OutFileName, mOptions.InFileName) == 0) {
    Error (NULL, 0, 0, mOptions.OutFileName, "input and output file names must be different");
    goto Finish;
  }
  //
  // Process the file
  //
  ProcessFile (mOptions.InFileName, mOptions.OutFileName);
Finish:
  Status = GetUtilityStatus ();
  return Status;
}

static
STATUS
ProcessFile (
  INT8      *InFileName,
  INT8      *OutFileName
  )
/*++

Routine Description:
  
  Process a PE32 EFI file.

Arguments:

  InFileName - Name of the PE32 EFI file to process.
  OutFileName - Name of the output file for the processed data.

Returns:

  0 - successful

--*/
{
  STATUS                      Status;
  UINTN                       Index;
  FILE                        *InFptr;
  FILE                        *OutFptr;
  UINT16                      MachineType;
  UINT16                      SubSystem;
  UINT32                      PESigOffset;
  EFI_IMAGE_FILE_HEADER       FileHeader;
  EFI_IMAGE_OPTIONAL_HEADER32 OptionalHeader32;
  EFI_IMAGE_OPTIONAL_HEADER64 OptionalHeader64;
  EFI_IMAGE_SECTION_HEADER    SectionHeader;
  UINT8                       *Buffer;
  long                        SaveFilePosition;

  InFptr  = NULL;
  OutFptr = NULL;
  Buffer  = NULL;
  Status  = STATUS_ERROR;
  //
  // Try to open the input file
  //
  if ((InFptr = fopen (InFileName, "rb")) == NULL) {
    Error (NULL, 0, 0, InFileName, "failed to open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Double-check the file to make sure it's what we expect it to be
  //
  if (CheckPE32File (InFileName, InFptr, &MachineType, &SubSystem) != STATUS_SUCCESS) {
    goto Finish;
  }
  //
  // Per the PE/COFF specification, at offset 0x3C in the file is a 32-bit
  // offset (from the start of the file) to the PE signature, which always
  // follows the MSDOS stub. The PE signature is immediately followed by the
  // COFF file header.
  //
  //
  if (fseek (InFptr, 0x3C, SEEK_SET) != 0) {
    Error (NULL, 0, 0, InFileName, "failed to seek to PE signature in file", NULL);
    goto Finish;
  }

  if (fread (&PESigOffset, sizeof (PESigOffset), 1, InFptr) != 1) {
    Error (NULL, 0, 0, InFileName, "failed to read PE signature offset from file");
    goto Finish;
  }

  if (fseek (InFptr, PESigOffset + 4, SEEK_SET) != 0) {
    Error (NULL, 0, 0, InFileName, "failed to seek to PE signature");
    goto Finish;
  }
  //
  // We should now be at the COFF file header. Read it in and verify it's
  // of an image type we support.
  //
  if (fread (&FileHeader, sizeof (EFI_IMAGE_FILE_HEADER), 1, InFptr) != 1) {
    Error (NULL, 0, 0, InFileName, "failed to read file header from image");
    goto Finish;
  }

  if ((FileHeader.Machine != EFI_IMAGE_MACHINE_IA32) && (FileHeader.Machine != EFI_IMAGE_MACHINE_IA64) && (FileHeader.Machine != EFI_IMAGE_MACHINE_X64)) {
    Error (NULL, 0, 0, InFileName, "image is of an unsupported machine type 0x%X", (UINT32) FileHeader.Machine);
    goto Finish;
  }
  //
  // Read in the optional header. Assume PE32, and if not, then re-read as PE32+
  //
  SaveFilePosition = ftell (InFptr);
  if (fread (&OptionalHeader32, sizeof (EFI_IMAGE_OPTIONAL_HEADER32), 1, InFptr) != 1) {
    Error (NULL, 0, 0, InFileName, "failed to read optional header from input file");
    goto Finish;
  }

  if (OptionalHeader32.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    if (fseek (InFptr, SaveFilePosition, SEEK_SET) != 0) {
      Error (NULL, 0, 0, InFileName, "failed to seek to .data section");
      goto Finish;
    }

    if (fread (&OptionalHeader64, sizeof (EFI_IMAGE_OPTIONAL_HEADER64), 1, InFptr) != 1) {
      Error (NULL, 0, 0, InFileName, "failed to read optional header from input file");
      goto Finish;
    }
  }
  //
  // Search for the ".data" section
  //
  for (Index = 0; Index < FileHeader.NumberOfSections; Index++) {
    if (fread (&SectionHeader, sizeof (EFI_IMAGE_SECTION_HEADER), 1, InFptr) != 1) {
      Error (NULL, 0, 0, InFileName, "failed to read optional header from input file");
      goto Finish;
    }

    if (strcmp (SectionHeader.Name, ".data") == 0 || strcmp (SectionHeader.Name, ".sdata") == 0) {
      if (fseek (InFptr, SectionHeader.PointerToRawData, SEEK_SET) != 0) {
        Error (NULL, 0, 0, InFileName, "failed to seek to .data section");
        goto Finish;
      }

      Buffer = (UINT8 *) malloc (SectionHeader.Misc.VirtualSize);
      if (Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Finish;
      }
      if (fread (Buffer, SectionHeader.Misc.VirtualSize, 1, InFptr) != 1) {
        Error (NULL, 0, 0, InFileName, "failed to .data section");
        goto Finish;
      }

      //
      // Check Acpi Table
      //
      if (CheckAcpiTable (Buffer, SectionHeader.Misc.VirtualSize) != STATUS_SUCCESS) {
        Error (NULL, 0, 0, InFileName, "failed to check ACPI table");
        goto Finish;
      }

      //
      // Now open our output file
      //
      if ((OutFptr = fopen (OutFileName, "wb")) == NULL) {
        Error (NULL, 0, 0, OutFileName, "failed to open output file for writing");
        goto Finish;
      }

      if (fwrite (Buffer, SectionHeader.Misc.VirtualSize, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, OutFileName, "failed to write .data section");
        goto Finish;
      }

      Status = STATUS_SUCCESS;
      goto Finish;
    }
  }

  Status = STATUS_ERROR;

Finish:
  if (InFptr != NULL) {
    fclose (InFptr);
  }
  //
  // Close the output file. If there was an error, delete the output file so
  // that a subsequent build will rebuild it.
  //
  if (OutFptr != NULL) {
    fclose (OutFptr);
    if (GetUtilityStatus () == STATUS_ERROR) {
      remove (OutFileName);
    }
  }

  //
  // Free up our buffer
  //
  if (Buffer != NULL) {
    free (Buffer);
  }

  return Status;
}

static
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
    Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass AcpiTable Length check");
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
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass FACP revision check");
      return STATUS_ERROR;
    }
    if (ExpectedLength != AcpiHeader->Length) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass FACP Length check");
      return STATUS_ERROR;
    }
    break;

  //
  // "FACS" Firmware ACPI Control Structure
  //
  case EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE:
    Facs = (EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)AcpiTable;
    if ((Facs->Version != 0) &&
        (Facs->Version != EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION) &&
        (Facs->Version != EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION)){
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass FACS version check");
      return STATUS_ERROR;
    }
    if ((Facs->Length != sizeof(EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE)) &&
        (Facs->Length != sizeof(EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE)) &&
        (Facs->Length != sizeof(EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE))) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass FACS Length check");
      return STATUS_ERROR;
    }
    break;

  //
  // "DSDT" Differentiated System Description Table
  //
  case EFI_ACPI_3_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
    if (AcpiHeader->Revision > EFI_ACPI_3_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_REVISION) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass DSDT revision check");
      return STATUS_ERROR;
    }
    if (AcpiHeader->Length <= sizeof(EFI_ACPI_DESCRIPTION_HEADER)) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass DSDT Length check");
      return STATUS_ERROR;
    }
    break;

  //
  // "APIC" Multiple APIC Description Table
  //
  case EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:
    if ((AcpiHeader->Revision != EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION) &&
        (AcpiHeader->Revision != EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION) &&
        (AcpiHeader->Revision != EFI_ACPI_3_0_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION)) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass APIC revision check");
      return STATUS_ERROR;
    }
    if (AcpiHeader->Length <= sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(UINT32) + sizeof(UINT32)) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass APIC Length check");
      return STATUS_ERROR;
    }
    break;

  //
  // "MCFG" PCI Express Memory Mapped Configuration Space Base Address Description Table
  //
  case EFI_ACPI_3_0_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE:
    if (AcpiHeader->Revision != EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass MCFG revision check");
      return STATUS_ERROR;
    }
    if (AcpiHeader->Length <= sizeof(EFI_ACPI_DESCRIPTION_HEADER) + sizeof(UINT64)) {
      Error (NULL, 0, 0, "CheckAcpiTable", "failed to pass MCFG Length check");
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

static
STATUS
CheckPE32File (
  INT8      *FileName,
  FILE      *Fptr,
  UINT16    *MachineType,
  UINT16    *SubSystem
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FileName    - GC_TODO: add argument description
  Fptr        - GC_TODO: add argument description
  MachineType - GC_TODO: add argument description
  SubSystem   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  /*++

Routine Description:
  
  Given a file pointer to a supposed PE32 image file, verify that it is indeed a
  PE32 image file, and then return the machine type in the supplied pointer.

Arguments:

  Fptr          File pointer to the already-opened PE32 file
  MachineType   Location to stuff the machine type of the PE32 file. This is needed
                because the image may be Itanium-based, IA32, or EBC.

Returns:

  0             success
  non-zero      otherwise

--*/
  EFI_IMAGE_DOS_HEADER      DosHeader;
  EFI_IMAGE_FILE_HEADER     FileHdr;
  EFI_IMAGE_OPTIONAL_HEADER OptionalHdr;
  UINT32                    PESig;
  STATUS                    Status;

  Status = STATUS_ERROR;
  //
  // Position to the start of the file
  //
  fseek (Fptr, 0, SEEK_SET);
  //
  // Read the DOS header
  //
  if (fread (&DosHeader, sizeof (DosHeader), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read the DOS stub from the input file");
    goto Finish;
  }
  //
  // Check the magic number (0x5A4D)
  //
  if (DosHeader.e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    Error (NULL, 0, 0, FileName, "input file does not appear to be a PE32 image (magic number)");
    goto Finish;
  }
  //
  // Position into the file and check the PE signature
  //
  fseek (Fptr, (long) DosHeader.e_lfanew, SEEK_SET);
  if (fread (&PESig, sizeof (PESig), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read PE signature bytes");
    goto Finish;
  }
  //
  // Check the PE signature in the header "PE\0\0"
  //
  if (PESig != EFI_IMAGE_NT_SIGNATURE) {
    Error (NULL, 0, 0, FileName, "file does not appear to be a PE32 image (signature)");
    goto Finish;
  }
  //
  // Read the file header
  //
  if (fread (&FileHdr, sizeof (FileHdr), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read PE file header from input file");
    goto Finish;
  }
  //
  // Read the optional header so we can get the subsystem
  //
  if (fread (&OptionalHdr, sizeof (OptionalHdr), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read COFF optional header from input file");
    goto Finish;
  }

  *SubSystem = OptionalHdr.Subsystem;
  //
  // Good to go
  //
  Status = STATUS_SUCCESS;
Finish:
  fseek (Fptr, 0, SEEK_SET);
  return Status;
}

static
int
ParseCommandLine (
  int         Argc,
  char        *Argv[]
  )
/*++

Routine Description:
  
  Given the Argc/Argv program arguments, and a pointer to an options structure,
  parse the command-line options and check their validity.


Arguments:

  Argc            - standard C main() argument count
  Argv            - standard C main() argument list

Returns:

  STATUS_SUCCESS    success
  non-zero          otherwise

--*/
// GC_TODO:    ] - add argument and description to function comment
{
  //
  // Clear out the options
  //
  memset ((char *) &mOptions, 0, sizeof (mOptions));
  //
  // Skip over the program name
  //
  Argc--;
  Argv++;

  if (Argc < 1) {
    Usage();
    return STATUS_ERROR;
  }
  
  if ((strcmp(Argv[0], "-h") == 0) || (strcmp(Argv[0], "--help") == 0) ||
      (strcmp(Argv[0], "-?") == 0) || (strcmp(Argv[0], "/?") == 0)) {
    Usage();
    return STATUS_ERROR;
  }
  
  if ((strcmp(Argv[0], "-V") == 0) || (strcmp(Argv[0], "--version") == 0)) {
    Version();
    return STATUS_ERROR;
  }

  if (Argc != 2) {
    Usage ();
    return STATUS_ERROR;
  }

  strcpy (mOptions.InFileName, Argv[0]);
  //
  // Next argument
  //
  Argv++;
  Argc--;

  strcpy (mOptions.OutFileName, Argv[0]);

  return STATUS_SUCCESS;
}

static
void
Version (
  VOID
  )
{
  printf ("%s v%d.%d -EDK Utility for generating ACPI Table image from an EFI PE32 image.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2006 Intel Corporation. All rights reserved.\n");
}


static
void
Usage (
  VOID
  )
/*++

Routine Description:
  
  Print usage information for this utility.

Arguments:

  None.

Returns:

  Nothing.

--*/
{
  int               Index;
  static const char *Msg[] = {
    "\nUsage: "UTILITY_NAME " {-h|--help|-?|/?|-V|--version} InFileName OutFileName",
    "  where:",
    "    InFileName  - name of the input PE32 file",
    "    OutFileName - to write output to OutFileName rather than InFileName"DEFAULT_OUTPUT_EXTENSION,
    NULL
  };
  
  Version();
  for (Index = 0; Msg[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Msg[Index]);
  }
}
