/** @file
  This driver will report some MMIO/IO resources to dxe core, extract smbios and acpi 
  tables from coreboot and install.
  
  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "CbSupportDxe.h"

UINTN mPmCtrlReg = 0;
/**
  Reserve MMIO/IO resource in GCD

  @param  IsMMIO        Flag of whether it is mmio resource or io resource.
  @param  GcdType       Type of the space.
  @param  BaseAddress   Base address of the space.
  @param  Length        Length of the space.
  @param  Alignment     Align with 2^Alignment
  @param  ImageHandle   Handle for the image of this driver.

  @retval EFI_SUCCESS   Reserve successful
**/
EFI_STATUS
CbReserveResourceInGcd (
  IN BOOLEAN               IsMMIO,
  IN UINTN                 GcdType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINTN                 Alignment,
  IN EFI_HANDLE            ImageHandle
  )
{
	EFI_STATUS               Status;

  if (IsMMIO) {
    Status = gDS->AddMemorySpace (
                    GcdType,
                    BaseAddress,
                    Length,
                    EFI_MEMORY_UC
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_ERROR,
        "Failed to add memory space :0x%lx 0x%lx\n",
        BaseAddress,
        Length
        ));
    }
    ASSERT_EFI_ERROR (Status);
    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = gDS->AddIoSpace (
                    GcdType,
                    BaseAddress,
                    Length
                    );
    ASSERT_EFI_ERROR (Status);
    Status = gDS->AllocateIoSpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
  return Status;
}

/**
  Notification function of EVT_GROUP_READY_TO_BOOT event group.

  This is a notification function registered on EVT_GROUP_READY_TO_BOOT event group.
  When the Boot Manager is about to load and execute a boot option, it reclaims variable
  storage if free size is below the threshold.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
OnReadyToBoot (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{	
	//
	// Enable SCI
	//
	IoOr16 (mPmCtrlReg, BIT0);
	
	DEBUG ((EFI_D_ERROR, "Enable SCI bit at 0x%lx before boot\n", (UINT64)mPmCtrlReg));	
}

/**
  Main entry for the Coreboot Support DXE module.
  
  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
CbDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{  
	EFI_STATUS Status;
	EFI_EVENT  ReadyToBootEvent;
	EFI_HOB_GUID_TYPE  *GuidHob;
	SYSTEM_TABLE_INFO  *pSystemTableInfo;
	ACPI_BOARD_INFO    *pAcpiBoardInfo;
	
	Status = EFI_SUCCESS;
	//
	// Report MMIO/IO Resources
	//
	Status = CbReserveResourceInGcd (TRUE, EfiGcdMemoryTypeMemoryMappedIo, 0xFEE00000, SIZE_1MB, 0, SystemTable); // LAPIC 
	ASSERT_EFI_ERROR (Status);
	
	Status = CbReserveResourceInGcd (TRUE, EfiGcdMemoryTypeMemoryMappedIo, 0xFEC00000, SIZE_4KB, 0, SystemTable); // IOAPIC 
	ASSERT_EFI_ERROR (Status);
	
	Status = CbReserveResourceInGcd (TRUE, EfiGcdMemoryTypeMemoryMappedIo, 0xFED00000, SIZE_1KB, 0, SystemTable); // HPET 
	ASSERT_EFI_ERROR (Status);
	
	//
	// Find the system table information guid hob
	//
	GuidHob = GetFirstGuidHob (&gUefiSystemTableInfoGuid);
	ASSERT (GuidHob != NULL);
  pSystemTableInfo = (SYSTEM_TABLE_INFO *)GET_GUID_HOB_DATA (GuidHob);
	
	//
	// Install Acpi Table
	//
	if (pSystemTableInfo->AcpiTableBase != 0 && pSystemTableInfo->AcpiTableSize != 0) {		
		DEBUG ((EFI_D_ERROR, "Install Acpi Table at 0x%lx, length 0x%x\n", pSystemTableInfo->AcpiTableBase, pSystemTableInfo->AcpiTableSize));	
		Status = gBS->InstallConfigurationTable (&gEfiAcpiTableGuid, (VOID *)(UINTN)pSystemTableInfo->AcpiTableBase);
		ASSERT_EFI_ERROR (Status);
	}
	
	//
	// Install Smbios Table
	//
	if (pSystemTableInfo->SmbiosTableBase != 0 && pSystemTableInfo->SmbiosTableSize != 0) {			
		DEBUG ((EFI_D_ERROR, "Install Smbios Table at 0x%lx, length 0x%x\n", pSystemTableInfo->SmbiosTableBase, pSystemTableInfo->SmbiosTableSize));	
		Status = gBS->InstallConfigurationTable (&gEfiSmbiosTableGuid, (VOID *)(UINTN)pSystemTableInfo->SmbiosTableBase);
		ASSERT_EFI_ERROR (Status);
	}
	
	//
	// Find the acpi board information guid hob
	//
	GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
	ASSERT (GuidHob != NULL);
  pAcpiBoardInfo = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob); 
  
  mPmCtrlReg = (UINTN)pAcpiBoardInfo->PmCtrlRegBase;
	DEBUG ((EFI_D_ERROR, "PmCtrlReg at 0x%lx\n", (UINT64)mPmCtrlReg));	
	 
	//
	// Register callback on the ready to boot event 
	// in order to enable SCI
	// 	
	ReadyToBootEvent = NULL;
  Status = EfiCreateEventReadyToBootEx (
                    TPL_CALLBACK,
                    OnReadyToBoot,
                    NULL,
                    &ReadyToBootEvent
                    );
  ASSERT_EFI_ERROR (Status);
  
  return EFI_SUCCESS;
}

