/** @file
This file contains the relevant declarations required to generate Boot Strap File
  
Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Module Coded to EFI 2.0 Coding Conventions
//
#ifndef   __GEN_VTF_H__
#define   __GEN_VTF_H__

//
// External Files Referenced
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __GNUC__
#include <io.h>
#endif
#include "assert.h"
#include <Common/PiFirmwareFile.h>
#include "ParseInf.h"

//
// Internal Constants
//
#define CV_N_TYPE(a,b)            (UINT8)(((UINT8)a << 7) + (UINT8)b)  // Keeps the CV and Type in same byte field
#define MAKE_VERSION(a,b)         (UINT16)(((UINT16)a << 8) + (UINT16)b)

#define   FILE_NAME_SIZE          256
#define   COMPONENT_NAME_SIZE     128
#define   VTF_INPUT_FILE          "VTF.INF"
#define   VTF_OUTPUT_FILE1        "VTF1.RAW"
#define   VTF_OUTPUT_FILE2        "VTF2.RAW"
#define   VTF_SYM_FILE            "Vtf.SYM"
#define   FIT_SIGNATURE           "_FIT_   "

//
//Fit Type Definition
//
#define   COMP_TYPE_FIT_HEADER          0x00
#define   COMP_TYPE_FIT_PAL_B           0x01

//
// This is generic PAL_A
//
#define   COMP_TYPE_FIT_PAL_A           0x0F
#define   COMP_TYPE_FIT_PEICORE         0x10
#define   COMP_TYPE_FIT_AUTOSCAN        0x30
#define   COMP_TYPE_FIT_FV_BOOT         0x7E

//
//This is processor Specific PAL_A
//
#define   COMP_TYPE_FIT_PAL_A_SPECIFIC  0x0E
#define   COMP_TYPE_FIT_UNUSED    0x7F

#define   FIT_TYPE_MASK           0x7F
#define   CHECKSUM_BIT_MASK       0x80

//
// IPF processor address is cached bit
//
#define IPF_CACHE_BIT 0x8000000000000000ULL

//
// Size definition to calculate the location from top of address for
// each component
//
#define SIZE_IA32_RESET_VECT      0x10        // 16 Bytes
#define SIZE_SALE_ENTRY_POINT     0x08        // 8 Byte
#define SIZE_FIT_TABLE_ADD        0x08        // 8 Byte
#define SIZE_FIT_TABLE_PAL_A      0x10     
#define SIZE_RESERVED             0x10


#define SIZE_TO_OFFSET_PAL_A_END  (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT + \
                                  SIZE_FIT_TABLE_ADD + SIZE_FIT_TABLE_PAL_A + \
                                  SIZE_RESERVED)
#define SIZE_TO_PAL_A_FIT         (SIZE_IA32_RESET_VECT + SIZE_SALE_ENTRY_POINT + \
                                  SIZE_FIT_TABLE_ADD + SIZE_FIT_TABLE_PAL_A)

#define SIZE_OF_PAL_HEADER        0x40        //PAL has 64 byte header

//
// Utility Name
//
#define UTILITY_NAME  "GenVtf"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION   0
#define UTILITY_MINOR_VERSION   1
#define UTILITY_DATE            __DATE__

//
// The maximum number of arguments accepted from the command line.
//
#define ONE_VTF_ARGS  10
#define TWO_VTF_ARGS  12
#define THREE_VTF_ARGS  16

static BOOLEAN VerboseMode = FALSE;

//
// Internal Data Structure
//
typedef enum _LOC_TYPE 
{
  NONE,                   // In case there is - INF file
  FIRST_VTF,              // First VTF
  SECOND_VTF,             // Outside VTF
} LOC_TYPE;

typedef struct _PARSED_VTF_INFO {
  CHAR8       CompName[COMPONENT_NAME_SIZE];
  LOC_TYPE    LocationType;
  UINT8       CompType;
  UINT8       MajorVer;
  UINT8       MinorVer;
  UINT8       CheckSumRequired;
  BOOLEAN     VersionPresent;                // If it is TRUE, then, Version is in INF file
  BOOLEAN     PreferredSize;
  BOOLEAN     PreferredAddress;
  CHAR8       CompBinName[FILE_NAME_SIZE];
  CHAR8       CompSymName[FILE_NAME_SIZE];
  UINTN       CompSize;
  UINT64      CompPreferredAddress;
  UINT32      Align;

  //
  //  Fixed - warning C4133: '=' : incompatible types - from 'struct _ParsedVtfInfo *' to 'struct _PARSED_VTF_INFO *'
  //  Fixed - warning C4133: '=' : incompatible types - from 'struct _ParsedVtfInfo *' to 'struct _PARSED_VTF_INFO *'
  //  Fixed - warning C4133: '=' : incompatible types - from 'struct _ParsedVtfInfo *' to 'struct _PARSED_VTF_INFO *'
  //  Fixed - warning C4133: '=' : incompatible types - from 'struct _ParsedVtfInfo *' to 'struct _PARSED_VTF_INFO *'
  //
  struct      _PARSED_VTF_INFO   *NextVtfInfo;
} PARSED_VTF_INFO;

#pragma pack (1)
typedef struct {
  UINT64      CompAddress;
  UINT32      CompSize;
  UINT16      CompVersion;
  UINT8       CvAndType;
  UINT8       CheckSum;
} FIT_TABLE;
#pragma pack ()

//
// Function Prototype Declarations
//

EFI_STATUS
UpdateVtfBuffer(
  IN  UINT64    StartAddress,
  IN  UINT8     *Buffer,
  IN  UINT64    DataSize,
  IN  LOC_TYPE  LocType
  )
/*++

Routine Description:

  Update the Firmware Volume Buffer with requested buffer data
  
Arguments:

  StartAddress   - StartAddress in buffer. This number will automatically
                  point to right address in buffer where data needed 
                  to be updated.
  Buffer         - Buffer pointer from data will be copied to memory mapped buffer.
  DataSize       - Size of the data needed to be copied.
  LocType        - The type of the VTF

Returns:
  
  EFI_ABORTED  - The input parameter is error
  EFI_SUCCESS  - The function completed successfully

--*/
;

EFI_STATUS
UpdateSymFile (
  IN UINT64 BaseAddress,
  IN CHAR8  *DestFileName,
  IN CHAR8  *SourceFileName,
  IN UINT64 FileSize
  )
/*++

Routine Description:

  This function adds the SYM tokens in the source file to the destination file.
  The SYM tokens are updated to reflect the base address.

Arguments:

  BaseAddress    - The base address for the new SYM tokens.
  DestFileName   - The destination file.
  SourceFileName - The source file.
  FileSize       - Size of bin file.

Returns:

  EFI_SUCCESS             - The function completed successfully.
  EFI_INVALID_PARAMETER   - One of the input parameters was invalid.
  EFI_ABORTED             - An error occurred.

--*/
;

EFI_STATUS
CalculateFitTableChecksum (
  VOID
  )
/*++
  
Routine Description:

  This function will perform byte checksum on the FIT table, if the the checksum required
  field is set to CheckSum required. If the checksum is not required then checksum byte
  will have value as 0;.
  
Arguments:

  NONE
  
Returns:

  Status       - Value returned by call to CalculateChecksum8 ()
  EFI_SUCCESS  - The function completed successfully
    
--*/
;

EFI_STATUS
GenerateVtfImage (
  IN  UINT64  StartAddress1,
  IN  UINT64  Size1,
  IN  UINT64  StartAddress2,
  IN  UINT64  Size2,
  IN  FILE    *fp
  )
/*++

Routine Description:

  This is the main function which will be called from application.

Arguments:

  StartAddress1  - The start address of the first VTF      
  Size1          - The size of the first VTF
  StartAddress2  - The start address of the second VTF      
  Size2          - The size of the second VTF

Returns:
 
  EFI_OUT_OF_RESOURCES - Can not allocate memory
  The return value can be any of the values 
  returned by the calls to following functions:
      GetVtfRelatedInfoFromInfFile
      ProcessAndCreateVtf
      UpdateIA32ResetVector
      UpdateFfsHeader
      WriteVtfBinary
  
--*/
;

EFI_STATUS
PeimFixupInFitTable (
  IN  UINT64  StartAddress
  )
/*++

Routine Description:

  This function is an entry point to fixup SAL-E entry point.

Arguments:

  StartAddress - StartAddress for PEIM.....
    
Returns:
 
  EFI_SUCCESS   - The function completed successfully
  EFI_ABORTED   - Error Opening File 

--*/
;

VOID
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
#endif
