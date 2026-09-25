/** @file
  Publishes EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL so that the generic PCI bus
  driver leaves PCI memory BARs without firmware-assigned UC cache attributes
  on OVMF.

  Copyright (c) 2026, Collabora Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/PciHostMemoryPolicy.h>

/**
  OVMF policy: do not assign a firmware cache attribute to a PCI memory BAR
  and let the platform or operating system choose the policy.

  @param[in]  This       Instance of this protocol.
  @param[in]  PciIo      The EFI_PCI_IO_PROTOCOL instance for the PCI device
                         that owns the BAR.
  @param[in]  BarIndex   Index of the BAR being programmed.

  @retval TRUE   The BAR should be left without a firmware cache attribute.
  @retval FALSE  The BAR should receive the default UC policy.

**/
STATIC
BOOLEAN
EFIAPI
ShouldSkipDefaultUcPolicy (
  IN EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL  *This,
  IN EFI_PCI_IO_PROTOCOL                    *PciIo,
  IN UINT8                                  BarIndex
  )
{
  return TRUE;
}

STATIC EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL  mPciHostMemoryPolicy = {
  ShouldSkipDefaultUcPolicy
};

/**
  Entry point for this driver. Installs
  EDKII_PCI_HOST_MEMORY_POLICY_PROTOCOL unconditionally, before PCI bus
  enumeration programs any BAR.

  @param[in]  ImageHandle  The firmware allocated handle for this driver
                           image.
  @param[in]  SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The protocol was installed successfully.

**/
EFI_STATUS
EFIAPI
PciHostMemoryPolicyDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiPciHostMemoryPolicyProtocolGuid,
                  &mPciHostMemoryPolicy,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
