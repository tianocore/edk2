/** @file
  PEI Services Table Pointer Library.

  This library is used for PEIM which does executed from flash device directly but
  executed in memory.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Portiions copyrigth (c) 2011, Apple Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>

#include <Ppi/MemoryDiscovered.h>


CONST EFI_PEI_SERVICES  **gPeiServices = NULL;

/**
  Caches a pointer PEI Services Table.

  Caches the pointer to the PEI Services Table specified by PeiServicesTablePointer
  in a CPU specific manner as specified in the CPU binding section of the Platform Initialization
  Pre-EFI Initialization Core Interface Specification.

  If PeiServicesTablePointer is NULL, then ASSERT().

  @param    PeiServicesTablePointer   The address of PeiServices pointer.
**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  IN CONST EFI_PEI_SERVICES ** PeiServicesTablePointer
  )
{
  ASSERT (PeiServicesTablePointer != NULL);
  ASSERT (*PeiServicesTablePointer != NULL);
  gPeiServices = PeiServicesTablePointer;
}

/**
  Retrieves the cached value of the PEI Services Table pointer.

  Returns the cached value of the PEI Services Table pointer in a CPU specific manner
  as specified in the CPU binding section of the Platform Initialization Pre-EFI
  Initialization Core Interface Specification.

  If the cached PEI Services Table pointer is NULL, then ASSERT().

  @return  The pointer to PeiServices.

**/
CONST EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  ASSERT (gPeiServices != NULL);
  ASSERT (*gPeiServices != NULL);
  return gPeiServices;
}



/**
  Notification service to be called when gEmuThunkPpiGuid is installed.

  @param  PeiServices                 Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor          Address of the notification descriptor data structure. Type
          EFI_PEI_NOTIFY_DESCRIPTOR is defined above.
  @param  Ppi                             Address of the PPI that was installed.

  @retval   EFI_STATUS                This function will install a PPI to PPI database. The status
                                                  code will be the code for (*PeiServices)->InstallPpi.

**/
EFI_STATUS
EFIAPI
PeiServicesTablePointerNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{
  gPeiServices = (CONST EFI_PEI_SERVICES  **)PeiServices;

  return EFI_SUCCESS;
}


EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnThunkList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  PeiServicesTablePointerNotifyCallback
};


/**
  Constructor register notification on when PPI updates. If PPI is
  alreay installed registering the notify will cause the handle to
  run.

  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval EFI_SUCCESS  The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;

  gPeiServices = (CONST EFI_PEI_SERVICES  **)PeiServices;

  // register to be told when PeiServices pointer is updated
  Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyOnThunkList);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Perform CPU specific actions required to migrate the PEI Services Table
  pointer from temporary RAM to permanent RAM.

  For IA32 CPUs, the PEI Services Table pointer is stored in the 4 bytes
  immediately preceding the Interrupt Descriptor Table (IDT) in memory.
  For X64 CPUs, the PEI Services Table pointer is stored in the 8 bytes
  immediately preceding the Interrupt Descriptor Table (IDT) in memory.
  For Itanium and ARM CPUs, a the PEI Services Table Pointer is stored in
  a dedicated CPU register.  This means that there is no memory storage
  associated with storing the PEI Services Table pointer, so no additional
  migration actions are required for Itanium or ARM CPUs.

**/
VOID
EFIAPI
MigratePeiServicesTablePointer (
  VOID
  )
{
  //
  //  PEI Services Table pointer is cached in the global variable. No additional
  //  migration actions are required.
  //
  return;
}

