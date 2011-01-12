/** @file

  The common header file for SMM FTW module and SMM FTW DXE Module. 

Copyright (c) 2011, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/

#ifndef __SMM_FTW_COMMON_H__
#define __SMM_FTW_COMMON_H__

#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Protocol/SmmFaultTolerantWrite.h>

#define FTW_FUNCTION_GET_MAX_BLOCK_SIZE       1
#define FTW_FUNCTION_ALLOCATE                 2
#define FTW_FUNCTION_WRITE                    3
#define FTW_FUNCTION_RESTART                  4
#define FTW_FUNCTION_ABORT                    5
#define FTW_FUNCTION_GET_LAST_WRITE           6

typedef struct {
  UINTN       Function;
  EFI_STATUS  ReturnStatus;
  UINT8       Data[1];
} SMM_FTW_COMMUNICATE_FUNCTION_HEADER;

///
/// Size of SMM communicate header, without including the payload.
///
#define SMM_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data))

///
/// Size of SMM FTW communicate function header, without including the payload.
///
#define SMM_FTW_COMMUNICATE_HEADER_SIZE  (OFFSET_OF (SMM_FTW_COMMUNICATE_FUNCTION_HEADER, Data))

typedef struct {
  UINTN                                 BlockSize;
} SMM_FTW_GET_MAX_BLOCK_SIZE_HEADER;

typedef struct {
  EFI_GUID                              CallerId;
  UINTN                                 PrivateDataSize;
  UINTN                                 NumberOfWrites;
} SMM_FTW_ALLOCATE_HEADER;

typedef struct {
  EFI_LBA                               Lba;
  UINTN                                 Offset;
  UINTN                                 PrivateDataSize;
  EFI_PHYSICAL_ADDRESS                  FvbBaseAddress;
  EFI_FVB_ATTRIBUTES_2                  FvbAttributes;
  UINTN                                 Length;
  UINT8                                 Data[1];
} SMM_FTW_WRITE_HEADER;

typedef struct {
  EFI_PHYSICAL_ADDRESS                  FvbBaseAddress;
  EFI_FVB_ATTRIBUTES_2                  FvbAttributes;
} SMM_FTW_RESTART_HEADER;

typedef struct {
  EFI_GUID                              CallerId;
  EFI_LBA                               Lba;
  UINTN                                 Offset;
  UINTN                                 Length;
  UINTN                                 PrivateDataSize;
  BOOLEAN                               Complete;
  UINT8                                 Data[1];
} SMM_FTW_GET_LAST_WRITE_HEADER;

#endif
