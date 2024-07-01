/** @file
  PCIe Data Object Exchange (DOE)

Copyright (c) 2024, Western Digital Corporation or its affiliates.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EFI_PCI_DOE_H_
#define EFI_PCI_DOE_H_

/**
  Probe if the PCIe device supports DOE and if it does populate the
  EDKII_PCI_DOE_PROTOCOL instance.

  @param PciIoDevice      PCI device instance.

  @retval EFI_UNSUPPORTED PCI Device does not support DOE.
  @retval EFI_SUCCESS     PCI Device does support DOE.

**/
EFI_STATUS
ProbeDoeSupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  );

/**
  Check if a specific Vendor ID and Data Object Type is supported

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  VendorId              The Vendor ID to check against
  @param  Type                  The Data Object Type to check against

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       DOE is not supported or the VendorId and Type are not supported

**/
EFI_STATUS
EFIAPI
PciIoDoeTypeCheck (
  IN EDKII_PCI_DOE_PROTOCOL  *This,
  IN     UINTN               VendorId,
  IN     UINTN               Type
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
EFI_STATUS
EFIAPI
PciIoDoeRecieve (
  IN EDKII_PCI_DOE_PROTOCOL  *This,
  IN OUT UINTN               *Count,
  IN OUT VOID                *Buffer
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
EFI_STATUS
EFIAPI
PciIoDoeSend (
  IN EDKII_PCI_DOE_PROTOCOL  *This,
  IN     UINTN               Count,
  IN     VOID                *Buffer
  );

#endif
