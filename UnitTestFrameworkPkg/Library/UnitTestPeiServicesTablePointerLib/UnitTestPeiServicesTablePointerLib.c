/** @file
  This library supports a PEI Service table Pointer library implementation that
  allows code dependent upon PEI Service to operate in an isolated execution environment
  such as within the context of a host-based unit test framework.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UnitTestPeiServicesTablePointerLib.h"

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  This function copies Length bytes from SourceBuffer to DestinationBuffer, and returns
  DestinationBuffer.  The implementation must be reentrant, and it must handle the case
  where SourceBuffer overlaps DestinationBuffer.

  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then ASSERT().
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT().

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return None.

**/
STATIC
VOID
EFIAPI
EfiCopyMem (
  IN VOID   *DestinationBuffer,
  IN VOID   *SourceBuffer,
  IN UINTN  Length
  )
{
  CopyMem (
    DestinationBuffer,
    SourceBuffer,
    Length
    );
}

/**
  Fills a target buffer with a byte value, and returns the target buffer.

  This function fills Length bytes of Buffer with Value.

  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer    The memory to set.
  @param  Length    The number of bytes to set.
  @param  Value     The value with which to fill Length bytes of Buffer.

  @return None.

**/
STATIC
VOID
EFIAPI
EfiSetMem (
  IN VOID   *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  )
{
  SetMem (
    Buffer,
    Length,
    Value
    );
}

///
/// Pei service instance
///
EFI_PEI_SERVICES  mPeiServices = {
  {
    PEI_SERVICES_SIGNATURE,
    PEI_SERVICES_REVISION,
    sizeof (EFI_PEI_SERVICES),
    0,
    0
  },
  UnitTestInstallPpi,   // InstallPpi
  UnitTestReInstallPpi, // ReInstallPpi
  UnitTestLocatePpi,    // LocatePpi
  UnitTestNotifyPpi,    // NotifyPpi

  UnitTestGetBootMode,  // GetBootMode
  UnitTestSetBootMode,  // SetBootMode

  UnitTestGetHobList, // GetHobList
  UnitTestCreateHob,  // CreateHob

  UnitTestFfsFindNextVolume,  // FfsFindNextVolume
  UnitTestFfsFindNextFile,    // FfsFindNextFile
  UnitTestFfsFindSectionData, // FfsFindSectionData

  UnitTestInstallPeiMemory, // InstallPeiMemory
  UnitTestPeiAllocatePages, // AllocatePages
  UnitTestPeiAllocatePool,  // AllocatePool
  EfiCopyMem,
  EfiSetMem,

  UnitTestReportStatusCode, // ReportStatusCode
  UnitTestResetSystem,      // ResetSystem

  NULL,  // CpuIo
  NULL,  // PciCfg

  UnitTestFfsFindFileByName,   // FfsFindFileByName
  UnitTestFfsGetFileInfo,      // FfsGetFileInfo
  UnitTestFfsGetVolumeInfo,    // FfsGetVolumeInfo
  UnitTestRegisterForShadow,   // RegisterForShadow
  UnitTestFfsFindSectionData3, // FfsFindSectionData3
  UnitTestFfsGetFileInfo2,     // FfsGetFileInfo2
  UnitTestResetSystem2,        // ResetSystem2
  UnitTestPeiFreePages,        // FreePages
};

PEI_CORE_INSTANCE  mPrivateData;
UINT8              mHobBuffer[MAX_HOB_SIZE];
VOID               *mPeiServicesPointer;

/**
  Clear Buffer For Global Data.
**/
VOID
ClearGlobalData (
  VOID
  )
{
  ZeroMem (&mPrivateData, sizeof (mPrivateData));
  mPrivateData.PpiData.PpiList.MaxCount            = MAX_PPI_COUNT;
  mPrivateData.PpiData.CallbackNotifyList.MaxCount = MAX_PPI_COUNT;
  mPrivateData.PpiData.DispatchNotifyList.MaxCount = MAX_PPI_COUNT;

  ZeroMem (mHobBuffer, MAX_HOB_SIZE);
  mPrivateData.HobList.Raw = mHobBuffer;
  UnitTestCoreBuildHobHandoffInfoTable (0, (EFI_PHYSICAL_ADDRESS)(UINTN)mHobBuffer, MAX_HOB_SIZE);
}

/**
  Resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of ResetData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                            the data buffer starts with a Null-terminated string, optionally
                            followed by additional binary data. The string is a description
                            that the caller may use to further indicate the reason for the
                            system reset.

**/
VOID
EFIAPI
UnitTestResetSystem2 (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  ClearGlobalData ();
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
  mPeiServicesPointer = &mPeiServices;
  return (CONST EFI_PEI_SERVICES **)&mPeiServicesPointer;
}

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
  IN CONST EFI_PEI_SERVICES  **PeiServicesTablePointer
  )
{
  ASSERT (FALSE);
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
  ASSERT (FALSE);
}

/**
  The constructor function init PeiServicesTable with clean buffer.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UnitTestPeiServicesTablePointerLibConstructor (
  VOID
  )
{
  ClearGlobalData ();
  return EFI_SUCCESS;
}
