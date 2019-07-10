/** @file

 The API to parse the binary.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _BINARY_PARSE_H_
#define _BINARY_PARSE_H_ 1

#include <FvLib.h>
#include "Compress.h"
#include "Decompress.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"
#include "FirmwareVolumeBufferLib.h"
#include "OsPath.h"
#include "ParseGuidedSectionTools.h"
#include "StringFuncs.h"
#include "ParseInf.h"
#include <Common/UefiBaseTypes.h>
#include <Common/UefiInternalFormRepresentation.h>
#include <Common/UefiCapsule.h>
#include <Common/PiFirmwareFile.h>
#include <Common/PiFirmwareVolume.h>
#include <Guid/PiFirmwareFileSystem.h>
#include <IndustryStandard/PeImage.h>
#include <Protocol/GuidedSectionExtraction.h>

#ifdef __GNUC__
#define OS_SEP        '/'
#define OS_SEP_STR    "/"
#else
#define OS_SEP        '\\'
#define OS_SEP_STR    "\\"
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#define TEMP_DIR_NAME                  "Temp"
#define MAX_FILENAME_LEN               200
#define MAX_MATCH_GUID_NUM             100
#define MAX_EFI_IN_FFS_NUM             100

typedef struct {
  VOID       *Fd;
  UINT32     FdSize;
  UINTN      EfiVariableAddr;
  UINTN      Length[MAX_EFI_IN_FFS_NUM];
  VOID      *FfsArray[MAX_EFI_IN_FFS_NUM];
  VOID      *StorageFfsInBfv;
  VOID      *NvStoreDatabase;
  BOOLEAN   ExistNvStoreDatabase;
} G_EFI_FD_INFO;

///
///Define the structure for th sections
///
typedef struct {
  UINTN      BufferBase;
  UINTN      UncompressedBuffer[MAX_EFI_IN_FFS_NUM];
  UINTN      Length;
  UINT8      UnCompressIndex;
} EFI_SECTION_STRUCT;

// {d0bc7cb4-6a47-495f-aa11-710746da06a2}
#define EFI_VFR_ATTRACT_GUID \
{ 0xd0bc7cb4, 0x6a47, 0x495f, { 0xaa, 0x11, 0x71, 0x7, 0x46, 0xda, 0x6, 0xa2 } }

// {8913C5E0-33F6-4d86-9BF1-43EF89FC0666}
#define EFI_UNI_STR_ATTRACT_GUID \
{ 0x8913c5e0, 0x33f6, 0x4d86, { 0x9b, 0xf1, 0x43, 0xef, 0x89, 0xfc, 0x6, 0x66 } }

// {FFF12B8D-7696-4C8B-A985-2747075B4F50}
#define EFI_SYSTEM_NVDATA_FV_GUID  \
{ 0xFFF12B8D, 0x7696, 0x4C8B, { 0xA9, 0x85, 0x27, 0x47, 0x07, 0x5B, 0x4F, 0x50 }}

/**
  Parses FFS Sections, and remove the FFS headers. Tis function olny handle one efi in this FFS.

  @param  SectionBuffer     The section base address
  @param  BufferLength      The length of FFS.
  @param  EfiBufferHeader   The structure dual pointer to the efi informations

  @retval  EFI_SUCCESS      The application exited normally.
  @retval  EFI_ABORTED       An error occurred.

**/
EFI_STATUS
ParseSection (
  IN      BOOLEAN              IsFfsOrEfi,
  IN OUT  UINT8                *SectionBuffer,
  IN      UINT32               BufferLength,
  IN OUT  EFI_SECTION_STRUCT   **EfiBufferHeader
  );

/**
  Search the VfrBin Base address.

  According the known GUID gVfrArrayAttractGuid to get the base address from FFS.

  @param Fv                    the Pointer to the FFS
  @param EfiAddr               the Pointer to the EFI in FFS
  @param Length                the length of Fv
  @param Offset                the Pointer to the Addr (Offset)
  @param NumOfMachingOffset    the number of Addr (Offset)

  @retval EFI_SUCCESS          Get the address successfully.
**/
EFI_STATUS
SearchVfrBinInFFS (
   IN  VOID      *Fv,
   IN  VOID      *EfiAddr,
   IN  UINTN     Length,
   OUT UINTN    **Offset,
   OUT UINT8     *NumOfMachingOffset
  );

/**
  Search the UniBin Base address.

  According the known GUID gUniStrArrayAttractGuid to get the base address from FFS.

  @param Fv                    the Pointer to the FFS
  @param EfiAddr               the Pointer to the EFI in FFS
  @param Length                the length of Fv
  @param Offset                the Pointer to the Addr (Offset)

  @retval Base address         Get the address successfully.
**/
EFI_STATUS
SearchUniBinInFFS (
   IN VOID      *Fv,
   IN  VOID     *EfiAddr,
   IN  UINTN    Length,
   OUT UINTN    **Offset
  );

/**
  Read the file to memory.

  @param   InputFile        The file that contains the FV image.
  @param   Size             The size of the file.

  @retval The pointer to the begining position of memory.
**/
VOID *
ReadFileToMemory (
  IN CHAR8      *FileName,
  OUT UINT32    *Size
  );

/**
  Search the EFI variables address in Fd.

  Open and read the *.fd to the memory, initialize the global structure.
  Update the EFI variables addr and the begining position of memory.

  @retval EFI_SUCCESS          Get the address successfully.
**/
EFI_STATUS
GetEfiVariablesAddr (
  BOOLEAN UqiIsSet
  );

/**
  Pick up the FFS which includes IFR section.

  Parse all FFS extracted by BfmLib, and save all which includes IFR
  Binary to gEfiFdInfo structure.

  @retval EFI_SUCCESS          Get the address successfully.
  @retval EFI_BUFFER_TOO_SMALL Memory can't be allocated.
  @retval EFI_ABORTED          Read FFS Failed.
**/
EFI_STATUS
FindFileInFolder (
  IN   CHAR8    *FolderName,
  OUT  BOOLEAN  *ExistStorageInBfv,
  OUT  BOOLEAN  *SizeOptimized
);

#endif

