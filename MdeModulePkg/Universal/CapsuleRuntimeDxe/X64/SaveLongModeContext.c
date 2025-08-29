/** @file
  Create the variable to save the base address of page table and stack
  for transferring into long mode in IA32 capsule PEI.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/Capsule.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Library/VariablePolicyHelperLib.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/AcpiS3Context.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>

//
// 8 extra pages for PF handler.
//
#define EXTRA_PAGE_TABLE_PAGES  8

/**
  Allocate EfiReservedMemoryType below 4G memory address.

  This function allocates EfiReservedMemoryType below 4G memory address.

  @param  Size      Size of memory to allocate.

  @return Allocated Address for output.

**/
VOID *
AllocateReservedMemoryBelow4G (
  IN   UINTN  Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID                  *Buffer;

  Pages   = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *)(UINTN)Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Register callback function upon VariablePolicyProtocol
  to lock EFI_CAPSULE_LONG_MODE_BUFFER_NAME variable to avoid malicious code to update it.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
VariableLockCapsuleLongModeBufferVariable (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                         Status;
  IN EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;

  //
  // Mark EFI_CAPSULE_LONG_MODE_BUFFER_NAME variable to read-only if the Variable Policy protocol exists
  //
  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (!EFI_ERROR (Status)) {
    Status = RegisterBasicVariablePolicy (
               VariablePolicy,
               &gEfiCapsuleVendorGuid,
               EFI_CAPSULE_LONG_MODE_BUFFER_NAME,
               VARIABLE_POLICY_NO_MIN_SIZE,
               VARIABLE_POLICY_NO_MAX_SIZE,
               VARIABLE_POLICY_NO_MUST_ATTR,
               VARIABLE_POLICY_NO_CANT_ATTR,
               VARIABLE_POLICY_TYPE_LOCK_NOW
               );
    DEBUG ((DEBUG_INFO, "Status of RegisterBasicVariablePolicy for (%s) - %r\n", EFI_CAPSULE_LONG_MODE_BUFFER_NAME, Status));
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  1. Allocate Reserved memory for capsule PEIM to establish a 1:1 Virtual to Physical mapping.
  2. Allocate Reserved memroy as a stack for capsule PEIM to transfer from 32-bit mdoe to 64-bit mode.

**/
VOID
EFIAPI
PrepareContextForCapsulePei (
  VOID
  )
{
  UINTN                         ExtraPageTablePages;
  UINT32                        RegEax;
  UINT32                        RegEdx;
  UINTN                         TotalPagesNum;
  UINT8                         PhysicalAddressBits;
  UINT32                        NumberOfPml4EntriesNeeded;
  UINT32                        NumberOfPdpEntriesNeeded;
  BOOLEAN                       Page1GSupport;
  EFI_CAPSULE_LONG_MODE_BUFFER  LongModeBuffer;
  EFI_STATUS                    Status;
  VOID                          *Registration;

  //
  // Calculate the size of page table, allocate the memory.
  //
  Page1GSupport = FALSE;
  if (PcdGetBool (PcdUse1GPageTable)) {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000001) {
      AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
      if ((RegEdx & BIT26) != 0) {
        Page1GSupport = TRUE;
      }
    }
  }

  //
  // Create 4G page table by default,
  // and let PF handler to handle > 4G request.
  //
  PhysicalAddressBits = 32;
  ExtraPageTablePages = EXTRA_PAGE_TABLE_PAGES;

  //
  // Calculate the table entries needed.
  //
  if (PhysicalAddressBits <= 39 ) {
    NumberOfPml4EntriesNeeded = 1;
    NumberOfPdpEntriesNeeded  = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
  } else {
    NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
    NumberOfPdpEntriesNeeded  = 512;
  }

  if (!Page1GSupport) {
    TotalPagesNum = (NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1;
  } else {
    TotalPagesNum = NumberOfPml4EntriesNeeded + 1;
  }

  TotalPagesNum += ExtraPageTablePages;
  DEBUG ((DEBUG_INFO, "CapsuleRuntimeDxe X64 TotalPagesNum - 0x%x pages\n", TotalPagesNum));

  LongModeBuffer.PageTableAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateReservedMemoryBelow4G (EFI_PAGES_TO_SIZE (TotalPagesNum));
  ASSERT (LongModeBuffer.PageTableAddress != 0);

  //
  // Allocate stack
  //
  LongModeBuffer.StackSize        = PcdGet32 (PcdCapsulePeiLongModeStackSize);
  LongModeBuffer.StackBaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateReservedMemoryBelow4G (PcdGet32 (PcdCapsulePeiLongModeStackSize));
  ASSERT (LongModeBuffer.StackBaseAddress != 0);

  Status = gRT->SetVariable (
                  EFI_CAPSULE_LONG_MODE_BUFFER_NAME,
                  &gEfiCapsuleVendorGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (EFI_CAPSULE_LONG_MODE_BUFFER),
                  &LongModeBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Register callback function upon VariablePolicyProtocol
    // to lock EFI_CAPSULE_LONG_MODE_BUFFER_NAME variable to avoid malicious code to update it.
    //
    EfiCreateProtocolNotifyEvent (
      &gEdkiiVariablePolicyProtocolGuid,
      TPL_CALLBACK,
      VariableLockCapsuleLongModeBufferVariable,
      NULL,
      &Registration
      );
  } else {
    DEBUG ((DEBUG_ERROR, "FATAL ERROR: CapsuleLongModeBuffer cannot be saved: %r. Capsule in PEI may fail!\n", Status));
    gBS->FreePages (LongModeBuffer.StackBaseAddress, EFI_SIZE_TO_PAGES (LongModeBuffer.StackSize));
  }
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
  if ((FeaturePcdGet (PcdSupportUpdateCapsuleReset)) && (FeaturePcdGet (PcdDxeIplSwitchToLongMode))) {
    //
    // Allocate memory for Capsule IA32 PEIM, it will create page table to transfer to long mode to access capsule above 4GB.
    //
    PrepareContextForCapsulePei ();
  }
}
