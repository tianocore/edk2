/** @file
Runtime Lib function for QNC internal network access.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiDxe.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/QNCAccessLib.h>

///
/// Set Virtual Address Map Event
///
EFI_EVENT                               mDxeRuntimeQncAccessLibVirtualNotifyEvent = NULL;

///
/// Module global that contains the base physical address of the PCI Express MMIO range.
///
UINTN                                   mDxeRuntimeQncAccessLibPciExpressBaseAddress = 0;

/**
  Convert the physical PCI Express MMIO address to a virtual address.

  @param[in]    Event   The event that is being processed.
  @param[in]    Context The Event Context.
**/
VOID
EFIAPI
DxeRuntimeQncAccessLibVirtualNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                       Status;

  //
  // Convert the physical PCI Express MMIO address to a virtual address.
  //
  Status = EfiConvertPointer (0, (VOID **) &mDxeRuntimeQncAccessLibPciExpressBaseAddress);

  ASSERT_EFI_ERROR (Status);
}

/**
  The constructor function to setup globals and goto virtual mode notify.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor completed successfully.
  @retval Other value   The constructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
DxeRuntimeQncAccessLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Cache the physical address of the PCI Express MMIO range into a module global variable
  //
  mDxeRuntimeQncAccessLibPciExpressBaseAddress = (UINTN) PcdGet64(PcdPciExpressBaseAddress);

  //
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DxeRuntimeQncAccessLibVirtualNotify,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mDxeRuntimeQncAccessLibVirtualNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The destructor function frees any allocated buffers and closes the Set Virtual
  Address Map event.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Other value   The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
DxeRuntimeQncAccessLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Close the Set Virtual Address Map event
  //
  Status = gBS->CloseEvent (mDxeRuntimeQncAccessLibVirtualNotifyEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Gets the base address of PCI Express for Quark North Cluster.

  @return The base address of PCI Express for Quark North Cluster.

**/
UINTN
EFIAPI
QncGetPciExpressBaseAddress (
  VOID
  )
{
  //
  // If system goes to virtual mode then virtual notify callback will update
  // mDxeRuntimeQncAccessLibPciExpressBaseAddress with virtual address of
  // PCIe memory base.
  //
  return mDxeRuntimeQncAccessLibPciExpressBaseAddress;
}

