/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FlashDefFile.c

Abstract:

  Utility for flash management in the Intel Platform Innovation Framework
  for EFI build environment.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Common/UefiBaseTypes.h>
#include <Common/FirmwareVolumeHeader.h>
#include <Common/MultiPhase.h>

#include "EfiUtilityMsgs.h"
#include "FlashDefFile.h"
#include "SimpleFileParsing.h"
#include "Symbols.h"

//
// #include "TrackMallocFree.h"
//
#define WCHAR_T           char
#define MAX_STRING_LEN    256
#define MAX_NAME_LEN      128
#define BUFFER_SIZE       1024
#define MAX_ATTR_LEN      128
#define MAX_AREATYPE_LEN  128
#define COLUMN2_START     60
#define COLUMN3_START     70
//
// Information for each subregions defined in the fdf file will be saved in these
//
typedef struct _FLASH_SUBREGION_DESCRIPTION {
  struct _FLASH_SUBREGION_DESCRIPTION *Next;
  int                                 CreateHob;                  // to add to the auto-created HOB array
  WCHAR_T                             Name[MAX_NAME_LEN];         // each subregion within a region must have a unique name
  unsigned int                        Size;                       // size, in bytes, of this subregion
  unsigned int                        SizeLeft;                   // used when creating the image
  WCHAR_T                             Attributes[MAX_ATTR_LEN];   // subregion attributes used in the output HOB
  WCHAR_T                             AreaType[MAX_AREATYPE_LEN]; // subregion area type used in the output HOB
  EFI_GUID                            NameGuid;                   // used in the output HOB
  WCHAR_T                             NameGuidString[MAX_NAME_LEN];
  EFI_GUID                            AreaTypeGuid;               // used in the output HOB
  WCHAR_T                             AreaTypeGuidString[MAX_NAME_LEN];
  EFI_GUID                            FileSystemGuid;             // used in the output HOB
  WCHAR_T                             FileSystemGuidString[MAX_NAME_LEN];
} FLASH_SUBREGION_DESCRIPTION;

//
// Information for each block in a flash device will be saved in one of these.
// We'll also use it for region definitions.
//
typedef struct _FLASH_BLOCK_DESCRIPTION {
  struct _FLASH_BLOCK_DESCRIPTION *Next;                      // next block in the linked list
  WCHAR_T                         Name[MAX_NAME_LEN];         // each block must have a unique name
  unsigned int                    Size;                       // size, in bytes, of this block
  unsigned int                    SizeLeft;                   // for use when creating image
  unsigned int                    Flags;                      // user-defined flags for the block
  unsigned int                    Alignment;                  // power of 2 alignment
  WCHAR_T                         Attributes[MAX_ATTR_LEN];   // only used for Region definitions
  WCHAR_T                         AreaType[MAX_AREATYPE_LEN]; // only used for Region definitions
  EFI_GUID                        AreaTypeGuid;
  WCHAR_T                         AreaTypeGuidString[MAX_NAME_LEN];
  FLASH_SUBREGION_DESCRIPTION     *Subregions;
  FLASH_SUBREGION_DESCRIPTION     *LastSubregion;
} FLASH_BLOCK_DESCRIPTION;

//
// Information for each flash device will be saved in one of these
//
typedef struct _FLASH_DEVICE_DESCRIPTION {
  struct _FLASH_DEVICE_DESCRIPTION  *Next;              // next flash device in our linked list
  int                               ErasePolarity;      // erase polarity of the flash device
  unsigned int                      BaseAddress;        // base address of the flash device
  unsigned int                      Size;               // total size, in bytes, of the flash device
  WCHAR_T                           Name[MAX_NAME_LEN]; // name of the flash device
  FLASH_BLOCK_DESCRIPTION           *PBlocks;           // linked list of physical block descriptors
  FLASH_BLOCK_DESCRIPTION           *LastPBlock;        // last block in the linked list
  FLASH_BLOCK_DESCRIPTION           *Regions;           // linked list of flash region descriptors
  FLASH_BLOCK_DESCRIPTION           *LastRegion;        // last region in the linked list
} FLASH_DEVICE_DESCRIPTION;

//
// For image definitions, they can specify a file name or raw data bytes. Keep a linked list.
//
typedef struct _IMAGE_DEFINITION_ENTRY {
  struct _IMAGE_DEFINITION_ENTRY  *Next;
  WCHAR_T                         RegionName[MAX_NAME_LEN];
  WCHAR_T                         SubregionName[MAX_NAME_LEN];
  WCHAR_T                         Name[MAX_NAME_LEN]; // file or data name
  int                             IsRawData;          // non-zero if raw data bytes
  unsigned int                    RawDataSize;
  char                            *RawData;
  int                             Optional;           // optional file (don't include if it doesn't exist)
} IMAGE_DEFINITION_ENTRY;

//
// When we parse an image definition, save all the data for each in one of these
//
typedef struct _IMAGE_DEFINITION {
  struct _IMAGE_DEFINITION  *Next;
  WCHAR_T                   Name[MAX_NAME_LEN];
  IMAGE_DEFINITION_ENTRY    *Entries;
  IMAGE_DEFINITION_ENTRY    *LastEntry;
} IMAGE_DEFINITION;

typedef struct {
  char  *BufferStart;
  char  *BufferEnd;
  char  *BufferPos;
} BUFFER_DATA;

static const char               *CIncludeHeader = "/*++\n\n"
"  DO NOT EDIT -- file auto-generated by FlashMap utility\n\n""--*/\n""\n""#ifndef _FLASH_MAP_H_\n"
"#define _FLASH_MAP_H_\n\n";
//
//  "#include \"EfiFlashMap.h\"\n\n";
//
static const char               *CIncludeFooter = "#endif // #ifndef _FLASH_MAP_H_\n\n";

static const char               *CFlashMapDataFileHeader = "/*++\n\n"
"  DO NOT EDIT -- file auto-generated by FlashMap utility\n\n""--*/\n""\n";

static FLASH_DEVICE_DESCRIPTION *mFlashDevices      = NULL;
static IMAGE_DEFINITION         *mImageDefinitions  = NULL;

//
// Local function prototypes
//
static
BUFFER_DATA                     *
CreateBufferData (
  VOID
  );

static
BOOLEAN
AddBufferDataByte (
  BUFFER_DATA *Buffer,
  char        Data
  );

static
void
FreeBufferData (
  BUFFER_DATA *Buffer,
  BOOLEAN     FreeData
  );

static
char                            *
GetBufferData (
  BUFFER_DATA *Buffer,
  int         *BufferSize
  );

static
FLASH_SUBREGION_DESCRIPTION     *
ParseSubregionDefinition (
  unsigned int  SizeLeft
  );

void
FDFConstructor (
  VOID
  )
/*++

Routine Description:
  Initialization routine for the services that operate on a flash
  definition file.

Arguments:
  None.

Returns:
  NA

--*/
{
  mFlashDevices     = NULL;
  mImageDefinitions = NULL;
}

void
FDFDestructor (
  VOID
  )
/*++

Routine Description:
  Finalization/cleanup routine for the services that operate on a flash
  definition file.

Arguments:
  None.

Returns:
  NA

--*/
{
  FLASH_BLOCK_DESCRIPTION     *FBNext;
  FLASH_DEVICE_DESCRIPTION    *FDNext;
  IMAGE_DEFINITION            *IDNext;
  IMAGE_DEFINITION_ENTRY      *IDENext;
  FLASH_SUBREGION_DESCRIPTION *SubNext;
  //
  // Go through all our flash devices and free the memory
  //
  while (mFlashDevices != NULL) {
    //
    // Free the physical block definitions
    //
    while (mFlashDevices->PBlocks != NULL) {
      FBNext = mFlashDevices->PBlocks->Next;
      _free (mFlashDevices->PBlocks);
      mFlashDevices->PBlocks = FBNext;
    }
    //
    // Free the region definitions
    //
    while (mFlashDevices->Regions != NULL) {
      FBNext = mFlashDevices->Regions->Next;
      //
      // First free the subregion definitions
      //
      while (mFlashDevices->Regions->Subregions != NULL) {
        SubNext = mFlashDevices->Regions->Subregions->Next;
        _free (mFlashDevices->Regions->Subregions);
        mFlashDevices->Regions->Subregions = SubNext;
      }

      _free (mFlashDevices->Regions);
      mFlashDevices->Regions = FBNext;
    }

    FDNext = mFlashDevices->Next;
    _free (mFlashDevices);
    mFlashDevices = FDNext;
  }
  //
  // Free up the image definitions, and the data
  //
  while (mImageDefinitions != NULL) {
    //
    // Free the entries
    //
    while (mImageDefinitions->Entries != NULL) {
      IDENext = mImageDefinitions->Entries->Next;
      if (mImageDefinitions->Entries->RawData != NULL) {
        _free (mImageDefinitions->Entries->RawData);
      }

      _free (mImageDefinitions->Entries);
      mImageDefinitions->Entries = IDENext;
    }

    IDNext = mImageDefinitions->Next;
    _free (mImageDefinitions);
    mImageDefinitions = IDNext;
  }
}

STATUS
FDFParseFile (
  char    *FileName
  )
/*++

Routine Description:
  Parse the specified flash definition file, saving the definitions in
  file-static variables for use by other functions.
  
Arguments:
  FileName    - name of the input flash definition text file.

Returns:
  STATUS_SUCCESS    - file parsed with no errors or warnings
  STATUS_WARNING    - warnings, but no errors, were encountered while parsing
  STATUS_ERROR      - errors were encountered while parsing
  
--*/
{
  FILE                        *Fptr;
  STATUS                      Status;
  unsigned int                Num;
  FLASH_DEVICE_DESCRIPTION    *FDDesc;
  FLASH_BLOCK_DESCRIPTION     *FBlockDesc;
  FLASH_BLOCK_DESCRIPTION     *TempBlockDesc;
  FLASH_SUBREGION_DESCRIPTION *Subregion;
  FLASH_SUBREGION_DESCRIPTION *TempSubregion;
  unsigned int                BlockSizeLeft;
  unsigned int                RegionSizeLeft;
  unsigned int                SubregionSizeLeft;
  int                         ErrorCount;
  int                         WarningCount;
  IMAGE_DEFINITION            *ImageDef;
  IMAGE_DEFINITION_ENTRY      *ImageDefEntry;
  IMAGE_DEFINITION_ENTRY      *TempImageDefEntry;
  BUFFER_DATA                 *BufferData;
  char                        Str[100];
  BOOLEAN                     PreviousComma;

  if ((Fptr = fopen (FileName, "r")) == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open input flash definition file for reading");
    return STATUS_ERROR;
  }

  fclose (Fptr);
  Status        = STATUS_SUCCESS;
  ErrorCount    = 0;
  WarningCount  = 0;
  //
  // Initialize the simple-file-parsing routines
  //
  SFPInit ();
  //
  // Open the file
  //
  if ((Status = SFPOpenFile (FileName)) != STATUS_SUCCESS) {
    return Status;
  }
  //
  // Parse the file. Should start with a series of these:
  // FlashDevice {
  //   Name = "FLASH_1234", Size = 0x2004, BaseAddress = 0xFFF0000, ErasePolarity = 1,
  //   Block { Name = "BLOCK1",  Size = 0x1000, Flags = 0x0001 }
  //   Block { Name = "BLOCK2",  Size = 0x1004, Flags = 0x0002 }
  //   Region  { Name = "REGION_NAME", Size = 0x2004, Align= 4 }
  // }
  //
  while (SFPIsKeyword ("FlashDevice")) {
    //
    // Allocate memory for new flash device description block
    //
    FDDesc = (FLASH_DEVICE_DESCRIPTION *) _malloc (sizeof (FLASH_DEVICE_DESCRIPTION));
    if (FDDesc == NULL) {
      Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
      ErrorCount++;
      goto Done;
    }

    memset (FDDesc, 0, sizeof (FLASH_DEVICE_DESCRIPTION));
    //
    // Open brace -- warning if not there
    //
    if (!SFPIsToken ("{")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected {", NULL);
      WarningCount++;
    }
    //
    // Parse:  Name = "DeviceName",
    //
    if (!SFPIsKeyword ("Name")) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Name'", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken ("=")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
      WarningCount++;
    }

    if (!SFPGetQuotedString (FDDesc->Name, sizeof (FDDesc->Name))) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted name of flash device", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken (",")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following flash device name", NULL);
      WarningCount++;
    }
    //
    // Parse: Size = 0x20000,
    //
    if (!SFPIsKeyword ("Size")) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Size'", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken ("=")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
      WarningCount++;
    }

    if (!SFPGetNumber (&FDDesc->Size)) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric Size value", NULL);
      ErrorCount++;
      goto Done;
    }
    //
    // Check for 0 size
    //
    if (FDDesc->Size == 0) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, FDDesc->Name, "Size field cannot be 0", NULL);
      ErrorCount++;
      goto Done;
    }

    SFPIsToken (",");
    //
    // Parse: BaseAddress = 0xFFF0000,
    //
    if (!SFPIsKeyword ("BaseAddress")) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'BaseAddress'", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken ("=")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
      WarningCount++;
    }

    if (!SFPGetNumber (&FDDesc->BaseAddress)) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric value for BaseAddress", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken (",")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following BaseAddress value", NULL);
      WarningCount++;
    }
    //
    // Parse: ErasePolarity = 1,
    //
    if (!SFPIsKeyword ("ErasePolarity")) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'ErasePolarity'", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken ("=")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
      WarningCount++;
    }

    if (!SFPGetNumber (&Num) || ((Num != 0) && (Num != 1))) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric erase polarity value 1 or 0", NULL);
      ErrorCount++;
      goto Done;
    }

    FDDesc->ErasePolarity = Num;
    if (!SFPIsToken (",")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following erase polarity value", NULL);
      WarningCount++;
    }
    //
    // Parse array of:
    //   Block {  Name = "BLOCK1", Size = 0x1000, Flags = 0x0001 }
    //
    // Keep track of size to make sure the sum of the physical blocks and region sizes do not
    // exceed the size of the flash device.
    //
    BlockSizeLeft   = FDDesc->Size;
    RegionSizeLeft  = FDDesc->Size;
    while (SFPIsKeyword ("Block")) {
      //
      // Allocate memory for a new physical block descriptor
      //
      FBlockDesc = (FLASH_BLOCK_DESCRIPTION *) _malloc (sizeof (FLASH_BLOCK_DESCRIPTION));
      if (FBlockDesc == NULL) {
        Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
        ErrorCount++;
        goto Done;
      }

      memset (FBlockDesc, 0, sizeof (FLASH_BLOCK_DESCRIPTION));
      //
      // Open brace -- warning if not there
      //
      if (!SFPIsToken ("{")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected {", NULL);
        WarningCount++;
      }
      //
      // Parse:  Name = "BlockName",
      //
      if (!SFPIsKeyword ("Name")) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Name'", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
        WarningCount++;
      }

      if (!SFPGetQuotedString (FBlockDesc->Name, sizeof (FBlockDesc->Name))) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted name of physical block", NULL);
        ErrorCount++;
        goto Done;
      }
      //
      // Make sure there are no other physical block names with this same name
      //
      for (TempBlockDesc = FDDesc->PBlocks; TempBlockDesc != NULL; TempBlockDesc = TempBlockDesc->Next) {
        if (strcmp (TempBlockDesc->Name, FBlockDesc->Name) == 0) {
          Error (
            SFPGetFileName (),
            SFPGetLineNumber (),
            0,
            TempBlockDesc->Name,
            "physical block with this name already defined"
            );
          ErrorCount++;
        }
      }

      if (!SFPIsToken (",")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following physical block name", NULL);
        WarningCount++;
      }
      //
      // Parse: Size = 0x2000,
      //
      if (!SFPIsKeyword ("Size")) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Size'", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
        WarningCount++;
      }

      if (!SFPGetNumber (&FBlockDesc->Size)) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric Size value", NULL);
        ErrorCount++;
        goto Done;
      }
      //
      // Make sure the sum of physical blocks so far does not exceed flash device size
      //
      if (BlockSizeLeft < FBlockDesc->Size) {
        Error (
          SFPGetFileName (),
          SFPGetLineNumber (),
          0,
          "sum of physical block sizes exceeds flash device size",
          NULL
          );
        ErrorCount++;
      }

      BlockSizeLeft -= FBlockDesc->Size;
      SFPIsToken (",");
      //
      // Optional parse: Flags = 0xFFF0000,
      //
      if (SFPIsKeyword ("Flags")) {
        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPGetNumber (&FBlockDesc->Flags)) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric value for Flags", NULL);
          ErrorCount++;
          goto Done;
        }
      }

      if (!SFPIsToken ("}")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected PhysicalBlock closing brace '}'", NULL);
        WarningCount++;
      }
      //
      // Add the physical block descriptor to the end of the linked list
      //
      if (FDDesc->LastPBlock != NULL) {
        FDDesc->LastPBlock->Next = FBlockDesc;
      } else {
        FDDesc->PBlocks = FBlockDesc;
      }

      FDDesc->LastPBlock = FBlockDesc;
    }
    //
    // Make sure sum of sizes of physical blocks added up to size of flash device
    //
    if (BlockSizeLeft != 0) {
      Error (
        SFPGetFileName (),
        SFPGetLineNumber (),
        0,
        NULL,
        "sum of sizes of physical blocks (0x%08X) != flash device size (0x%08X) : delta = 0x%08X",
        FDDesc->Size - BlockSizeLeft,
        FDDesc->Size,
        BlockSizeLeft
        );
      ErrorCount++;
    }
    //
    // Parse array of:
    //   Region { Name = "REGION_1", Size = 0x2000, Flags = 0x1234, Alignment = 4, Attributes = "str", AreaType = "str" }
    //
    while (SFPIsKeyword ("Region")) {
      //
      // Allocate memory for a new physical block descriptor
      //
      FBlockDesc = (FLASH_BLOCK_DESCRIPTION *) _malloc (sizeof (FLASH_BLOCK_DESCRIPTION));
      if (FBlockDesc == NULL) {
        Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
        ErrorCount++;
        goto Done;
      }

      memset (FBlockDesc, 0, sizeof (FLASH_BLOCK_DESCRIPTION));
      //
      // Open brace -- warning if not there
      //
      if (!SFPIsToken ("{")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected {", NULL);
        WarningCount++;
      }
      //
      // Parse:  Name = "BlockName",
      //
      if (!SFPIsKeyword ("Name")) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Name'", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
        WarningCount++;
      }

      if (!SFPGetQuotedString (FBlockDesc->Name, sizeof (FBlockDesc->Name))) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted Region name", NULL);
        ErrorCount++;
        goto Done;
      }
      //
      // Make sure there are no other region names with this same name
      //
      for (TempBlockDesc = FDDesc->Regions; TempBlockDesc != NULL; TempBlockDesc = TempBlockDesc->Next) {
        if (strcmp (TempBlockDesc->Name, FBlockDesc->Name) == 0) {
          Error (
            SFPGetFileName (),
            SFPGetLineNumber (),
            0,
            TempBlockDesc->Name,
            "Region with this name already defined"
            );
          ErrorCount++;
        }
      }

      if (!SFPIsToken (",")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following Region name", NULL);
        WarningCount++;
      }
      //
      // Parse: Size = 0x2000,
      //
      if (!SFPIsKeyword ("Size")) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Size'", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
        WarningCount++;
      }

      if (!SFPGetNumber (&FBlockDesc->Size)) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric Size value", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken (",")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ','", NULL);
      }
      //
      // Make sure the sum of regions so far does not exceed flash device size
      //
      if (RegionSizeLeft < FBlockDesc->Size) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "sum of Region sizes exceeds flash device size", NULL);
        ErrorCount++;
      }

      RegionSizeLeft -= FBlockDesc->Size;
      //
      // Optional parse: Flags = 0xFFF0000,
      //
      if (SFPIsKeyword ("Flags")) {
        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPGetNumber (&FBlockDesc->Flags)) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric value for Flags", NULL);
          ErrorCount++;
          goto Done;
        }
        //
        // comma
        //
        if (!SFPIsToken (",")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ','", NULL);
        }
      }
      //
      // Optional parse: Alignment = 4
      //
      if (SFPIsKeyword ("Alignment")) {
        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPGetNumber (&FBlockDesc->Alignment)) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric Alignment value", NULL);
          ErrorCount++;
          goto Done;
        }
        //
        // comma
        //
        if (!SFPIsToken (",")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ','", NULL);
        }
      }
      //
      // Parse:  Attributes = "String",
      //
      if (!SFPIsKeyword ("Attributes")) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Attributes'", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
        WarningCount++;
      }

      if (!SFPGetQuotedString (FBlockDesc->Attributes, sizeof (FBlockDesc->Attributes))) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted Attributes string", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken (",")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ','", NULL);
      }
      //
      // Parse:  AreaType = "String",
      //
      if (!SFPIsKeyword ("AreaType")) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'AreaType'", NULL);
        ErrorCount++;
        goto Done;
      }

      if (!SFPIsToken ("=")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
        WarningCount++;
      }

      if (!SFPGetQuotedString (FBlockDesc->AreaType, sizeof (FBlockDesc->AreaType))) {
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted AreaType string", NULL);
        ErrorCount++;
        goto Done;
      }

      PreviousComma = SFPIsToken (",");
      //
      // Parse optional attribute "AreaTypeGuid"
      //
      if (SFPIsKeyword ("AreaTypeGuid")) {
        //
        // Check for preceeding comma now
        //
        if (!PreviousComma) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ',' before 'AreaTypeGuid'", NULL);
          WarningCount++;
        }
        
        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }
        
        if (SFPGetQuotedString (FBlockDesc->AreaTypeGuidString, sizeof (FBlockDesc->AreaTypeGuidString))) {
          //
          // Nothing else to do
          //
        } else if (!SFPGetGuid (PARSE_GUID_STYLE_5_FIELDS, &FBlockDesc->AreaTypeGuid)) {
          Error (
            SFPGetFileName (),
            SFPGetLineNumber (),
            0,
            "expected AreaTypeGuid quoted string or GUID of form 12345678-1234-1234-1234-123456789ABC",
            NULL
            );
          ErrorCount++;
          goto Done;
        }
        PreviousComma = SFPIsToken (",");
      }

      //
      // Parse optional Subregion definitions
      //
      SubregionSizeLeft = FBlockDesc->Size;
      while (SFPIsToken ("Subregion")) {
        if (!PreviousComma) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ',' before 'Subregion'", NULL);
          WarningCount++;
          PreviousComma = TRUE;
        }

        Subregion = ParseSubregionDefinition (SubregionSizeLeft);
        if (Subregion == NULL) {
          ErrorCount++;
          goto Done;
        }

        SubregionSizeLeft -= Subregion->Size;
        //
        // Add it to the end of our list
        //
        if (FBlockDesc->Subregions == NULL) {
          FBlockDesc->Subregions = Subregion;
        } else {
          FBlockDesc->LastSubregion->Next = Subregion;
        }

        FBlockDesc->LastSubregion = Subregion;
        //
        // Make sure all subregion names are unique. We do this each time
        // through so that we catch the error immediately after it happens, in
        // which case the reported line number is at least close to where the
        // problem lies. We don't exit on the error because we can continue parsing
        // the script to perhaps catch other errors or warnings.
        //
        for (Subregion = FBlockDesc->Subregions; Subregion != NULL; Subregion = Subregion->Next) {
          for (TempSubregion = Subregion->Next; TempSubregion != NULL; TempSubregion = TempSubregion->Next) {
            if (strcmp (Subregion->Name, TempSubregion->Name) == 0) {
              Error (SFPGetFileName (), SFPGetLineNumber (), 0, Subregion->Name, "duplicate Subregion name");
              ErrorCount++;
            }
          }
        }
      }

      if (!SFPIsToken ("}")) {
        Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected Region closing brace '}'", NULL);
        WarningCount++;
      }
      //
      // Add the region descriptor to the end of the linked list
      //
      if (FDDesc->LastRegion != NULL) {
        FDDesc->LastRegion->Next = FBlockDesc;
      } else {
        FDDesc->Regions = FBlockDesc;
      }

      FDDesc->LastRegion = FBlockDesc;
    }
    //
    // Make sure sum of sizes of regions adds up to size of flash device
    //
    if (RegionSizeLeft != 0) {
      Error (
        SFPGetFileName (),
        SFPGetLineNumber (),
        0,
        NULL,
        "sum of sizes of Regions (0x%08X) != flash device size (0x%08X) : delta = 0x%08X",
        FDDesc->Size - RegionSizeLeft,
        FDDesc->Size,
        RegionSizeLeft
        );
      ErrorCount++;
    }
    //
    // Look for closing brace
    //
    if (!SFPIsToken ("}")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected FlashDevice closing brace '}'", NULL);
      WarningCount++;
    }
    //
    // Add this flash description to the list
    //
    FDDesc->Next  = mFlashDevices;
    mFlashDevices = FDDesc;
  }

  while (SFPIsKeyword ("FlashDeviceImage")) {
    //
    // Allocate memory for a new FD image definition
    //
    ImageDef = (IMAGE_DEFINITION *) _malloc (sizeof (IMAGE_DEFINITION));
    if (ImageDef == NULL) {
      Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
      ErrorCount++;
      goto Done;
    }

    memset (ImageDef, 0, sizeof (IMAGE_DEFINITION));
    //
    // Open brace -- warning if not there
    //
    if (!SFPIsToken ("{")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected {", NULL);
      WarningCount++;
    }
    //
    // Parse:  Name = "ImageName",
    //
    if (!SFPIsKeyword ("Name")) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Name'", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken ("=")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
      WarningCount++;
    }

    if (!SFPGetQuotedString (ImageDef->Name, sizeof (ImageDef->Name))) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted name of image", NULL);
      ErrorCount++;
      goto Done;
    }

    if (!SFPIsToken (",")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following image name", NULL);
      WarningCount++;
    }

    while (1) {
      //
      // Parse: File { Name = "FV\FvOem.fv", Region = "REGION_OEM", Optional = TRUE }
      //
      if (SFPIsKeyword ("File")) {
        ImageDefEntry = (IMAGE_DEFINITION_ENTRY *) _malloc (sizeof (IMAGE_DEFINITION_ENTRY));
        if (ImageDefEntry == NULL) {
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          ErrorCount++;
          goto Done;
        }

        memset (ImageDefEntry, 0, sizeof (IMAGE_DEFINITION_ENTRY));
        //
        // Open brace -- warning if not there
        //
        if (!SFPIsToken ("{")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected {", NULL);
          WarningCount++;
        }
        //
        // Parse: Name = "FileName.txt"
        //
        if (!SFPIsKeyword ("Name")) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Name'", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPGetQuotedString (ImageDefEntry->Name, sizeof (ImageDefEntry->Name))) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted name of file", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken (",")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following file name", NULL);
          WarningCount++;
        }
        //
        // Parse: Region = "REGION_NAME"
        //
        if (!SFPIsKeyword ("Region")) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Region'", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPGetQuotedString (ImageDefEntry->RegionName, sizeof (ImageDefEntry->RegionName))) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted Region name", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken (",")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following Region name", NULL);
          WarningCount++;
        }
        //
        // Parse optional: Subregion = "SUBREGION_NAME"
        //
        if (SFPIsKeyword ("Subregion")) {
          if (!SFPIsToken ("=")) {
            Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
            WarningCount++;
          }

          if (!SFPGetQuotedString (ImageDefEntry->SubregionName, sizeof (ImageDefEntry->SubregionName))) {
            Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted Subegion name", NULL);
            ErrorCount++;
            goto Done;
          }

          if (!SFPIsToken (",")) {
            Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following Subregion name", NULL);
            WarningCount++;
          }
          //
          // For a given region, you can only place data using the region name, or the subregion names.
          // In other words, you can't say File1->Region1 and File2->Region1.Subregion1. Enforce that
          // here by checking that any previous entries with the same Region name had a Subregion specified
          // as well.
          //
          for (TempImageDefEntry = ImageDef->Entries;
               TempImageDefEntry != NULL;
               TempImageDefEntry = TempImageDefEntry->Next
              ) {
            if (strcmp (TempImageDefEntry->Name, ImageDefEntry->Name) == 0) {
              if (TempImageDefEntry->SubregionName[0] == 0) {
                Error (
                  SFPGetFileName (),
                  SFPGetLineNumber (),
                  0,
                  TempImageDefEntry->RegionName,
                  "data already placed on a region-basis in the region, can't place data using subregions"
                  );
                ErrorCount++;
              }
            }
          }
        }
        //
        // Optional parse: Optional = TRUE | FALSE
        //
        if (SFPIsKeyword ("Optional")) {
          if (!SFPIsToken ("=")) {
            Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
            WarningCount++;
          }

          if (!SFPIsKeyword ("TRUE")) {
            ImageDefEntry->Optional = 1;
          } else if (SFPIsKeyword ("FALSE")) {
            //
            // Already set to 0
            //
          } else {
            Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'TRUE' or 'FALSE'", NULL);
            ErrorCount++;
            goto Done;
          }

          SFPIsToken (",");
        }
        //
        // Closing brace
        //
        if (!SFPIsToken ("}")) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '}' closing brace to File entry", NULL);
          ErrorCount++;
          goto Done;
        }
        //
        // Add the entry to the end of the list
        //
        if (ImageDef->LastEntry != NULL) {
          ImageDef->LastEntry->Next = ImageDefEntry;
        } else {
          ImageDef->Entries = ImageDefEntry;
        }

        ImageDef->LastEntry = ImageDefEntry;
      } else if (SFPIsKeyword ("RawData")) {
        //
        // Parse: RawData { Name = "PadBytes", Region = "REGION_1", Data = { 0x78, 0x56, 0x34, 0x12 }}
        //
        ImageDefEntry = (IMAGE_DEFINITION_ENTRY *) _malloc (sizeof (IMAGE_DEFINITION_ENTRY));
        if (ImageDefEntry == NULL) {
          Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
          ErrorCount++;
          goto Done;
        }

        memset (ImageDefEntry, 0, sizeof (IMAGE_DEFINITION_ENTRY));
        ImageDefEntry->IsRawData = 1;
        //
        // Open brace -- warning if not there
        //
        if (!SFPIsToken ("{")) {
          Warning (
            SFPGetFileName (),
            SFPGetLineNumber (),
            0,
            "expected '{' opening brace for RawData definition",
            NULL
            );
          WarningCount++;
        }
        //
        // Parse: Name = "PadBytes"
        //
        if (!SFPIsKeyword ("Name")) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Name'", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPGetQuotedString (ImageDefEntry->Name, sizeof (ImageDefEntry->Name))) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted name of raw data", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken (",")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following raw data name", NULL);
          WarningCount++;
        }
        //
        // Parse: Region = "REGION_NAME"
        //
        if (!SFPIsKeyword ("Region")) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Region'", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPGetQuotedString (ImageDefEntry->RegionName, sizeof (ImageDefEntry->RegionName))) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted Region name", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken (",")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following Region name", NULL);
          WarningCount++;
        }
        //
        // Parse optional: Subregion = "SUBREGION_NAME"
        //
        if (SFPIsKeyword ("Subregion")) {
          if (!SFPIsToken ("=")) {
            Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
            WarningCount++;
          }

          if (!SFPGetQuotedString (ImageDefEntry->SubregionName, sizeof (ImageDefEntry->SubregionName))) {
            Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted Subegion name", NULL);
            ErrorCount++;
            goto Done;
          }

          if (!SFPIsToken (",")) {
            Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following Subregion name", NULL);
            WarningCount++;
          }
          //
          // For a given region, you can only place data using the region name, or the subregion names.
          // In other words, you can't say File1->Region1 and File2->Region1.Subregion1. Enforce that
          // here by checking that any previous entries with the same Region name had a Subregion specified
          // as well.
          //
          for (TempImageDefEntry = ImageDef->Entries;
               TempImageDefEntry != NULL;
               TempImageDefEntry = TempImageDefEntry->Next
              ) {
            if (strcmp (TempImageDefEntry->Name, ImageDefEntry->Name) == 0) {
              if (TempImageDefEntry->SubregionName[0] == 0) {
                Error (
                  SFPGetFileName (),
                  SFPGetLineNumber (),
                  0,
                  TempImageDefEntry->RegionName,
                  "data already placed on a region-basis in the region, can't place data using subregions"
                  );
                ErrorCount++;
              }
            }
          }
        }
        //
        // Parse: Data = { 0x78, 0x56, 0x34, 0x12 }
        //
        if (!SFPIsKeyword ("Data")) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Data'", NULL);
          ErrorCount++;
          goto Done;
        }

        if (!SFPIsToken ("=")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
          WarningCount++;
        }

        if (!SFPIsToken ("{")) {
          Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '{' preceeding data list", NULL);
          WarningCount++;
        }

        if ((BufferData = CreateBufferData ()) == NULL) {
          ErrorCount++;
          goto Done;
        }
        //
        // Read bytes from input file until closing brace
        //
        while (!SFPIsToken ("}")) {
          if (!SFPGetNumber (&Num)) {
            SFPGetNextToken (Str, sizeof (Str));
            Error (SFPGetFileName (), SFPGetLineNumber (), 0, Str, "expected data value", Str);
            ErrorCount++;
            FreeBufferData (BufferData, TRUE);
            goto Done;
          } else {
            //
            // Only allow bytes
            //
            if (Num > 0xFF) {
              Error (SFPGetFileName (), SFPGetLineNumber (), 0, "only values 0-255 (0x00-0xFF) allowed", NULL);
              ErrorCount++;
              FreeBufferData (BufferData, TRUE);
              goto Done;
            }

            AddBufferDataByte (BufferData, (char) Num);
            SFPIsToken (",");
          }
        }
        //
        // Now get the data and save it in our image entry
        //
        ImageDefEntry->RawData = GetBufferData (BufferData, &ImageDefEntry->RawDataSize);
        FreeBufferData (BufferData, 0);
        //
        // Closing brace for RawData {}
        //
        if (!SFPIsToken ("}")) {
          Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '}' closing brace for RawData", NULL);
          ErrorCount++;
          goto Done;
        }
        //
        // Add the entry to the end of the list
        //
        if (ImageDef->LastEntry != NULL) {
          ImageDef->LastEntry->Next = ImageDefEntry;
        } else {
          ImageDef->Entries = ImageDefEntry;
        }

        ImageDef->LastEntry = ImageDefEntry;
      } else if (SFPIsToken ("}")) {
        //
        // Closing brace for FDImage {}
        //
        break;
      } else {
        SFPGetNextToken (Str, sizeof (Str));
        Error (SFPGetFileName (), SFPGetLineNumber (), 0, Str, "unrecognized token", Str);
        ErrorCount++;
        goto Done;
      }
    }
    //
    // Add this image definition to our global list
    //
    ImageDef->Next    = mImageDefinitions;
    mImageDefinitions = ImageDef;
  }
  //
  // Check for end-of-file
  //
  if (!SFPIsEOF ()) {
    SFPGetNextToken (Str, sizeof (Str));
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, Str, "expected end-of-file", Str);
    ErrorCount++;
  }

Done:
  SFPCloseFile ();
  if (ErrorCount != 0) {
    return STATUS_ERROR;
  } else if (WarningCount != 0) {
    return STATUS_WARNING;
  }

  return STATUS_SUCCESS;
}

static
FLASH_SUBREGION_DESCRIPTION *
ParseSubregionDefinition (
  unsigned int  SizeLeft
  )
/*++
  
Routine Description:

  Parse Subregion definitions from the input flash definition file. Format:

    Subregion {
      CreateHob       = TRUE,
      Name            = "FOO",
      Size            = 0xA000,
      Attributes      = "EFI_FLASH_AREA_SUBFV | EFI_FLASH_AREA_MEMMAPPED_FV",
      AreaType        = "EFI_FLASH_AREA_EFI_VARIABLES",
      NameGuid        = 12345678-1234-5678-AAAA-BBBBCCCCDDDD (or "EFI_SOME_GUID"),
      AreaTypeGuid    = 11111111-2222-3333-4444-1, (or "EFI_SOME_GUID") (optional)
      FileSystemGuid  = 11111111-2222-3333-4444-1, (or "EFI_SOME_GUID") (optional)
    }

    NOTE: The caller has already parsed the "Subregion" token, so start with the opening brace.

Arguments:
   
   SizeLeft   - in the flash definition file, a Region can be broken up into
                one or more subregions. As we parse the subregion definitions,
                the caller keeps track of how much space is left in the region
                that we're parsing subregions for. SizeLeft is that size, and
                so the size of the subregion we're now parsing better not
                exceed the size left.
  Returns:

    NULL    - unrecoverable errors detected while parsing the subregion definition

    pointer to a subregion definition created from the parsed subregion

--*/
{
  FLASH_SUBREGION_DESCRIPTION *Subregion;
  int                         ErrorCount;
  int                         WarningCount;
  unsigned int                Number;
  BOOLEAN                     PreviousComma;
  //
  // Allocate memory for the new subregion descriptor
  //
  ErrorCount    = 0;
  WarningCount  = 0;
  Subregion     = (FLASH_SUBREGION_DESCRIPTION *) _malloc (sizeof (FLASH_SUBREGION_DESCRIPTION));
  if (Subregion == NULL) {
    Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
    ErrorCount++;
    goto Done;
  }

  memset (Subregion, 0, sizeof (FLASH_SUBREGION_DESCRIPTION));
  //
  // Open brace -- warning if not there
  //
  if (!SFPIsToken ("{")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected {", NULL);
    WarningCount++;
  }
  //
  // Parse:  CreateHob = TRUE | FALSE,
  //
  if (!SFPIsKeyword ("CreateHob")) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'CreateHob'", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken ("=")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
    WarningCount++;
  }

  if (SFPIsToken ("TRUE")) {
    Subregion->CreateHob = 1;
  } else if (SFPIsToken ("FALSE")) {
    //
    // Subregion->CreateHob = 0; -- not required since we did a memset earlier
    //
  } else {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'TRUE' or 'FALSE'", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken (",")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ',' following CreateHob value", NULL);
    WarningCount++;
  }
  //
  // Parse:  Name = "Name",
  //
  if (!SFPIsKeyword ("Name")) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Name'", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken ("=")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
    WarningCount++;
  }

  if (!SFPGetQuotedString (Subregion->Name, sizeof (Subregion->Name))) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected Subregion name", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken (",")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected comma following Region name", NULL);
    WarningCount++;
  }
  //
  // Parse: Size = 0x2000,
  //
  if (!SFPIsKeyword ("Size")) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Size'", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken ("=")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
    WarningCount++;
  }

  if (!SFPGetNumber (&Subregion->Size)) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected numeric Size value", NULL);
    ErrorCount++;
    goto Done;
  }

  //
  // Check that the size does not exceed the size left passed in
  //
  if (Subregion->Size > SizeLeft) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "sum of Subregion sizes exceeds Region size", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken (",")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ',' following Size value", NULL);
  }
  //
  // Parse:  Attributes = Number | "String",
  //
  if (!SFPIsKeyword ("Attributes")) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'Attributes'", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken ("=")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
    WarningCount++;
  }

  if (SFPGetNumber (&Number)) {
    sprintf (Subregion->Attributes, "0x%X", Number);
  } else if (!SFPGetQuotedString (Subregion->Attributes, sizeof (Subregion->Attributes))) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted Attributes string", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken (",")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ','", NULL);
  }
  //
  // Parse:  AreaType = Number | "String",
  // AreaType is a UINT8, so error if it exceeds the size
  //
  if (!SFPIsKeyword ("AreaType")) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'AreaType'", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken ("=")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
    WarningCount++;
  }

  if (SFPGetNumber (&Number)) {
    if (Number > 0xFF) {
      Error (SFPGetFileName (), SFPGetLineNumber (), 0, "AreaType value exceeds 255", NULL);
      ErrorCount++;
    }

    sprintf (Subregion->AreaType, "0x%X", Number & 0x00FF);
  } else if (!SFPGetQuotedString (Subregion->AreaType, sizeof (Subregion->AreaType))) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected quoted AreaType string", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken (",")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ',' following AreaType value", NULL);
  }
  //
  // Parse the three GUIDs (last two are optional)
  //
  //    NameGuid        = 12345678-1234-5678-AAAA-BBBBCCCCDDDD, (or "EFI_SOME_GUID")
  //    AreaTypeGuid    = 11111111-2222-3333-4444-1, (or "EFI_SOME_GUID")
  //    FileSysteGuid   = 11111111-2222-3333-4444-1, (or "EFI_SOME_GUID")
  //
  if (!SFPIsKeyword ("NameGuid")) {
    Error (SFPGetFileName (), SFPGetLineNumber (), 0, "expected 'NameGuid'", NULL);
    ErrorCount++;
    goto Done;
  }

  if (!SFPIsToken ("=")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
    WarningCount++;
  }
  //
  // Allow a GUID or a quoted string identifier, which we'll just copy as a string
  //
  if (SFPGetQuotedString (Subregion->NameGuidString, sizeof (Subregion->NameGuidString))) {
    //
    // Nothing else to do
    //
  } else if (!SFPGetGuid (PARSE_GUID_STYLE_5_FIELDS, &Subregion->NameGuid)) {
    Error (
      SFPGetFileName (),
      SFPGetLineNumber (),
      0,
      "expected NameGuid quoted string or GUID of form 12345678-1234-1234-1234-123456789ABC",
      NULL
      );
    ErrorCount++;
    goto Done;
  }
  //
  // Comma following NameGuid is optional if they don't specify AreaTypeGuid or FileSystemGuid
  //
  PreviousComma = SFPIsToken (",");
  if (SFPIsKeyword ("AreaTypeGuid")) {
    //
    // Check for preceeding comma now
    //
    if (!PreviousComma) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ',' before 'AreaTypeGuid'", NULL);
      WarningCount++;
    }

    if (!SFPIsToken ("=")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
      WarningCount++;
    }

    if (SFPGetQuotedString (Subregion->AreaTypeGuidString, sizeof (Subregion->AreaTypeGuidString))) {
      //
      // Nothing else to do
      //
    } else if (!SFPGetGuid (PARSE_GUID_STYLE_5_FIELDS, &Subregion->AreaTypeGuid)) {
      Error (
        SFPGetFileName (),
        SFPGetLineNumber (),
        0,
        "expected AreaTypeGuid quoted string or GUID of form 12345678-1234-1234-1234-123456789ABC",
        NULL
        );
      ErrorCount++;
      goto Done;
    }

    PreviousComma = SFPIsToken (",");
  }

  if (SFPIsKeyword ("FileSystemGuid")) {
    //
    // Check for preceeding comma now
    //
    if (!PreviousComma) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected ',' before 'FileSystemGuid'", NULL);
      WarningCount++;
    }

    if (!SFPIsToken ("=")) {
      Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected '='", NULL);
      WarningCount++;
    }
    //
    // Allow a GUID or a quoted string identifier, which we'll just copy as a string
    //
    if (SFPGetQuotedString (Subregion->FileSystemGuidString, sizeof (Subregion->FileSystemGuidString))) {
      //
      // Nothing else to do
      //
    } else if (!SFPGetGuid (PARSE_GUID_STYLE_5_FIELDS, &Subregion->FileSystemGuid)) {
      Error (
        SFPGetFileName (),
        SFPGetLineNumber (),
        0,
        "expected FileSystemGuid quoted string or GUID of form 12345678-1234-1234-1234-123456789ABC",
        NULL
        );
      ErrorCount++;
      goto Done;
    }

    SFPIsToken (",");
  }
  //
  // Look for subregion closing brace
  //
  if (!SFPIsToken ("}")) {
    Warning (SFPGetFileName (), SFPGetLineNumber (), 0, "expected Subregion closing brace '}'", NULL);
    WarningCount++;
  }

Done:
  //
  // If any errors were encountered, then delete the subregion definition
  //
  if (ErrorCount != 0) {
    _free (Subregion);
    Subregion = NULL;
  }

  return Subregion;
}

STATUS
FDFCreateCIncludeFile (
  char      *FlashDeviceName,
  char      *FileName
  )
/*++

Routine Description:
  Create a header file with #define definitions per an already-parsed
  flash definition file.

Arguments:
  FlashDeviceName - name of flash device (from the flash definition file)
                    to use
  FileName        - name of output file to create

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_WARNING    - warnings, but no errors, were encountered
  STATUS_ERROR      - errors were encountered

--*/
{
  FILE                        *OutFptr;
  FLASH_BLOCK_DESCRIPTION     *FBlock;
  FLASH_DEVICE_DESCRIPTION    *FDev;
  FLASH_SUBREGION_DESCRIPTION *Subregion;
  unsigned int                Offset;
  unsigned int                SubregionOffset;
  int                         CreateHobs;
  //
  // Find the definition we're supposed to use
  //
  for (FDev = mFlashDevices; FDev != NULL; FDev = FDev->Next) {
    if (strcmp (FDev->Name, FlashDeviceName) == 0) {
      break;
    }
  }

  if (FDev == NULL) {
    Error (NULL, 0, 0, NULL, FlashDeviceName, "flash device not found in flash definitions");
    return STATUS_ERROR;
  }

  if ((OutFptr = fopen (FileName, "w")) == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open output file for writing");
    return STATUS_ERROR;
  }
  //
  // Write a header
  //
  fprintf (OutFptr, CIncludeHeader);
  //
  // Write flash block base and size defines
  //
  fprintf (OutFptr, "#define FLASH_BASE                                          0x%08X\n", FDev->BaseAddress);
  fprintf (OutFptr, "#define FLASH_SIZE                                          0x%08X\n\n", FDev->Size);
  //
  // Write flash regions base, size and offset defines
  //
  Offset      = 0;
  CreateHobs  = 0;
  for (FBlock = FDev->Regions; FBlock != NULL; FBlock = FBlock->Next) {
    fprintf (
      OutFptr,
      "#define FLASH_REGION_%s_BASE              %*c0x%08X\n",
      FBlock->Name,
      COLUMN2_START - 40 - strlen (FBlock->Name),
      ' ',
      Offset + FDev->BaseAddress
      );
    fprintf (
      OutFptr,
      "#define FLASH_REGION_%s_SIZE              %*c0x%08X\n",
      FBlock->Name,
      COLUMN2_START - 40 - strlen (FBlock->Name),
      ' ',
      FBlock->Size
      );
    fprintf (
      OutFptr,
      "#define FLASH_REGION_%s_OFFSET            %*c0x%08X\n",
      FBlock->Name,
      COLUMN2_START - 40 - strlen (FBlock->Name),
      ' ',
      Offset
      );
    //
    // Create defines for any subregions
    //
    SubregionOffset = 0;
    for (Subregion = FBlock->Subregions; Subregion != NULL; Subregion = Subregion->Next) {
      fprintf (
        OutFptr,
        "#define FLASH_REGION_%s_SUBREGION_%s_BASE     %*c0x%08X\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 43 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        FDev->BaseAddress + Offset + SubregionOffset
        );
      fprintf (
        OutFptr,
        "#define FLASH_REGION_%s_SUBREGION_%s_SIZE     %*c0x%08X\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 43 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        Subregion->Size
        );
      fprintf (
        OutFptr,
        "#define FLASH_REGION_%s_SUBREGION_%s_OFFSET   %*c0x%08X\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 43 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        Offset + SubregionOffset
        );
      SubregionOffset += Subregion->Size;
      if (Subregion->CreateHob != 0) {
        CreateHobs = 1;
      }
    }

    Offset += FBlock->Size;
  }
  //
  // Now create a #define for the flash map data definition
  //
  fprintf (OutFptr, "\n\n#define EFI_FLASH_AREA_DATA_DEFINITION \\\n");
  //
  // Emit entry for each region
  //
  Offset = 0;
  for (FBlock = FDev->Regions; FBlock != NULL; FBlock = FBlock->Next) {
    fprintf (OutFptr, "  /* %s region */\\\n", FBlock->Name);
    fprintf (OutFptr, "  {\\\n");
    fprintf (OutFptr, "    FLASH_REGION_%s_BASE,\\\n", FBlock->Name);
    fprintf (OutFptr, "    FLASH_REGION_%s_SIZE,\\\n", FBlock->Name);
    fprintf (OutFptr, "    %s,\\\n", FBlock->Attributes);
    fprintf (OutFptr, "    %s,\\\n", FBlock->AreaType);
    fprintf (OutFptr, "    0, 0, 0,\\\n");
    //
    // The AreaTypeGuid may have been specified in the input flash definition file as a GUID, or
    // as a quoted string. Do the right one.
    //
    if (FBlock->AreaTypeGuidString[0] != 0) {
      fprintf (OutFptr, "    %s, \\\n", FBlock->AreaTypeGuidString);
    } else {
      fprintf (
        OutFptr,
        "    { 0x%08X, 0x%04X, 0x%04X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X },\\\n",
        FBlock->AreaTypeGuid.Data1,
        (unsigned int) FBlock->AreaTypeGuid.Data2,
        (unsigned int) FBlock->AreaTypeGuid.Data3,
        (unsigned int) FBlock->AreaTypeGuid.Data4[0],
        (unsigned int) FBlock->AreaTypeGuid.Data4[1],
        (unsigned int) FBlock->AreaTypeGuid.Data4[2],
        (unsigned int) FBlock->AreaTypeGuid.Data4[3],
        (unsigned int) FBlock->AreaTypeGuid.Data4[4],
        (unsigned int) FBlock->AreaTypeGuid.Data4[5],
        (unsigned int) FBlock->AreaTypeGuid.Data4[6],
        (unsigned int) FBlock->AreaTypeGuid.Data4[7]
        );
    }
    fprintf (OutFptr, "  },\\\n");
  }

  fprintf (OutFptr, "\n\n");
  //
  // Now walk the list again to create the EFI_HOB_FLASH_MAP_ENTRY_TYPE definition
  //
  if (CreateHobs != 0) {
    fprintf (OutFptr, "//\n// EFI_HOB_FLASH_MAP_ENTRY_TYPE definition\n//\n");
    fprintf (OutFptr, "#define EFI_HOB_FLASH_MAP_ENTRY_TYPE_DATA_DEFINITION");
    for (FBlock = FDev->Regions; FBlock != NULL; FBlock = FBlock->Next) {
      //
      // See if the block has subregions, and that the CreateHobs flag is set
      // for any of them.
      //
      CreateHobs = 0;
      for (Subregion = FBlock->Subregions; Subregion != NULL; Subregion = Subregion->Next) {
        if (Subregion->CreateHob != 0) {
          CreateHobs = 1;
          break;
        }
      }
      //
      // If any of the subregions had the CreateHobs flag set, then create the entries in the
      // output file
      //
      if (CreateHobs != 0) {
        for (Subregion = FBlock->Subregions; Subregion != NULL; Subregion = Subregion->Next) {
          if (Subregion->CreateHob != 0) {
            fprintf (OutFptr, " \\\n");
            fprintf (OutFptr, "  /* %s.%s Subregion */\\\n", FBlock->Name, Subregion->Name);
            fprintf (OutFptr, "  {\\\n");
            fprintf (OutFptr, "    {EFI_HOB_TYPE_GUID_EXTENSION,\\\n");
            fprintf (OutFptr, "    sizeof (EFI_HOB_FLASH_MAP_ENTRY_TYPE ),\\\n");
            fprintf (OutFptr, "    0},\\\n");
            //
            // The NameGuid may have been specified in the input flash definition file as a GUID, or
            // as a quoted string. Do the right one.
            //
            if (Subregion->NameGuidString[0] != 0) {
              fprintf (OutFptr, "    %s, \\\n", Subregion->NameGuidString);
            } else {
              fprintf (
                OutFptr,
                "    { 0x%08X, 0x%04X, 0x%04X, {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}},\\\n",
                Subregion->NameGuid.Data1,
                (unsigned int) Subregion->NameGuid.Data2,
                (unsigned int) Subregion->NameGuid.Data3,
                (unsigned int) Subregion->NameGuid.Data4[0],
                (unsigned int) Subregion->NameGuid.Data4[1],
                (unsigned int) Subregion->NameGuid.Data4[2],
                (unsigned int) Subregion->NameGuid.Data4[3],
                (unsigned int) Subregion->NameGuid.Data4[4],
                (unsigned int) Subregion->NameGuid.Data4[5],
                (unsigned int) Subregion->NameGuid.Data4[6],
                (unsigned int) Subregion->NameGuid.Data4[7]
                );
            }

            fprintf (OutFptr, "    {0, 0, 0},\\\n");
            fprintf (OutFptr, "    %s,\\\n", Subregion->AreaType);
            //
            // The AreaTypeGuid may have been specified in the input flash definition file as a GUID, or
            // as a quoted string. Do the right one.
            //
            if (Subregion->AreaTypeGuidString[0] != 0) {
              fprintf (OutFptr, "    %s, \\\n", Subregion->AreaTypeGuidString);
            } else {
              fprintf (
                OutFptr,
                "    { 0x%08X, 0x%04X, 0x%04X, {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}},\\\n",
                Subregion->AreaTypeGuid.Data1,
                (unsigned int) Subregion->AreaTypeGuid.Data2,
                (unsigned int) Subregion->AreaTypeGuid.Data3,
                (unsigned int) Subregion->AreaTypeGuid.Data4[0],
                (unsigned int) Subregion->AreaTypeGuid.Data4[1],
                (unsigned int) Subregion->AreaTypeGuid.Data4[2],
                (unsigned int) Subregion->AreaTypeGuid.Data4[3],
                (unsigned int) Subregion->AreaTypeGuid.Data4[4],
                (unsigned int) Subregion->AreaTypeGuid.Data4[5],
                (unsigned int) Subregion->AreaTypeGuid.Data4[6],
                (unsigned int) Subregion->AreaTypeGuid.Data4[7]
                );
            }

            fprintf (OutFptr, "    1,\\\n");
            fprintf (OutFptr, "    {{\\\n");
            fprintf (OutFptr, "      %s,\\\n", Subregion->Attributes);
            fprintf (OutFptr, "      0,\\\n");
            fprintf (OutFptr, "      FLASH_REGION_%s_SUBREGION_%s_BASE,\\\n", FBlock->Name, Subregion->Name);
            fprintf (OutFptr, "      FLASH_REGION_%s_SUBREGION_%s_SIZE,\\\n", FBlock->Name, Subregion->Name);
            //
            // The FileSystemGuid may have been specified in the input flash definition file as a GUID, or
            // as a quoted string. Do the right one.
            //
            if (Subregion->FileSystemGuidString[0] != 0) {
              fprintf (OutFptr, "      %s, \\\n", Subregion->FileSystemGuidString);
            } else {
              fprintf (
                OutFptr,
                "      { 0x%08X, 0x%04X, 0x%04X, {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}},\\\n",
                Subregion->FileSystemGuid.Data1,
                (unsigned int) Subregion->FileSystemGuid.Data2,
                (unsigned int) Subregion->FileSystemGuid.Data3,
                (unsigned int) Subregion->FileSystemGuid.Data4[0],
                (unsigned int) Subregion->FileSystemGuid.Data4[1],
                (unsigned int) Subregion->FileSystemGuid.Data4[2],
                (unsigned int) Subregion->FileSystemGuid.Data4[3],
                (unsigned int) Subregion->FileSystemGuid.Data4[4],
                (unsigned int) Subregion->FileSystemGuid.Data4[5],
                (unsigned int) Subregion->FileSystemGuid.Data4[6],
                (unsigned int) Subregion->FileSystemGuid.Data4[7]
                );
            }

            fprintf (OutFptr, "    }},\\\n");
            fprintf (OutFptr, "  },");
          }
        }
      }
    }

    fprintf (OutFptr, "\n\n");
  }

  //
  // Write the file's closing #endif
  //
  fprintf (OutFptr, CIncludeFooter);
  fclose (OutFptr);
  return STATUS_SUCCESS;
}

STATUS
FDFCreateAsmIncludeFile (
  char      *FlashDeviceName,
  char      *FileName
  )
/*++

Routine Description:
  Create an assembly header file with equate definitions per an already-parsed
  flash definition file.

Arguments:
  FlashDeviceName - name of flash device (from the flash definition file)
                    to use
  FileName        - name of output file to create

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_WARNING    - warnings, but no errors, were encountered
  STATUS_ERROR      - errors were encountered

--*/
{
  FILE                        *OutFptr;
  FLASH_BLOCK_DESCRIPTION     *FBlock;
  FLASH_DEVICE_DESCRIPTION    *FDev;
  unsigned int                Offset;
  FLASH_SUBREGION_DESCRIPTION *Subregion;
  unsigned int                SubregionOffset;
  //
  // Find the definition we're supposed to use
  //
  for (FDev = mFlashDevices; FDev != NULL; FDev = FDev->Next) {
    if (strcmp (FDev->Name, FlashDeviceName) == 0) {
      break;
    }
  }

  if (FDev == NULL) {
    Error (NULL, 0, 0, NULL, FlashDeviceName, "flash device not found in flash definitions");
    return STATUS_ERROR;
  }

  if ((OutFptr = fopen (FileName, "w")) == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open output file for writing");
    return STATUS_ERROR;
  }
  //
  // Write a header
  //
  fprintf (OutFptr, "\n\n");
  //
  // Write flash block size and offset defines
  //
  fprintf (
    OutFptr,
    "FLASH_BASE                               %*cequ 0%08Xh\n",
    COLUMN2_START - 40,
    ' ',
    FDev->BaseAddress
    );
  fprintf (OutFptr, "FLASH_SIZE                               %*cequ 0%08Xh\n", COLUMN2_START - 40, ' ', FDev->Size);
  //
  // Write flash region size and offset defines
  //
  fprintf (OutFptr, "\n");
  Offset = 0;
  for (FBlock = FDev->Regions; FBlock != NULL; FBlock = FBlock->Next) {
    fprintf (
      OutFptr,
      "FLASH_REGION_%s_BASE   %*cequ 0%08Xh\n",
      FBlock->Name,
      COLUMN2_START - 20 - strlen (FBlock->Name),
      ' ',
      FDev->BaseAddress + Offset
      );
    fprintf (
      OutFptr,
      "FLASH_REGION_%s_SIZE   %*cequ 0%08Xh\n",
      FBlock->Name,
      COLUMN2_START - 20 - strlen (FBlock->Name),
      ' ',
      FBlock->Size
      );
    fprintf (
      OutFptr,
      "FLASH_REGION_%s_OFFSET %*cequ 0%08Xh\n",
      FBlock->Name,
      COLUMN2_START - 20 - strlen (FBlock->Name),
      ' ',
      Offset
      );
    //
    // Create defines for any subregions
    //
    SubregionOffset = 0;
    for (Subregion = FBlock->Subregions; Subregion != NULL; Subregion = Subregion->Next) {
      fprintf (
        OutFptr,
        "FLASH_REGION_%s_SUBREGION_%s_BASE     %*cequ 0%08Xh\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 39 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        FDev->BaseAddress + Offset + SubregionOffset
        );
      fprintf (
        OutFptr,
        "FLASH_REGION_%s_SUBREGION_%s_SIZE     %*cequ 0%08Xh\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 39 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        Subregion->Size
        );
      fprintf (
        OutFptr,
        "FLASH_REGION_%s_SUBREGION_%s_OFFSET   %*cequ 0%08Xh\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 39 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        Offset + SubregionOffset
        );
      SubregionOffset += Subregion->Size;
    }

    Offset += FBlock->Size;
  }

  //
  // Write closing \n
  //
  fprintf (OutFptr, "\n\n");
  fclose (OutFptr);
  return STATUS_SUCCESS;
}

STATUS
FDFCreateSymbols (
  char      *FlashDeviceName
  )
/*++

Routine Description:
  Using the given flash device name, add symbols to the global symbol table. This
  allows other functions to use the symbol definitions for other purposes.

Arguments:
  FlashDeviceName - name of flash device (from the flash definition file)
                    to use

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_WARNING    - warnings, but no errors, were encountered
  STATUS_ERROR      - errors were encountered

--*/
{
  FLASH_BLOCK_DESCRIPTION     *FBlock;
  FLASH_DEVICE_DESCRIPTION    *FDev;
  unsigned int                Offset;
  char                        SymName[120];
  char                        SymValue[120];
  FLASH_SUBREGION_DESCRIPTION *Subregion;
  unsigned int                SubregionOffset;
  //
  // Find the definition we're supposed to use
  //
  for (FDev = mFlashDevices; FDev != NULL; FDev = FDev->Next) {
    if (strcmp (FDev->Name, FlashDeviceName) == 0) {
      break;
    }
  }

  if (FDev == NULL) {
    Error (NULL, 0, 0, NULL, FlashDeviceName, "flash device not found in flash definitions");
    return STATUS_ERROR;
  }

  sprintf (SymValue, "0x%08X", FDev->BaseAddress);
  SymbolAdd ("FLASH_BASE", SymValue, 0);
  sprintf (SymValue, "0x%08X", FDev->Size);
  SymbolAdd ("FLASH_SIZE", SymValue, 0);
  //
  // Add flash block size and offset defines
  //
  // Offset = 0;
  // for (FBlock = FDev->PBlocks; FBlock != NULL; FBlock = FBlock->Next) {
  //  sprintf (SymName, "FLASH_BLOCK_%s_BASE", FBlock->Name);
  //  sprintf (SymValue, "0x%08X", FDev->BaseAddress + Offset);
  //  SymbolAdd (SymName, SymValue, 0);
  //  sprintf (SymName, "FLASH_BLOCK_%s_SIZE", FBlock->Name);
  //  sprintf (SymValue, "0x%08X", FBlock->Size);
  //  SymbolAdd (SymName, SymValue, 0);
  //  sprintf (SymName, "FLASH_BLOCK_%s_OFFSET", FBlock->Name);
  //  sprintf (SymValue, "0x%08X", Offset);
  //  SymbolAdd (SymName, SymValue, 0);
  //  Offset += FBlock->Size;
  // }
  //
  // Add flash region block base, size, and offset defines
  //
  Offset = 0;
  for (FBlock = FDev->Regions; FBlock != NULL; FBlock = FBlock->Next) {
    sprintf (SymName, "FLASH_REGION_%s_BASE", FBlock->Name);
    sprintf (SymValue, "0x%08X", FDev->BaseAddress + Offset);
    SymbolAdd (SymName, SymValue, 0);
    sprintf (SymName, "FLASH_REGION_%s_SIZE", FBlock->Name);
    sprintf (SymValue, "0x%08X", FBlock->Size);
    SymbolAdd (SymName, SymValue, 0);
    sprintf (SymName, "FLASH_REGION_%s_OFFSET", FBlock->Name);
    sprintf (SymValue, "0x%08X", Offset);
    SymbolAdd (SymName, SymValue, 0);
    //
    // Add subregion symbols
    //
    if (FBlock->Subregions != NULL) {
      SubregionOffset = 0;
      for (Subregion = FBlock->Subregions; Subregion != NULL; Subregion = Subregion->Next) {
        sprintf (SymName, "FLASH_REGION_%s_SUBREGION_%s_BASE", FBlock->Name, Subregion->Name);
        sprintf (SymValue, "0x%08X", FDev->BaseAddress + Offset + SubregionOffset);
        SymbolAdd (SymName, SymValue, 0);
        sprintf (SymName, "FLASH_REGION_%s_SUBREGION_%s_SIZE", FBlock->Name, Subregion->Name);
        sprintf (SymValue, "0x%08X", Subregion->Size);
        SymbolAdd (SymName, SymValue, 0);
        sprintf (SymName, "FLASH_REGION_%s_SUBREGION_%s_OFFSET", FBlock->Name, Subregion->Name);
        sprintf (SymValue, "0x%08X", Offset + SubregionOffset);
        SymbolAdd (SymName, SymValue, 0);
        SubregionOffset += Subregion->Size;
      }
    }

    Offset += FBlock->Size;
  }

  return STATUS_SUCCESS;
}

STATUS
FDFCreateImage (
  char      *FlashDeviceName,
  char      *ImageName,
  char      *FileName
  )
/*++

Routine Description:
  Create a flash image using the given device and image names.

Arguments:
  FlashDeviceName - name of flash device (from the flash definition file)
                    to use
  ImageName       - name of image (from the flash definition file) to create
  FileName        - name of output file to create

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_WARNING    - warnings, but no errors, were encountered
  STATUS_ERROR      - errors were encountered

--*/
{
  STATUS                      Status;
  FILE                        *OutFptr;
  FLASH_BLOCK_DESCRIPTION     *RegionDef;
  FLASH_DEVICE_DESCRIPTION    *FDev;
  IMAGE_DEFINITION            *ImageDef;
  unsigned int                Offset;
  char                        *Buffer;
  FILE                        *InFptr;
  long                        FileSize;
  IMAGE_DEFINITION_ENTRY      *IDefEntry;
  FLASH_SUBREGION_DESCRIPTION *SubregionDef;
  //
  // Find the flash definition we're supposed to use
  //
  InFptr  = NULL;
  Status  = STATUS_ERROR;
  for (FDev = mFlashDevices; FDev != NULL; FDev = FDev->Next) {
    if (strcmp (FDev->Name, FlashDeviceName) == 0) {
      break;
    }
  }

  if (FDev == NULL) {
    Error (NULL, 0, 0, FlashDeviceName, "flash device not found in flash definitions");
    return STATUS_ERROR;
  }
  //
  // Find the image name we're supposed to create
  //
  for (ImageDef = mImageDefinitions; ImageDef != NULL; ImageDef = ImageDef->Next) {
    if (strcmp (ImageDef->Name, ImageName) == 0) {
      break;
    }
  }

  if (ImageDef == NULL) {
    Error (NULL, 0, 0, ImageName, "image definition not found in image definitions");
    return STATUS_ERROR;
  }
  //
  // Open the output file
  //
  if ((OutFptr = fopen (FileName, "wb")) == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open output file for writing");
    return STATUS_ERROR;
  }
  //
  // Allocate a buffer to copy the input data to
  //
  Buffer = (char *) _malloc (FDev->Size);
  if (Buffer == NULL) {
    Error (NULL, 0, 0, (INT8 *) "failed to allocate memory", NULL);
    goto Done;
  }
  //
  // Set contents of buffer to the erased value
  //
  if (FDev->ErasePolarity) {
    memset (Buffer, 0xFF, FDev->Size);
  } else {
    memset (Buffer, 0, FDev->Size);
  }
  //
  // Set all region and subregion size-left fields to the size of the region/subregion
  //
  for (RegionDef = FDev->Regions; RegionDef != NULL; RegionDef = RegionDef->Next) {
    RegionDef->SizeLeft = RegionDef->Size;
    for (SubregionDef = RegionDef->Subregions; SubregionDef != NULL; SubregionDef = SubregionDef->Next) {
      SubregionDef->SizeLeft = SubregionDef->Size;
    }
  }
  //
  // Now go through the image list, read files into the buffer.
  //
  for (IDefEntry = ImageDef->Entries; IDefEntry != NULL; IDefEntry = IDefEntry->Next) {
    //
    // If it's a file name, open the file, get the size, find the corresponding
    // flash region it's in, and copy the data.
    //
    if (IDefEntry->IsRawData == 0) {
      if ((InFptr = fopen (IDefEntry->Name, "rb")) == NULL) {
        Error (NULL, 0, 0, IDefEntry->Name, "failed to open input file for reading");
        goto Done;
      }

      fseek (InFptr, 0, SEEK_END);
      FileSize = ftell (InFptr);
      fseek (InFptr, 0, SEEK_SET);
    } else {
      FileSize = IDefEntry->RawDataSize;
    }
    //
    // Find the region/subregion it's in, see if we have space left
    //
    Offset = 0;
    for (RegionDef = FDev->Regions; RegionDef != NULL; RegionDef = RegionDef->Next) {
      if (strcmp (RegionDef->Name, IDefEntry->RegionName) == 0) {
        break;
      }

      Offset += RegionDef->Size;
    }

    if (RegionDef == NULL) {
      Error (NULL, 0, 0, IDefEntry->RegionName, "Region name not found in FlashDevice %s definition", FDev->Name);
      goto Done;
    }

    //
    // Check for subregion
    //
    if (IDefEntry->SubregionName[0] != 0) {
      for (SubregionDef = RegionDef->Subregions; SubregionDef != NULL; SubregionDef = SubregionDef->Next) {
        if (strcmp (SubregionDef->Name, IDefEntry->SubregionName) == 0) {
          break;
        }

        Offset += SubregionDef->Size;
      }

      if (SubregionDef == NULL) {
        Error (
          NULL,
          0,
          0,
          IDefEntry->SubregionName,
          "Subregion name not found in FlashDevice %s.%s Region definition",
          FDev->Name,
          RegionDef->Name
          );
        goto Done;
      }
      //
      // Enough space in the subregion?
      //
      if (SubregionDef->SizeLeft < (unsigned int) FileSize) {
        Error (
          NULL,
          0,
          0,
          IDefEntry->Name,
          "insufficient space in Subregion (at least 0x%X additional bytes required)",
          FileSize - SubregionDef->SizeLeft
          );
        goto Done;
      }

      //
      // Read the file into the buffer if it's a file. Otherwise copy the raw data
      //
      if (IDefEntry->IsRawData == 0) {
        if (fread (Buffer + Offset + (SubregionDef->Size - SubregionDef->SizeLeft), FileSize, 1, InFptr) != 1) {
          Error (NULL, 0, 0, IDefEntry->Name, "failed to read file contents");
          goto Done;
        }

        fclose (InFptr);
        InFptr = NULL;
      } else {
        memcpy (
          Buffer + Offset + (SubregionDef->Size - SubregionDef->SizeLeft),
          IDefEntry->RawData,
          IDefEntry->RawDataSize
          );
      }

      SubregionDef->SizeLeft -= FileSize;
      //
      // Align based on the Region alignment requirements.
      //
      if (RegionDef->Alignment != 0) {
        while (((unsigned int) (SubregionDef->Size - SubregionDef->SizeLeft) &~RegionDef->Alignment) != 0) {
          if (SubregionDef->SizeLeft == 0) {
            break;
          }

          SubregionDef->SizeLeft--;
        }
      }
    } else {
      //
      // Placing data in a region. Check for enough space in the region left.
      //
      if (RegionDef->SizeLeft < (unsigned int) FileSize) {
        Error (
          NULL,
          0,
          0,
          IDefEntry->Name,
          "insufficient space in Region (at least 0x%X additional bytes required)",
          FileSize - RegionDef->SizeLeft
          );
        goto Done;
      }

      //
      // Read the file into the buffer if it's a file. Otherwise copy the raw data
      //
      if (IDefEntry->IsRawData == 0) {
        if (fread (Buffer + Offset + (RegionDef->Size - RegionDef->SizeLeft), FileSize, 1, InFptr) != 1) {
          Error (NULL, 0, 0, IDefEntry->Name, "failed to read file contents");
          goto Done;
        }

        fclose (InFptr);
        InFptr = NULL;
      } else {
        memcpy (Buffer + Offset + (RegionDef->Size - RegionDef->SizeLeft), IDefEntry->RawData, IDefEntry->RawDataSize);
      }

      RegionDef->SizeLeft -= FileSize;
      //
      // Align
      //
      if (RegionDef->Alignment != 0) {
        while (((unsigned int) (RegionDef->Size - RegionDef->SizeLeft) &~RegionDef->Alignment) != 0) {
          if (RegionDef->SizeLeft == 0) {
            break;
          }

          RegionDef->SizeLeft--;
        }
      }
    }
  }

  if (fwrite (Buffer, FDev->Size, 1, OutFptr) != 1) {
    Error (NULL, 0, 0, "failed to write buffer contents to output file", NULL);
    goto Done;
  }

  Status = STATUS_SUCCESS;
Done:
  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (Buffer != NULL) {
    _free (Buffer);
  }

  if (OutFptr != NULL) {
    fclose (OutFptr);
    if (Status == STATUS_ERROR) {
      remove (FileName);
    }
  }

  return Status;
}

STATUS
FDFCreateDscFile (
  char      *FlashDeviceName,
  char      *FileName
  )
/*++

Routine Description:
  Create a DSC-style output file with equates for flash management.

Arguments:
  FlashDeviceName - name of flash device (from the flash definition file)
                    to use
  FileName        - name of output file to create

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_WARNING    - warnings, but no errors, were encountered
  STATUS_ERROR      - errors were encountered

--*/
{
  FILE                        *OutFptr;
  FLASH_BLOCK_DESCRIPTION     *FBlock;
  FLASH_DEVICE_DESCRIPTION    *FDev;
  unsigned int                Offset;
  FLASH_SUBREGION_DESCRIPTION *Subregion;
  unsigned int                SubregionOffset;
  //
  // Find the definition we're supposed to use
  //
  for (FDev = mFlashDevices; FDev != NULL; FDev = FDev->Next) {
    if (strcmp (FDev->Name, FlashDeviceName) == 0) {
      break;
    }
  }

  if (FDev == NULL) {
    Error (NULL, 0, 0, FlashDeviceName, "flash device not found in flash definitions");
    return STATUS_ERROR;
  }

  if ((OutFptr = fopen (FileName, "w")) == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open output file for writing");
    return STATUS_ERROR;
  }
  //
  // Write the flash base address and size
  //
  fprintf (OutFptr, "\n");
  fprintf (OutFptr, "FLASH_BASE                                      = 0x%08X\n", FDev->BaseAddress);
  fprintf (OutFptr, "FLASH_SIZE                                      = 0x%08X\n\n", FDev->Size);
  //
  // Write flash block size and offset defines
  //
  Offset = 0;
  for (FBlock = FDev->Regions; FBlock != NULL; FBlock = FBlock->Next) {
    fprintf (
      OutFptr,
      "FLASH_REGION_%s_BASE          %*c= 0x%08X\n",
      FBlock->Name,
      COLUMN2_START - 40 - strlen (FBlock->Name),
      ' ',
      Offset + FDev->BaseAddress
      );
    fprintf (
      OutFptr,
      "FLASH_REGION_%s_SIZE          %*c= 0x%08X\n",
      FBlock->Name,
      COLUMN2_START - 40 - strlen (FBlock->Name),
      ' ',
      FBlock->Size
      );
    fprintf (
      OutFptr,
      "FLASH_REGION_%s_SIZE_BLOCKS   %*c= 0x%x\n",
      FBlock->Name,
      COLUMN2_START - 40 - strlen (FBlock->Name),
      ' ',
      (FBlock->Size)/(FDev->PBlocks->Size)
      );      
    //
    // Create defines for any subregions
    //
    SubregionOffset = 0;
    for (Subregion = FBlock->Subregions; Subregion != NULL; Subregion = Subregion->Next) {
      fprintf (
        OutFptr,
        "FLASH_REGION_%s_SUBREGION_%s_BASE     %*c= 0x%08X\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 48 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        FDev->BaseAddress + Offset + SubregionOffset
        );
      fprintf (
        OutFptr,
        "FLASH_REGION_%s_SUBREGION_%s_SIZE     %*c= 0x%08X\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 48 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        Subregion->Size
        );
      fprintf (
        OutFptr,
        "FLASH_REGION_%s_SUBREGION_%s_OFFSET   %*c= 0x%08X\n",
        FBlock->Name,
        Subregion->Name,
        COLUMN3_START - 48 - strlen (FBlock->Name) - strlen (Subregion->Name),
        ' ',
        Offset + SubregionOffset
        );

      SubregionOffset += Subregion->Size;
    }

    Offset += FBlock->Size;
  }
  //
  // Close file
  //
  fprintf (OutFptr, "\n");
  fclose (OutFptr);
  return STATUS_SUCCESS;
}


/*++

Routine Description:
  The following buffer management routines are used to encapsulate functionality
  for managing a growable data buffer.

Arguments:
  BUFFER_DATA     - structure that is used to maintain a data buffer

Returns:
  NA

--*/
static
BUFFER_DATA *
CreateBufferData (
  VOID
  )
/*++

Routine Description:

  Create a growable data buffer with default buffer length.

Arguments:

  None

Returns:

  NULL          - error occured during data buffer creation
  Not NULL      - the pointer to the newly created data buffer

--*/
{
  BUFFER_DATA *BD;
  BD = (BUFFER_DATA *) _malloc (sizeof (BUFFER_DATA));
  if (BD == NULL) {
    Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
    return NULL;
  }

  memset (BD, 0, sizeof (BUFFER_DATA));
  BD->BufferStart = (char *) _malloc (BUFFER_SIZE);
  if (BD->BufferStart == NULL) {
    _free (BD);
    Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
    return NULL;
  }

  BD->BufferEnd = BD->BufferStart + BUFFER_SIZE;
  BD->BufferPos = BD->BufferStart;
  return BD;
}

static
BOOLEAN
AddBufferDataByte (
  BUFFER_DATA *Buffer,
  char        Data
  )
/*++

Routine Description:

  Add a single byte to a growable data buffer, growing the buffer if required.

Arguments:

  Buffer  - pointer to the growable data buffer to add a single byte to
  Data    - value of the single byte data to be added

Returns:

  TRUE    - the single byte data was successfully added
  FALSE   - error occurred, the single byte data was not added

--*/
{
  int   Size;
  char  *NewBuffer;
  //
  // See if we have to grow the buffer
  //
  if (Buffer->BufferPos >= Buffer->BufferEnd) {
    Size      = (int) Buffer->BufferEnd - (int) Buffer->BufferStart;
    NewBuffer = (char *) _malloc (Size + BUFFER_SIZE);
    if (NewBuffer == NULL) {
      Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
      return FALSE;
    }

    memcpy (NewBuffer, Buffer->BufferStart, Size);
    _free (Buffer->BufferStart);
    Buffer->BufferStart = NewBuffer;
    Buffer->BufferPos   = Buffer->BufferStart + Size;
    Buffer->BufferEnd   = Buffer->BufferStart + Size + BUFFER_SIZE;
  }

  *Buffer->BufferPos = Data;
  Buffer->BufferPos++;
  return TRUE;
}

static
void
FreeBufferData (
  BUFFER_DATA *Buffer,
  BOOLEAN     FreeData
  )
/*++

Routine Description:

  Free memory used to manage a growable data buffer.

Arguments:

  Buffer    - pointer to the growable data buffer to be destructed
  FreeData  - TRUE, free memory containing the buffered data
              FALSE, do not free the buffered data memory

Returns:

  None

--*/
{
  if (Buffer != NULL) {
    if (FreeData && (Buffer->BufferStart != NULL)) {
      _free (Buffer->BufferStart);
    }

    _free (Buffer);
  }
}

static
char *
GetBufferData (
  BUFFER_DATA *Buffer,
  int         *BufferSize
  )
/*++

Routine Description:

  Return a pointer and size of the data in a growable data buffer.

Arguments:

  Buffer      - pointer to the growable data buffer
  BufferSize  - size of the data in the growable data buffer

Returns:

  pointer of the data in the growable data buffer

--*/
{
  *BufferSize = (int) Buffer->BufferPos - (int) Buffer->BufferStart;
  return Buffer->BufferStart;
}

STATUS
FDDiscover (
  char          *FDFileName,
  unsigned int  BaseAddr
  )
/*++

Routine Description:
  Walk a binary image and see if you find anything that looks like a
  firmware volume.

Arguments:
  FDFileName        - name of input FD image to parse
  BaseAddr          - base address of input FD image

Returns:
  STATUS_SUCCESS    - no errors or warnings
  STATUS_WARNING    - warnings, but no errors, were encountered
  STATUS_ERROR      - errors were encountered

NOTE:
  This routine is used for debug purposes only.

--*/
{
  FILE                        *InFptr;
  long                        FileSize;
  long                        Offset;
  EFI_FIRMWARE_VOLUME_HEADER  FVHeader;
  EFI_GUID
    FileSystemGuid = { 0x7A9354D9, 0x0468, 0x444a, 0x81, 0xCE, 0x0B, 0xF6, 0x17, 0xD8, 0x90, 0xDF };

  if ((InFptr = fopen (FDFileName, "rb")) == NULL) {
    Error (NULL, 0, 0, FDFileName, "failed to open file for reading");
    return STATUS_ERROR;
  }

  fseek (InFptr, 0, SEEK_END);
  FileSize = ftell (InFptr);
  fseek (InFptr, 0, SEEK_SET);
  Offset = 0;
  while (Offset < FileSize) {
    fseek (InFptr, Offset, SEEK_SET);
    //
    // Read the contents of the file, see if it's an FV header
    //
    if (fread (&FVHeader, sizeof (EFI_FIRMWARE_VOLUME_HEADER), 1, InFptr) == 1) {
      //
      // Check version and GUID
      //
      if ((FVHeader.Revision == EFI_FVH_REVISION) && (FVHeader.Signature == EFI_FVH_SIGNATURE)) {
        fprintf (stdout, "FV header at 0x%08X FVSize=0x%08X ", Offset + BaseAddr, (UINT32) FVHeader.FvLength);
        if (memcmp (&FVHeader.FileSystemGuid, &FileSystemGuid, sizeof (EFI_GUID)) == 0) {
          fprintf (stdout, "standard FFS file system\n");
        } else {
          fprintf (stdout, "non-standard FFS file system\n");
        }
      }
    }

    Offset += 16 * 1024;
  }

  fclose (InFptr);
  return STATUS_SUCCESS;
}
