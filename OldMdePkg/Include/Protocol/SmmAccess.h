/** @file
  This file declares SMM SMRAM Access abstraction protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmAccess.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.
**/

#ifndef _SMM_ACCESS_H_
#define _SMM_ACCESS_H_

typedef struct _EFI_SMM_ACCESS_PROTOCOL  EFI_SMM_ACCESS_PROTOCOL;

#define EFI_SMM_ACCESS_PROTOCOL_GUID \
  { \
    0x3792095a, 0xe309, 0x4c1e, {0xaa, 0x01, 0x85, 0xf5, 0x65, 0x5a, 0x17, 0xf1 } \
  }

//
// SMM Access specification constant and types
//
// *******************************************************
//  EFI_SMRAM_STATE
// *******************************************************
//
#define EFI_SMRAM_OPEN    0x00000001
#define EFI_SMRAM_CLOSED  0x00000002
#define EFI_SMRAM_LOCKED  0x00000004
#define EFI_CACHEABLE     0x00000008
#define EFI_ALLOCATED     0x00000010

//
// SMM Access specification Member Function
//
/**
  Opens the SMRAM area to be accessible by a boot-service driver.

  @param  This                  The EFI_SMM_ACCESS_PROTOCOL instance.
  @param  DescriptorIndex       Indicates that the driver wishes to open
                                the memory tagged by this index.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_INVALID_PARAMETER The given DescriptorIndex is not supported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_OPEN) (
  IN EFI_SMM_ACCESS_PROTOCOL         *This,
  UINTN                              DescriptorIndex
  );

/**
  Inhibits access to the SMRAM.

  @param  This                  The EFI_SMM_ACCESS_PROTOCOL instance.
  @param  DescriptorIndex       Indicates that the driver wishes to open
                                the memory tagged by this index.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_DEVICE_ERROR      The given DescriptorIndex is not open.
  @retval EFI_INVALID_PARAMETER The given DescriptorIndex is not supported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CLOSE) (
  IN EFI_SMM_ACCESS_PROTOCOL          *This,
  UINTN                               DescriptorIndex
  );

/**
  Inhibits access to the SMRAM.
  @param  This                  The EFI_SMM_ACCESS_PROTOCOL instance.
  @param  DescriptorIndex       Indicates that the driver wishes to open
                                the memory tagged by this index.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_DEVICE_ERROR      The given DescriptorIndex is not open.
  @retval EFI_INVALID_PARAMETER The given DescriptorIndex is not supported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_LOCK) (
  IN EFI_SMM_ACCESS_PROTOCOL         *This,
  UINTN                              DescriptorIndex
  );

/**
  Queries the memory controller for the possible regions that will support SMRAM.

  @param  This                  The EFI_SMM_ACCESS_PROTOCOL instance.
  @param  SmramMapSize          A pointer to the size, in bytes, of the SmramMemoryMap buffer.
  @param  SmramMap              A pointer to the buffer in which firmware places the current memory map.

  @retval EFI_SUCCESS           The chipset supported the given resource.
  @retval EFI_BUFFER_TOO_SMALL  The SmramMap parameter was too small.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CAPABILITIES) (
  IN EFI_SMM_ACCESS_PROTOCOL             *This,
  IN OUT UINTN                           *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR            *SmramMap
  );

/**
  @par Protocol Description:
  This protocol is used to control the visibility of the SMRAM on the platform.

  @param Open
  Opens the SMRAM. 

  @param Close
  Closes the SMRAM.

  @param Lock
  Locks the SMRAM. 

  @param GetCapabilities
  Gets information on possible SMRAM regions.

  @param LockState
Indicates the current state of the SMRAM. Set to TRUE if any region is locked. 

  @param OpenState
Indicates the current state of the SMRAM. Set to TRUE if any region is open. 

**/
struct _EFI_SMM_ACCESS_PROTOCOL {
  EFI_SMM_OPEN          Open;
  EFI_SMM_CLOSE         Close;
  EFI_SMM_LOCK          Lock;
  EFI_SMM_CAPABILITIES  GetCapabilities;
  BOOLEAN               LockState;
  BOOLEAN               OpenState;
};

extern EFI_GUID gEfiSmmAccessProtocolGuid;

#endif
