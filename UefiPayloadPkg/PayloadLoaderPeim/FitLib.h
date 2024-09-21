/** @file
  FIT Load Image Support
Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FIT_LIB_H_
#define FIT_LIB_H_

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>

typedef struct {
  UINT64    Offset;
  UINT64    RelocateType;
} FIT_RELOCATE_ITEM;

typedef struct {
  EFI_PHYSICAL_ADDRESS    ImageBase;
  EFI_PHYSICAL_ADDRESS    PayloadBaseAddress;
  UINT64                  PayloadSize;
  UINTN                   PayloadEntryOffset;
  UINTN                   PayloadEntrySize;
  EFI_PHYSICAL_ADDRESS    PayloadEntryPoint;
  UINTN                   RelocateTableOffset;
  UINTN                   RelocateTableCount;
  EFI_PHYSICAL_ADDRESS    PayloadLoadAddress;
} FIT_IMAGE_CONTEXT;

typedef struct {
  CHAR8     *Name;
  UINT32    Offset;
} PROPERTY_DATA;

#define IMAGE_BASE_OFFSET             OFFSET_OF (FIT_IMAGE_CONTEXT, ImageBase)
#define PAYLOAD_BASE_ADDR_OFFSET      OFFSET_OF (FIT_IMAGE_CONTEXT, PayloadBaseAddress)
#define PAYLOAD_BASE_SIZE_OFFSET      OFFSET_OF (FIT_IMAGE_CONTEXT, PayloadSize)
#define PAYLOAD_ENTRY_OFFSET_OFFSET   OFFSET_OF (FIT_IMAGE_CONTEXT, PayloadEntryOffset)
#define PAYLOAD_ENTRY_SIZE_OFFSET     OFFSET_OF (FIT_IMAGE_CONTEXT, PayloadEntrySize)
#define PAYLOAD_ENTRY_POINT_OFFSET    OFFSET_OF (FIT_IMAGE_CONTEXT, PayloadEntryPoint)
#define RELOCATE_TABLE_OFFSET_OFFSET  OFFSET_OF (FIT_IMAGE_CONTEXT, RelocateTableOffset)
#define RELOCATE_TABLE_COUNT_OFFSET   OFFSET_OF (FIT_IMAGE_CONTEXT, RelocateTableCount)
#define PAYLOAD_LOAD_ADDR_OFFSET      OFFSET_OF (FIT_IMAGE_CONTEXT, PayloadLoadAddress)

/**
  Parse the FIT image info.
  @param[in]  ImageBase      Memory address of an image.
  @param[out] Context        The FIT image context pointer.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_SUCCESS             FIT binary is loaded successfully.
**/
EFI_STATUS
EFIAPI
ParseFitImage (
  IN   VOID               *ImageBase,
  OUT  FIT_IMAGE_CONTEXT  *Context
  );

#endif
