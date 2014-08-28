/** @file
This file contains the relevant declarations required to generate Option Rom File

Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_ROM_H__
#define __EFI_ROM_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Common/UefiBaseTypes.h>
#include <IndustryStandard/PeImage.h> // for PE32 structure definitions

#include <IndustryStandard/pci22.h>  // for option ROM header structures
#include <IndustryStandard/pci30.h>

#include "Compress.h"
#include "CommonLib.h"

//
// Version of this utility
//
#define UTILITY_NAME "EfiRom"
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

//
// Define the max length of a filename
//
#define MAX_PATH                  200

//
// Define the default file extension name
//
#define DEFAULT_OUTPUT_EXTENSION  ".rom"

//
// Max size for an option ROM image
//
#define MAX_OPTION_ROM_SIZE (1024 * 1024 * 16)  // 16MB

//
// Values for the indicator field in the PCI data structure
//
#define INDICATOR_LAST  0x80  // last file in series of files

//
// Masks for the FILE_LIST.FileFlags field
//
#define FILE_FLAG_BINARY    0x01
#define FILE_FLAG_EFI       0x02
#define FILE_FLAG_COMPRESS  0x04

//
// Use this linked list structure to keep track of all the filenames
// specified on the command line.
//
typedef struct _FILE_LIST {
  struct _FILE_LIST *Next;
  CHAR8             *FileName;
  UINT32            FileFlags;
  UINT32            ClassCode;
  UINT16            CodeRevision;
} FILE_LIST;

//
// Use this to track our command-line options
//
typedef struct {
  CHAR8     OutFileName[MAX_PATH];
  INT8      NoLast;
  UINT16    ClassCode;
  UINT16    PciRevision;
  UINT16    VendId;
  UINT16    DevId;
  UINT8     VendIdValid;
  UINT8     DevIdValid;
  INT8      Verbose;
  INT8      Quiet;
  INT8      Debug;
  INT8      Pci23;
  INT8      Pci30;
  INT8      DumpOption;
//  INT8      Help;
//  INT8      Version;  
  FILE_LIST *FileList;
} OPTIONS;

//
// Make a global structure to keep track of command-line options
//
static OPTIONS  mOptions;

//
// Use these to convert from machine type value to a named type
//
typedef struct {
  UINT16  Value;
  CHAR8   *Name;
} STRING_LOOKUP;

//
// Machine Types
//
static STRING_LOOKUP  mMachineTypes[] = {
  { EFI_IMAGE_MACHINE_IA32, "IA32" },
  { EFI_IMAGE_MACHINE_IA64, "IA64" },
  { EFI_IMAGE_MACHINE_EBC, "EBC" },
  { 0, NULL }
};

//
// Subsystem Types
//
static STRING_LOOKUP  mSubsystemTypes[] = {
  { EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION, "EFI application" },
  { EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER, "EFI boot service driver" },
  { EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER, "EFI runtime driver" },
  { 0, NULL }
};

//
//  Function prototypes
//
static
void
Version (
  VOID
  )
/*++

Routine Description:

  Displays the utility version to STDOUT

Arguments:

  None

Returns:

  None

--*/
;

static
void
Usage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/  
;

static
int
ParseCommandLine (
  int       Argc,
  char      *Argv[],
  OPTIONS   *Options
  )
/*++

Routine Description:
  
  Given the Argc/Argv program arguments, and a pointer to an options structure,
  parse the command-line options and check their validity.

Arguments:

  Argc            - standard C main() argument count
  Argv[]          - standard C main() argument list
  Options         - pointer to a structure to store the options in

Returns:

  STATUS_SUCCESS    success
  non-zero          otherwise

--*/
;

static
int
CheckPE32File (
  FILE      *Fptr,
  UINT16    *MachineType,
  UINT16    *SubSystem
  )
/*++

Routine Description:
  
  Given the Argc/Argv program arguments, and a pointer to an options structure,
  parse the command-line options and check their validity.

Arguments:

  Argc            - standard C main() argument count
  Argv[]          - standard C main() argument list
  Options         - pointer to a structure to store the options in

Returns:

  STATUS_SUCCESS    success
  non-zero          otherwise

--*/  
;

static
int
ProcessEfiFile (
  FILE      *OutFptr,
  FILE_LIST *InFile,
  UINT16    VendId,
  UINT16    DevId,
  UINT32    *Size
  )
/*++

Routine Description:
  
  Process a PE32 EFI file.

Arguments:

  OutFptr     - file pointer to output binary ROM image file we're creating
  InFile      - structure contains information on the PE32 file to process
  VendId      - vendor ID as required in the option ROM header
  DevId       - device ID as required in the option ROM header
  Size        - pointer to where to return the size added to the output file

Returns:

  0 - successful

--*/
;

static
int
ProcessBinFile (
  FILE      *OutFptr,
  FILE_LIST *InFile,
  UINT32    *Size
  )
/*++

Routine Description:
  
  Process a binary input file.

Arguments:

  OutFptr     - file pointer to output binary ROM image file we're creating
  InFile      - structure contains information on the binary file to process
  Size        - pointer to where to return the size added to the output file

Returns:

  0 - successful

--*/  
;

static
void
DumpImage (
  FILE_LIST *InFile
  )
/*++

Routine Description:

  Dump the headers of an existing option ROM image

Arguments:

  InFile  - the file name of an existing option ROM image

Returns:

  none

--*/
;

char                  *
GetMachineTypeStr (
  UINT16    MachineType
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  MachineType - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

static
char                  *
GetSubsystemTypeStr (
  UINT16  SubsystemType
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SubsystemType - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
