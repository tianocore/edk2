/** @file

 Structures and functions declaration.

 Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef    _BIN_FILE_MANAGER_
#define    _BIN_FILE_MANAGER_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <io.h>
#include <direct.h>
#include <windows.h>
#endif
#include <FvLib.h>
#include <Common/UefiBaseTypes.h>
#include <Common/PiFirmwareVolume.h>
#include <Common/PiFirmwareFile.h>
#include <Protocol/GuidedSectionExtraction.h>

#include "CommonLib.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"
#include "ParseGuidedSectionTools.h"
#include "StringFuncs.h"
#include "Compress.h"
#include "Decompress.h"

#ifndef _MAX_PATH
#define _MAX_PATH 500
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#ifdef __GNUC__
#define OS_SEP        '/'
#define OS_SEP_STR    "/"
#define COPY_STR      "cp \"%s\" \"%s\" > /dev/null"
#define RMDIR_STR     "rm -r \"%s\" > /dev/null"
#define DEL_STR       "rm \"%s\" > /dev/null"
#else
#define OS_SEP        '\\'
#define OS_SEP_STR    "\\"
#define COPY_STR      "copy \"%s\" \"%s\" > NUL"
#define RMDIR_STR     "rmdir /S /Q \"%s\" > NUL"
#define DEL_STR       "del \"%s\" > NUL"
#endif

#define UTILITY_NAME               "Firmware Module Management Tool(FMMT)"
#define UTILITY_SHORT_NAME         "FMMT"
#define UTILITY_MAJOR_VERSION      0
#define UTILITY_MINOR_VERSION      23
#define MAX_BASENAME_LEN           60  // not good to HardCode, but let's be reasonable
#define EFI_SECTION_ERROR EFIERR   (100)
//
// The maximum number of Pad file guid entries.
//
#define MAX_NUMBER_OF_PAD_FILE_GUIDS    1024

//
// The maximum number of block map entries supported by the library
//
#define MAX_NUMBER_OF_FV_BLOCKS         100


//
// The maximum number of sections in an FFS file.
//
#define MAX_NUMBER_OF_SECTION_IN_FFS    100

//
// The maximum number of files in the FV supported by the library
//
#define MAX_NUMBER_OF_FILES_IN_FV       1000
#define MAX_NUMBER_OF_FILES_IN_CAP      1000



///
/// If present, this must be the first and only opcode,
/// EFI_DEP_BEFORE is only used by DXE driver.
///
#define EFI_DEP_BEFORE        0x00

///
/// If present, this must be the first and only opcode,
/// EFI_DEP_AFTER is only used by DXE driver.
///
#define EFI_DEP_AFTER         0x01

#define EFI_DEP_PUSH          0x02
#define EFI_DEP_AND           0x03
#define EFI_DEP_OR            0x04
#define EFI_DEP_NOT           0x05
#define EFI_DEP_TRUE          0x06
#define EFI_DEP_FALSE         0x07
#define EFI_DEP_END           0x08


///
/// If present, this must be the first opcode,
/// EFI_DEP_SOR is only used by DXE driver.
///
#define EFI_DEP_SOR           0x09

//
// INF file strings
//
#define OPTIONS_SECTION_STRING                "[options]"
#define ATTRIBUTES_SECTION_STRING             "[attributes]"
#define FILES_SECTION_STRING                  "[files]"
#define FV_BASE_ADDRESS_STRING                "[FV_BASE_ADDRESS]"

//
// Options section
//
#define EFI_FV_BASE_ADDRESS_STRING        "EFI_BASE_ADDRESS"
#define EFI_FV_FILE_NAME_STRING           "EFI_FILE_NAME"
#define EFI_NUM_BLOCKS_STRING             "EFI_NUM_BLOCKS"
#define EFI_BLOCK_SIZE_STRING             "EFI_BLOCK_SIZE"
#define EFI_GUID_STRING                   "EFI_GUID"
#define EFI_FV_FILESYSTEMGUID_STRING      "EFI_FV_GUID"
#define EFI_FV_NAMEGUID_STRING            "EFI_FVNAME_GUID"
#define EFI_CAPSULE_GUID_STRING           "EFI_CAPSULE_GUID"
#define EFI_CAPSULE_HEADER_SIZE_STRING    "EFI_CAPSULE_HEADER_SIZE"
#define EFI_CAPSULE_FLAGS_STRING          "EFI_CAPSULE_FLAGS"
#define EFI_CAPSULE_VERSION_STRING        "EFI_CAPSULE_VERSION"

#define EFI_FV_TOTAL_SIZE_STRING    "EFI_FV_TOTAL_SIZE"
#define EFI_FV_TAKEN_SIZE_STRING    "EFI_FV_TAKEN_SIZE"
#define EFI_FV_SPACE_SIZE_STRING    "EFI_FV_SPACE_SIZE"


typedef UINT32 FMMT_ENCAP_TYPE;

#define MAX_LEVEL_IN_FV_FILE  32

//
// Types of FMMT_ENCAP_TREENODE_TYPE
//
#define FMMT_ENCAP_TREE_FV                    0x1
#define FMMT_ENCAP_TREE_FFS                   0x2
#define FMMT_ENCAP_TREE_GUIDED_SECTION        0x3
#define FMMT_ENCAP_TREE_COMPRESS_SECTION      0x4
#define FMMT_ENCAP_TREE_FV_SECTION            0x5

extern EFI_HANDLE mParsedGuidedSectionTools;


#define TEMP_DIR_NAME  "FmmtTemp"

//
// Structure to keep a list of GUID-To-BaseNames
//
typedef struct _GUID_TO_BASENAME {
  struct _GUID_TO_BASENAME  *Next;
  INT8                      Guid[PRINTED_GUID_BUFFER_SIZE];
  INT8                      BaseName[MAX_BASENAME_LEN];
} GUID_TO_BASENAME;


typedef struct _GUID_SEC_TOOL_ENTRY {
  EFI_GUID   Guid;
  CHAR8*     Name;
  CHAR8*     Path;
  struct _GUID_SEC_TOOL_ENTRY *Next;
} GUID_SEC_TOOL_ENTRY;


//
// Private data types
//
//
// Component information
//
typedef struct {
  UINTN Size;
  CHAR8 ComponentName[_MAX_PATH];
} COMPONENT_INFO;

typedef struct {
  CHAR8            FfsName[_MAX_PATH];

  //
  // UI Name for this FFS file, if has.
  //
  CHAR16           UiName[_MAX_PATH];
   UINT32           UiNameSize;
  //
  // Total section number in this FFS.
  //
  UINT32           TotalSectionNum;

  //
  // Describe the position of the FFS file.
  //
  UINT8            Level;
  //
  // If this FFS has no encapsulate section, this flag will set to True.
  //
  BOOLEAN          IsLeaf;
  //
  // Section type for each section in FFS.
  //
  EFI_SECTION_TYPE SectionType[MAX_NUMBER_OF_SECTION_IN_FFS];
  //
  // Offset relative to current FV
  //
  UINT32                  Offset;
  UINT8                   FvLevel;
  EFI_GUID                GuidName;
  UINT8                   *Depex;
  UINT32                  DepexLen;
  BOOLEAN                 IsHandle;
  BOOLEAN                 IsFvStart;
  BOOLEAN                 IsFvEnd;
}FFS_ATTRIBUTES;


typedef struct __ENCAP_INFO_DATA{
  //
  // Now Level
  //
  UINT8                      Level;

  //
  // Encapsulate type.
  //
  FMMT_ENCAP_TYPE             Type;

  //
  // Data, if it's FV, should be FV header.
  //
  VOID                       *Data;

  //
  //FvId, match FvId with FvGuidName.
  //
  UINT8                       FvId;

  //
  // if FV ExtHeaderOffset not to zero, should also have FvExtHeader information
  //
  EFI_FIRMWARE_VOLUME_EXT_HEADER *FvExtHeader;

  CHAR16           UiName[_MAX_PATH];
  UINT32           UiNameSize;
  UINT8            *Depex;
  UINT32           DepexLen;

  //
  // Next node.
  //
  struct __ENCAP_INFO_DATA   *NextNode;

  //
  // Right node.
  //
  struct __ENCAP_INFO_DATA   *RightNode;
} ENCAP_INFO_DATA;

typedef struct _FFS_INFOMATION{
  CHAR8                      *FFSName;
  UINT32                     InFvId;
  UINT8                      ParentLevel;
  BOOLEAN                    IsFFS;
  CHAR16                     UiName[_MAX_PATH];
  UINT32                     UiNameSize;
  UINT8                      *Depex;
  UINT32                     DepexLen;
  BOOLEAN                    FfsFoundFlag;
  struct _FFS_INFOMATION     *Next;
} FFS_INFORMATION;

//
// FV and capsule information holder
//
typedef struct _FV_INFOMATION{
  EFI_FIRMWARE_VOLUME_HEADER *FvHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER *FvExtHeader;
  UINT32                     ImageAddress;
  UINT32                     FfsNumbers;
  CHAR8                      FvName[_MAX_PATH];
  EFI_FV_BLOCK_MAP_ENTRY     FvBlocks[MAX_NUMBER_OF_FV_BLOCKS];
  FFS_ATTRIBUTES             FfsAttuibutes[MAX_NUMBER_OF_FILES_IN_FV];
  EFI_FFS_FILE_HEADER2       FfsHeader[MAX_NUMBER_OF_FILES_IN_FV];
  struct _FV_INFOMATION      *FvNext;
  ENCAP_INFO_DATA            *EncapData;
  UINT8                      FvLevel;
  CHAR8                      *FvUiName;
  UINT8                      MulFvLevel;
  CHAR8                      AlignmentStr[16];
  FFS_INFORMATION            *ChildFvFFS;
} FV_INFORMATION;

typedef struct _FIRMWARE_DEVICE {
  ///
  /// Size of FD file
  ///
  UINT32                   Size;
  FV_INFORMATION           *Fv;
} FIRMWARE_DEVICE;

typedef struct _FILENode {
  CHAR8              *FileName;
  UINT8              SubLevel;
  struct _FILENode   *Next;
} FILENode;

typedef struct {
  CHAR8              *FvId;
  FILENode           *NewFile;
  FILENode           *OldFile;
  FIRMWARE_DEVICE    *FdData;
  UINT8              FvLevel;
  FV_INFORMATION     *FvInFd;
} Data;

EFI_STATUS
LibFindFvInFd (
  IN     FILE             *InputFile,
  IN OUT FIRMWARE_DEVICE  **FdData
);

/**

  TODO: Add function description

  @param[in]    Fv            - Firmware Volume to get information from

  @return       EFI_STATUS

**/
EFI_STATUS
LibGetFvInfo (
  IN     VOID                         *Fv,
  IN OUT FV_INFORMATION               *CurrentFv,
  IN     CHAR8                        *FvName,
  IN     UINT8                        Level,
  IN     ENCAP_INFO_DATA              **CurrentFvEncapData,
  IN     UINT32                       *FfsCount,
  IN OUT UINT8                        *FvCount,
  IN     BOOLEAN                      ViewFlag,
  IN     BOOLEAN                      IsChildFv
  );

/*
  Get size info from FV file.

  @param[in]
  @param[out]

  @retval

*/
EFI_STATUS
LibGetFvSize (
  IN   FILE                       *InputFile,
  OUT  UINT32                     *FvSize
  );

 /**

  This function returns the next larger size that meets the alignment
  requirement specified.

  @param[in]      ActualSize      The size.
  @param[in]      Alignment       The desired alignment.

  @retval         EFI_SUCCESS     Function completed successfully.
  @retval         EFI_ABORTED     The function encountered an error.

**/
UINT32
GetOccupiedSize (
  IN UINT32  ActualSize,
  IN UINT32  Alignment
  );

/**
  Converts ASCII characters to Unicode.
  Assumes that the Unicode characters are only these defined in the ASCII set.

  String      - Pointer to string that is written to FILE.
  UniString   - Pointer to unicode string

  The address to the ASCII string - same as AsciiStr.

**/
VOID
LibAscii2Unicode (
  IN   CHAR8          *String,
  OUT  CHAR16         *UniString
  );

/**
  Delete a directory and files in it.

  @param[in]   DirName   Name of the directory need to be deleted.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
LibRmDir (
  IN  CHAR8*  DirName
  );

/**
  Delete a file.

  @param[in]   FileName   Name of the file need to be deleted.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
LibFmmtDeleteFile(
  IN   CHAR8    *FileName
);


/**

  Free the whole Fd data structure.

  @param[in]  Fd  The pointer point to the Fd data structure.

**/
VOID
LibFmmtFreeFd (
  FIRMWARE_DEVICE *Fd
);


EFI_STATUS
LibEncapNewFvFile(
  IN  FV_INFORMATION   *FvInFd,
  IN  CHAR8            *TemDir,
  IN  ENCAP_INFO_DATA  *CurrentEncapData,
  IN  UINT32           Level_Break,
  OUT FFS_INFORMATION  **OutputFile
);


EFI_STATUS
LibLocateFvViaFvId (
  IN     FIRMWARE_DEVICE     *FdData,
  IN     CHAR8               *FvId,
  IN OUT FV_INFORMATION      **FvInFd
);

EFI_HANDLE
LibPreDefinedGuidedTools (
  VOID
);

EFI_STATUS
FvBufGetSize(
IN  VOID   *Fv,
OUT UINTN  *Size
);

EFI_STATUS
FvBufFindNextFile(
IN     VOID      *Fv,
IN OUT UINTN     *Key,
OUT    VOID      **File
);
#endif
