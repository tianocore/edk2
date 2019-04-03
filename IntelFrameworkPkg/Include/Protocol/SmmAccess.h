/** @file
  This file declares the SMM SMRAM Access abstraction protocol, which is used to control
  the visibility of the SMRAM on the platform. The expectation is
  that the north bridge or memory controller would publish this protocol.
  For example, the Memory Controller Hub (MCH) has the hardware provision for this
  type of control. Because of the protected, distinguished class of memory for IA-32
  systems, the expectation is that this protocol would be supported only on IA-32 systems.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.
**/

#ifndef _SMM_ACCESS_H_
#define _SMM_ACCESS_H_

#include <Guid/SmramMemoryReserve.h>

typedef struct _EFI_SMM_ACCESS_PROTOCOL  EFI_SMM_ACCESS_PROTOCOL;

#define EFI_SMM_ACCESS_PROTOCOL_GUID \
  { \
    0x3792095a, 0xe309, 0x4c1e, {0xaa, 0x01, 0x85, 0xf5, 0x65, 0x5a, 0x17, 0xf1 } \
  }

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
(EFIAPI *EFI_SMM_OPEN)(
  IN EFI_SMM_ACCESS_PROTOCOL         *This,
  UINTN                              DescriptorIndex
  );

/**
  Inhibits access to the SMRAM.

  @param  This                  The EFI_SMM_ACCESS_PROTOCOL instance.
  @param  DescriptorIndex       Indicates that the driver wishes to close
                                the memory tagged by this index.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_DEVICE_ERROR      The given DescriptorIndex is not open.
  @retval EFI_INVALID_PARAMETER The given DescriptorIndex is not supported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_CLOSE)(
  IN EFI_SMM_ACCESS_PROTOCOL          *This,
  UINTN                               DescriptorIndex
  );

/**
  Inhibits access to the SMRAM.

  @param  This                  The EFI_SMM_ACCESS_PROTOCOL instance.
  @param  DescriptorIndex       Indicates that the driver wishes to lock
                                the memory tagged by this index.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_DEVICE_ERROR      The given DescriptorIndex is not open.
  @retval EFI_INVALID_PARAMETER The given DescriptorIndex is not supported.
  @retval EFI_NOT_STARTED       The SMM base service has not been initialized.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_LOCK)(
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
(EFIAPI *EFI_SMM_CAPABILITIES)(
  IN EFI_SMM_ACCESS_PROTOCOL             *This,
  IN OUT UINTN                           *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR            *SmramMap
  );

/**
  This protocol is used to control the visibility of the SMRAM on the platform.
**/
struct _EFI_SMM_ACCESS_PROTOCOL {
  EFI_SMM_OPEN          Open;             ///< Opens the SMRAM.
  EFI_SMM_CLOSE         Close;            ///< Closes the SMRAM.
  EFI_SMM_LOCK          Lock;             ///< Locks the SMRAM.
  EFI_SMM_CAPABILITIES  GetCapabilities;  ///< Gets information on possible SMRAM regions.
  BOOLEAN               LockState;        ///< Indicates the current state of the SMRAM. Set to TRUE if any region is locked.
  BOOLEAN               OpenState;        ///< Indicates the current state of the SMRAM. Set to TRUE if any region is open.
};

extern EFI_GUID gEfiSmmAccessProtocolGuid;

#endif
