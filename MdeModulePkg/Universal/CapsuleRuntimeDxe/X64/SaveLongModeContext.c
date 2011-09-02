/** @file
  Create the variable to save the base address of page table and stack
  for transferring into long mode in IA32 capsule PEI.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/Capsule.h>
#include <Protocol/DxeSmmReadyToLock.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/AcpiS3Context.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseLib.h>
#include <Library/LockBoxLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  DxeSmmReadyToLock Protocol notification event handler.
  We reuse S3 ACPI NVS reserved memory to do capsule process
  after reset.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
DxeSmmReadyToLockNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                    Status;
  VOID                          *DxeSmmReadyToLock;
  UINTN                         VarSize;
  EFI_PHYSICAL_ADDRESS          TempAcpiS3Context;
  ACPI_S3_CONTEXT               *AcpiS3Context;
  EFI_CAPSULE_LONG_MODE_BUFFER  LongModeBuffer;
  UINTN                         TotalPagesNum;
  UINT8                         PhysicalAddressBits;
  VOID                          *Hob;
  UINT32                        NumberOfPml4EntriesNeeded;
  UINT32                        NumberOfPdpEntriesNeeded;
  BOOLEAN                       LockBoxFound;

  Status = gBS->LocateProtocol (
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  NULL,
                  &DxeSmmReadyToLock
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Get the ACPI NVS pages reserved by AcpiS3Save
  //
  LockBoxFound = FALSE;
  VarSize = sizeof (EFI_PHYSICAL_ADDRESS);
  Status = RestoreLockBox (
             &gEfiAcpiVariableGuid,
             &TempAcpiS3Context,
             &VarSize
             );
  if (!EFI_ERROR (Status)) {
    AcpiS3Context = (ACPI_S3_CONTEXT *)(UINTN)TempAcpiS3Context;
    ASSERT (AcpiS3Context != NULL);
    
    Status = RestoreLockBox (
               &gEfiAcpiS3ContextGuid,
               NULL,
               NULL
               );
    if (!EFI_ERROR (Status)) {
      LongModeBuffer.PageTableAddress = AcpiS3Context->S3NvsPageTableAddress;
      LongModeBuffer.StackBaseAddress = AcpiS3Context->BootScriptStackBase;
      LongModeBuffer.StackSize        = AcpiS3Context->BootScriptStackSize;
      LockBoxFound                    = TRUE;
    }
  }
  
  if (!LockBoxFound) {
    //
    // Page table base address and stack base address can not be found in lock box,
    // allocate both here. 
    //

    //
    // Get physical address bits supported from CPU HOB.
    //
    PhysicalAddressBits = 36;
    
    Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
    if (Hob != NULL) {
      PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;
    }
    
    //
    // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses.
    //
    ASSERT (PhysicalAddressBits <= 52);
    if (PhysicalAddressBits > 48) {
      PhysicalAddressBits = 48;
    }
    
    //
    // Calculate page table size and allocate memory for it.
    //
    if (PhysicalAddressBits <= 39 ) {
      NumberOfPml4EntriesNeeded = 1;
      NumberOfPdpEntriesNeeded =  1 << (PhysicalAddressBits - 30);
    } else {
      NumberOfPml4EntriesNeeded = 1 << (PhysicalAddressBits - 39);
      NumberOfPdpEntriesNeeded = 512;
    }
    
    TotalPagesNum = (NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1;
    LongModeBuffer.PageTableAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateAcpiNvsMemoryBelow4G (EFI_PAGES_TO_SIZE (TotalPagesNum));
    ASSERT (LongModeBuffer.PageTableAddress != 0);
    
    //
    // Allocate stack
    //
    LongModeBuffer.StackSize        = PcdGet32 (PcdCapsulePeiLongModeStackSize);
    LongModeBuffer.StackBaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateAcpiNvsMemoryBelow4G (PcdGet32 (PcdCapsulePeiLongModeStackSize));
    ASSERT (LongModeBuffer.StackBaseAddress != 0);
  }

  Status = gRT->SetVariable (
                  EFI_CAPSULE_LONG_MODE_BUFFER_NAME,
                  &gEfiCapsuleVendorGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (EFI_CAPSULE_LONG_MODE_BUFFER),
                  &LongModeBuffer
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Close event, so it will not be invoked again.
  //
  gBS->CloseEvent (Event);

  return ;
}

/**
  Create the variable to save the base address of page table and stack
  for transferring into long mode in IA32 capsule PEI.
**/
VOID
SaveLongModeContext (
  VOID
  )
{
  VOID        *Registration;
  
  if ((FeaturePcdGet(PcdSupportUpdateCapsuleReset)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    //
    // Register event to get ACPI NVS pages reserved from lock box, these pages will be used by
    // Capsule IA32 PEI to transfer to long mode to access capsule above 4GB.
    //
    EfiCreateProtocolNotifyEvent (
      &gEfiDxeSmmReadyToLockProtocolGuid,
      TPL_CALLBACK,
      DxeSmmReadyToLockNotification,
      NULL,
      &Registration
      );
  }
}
