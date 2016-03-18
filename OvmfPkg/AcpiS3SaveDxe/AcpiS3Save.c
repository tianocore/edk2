/** @file
  This is a replacement for the ACPI S3 Save protocol.

  The ACPI S3 Save protocol used to be defined in the S3 boot path
  specification 0.9. Instead, the same functionality is now hooked to the
  End-of-Dxe event.

Copyright (c) 2014-2015, Red Hat, Inc.<BR>
Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/LockBoxLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Guid/AcpiVariableCompatibility.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/Acpi.h>
#include <Guid/EventGroup.h>
#include <Protocol/LockBox.h>
#include <IndustryStandard/Acpi.h>

EFI_GUID              mAcpiS3IdtrProfileGuid = {
  0xdea652b0, 0xd587, 0x4c54, { 0xb5, 0xb4, 0xc6, 0x82, 0xe7, 0xa0, 0xaa, 0x3d }
};

/**
  Allocate memory below 4G memory address.

  This function allocates memory below 4G memory address.

  @param  MemoryType   Memory type of memory to allocate.
  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateMemoryBelow4G (
  IN EFI_MEMORY_TYPE    MemoryType,
  IN UINTN              Size
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
                   MemoryType,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**

  This function scan ACPI table in RSDT.

  @param Rsdt      ACPI RSDT
  @param Signature ACPI table signature

  @return ACPI table

**/
VOID *
ScanTableInRSDT (
  IN EFI_ACPI_DESCRIPTION_HEADER    *Rsdt,
  IN UINT32                         Signature
  )
{
  UINTN                              Index;
  UINT32                             EntryCount;
  UINT32                             *EntryPtr;
  EFI_ACPI_DESCRIPTION_HEADER        *Table;

  if (Rsdt == NULL) {
    return NULL;
  }

  EntryCount = (Rsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT32);
  
  EntryPtr = (UINT32 *)(Rsdt + 1);
  for (Index = 0; Index < EntryCount; Index ++, EntryPtr ++) {
    Table = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(*EntryPtr));
    if (Table->Signature == Signature) {
      return Table;
    }
  }
  
  return NULL;
}

/**

  This function scan ACPI table in XSDT.

  @param Xsdt      ACPI XSDT
  @param Signature ACPI table signature

  @return ACPI table

**/
VOID *
ScanTableInXSDT (
  IN EFI_ACPI_DESCRIPTION_HEADER    *Xsdt,
  IN UINT32                         Signature
  )
{
  UINTN                          Index;
  UINT32                         EntryCount;
  UINT64                         EntryPtr;
  UINTN                          BasePtr;
  EFI_ACPI_DESCRIPTION_HEADER    *Table;

  if (Xsdt == NULL) {
    return NULL;
  }

  EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof(UINT64);
  
  BasePtr = (UINTN)(Xsdt + 1);
  for (Index = 0; Index < EntryCount; Index ++) {
    CopyMem (&EntryPtr, (VOID *)(BasePtr + Index * sizeof(UINT64)), sizeof(UINT64));
    Table = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(EntryPtr));
    if (Table->Signature == Signature) {
      return Table;
    }
  }
  
  return NULL;
}

/**
  To find Facs in FADT.

  @param Fadt   FADT table pointer
  
  @return  Facs table pointer.
**/
EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *
FindAcpiFacsFromFadt (
  IN EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt
  )
{
  EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  UINT64                                        Data64;

  if (Fadt == NULL) {
    return NULL;
  }

  if (Fadt->Header.Revision < EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_REVISION) {
    Facs = (EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)Fadt->FirmwareCtrl;
  } else {
    if (Fadt->FirmwareCtrl != 0) {
      Facs = (EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)Fadt->FirmwareCtrl;
    } else {
      CopyMem (&Data64, &Fadt->XFirmwareCtrl, sizeof(UINT64));
      Facs = (EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)Data64;
    }
  }
  return Facs;
}

/**
  To find Facs in Acpi tables.
 
  To find Firmware ACPI control strutcure in Acpi Tables since the S3 waking vector is stored 
  in the table.

  @param AcpiTableGuid   The guid used to find ACPI table in UEFI ConfigurationTable.
  
  @return  Facs table pointer.
**/
EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *
FindAcpiFacsTableByAcpiGuid (
  IN EFI_GUID  *AcpiTableGuid
  )
{
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt;
  EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  UINTN                                         Index;

  Rsdp  = NULL;
  //
  // found ACPI table RSD_PTR from system table
  //
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), AcpiTableGuid)) {
      //
      // A match was found.
      //
      Rsdp = gST->ConfigurationTable[Index].VendorTable;
      break;
    }
  }

  if (Rsdp == NULL) {
    return NULL;
  }

  //
  // Search XSDT
  //
  if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION) {
    Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN) Rsdp->XsdtAddress;
    Fadt = ScanTableInXSDT (Xsdt, EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE);
    if (Fadt != NULL) {
      Facs = FindAcpiFacsFromFadt (Fadt);
      if (Facs != NULL) {
        return Facs;
      }
    }
  }

  //
  // Search RSDT
  //
  Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN) Rsdp->RsdtAddress;
  Fadt = ScanTableInRSDT (Rsdt, EFI_ACPI_2_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE);
  if (Fadt != NULL) {
    Facs = FindAcpiFacsFromFadt (Fadt);
    if (Facs != NULL) {
      return Facs;
    }
  }

  return NULL;
}

/**
  To find Facs in Acpi tables.
 
  To find Firmware ACPI control strutcure in Acpi Tables since the S3 waking vector is stored 
  in the table.
  
  @return  Facs table pointer.
**/
EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE  *
FindAcpiFacsTable (
  VOID
  )
{
  EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *Facs;

  Facs = FindAcpiFacsTableByAcpiGuid (&gEfiAcpi20TableGuid);
  if (Facs != NULL) {
    return Facs;
  }

  return FindAcpiFacsTableByAcpiGuid (&gEfiAcpi10TableGuid);
}

/**
  Allocates and fills in the Page Directory and Page Table Entries to
  establish a 1:1 Virtual to Physical mapping.
  If BootScriptExector driver will run in 64-bit mode, this function will establish the 1:1 
  virtual to physical mapping page table.
  If BootScriptExector driver will not run in 64-bit mode, this function will do nothing. 
  
  @return  the 1:1 Virtual to Physical identity mapping page table base address. 

**/
EFI_PHYSICAL_ADDRESS
S3CreateIdentityMappingPageTables (
  VOID
  )
{  
  if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    UINT32                                        RegEax;
    UINT32                                        RegEdx;
    UINT8                                         PhysicalAddressBits;
    UINT32                                        NumberOfPml4EntriesNeeded;
    UINT32                                        NumberOfPdpEntriesNeeded;
    EFI_PHYSICAL_ADDRESS                          S3NvsPageTableAddress;
    UINTN                                         TotalPageTableSize;
    VOID                                          *Hob;
    BOOLEAN                                       Page1GSupport;

    Page1GSupport = FALSE;
    if (PcdGetBool(PcdUse1GPageTable)) {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000001) {
        AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
        if ((RegEdx & BIT26) != 0) {
          Page1GSupport = TRUE;
        }
      }
    }

    //
    // Get physical address bits supported.
    //
    Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
    if (Hob != NULL) {
      PhysicalAddressBits = ((EFI_HOB_CPU *) Hob)->SizeOfMemorySpace;
    } else {
      AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
      if (RegEax >= 0x80000008) {
        AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
        PhysicalAddressBits = (UINT8) RegEax;
      } else {
        PhysicalAddressBits = 36;
      }
    }
    
    //
    // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses.
    //
    ASSERT (PhysicalAddressBits <= 52);
    if (PhysicalAddressBits > 48) {
      PhysicalAddressBits = 48;
    }

    //
    // Calculate the table entries needed.
    //
    if (PhysicalAddressBits <= 39 ) {
      NumberOfPml4EntriesNeeded = 1;
      NumberOfPdpEntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
    } else {
      NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
      NumberOfPdpEntriesNeeded = 512;
    }

    //
    // We need calculate whole page size then allocate once, because S3 restore page table does not know each page in Nvs.
    //
    if (!Page1GSupport) {
      TotalPageTableSize = (UINTN)(1 + NumberOfPml4EntriesNeeded + NumberOfPml4EntriesNeeded * NumberOfPdpEntriesNeeded);
    } else {
      TotalPageTableSize = (UINTN)(1 + NumberOfPml4EntriesNeeded);
    }
    DEBUG ((EFI_D_ERROR, "TotalPageTableSize - %Lx pages\n",
      (UINT64)TotalPageTableSize));

    //
    // By architecture only one PageMapLevel4 exists - so lets allocate storage for it.
    //
    S3NvsPageTableAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateMemoryBelow4G (EfiReservedMemoryType, EFI_PAGES_TO_SIZE(TotalPageTableSize));
    ASSERT (S3NvsPageTableAddress != 0);
    return S3NvsPageTableAddress;
  } else {
    //
    // If DXE is running 32-bit mode, no need to establish page table.
    //
    return  (EFI_PHYSICAL_ADDRESS) 0;  
  }
}

/**
  Prepares all information that is needed in the S3 resume boot path.
  
  Allocate the resources or prepare informations and save in ACPI variable set for S3 resume boot path  
  
  @retval EFI_SUCCESS           All information was saved successfully.
**/
STATIC
EFI_STATUS
EFIAPI
S3Ready (
  VOID
  )
{
  EFI_STATUS                                    Status;
  EFI_PHYSICAL_ADDRESS                          AcpiS3ContextBuffer;
  ACPI_S3_CONTEXT                               *AcpiS3Context;
  STATIC BOOLEAN                                AlreadyEntered;
  IA32_DESCRIPTOR                               *Idtr;
  IA32_IDT_GATE_DESCRIPTOR                      *IdtGate;

  DEBUG ((EFI_D_INFO, "S3Ready!\n"));

  ASSERT (!AlreadyEntered);
  if (AlreadyEntered) {
    return EFI_SUCCESS;
  }
  AlreadyEntered = TRUE;

  AcpiS3Context = AllocateMemoryBelow4G (EfiReservedMemoryType, sizeof(*AcpiS3Context));
  ASSERT (AcpiS3Context != NULL);
  AcpiS3ContextBuffer = (EFI_PHYSICAL_ADDRESS)(UINTN)AcpiS3Context;

  //
  // Get ACPI Table because we will save its position to variable
  //
  AcpiS3Context->AcpiFacsTable = (EFI_PHYSICAL_ADDRESS)(UINTN)FindAcpiFacsTable ();
  ASSERT (AcpiS3Context->AcpiFacsTable != 0);

  IdtGate = AllocateMemoryBelow4G (EfiReservedMemoryType, sizeof(IA32_IDT_GATE_DESCRIPTOR) * 0x100 + sizeof(IA32_DESCRIPTOR));
  Idtr = (IA32_DESCRIPTOR *)(IdtGate + 0x100);
  Idtr->Base  = (UINTN)IdtGate;
  Idtr->Limit = (UINT16)(sizeof(IA32_IDT_GATE_DESCRIPTOR) * 0x100 - 1);
  AcpiS3Context->IdtrProfile = (EFI_PHYSICAL_ADDRESS)(UINTN)Idtr;

  Status = SaveLockBox (
             &mAcpiS3IdtrProfileGuid,
             (VOID *)(UINTN)Idtr,
             (UINTN)sizeof(IA32_DESCRIPTOR)
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (&mAcpiS3IdtrProfileGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate page table
  //
  AcpiS3Context->S3NvsPageTableAddress = S3CreateIdentityMappingPageTables ();

  //
  // Allocate stack
  //
  AcpiS3Context->BootScriptStackSize = PcdGet32 (PcdS3BootScriptStackSize);
  AcpiS3Context->BootScriptStackBase = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateMemoryBelow4G (EfiReservedMemoryType, PcdGet32 (PcdS3BootScriptStackSize));
  ASSERT (AcpiS3Context->BootScriptStackBase != 0);

  //
  // Allocate a code buffer < 4G for S3 debug to load external code, set invalid code instructions in it.
  //
  AcpiS3Context->S3DebugBufferAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateMemoryBelow4G (EfiReservedMemoryType, EFI_PAGE_SIZE);
  SetMem ((VOID *)(UINTN)AcpiS3Context->S3DebugBufferAddress, EFI_PAGE_SIZE, 0xff);

  DEBUG ((EFI_D_INFO, "AcpiS3Context: AcpiFacsTable is 0x%8Lx\n",
    AcpiS3Context->AcpiFacsTable));
  DEBUG ((EFI_D_INFO, "AcpiS3Context: IdtrProfile is 0x%8Lx\n",
    AcpiS3Context->IdtrProfile));
  DEBUG ((EFI_D_INFO, "AcpiS3Context: S3NvsPageTableAddress is 0x%8Lx\n",
    AcpiS3Context->S3NvsPageTableAddress));
  DEBUG ((EFI_D_INFO, "AcpiS3Context: S3DebugBufferAddress is 0x%8Lx\n",
    AcpiS3Context->S3DebugBufferAddress));

  Status = SaveLockBox (
             &gEfiAcpiVariableGuid,
             &AcpiS3ContextBuffer,
             sizeof(AcpiS3ContextBuffer)
             );
  ASSERT_EFI_ERROR (Status);

  Status = SaveLockBox (
             &gEfiAcpiS3ContextGuid,
             (VOID *)(UINTN)AcpiS3Context,
             (UINTN)sizeof(*AcpiS3Context)
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (&gEfiAcpiS3ContextGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Callback function executed when the EndOfDxe event group is signaled.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context, which
                      is implementation-dependent.
**/
VOID
EFIAPI
OnEndOfDxe (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS Status;

  //
  // Our S3Ready() function always succeeds.
  //
  Status = S3Ready ();
  ASSERT_EFI_ERROR (Status);

  //
  // Close the event, deregistering the callback and freeing resources.
  //
  Status = gBS->CloseEvent (Event);
  ASSERT_EFI_ERROR (Status);
}


/**
  The Driver Entry Point.
  
  The function is the driver Entry point that will register the End-of-Dxe
  callback.

  @param ImageHandle   A handle for the image that is initializing this driver
  @param SystemTable   A pointer to the EFI system table

  @retval EFI_SUCCESS:              Driver initialized successfully
  @retval EFI_LOAD_ERROR:           Failed to Initialize or has been loaded
  @retval EFI_OUT_OF_RESOURCES      Could not allocate needed resources

**/
EFI_STATUS
EFIAPI
InstallEndOfDxeCallback (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS        Status;
  EFI_EVENT         EndOfDxeEvent;

  if (!QemuFwCfgS3Enabled()) {
    return EFI_LOAD_ERROR;
  }

  if (!FeaturePcdGet (PcdSmmSmramRequire)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ImageHandle,
                    &gEfiLockBoxProtocolGuid, NULL,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnEndOfDxe,
                  NULL, /* NotifyContext */
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
