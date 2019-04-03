/** @file

  The common header file for SMM FVB module and SMM FVB runtime Module.

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved. <BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

**/

#ifndef _SMM_FVB_COMMON_H_
#define _SMM_FVB_COMMON_H_

#include <Protocol/SmmFirmwareVolumeBlock.h>

#define EFI_FUNCTION_GET_ATTRIBUTES           1
#define EFI_FUNCTION_SET_ATTRIBUTES           2
#define EFI_FUNCTION_GET_PHYSICAL_ADDRESS     3
#define EFI_FUNCTION_GET_BLOCK_SIZE           4
#define EFI_FUNCTION_READ                     5
#define EFI_FUNCTION_WRITE                    6
#define EFI_FUNCTION_ERASE_BLOCKS             7

typedef struct {
  UINTN       Function;
  EFI_STATUS  ReturnStatus;
  UINT8       Data[1];
} SMM_FVB_COMMUNICATE_FUNCTION_HEADER;


///
/// Size of SMM communicate header, without including the payload.
///
#define SMM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))

///
/// Size of SMM FVB communicate function header, without including the payload.
///
#define SMM_FVB_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (SMM_FVB_COMMUNICATE_FUNCTION_HEADER, Data))

typedef struct {
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *SmmFvb;
  EFI_FVB_ATTRIBUTES_2                       Attributes;
} SMM_FVB_ATTRIBUTES_HEADER;

typedef struct {
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *SmmFvb;
  EFI_PHYSICAL_ADDRESS                       Address;
} SMM_FVB_PHYSICAL_ADDRESS_HEADER;

typedef struct {
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *SmmFvb;
  EFI_LBA                                    Lba;
  UINTN                                      BlockSize;
  UINTN                                      NumOfBlocks;
} SMM_FVB_BLOCK_SIZE_HEADER;

typedef struct {
  EFI_SMM_FIRMWARE_VOLUME_BLOCK_PROTOCOL     *SmmFvb;
  EFI_LBA                                    Lba;
  UINTN                                      Offset;
  UINTN                                      NumBytes;
} SMM_FVB_READ_WRITE_HEADER;

typedef struct {
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL         *SmmFvb;
  EFI_LBA                                    StartLba;
  UINTN                                      NumOfLba;
} SMM_FVB_BLOCKS_HEADER;

#endif
