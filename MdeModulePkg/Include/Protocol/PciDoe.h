/** @file
  EFI PCI I/O Protocol provides the basic Memory, I/O, PCI configuration,
  and DMA interfaces that a driver uses to access its PCI controller.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCI_PROTOCOL_DOE_H_
#define PCI_PROTOCOL_DOE_H_

typedef struct _EDKII_PCI_DOE_PROTOCOL EDKII_PCI_DOE_PROTOCOL;

/**
  Check if a specific Vendor ID and Data Object Type is supported

  @param  This                  A pointer to the EDKII_PCI_DOE_PROTOCOL instance.
  @param  VendorId              The Vendor ID to check against
  @param  Type                  The Data Object Type to check against

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       DOE is not supported or the VendorId and Type are not supported

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PCI_DOE_PROTOCOL_TYPE_CHECK)(
  IN EDKII_PCI_DOE_PROTOCOL        *This,
  IN     UINTN                     VendorId,
  IN     UINTN                     Type
  );

/**
  Performs a send operation on a DOE Mailbox

  @param  This                  A pointer to the EDKII_PCI_DOE_PROTOCOL instance.
  @param  Count                 The number of bytes to use in the write operation.
  @param  Buffer                The source buffer to write data from.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       DOE is not supported

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PCI_DOE_PROTOCOL_WRITE_MAILBOX)(
  IN EDKII_PCI_DOE_PROTOCOL        *This,
  IN     UINTN                     Count,
  IN     VOID                      *Buffer
  );

/**
  Performs a receive operation on a DOE Mailbox

  @param  This                  A pointer to the EDKII_PCI_DOE_PROTOCOL instance.
  @param  Count                 The maximum number of bytes to read for a read operation.
                                It is updated to the number of bytes read.
  @param  Buffer                The destination buffer to store the results.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       DOE is not supported

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PCI_DOE_PROTOCOL_READ_MAILBOX)(
  IN EDKII_PCI_DOE_PROTOCOL        *This,
  IN OUT UINTN                     *Count,
  IN OUT VOID                      *Buffer
  );

struct _EDKII_PCI_DOE_PROTOCOL {
  ///
  /// Check if a specific Vendor ID and Data Object Type is supported
  ///
  EDKII_PCI_DOE_PROTOCOL_TYPE_CHECK       CheckType;
  ///
  /// Send data to the DOE mailbox
  ///
  EDKII_PCI_DOE_PROTOCOL_WRITE_MAILBOX    Send;
  ///
  /// Receive data from the DOE mailbox
  ///
  EDKII_PCI_DOE_PROTOCOL_READ_MAILBOX     Receive;
};

#endif
