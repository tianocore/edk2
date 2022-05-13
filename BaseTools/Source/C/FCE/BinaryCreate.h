/** @file

 The API to create the binary.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BINARY_CREATE_H_
#define _BINARY_CREATE_H_ 1

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

//1AE42876-008F-4161-B2B7-1C0D15C5EF43
#define EFI_FFS_BFV_FOR_MULTIPLATFORM_GUID \
  { 0x1ae42876, 0x008f, 0x4161, { 0xb2, 0xb7, 0x1c, 0xd, 0x15, 0xc5, 0xef, 0x43 }}

extern EFI_GUID gEfiFfsBfvForMultiPlatformGuid;

// {003E7B41-98A2-4BE2-B27A-6C30C7655225}
#define EFI_FFS_BFV_FOR_MULTIPLATFORM_GUID2 \
  { 0x3e7b41, 0x98a2, 0x4be2, { 0xb2, 0x7a, 0x6c, 0x30, 0xc7, 0x65, 0x52, 0x25 }}

extern EFI_GUID gEfiFfsBfvForMultiPlatformGuid2;

typedef UINT64 SKU_ID;

typedef struct {
  UINT32 Offset:24;
  UINT32 Value:8;
} PCD_DATA_DELTA;

typedef struct {
  SKU_ID SkuId;
  UINT16 DefaultId;
  UINT8  Reserved[6];
} PCD_DEFAULT_INFO;

typedef struct {
  //
  // Full size, it must be at 8 byte alignment.
  //
  UINT32 DataSize;
  //
  // HeaderSize includes HeaderSize fields and DefaultInfo arrays
  //
  UINT32 HeaderSize;
  //
  // DefaultInfo arrays those have the same default setting.
  //
  PCD_DEFAULT_INFO DefaultInfo[1];
  //
  // Default data is stored as variable storage or the array of DATA_DELTA.
  //
} PCD_DEFAULT_DATA;

#define PCD_NV_STORE_DEFAULT_BUFFER_SIGNATURE SIGNATURE_32('N', 'S', 'D', 'B')

typedef struct {
  //
  // PCD_NV_STORE_DEFAULT_BUFFER_SIGNATURE
  //
  UINT32    Signature;
  //
  // Length of the taken default buffer
  //
  UINT32    Length;
  //
  // Length of the total reserved buffer
  //
  UINT32    MaxLength;
  //
  // Reserved for 8 byte alignment
  //
  UINT32    Reserved;
  // one or more PCD_DEFAULT_DATA
} PCD_NV_STORE_DEFAULT_BUFFER_HEADER;

//
// NvStoreDefaultValueBuffer layout:
// +-------------------------------------+
// | PCD_NV_STORE_DEFAULT_BUFFER_HEADER  |
// +-------------------------------------+
// | PCD_DEFAULT_DATA (DEFAULT, Standard)|
// +-------------------------------------+
// | PCD_DATA_DELTA   (DEFAULT, Standard)|
// +-------------------------------------+
// | ......                              |
// +-------------------------------------+
// | PCD_DEFAULT_DATA (SKU A, Standard)  |
// +-------------------------------------+
// | PCD_DATA_DELTA   (SKU A, Standard)  |
// +-------------------------------------+
// | ......                              |
// +-------------------------------------+
//

#pragma pack(1)

typedef struct {
  UINT16 Offset;
  UINT8  Value;
} DATA_DELTA;

#pragma pack()

/**
  Create the Ras section in FFS

  @param[in]   InputFilePath   The input file path and name.
  @param[in]   OutputFilePath  The output file path and name.

  @retval EFI_SUCCESS

**/
EFI_STATUS
CreateRawSection (
  IN CHAR8*     InputFilePath,
  IN CHAR8*     OutputFilePath
  );

/**
  Create the Ras type of FFS

  @param[in]   InputFilePath   .efi file, it's optional unless process PE/TE section.
  @param[in]   OutputFilePath  .te or .pe file

  @retval EFI_SUCCESS

**/
EFI_STATUS
CreateRawFfs (
  IN CHAR8**    InputFilePaths,
  IN CHAR8*     OutputFilePath,
  IN BOOLEAN    SizeOptimized
  );

#endif

