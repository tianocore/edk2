/** @file
  Dxe ResetSystem library implementation.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h> // EfiConvertPointer()
#include "ResetSystemAcpiGed.h"

/**
  Modifies the attributes to Runtime type for a page size memory region.

  @param  BaseAddress            Specified start address

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
  @retval EFI_UNSUPPORTED       The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.
  @retval EFI_ACCESS_DEFINED    The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_NOT_AVAILABLE_YET The attributes cannot be set because CPU architectural protocol is
                                not available yet.
**/
STATIC
EFI_STATUS
SetMemoryAttributesRunTime (
  UINTN  Address
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;

  Address &= ~EFI_PAGE_MASK;

  Status = gDS->GetMemorySpaceDescriptor (Address, &Descriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: GetMemorySpaceDescriptor failed\n", __func__));
    return Status;
  }

  if (Descriptor.GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    Address,
                    EFI_PAGE_SIZE,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a: AddMemorySpace failed\n", __func__));
      return Status;
    }

    Status = gDS->SetMemorySpaceAttributes (
                    Address,
                    EFI_PAGE_SIZE,
                    EFI_MEMORY_RUNTIME
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a:%d SetMemorySpaceAttributes failed\n", __func__, __LINE__));
      return Status;
    }
  } else if (!(Descriptor.Attributes & EFI_MEMORY_RUNTIME)) {
    Status = gDS->SetMemorySpaceAttributes (
                    Address,
                    EFI_PAGE_SIZE,
                    Descriptor.Attributes | EFI_MEMORY_RUNTIME
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a:%d SetMemorySpaceAttributes failed\n", __func__, __LINE__));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Find the power manager related info from ACPI table

  @retval RETURN_SUCCESS     Successfully find out all the required information.
  @retval RETURN_NOT_FOUND   Failed to find the required info.
**/
STATIC
EFI_STATUS
GetPowerManagerByParseAcpiInfo (
  VOID
  )
{
  EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt    = NULL;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp    = NULL;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt    = NULL;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt    = NULL;
  UINT32                                        *Entry32 = NULL;
  UINTN                                         Entry32Num;
  UINT32                                        *Signature = NULL;
  UINTN                                         Idx;
  EFI_STATUS                                    Status;

  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, (VOID **)&Rsdp);
  if (EFI_ERROR (Status)) {
    Status = EfiGetSystemConfigurationTable (&gEfiAcpi10TableGuid, (VOID **)&Rsdp);
  }

  if (EFI_ERROR (Status) || (Rsdp == NULL)) {
    DEBUG ((DEBUG_ERROR, "EFI_ERROR or Rsdp == NULL\n"));
    return RETURN_NOT_FOUND;
  }

  Rsdt       = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->RsdtAddress;
  Entry32    = (UINT32 *)(UINTN)(Rsdt + 1);
  Entry32Num = (Rsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
  for (Idx = 0; Idx < Entry32Num; Idx++) {
    Signature = (UINT32 *)(UINTN)Entry32[Idx];
    if (*Signature == EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      Fadt = (EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
      DEBUG ((DEBUG_INFO, "Found Fadt in Rsdt\n"));
      goto Done;
    }
  }

  Xsdt       = (EFI_ACPI_DESCRIPTION_HEADER *)Rsdp->XsdtAddress;
  Entry32    = (UINT32 *)(Xsdt + 1);
  Entry32Num = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) >> 2;
  for (Idx = 0; Idx < Entry32Num; Idx++) {
    Signature = (UINT32 *)(UINTN)Entry32[Idx];
    if (*Signature == EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
      Fadt = (EFI_ACPI_5_0_FIXED_ACPI_DESCRIPTION_TABLE *)Signature;
      DEBUG ((DEBUG_INFO, "Found Fadt in Xsdt\n"));
      goto Done;
    }
  }

  DEBUG ((DEBUG_ERROR, " Fadt Not Found\n"));
  return RETURN_NOT_FOUND;

Done:
  mPowerManager.ResetRegAddr        = Fadt->ResetReg.Address;
  mPowerManager.ResetValue          = Fadt->ResetValue;
  mPowerManager.SleepControlRegAddr = Fadt->SleepControlReg.Address;
  mPowerManager.SleepStatusRegAddr  = Fadt->SleepStatusReg.Address;
  return RETURN_SUCCESS;
}

/**
  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  event. It converts a pointer to a new virtual address.

  @param[in] Event        Event whose notification function is being invoked.
  @param[in] Context      Pointer to the notification function's context
**/
STATIC
VOID
ResetSystemLibAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0, (VOID **)&mPowerManager.SleepControlRegAddr);
  EfiConvertPointer (0, (VOID **)&mPowerManager.SleepStatusRegAddr);
  EfiConvertPointer (0, (VOID **)&mPowerManager.ResetRegAddr);
}

/**
  Notification function of ACPI Table change.

  This is a notification function registered on ACPI Table change event.
  It saves the Century address stored in ACPI FADT table.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.
**/
STATIC
VOID
AcpiNotificationEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = GetPowerManagerByParseAcpiInfo ();
  if (EFI_ERROR (Status)) {
    return;
  }

  DEBUG ((DEBUG_INFO, "%a: sleepControl %llx\n", __func__, mPowerManager.SleepControlRegAddr));
  ASSERT (mPowerManager.SleepControlRegAddr);
  Status =  SetMemoryAttributesRunTime (mPowerManager.SleepControlRegAddr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a:%d\n", __func__, __LINE__));
    return;
  }

  DEBUG ((DEBUG_INFO, "%a: sleepStatus %llx\n", __func__, mPowerManager.SleepStatusRegAddr));
  ASSERT (mPowerManager.SleepStatusRegAddr);
  Status =  SetMemoryAttributesRunTime (mPowerManager.SleepStatusRegAddr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a:%d\n", __func__, __LINE__));
    return;
  }

  DEBUG ((DEBUG_INFO, "%a: ResetReg %llx\n", __func__, mPowerManager.ResetRegAddr));
  ASSERT (mPowerManager.ResetRegAddr);
  Status =  SetMemoryAttributesRunTime (mPowerManager.ResetRegAddr);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a:%d\n", __func__, __LINE__));
  }

  return;
}

/**
  The constructor function to Register ACPI Table change event and Address Change Event.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.
**/
EFI_STATUS
EFIAPI
ResetSystemLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  EFI_EVENT   ResetSystemVirtualNotifyEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiNotificationEvent,
                  NULL,
                  &gEfiAcpiTableGuid,
                  &Event
                  );

  //
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  ResetSystemLibAddressChangeEvent,
                  NULL,
                  &ResetSystemVirtualNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
