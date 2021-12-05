/** @file
  Legacy Region Support

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LegacyRegion.h"

//
// 440/Q35 PAM map.
//
// PAM Range          Offset    Bits  Operation
//                  440   Q35
// ===============  ====  ====  ====  ===============================================================
// 0xC0000-0xC3FFF  0x5a  0x91  1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC4000-0xC7FFF  0x5a  0x91  5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC8000-0xCBFFF  0x5b  0x92  1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xCC000-0xCFFFF  0x5b  0x92  5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD0000-0xD3FFF  0x5c  0x93  1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD4000-0xD7FFF  0x5c  0x93  5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD8000-0xDBFFF  0x5d  0x94  1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xDC000-0xDFFFF  0x5d  0x94  5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE0000-0xE3FFF  0x5e  0x95  1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE4000-0xE7FFF  0x5e  0x95  5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE8000-0xEBFFF  0x5f  0x96  1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xEC000-0xEFFFF  0x5f  0x96  5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xF0000-0xFFFFF  0x59  0x90  5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
//
STATIC LEGACY_MEMORY_SECTION_INFO  mSectionArray[] = {
  { 0xC0000, SIZE_16KB, FALSE, FALSE },
  { 0xC4000, SIZE_16KB, FALSE, FALSE },
  { 0xC8000, SIZE_16KB, FALSE, FALSE },
  { 0xCC000, SIZE_16KB, FALSE, FALSE },
  { 0xD0000, SIZE_16KB, FALSE, FALSE },
  { 0xD4000, SIZE_16KB, FALSE, FALSE },
  { 0xD8000, SIZE_16KB, FALSE, FALSE },
  { 0xDC000, SIZE_16KB, FALSE, FALSE },
  { 0xE0000, SIZE_16KB, FALSE, FALSE },
  { 0xE4000, SIZE_16KB, FALSE, FALSE },
  { 0xE8000, SIZE_16KB, FALSE, FALSE },
  { 0xEC000, SIZE_16KB, FALSE, FALSE },
  { 0xF0000, SIZE_64KB, FALSE, FALSE }
};

STATIC PAM_REGISTER_VALUE  mRegisterValues440[] = {
  { PMC_REGISTER_PIIX4 (PIIX4_PAM1), 0x01, 0x02 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM1), 0x10, 0x20 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM2), 0x01, 0x02 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM2), 0x10, 0x20 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM3), 0x01, 0x02 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM3), 0x10, 0x20 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM4), 0x01, 0x02 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM4), 0x10, 0x20 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM5), 0x01, 0x02 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM5), 0x10, 0x20 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM6), 0x01, 0x02 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM6), 0x10, 0x20 },
  { PMC_REGISTER_PIIX4 (PIIX4_PAM0), 0x10, 0x20 }
};

STATIC PAM_REGISTER_VALUE  mRegisterValuesQ35[] = {
  { DRAMC_REGISTER_Q35 (MCH_PAM1), 0x01, 0x02 },
  { DRAMC_REGISTER_Q35 (MCH_PAM1), 0x10, 0x20 },
  { DRAMC_REGISTER_Q35 (MCH_PAM2), 0x01, 0x02 },
  { DRAMC_REGISTER_Q35 (MCH_PAM2), 0x10, 0x20 },
  { DRAMC_REGISTER_Q35 (MCH_PAM3), 0x01, 0x02 },
  { DRAMC_REGISTER_Q35 (MCH_PAM3), 0x10, 0x20 },
  { DRAMC_REGISTER_Q35 (MCH_PAM4), 0x01, 0x02 },
  { DRAMC_REGISTER_Q35 (MCH_PAM4), 0x10, 0x20 },
  { DRAMC_REGISTER_Q35 (MCH_PAM5), 0x01, 0x02 },
  { DRAMC_REGISTER_Q35 (MCH_PAM5), 0x10, 0x20 },
  { DRAMC_REGISTER_Q35 (MCH_PAM6), 0x01, 0x02 },
  { DRAMC_REGISTER_Q35 (MCH_PAM6), 0x10, 0x20 },
  { DRAMC_REGISTER_Q35 (MCH_PAM0), 0x10, 0x20 }
};

STATIC PAM_REGISTER_VALUE  *mRegisterValues;

//
// Handle used to install the Legacy Region Protocol
//
STATIC EFI_HANDLE  mHandle = NULL;

//
// Instance of the Legacy Region Protocol to install into the handle database
//
STATIC EFI_LEGACY_REGION2_PROTOCOL  mLegacyRegion2 = {
  LegacyRegion2Decode,
  LegacyRegion2Lock,
  LegacyRegion2BootLock,
  LegacyRegion2Unlock,
  LegacyRegionGetInfo
};

STATIC
EFI_STATUS
LegacyRegionManipulationInternal (
  IN  UINT32   Start,
  IN  UINT32   Length,
  IN  BOOLEAN  *ReadEnable,
  IN  BOOLEAN  *WriteEnable,
  OUT UINT32   *Granularity
  )
{
  UINT32  EndAddress;
  UINTN   Index;
  UINTN   StartIndex;

  //
  // Validate input parameters.
  //
  if ((Length == 0) || (Granularity == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  EndAddress = Start + Length - 1;
  if ((Start < PAM_BASE_ADDRESS) || (EndAddress > PAM_LIMIT_ADDRESS)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Loop to find the start PAM.
  //
  StartIndex = 0;
  for (Index = 0; Index < ARRAY_SIZE (mSectionArray); Index++) {
    if ((Start >= mSectionArray[Index].Start) && (Start < (mSectionArray[Index].Start + mSectionArray[Index].Length))) {
      StartIndex = Index;
      break;
    }
  }

  ASSERT (Index < ARRAY_SIZE (mSectionArray));

  //
  // Program PAM until end PAM is encountered
  //
  for (Index = StartIndex; Index < ARRAY_SIZE (mSectionArray); Index++) {
    if (ReadEnable != NULL) {
      if (*ReadEnable) {
        PciOr8 (
          mRegisterValues[Index].PAMRegPciLibAddress,
          mRegisterValues[Index].ReadEnableData
          );
      } else {
        PciAnd8 (
          mRegisterValues[Index].PAMRegPciLibAddress,
          (UINT8)(~mRegisterValues[Index].ReadEnableData)
          );
      }
    }

    if (WriteEnable != NULL) {
      if (*WriteEnable) {
        PciOr8 (
          mRegisterValues[Index].PAMRegPciLibAddress,
          mRegisterValues[Index].WriteEnableData
          );
      } else {
        PciAnd8 (
          mRegisterValues[Index].PAMRegPciLibAddress,
          (UINT8)(~mRegisterValues[Index].WriteEnableData)
          );
      }
    }

    //
    // If the end PAM is encountered, record its length as granularity and jump out.
    //
    if ((EndAddress >= mSectionArray[Index].Start) && (EndAddress < (mSectionArray[Index].Start + mSectionArray[Index].Length))) {
      *Granularity = mSectionArray[Index].Length;
      break;
    }
  }

  ASSERT (Index < ARRAY_SIZE (mSectionArray));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
LegacyRegionGetInfoInternal (
  OUT UINT32                      *DescriptorCount,
  OUT LEGACY_MEMORY_SECTION_INFO  **Descriptor
  )
{
  UINTN  Index;
  UINT8  PamValue;

  //
  // Check input parameters
  //
  if ((DescriptorCount == NULL) || (Descriptor == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Fill in current status of legacy region.
  //
  *DescriptorCount = sizeof (mSectionArray) / sizeof (mSectionArray[0]);
  for (Index = 0; Index < *DescriptorCount; Index++) {
    PamValue                         = PciRead8 (mRegisterValues[Index].PAMRegPciLibAddress);
    mSectionArray[Index].ReadEnabled = FALSE;
    if ((PamValue & mRegisterValues[Index].ReadEnableData) != 0) {
      mSectionArray[Index].ReadEnabled = TRUE;
    }

    mSectionArray[Index].WriteEnabled = FALSE;
    if ((PamValue & mRegisterValues[Index].WriteEnableData) != 0) {
      mSectionArray[Index].WriteEnabled = TRUE;
    }
  }

  *Descriptor = mSectionArray;
  return EFI_SUCCESS;
}

/**
  Modify the hardware to allow (decode) or disallow (not decode) memory reads in a region.

  If the On parameter evaluates to TRUE, this function enables memory reads in the address range
  Start to (Start + Length - 1).
  If the On parameter evaluates to FALSE, this function disables memory reads in the address range
  Start to (Start + Length - 1).

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose attributes
                                should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address
                                was not aligned to a region's starting address or if the length
                                was greater than the number of bytes in the first region.
  @param  On[in]                Decode / Non-Decode flag.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Decode (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity,
  IN  BOOLEAN                      *On
  )
{
  return LegacyRegionManipulationInternal (Start, Length, On, NULL, Granularity);
}

/**
  Modify the hardware to disallow memory attribute changes in a region.

  This function makes the attributes of a region read only. Once a region is boot-locked with this
  function, the read and write attributes of that region cannot be changed until a power cycle has
  reset the boot-lock attribute. Calls to Decode(), Lock() and Unlock() will have no effect.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.
  @retval EFI_UNSUPPORTED       The chipset does not support locking the configuration registers in
                                a way that will not affect memory regions outside the legacy memory
                                region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2BootLock (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity
  )
{
  if ((Start < 0xC0000) || ((Start + Length - 1) > 0xFFFFF)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_UNSUPPORTED;
}

/**
  Modify the hardware to disallow memory writes in a region.

  This function changes the attributes of a memory range to not allow writes.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Lock (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity
  )
{
  BOOLEAN  WriteEnable;

  WriteEnable = FALSE;
  return LegacyRegionManipulationInternal (Start, Length, NULL, &WriteEnable, Granularity);
}

/**
  Modify the hardware to allow memory writes in a region.

  This function changes the attributes of a memory range to allow writes.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Unlock (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity
  )
{
  BOOLEAN  WriteEnable;

  WriteEnable = TRUE;
  return LegacyRegionManipulationInternal (Start, Length, NULL, &WriteEnable, Granularity);
}

/**
  Get region information for the attributes of the Legacy Region.

  This function is used to discover the granularity of the attributes for the memory in the legacy
  region. Each attribute may have a different granularity and the granularity may not be the same
  for all memory ranges in the legacy region.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  DescriptorCount[out]  The number of region descriptor entries returned in the Descriptor
                                buffer.
  @param  Descriptor[out]       A pointer to a pointer used to return a buffer where the legacy
                                region information is deposited. This buffer will contain a list of
                                DescriptorCount number of region descriptors.  This function will
                                provide the memory for the buffer.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegionGetInfo (
  IN  EFI_LEGACY_REGION2_PROTOCOL   *This,
  OUT UINT32                        *DescriptorCount,
  OUT EFI_LEGACY_REGION_DESCRIPTOR  **Descriptor
  )
{
  LEGACY_MEMORY_SECTION_INFO    *SectionInfo;
  UINT32                        SectionCount;
  EFI_LEGACY_REGION_DESCRIPTOR  *DescriptorArray;
  UINTN                         Index;
  UINTN                         DescriptorIndex;

  //
  // Get section numbers and information
  //
  LegacyRegionGetInfoInternal (&SectionCount, &SectionInfo);

  //
  // Each section has 3 descriptors, corresponding to readability, writeability, and lock status.
  //
  DescriptorArray = AllocatePool (sizeof (EFI_LEGACY_REGION_DESCRIPTOR) * SectionCount * 3);
  if (DescriptorArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DescriptorIndex = 0;
  for (Index = 0; Index < SectionCount; Index++) {
    DescriptorArray[DescriptorIndex].Start       = SectionInfo[Index].Start;
    DescriptorArray[DescriptorIndex].Length      = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Granularity = SectionInfo[Index].Length;
    if (SectionInfo[Index].ReadEnabled) {
      DescriptorArray[DescriptorIndex].Attribute = LegacyRegionDecoded;
    } else {
      DescriptorArray[DescriptorIndex].Attribute = LegacyRegionNotDecoded;
    }

    DescriptorIndex++;

    //
    // Create descriptor for writeability, according to lock status
    //
    DescriptorArray[DescriptorIndex].Start       = SectionInfo[Index].Start;
    DescriptorArray[DescriptorIndex].Length      = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Granularity = SectionInfo[Index].Length;
    if (SectionInfo[Index].WriteEnabled) {
      DescriptorArray[DescriptorIndex].Attribute = LegacyRegionWriteEnabled;
    } else {
      DescriptorArray[DescriptorIndex].Attribute = LegacyRegionWriteDisabled;
    }

    DescriptorIndex++;

    //
    // Chipset does not support bootlock.
    //
    DescriptorArray[DescriptorIndex].Start       = SectionInfo[Index].Start;
    DescriptorArray[DescriptorIndex].Length      = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Granularity = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Attribute   = LegacyRegionNotLocked;
    DescriptorIndex++;
  }

  *DescriptorCount = (UINT32)DescriptorIndex;
  *Descriptor      = DescriptorArray;

  return EFI_SUCCESS;
}

/**
  Initialize Legacy Region support

  @retval EFI_SUCCESS   Successfully initialized

**/
EFI_STATUS
LegacyRegionInit (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      HostBridgeDevId;

  //
  // Query Host Bridge DID to determine platform type
  //
  HostBridgeDevId = PcdGet16 (PcdOvmfHostBridgePciDevId);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      mRegisterValues = mRegisterValues440;
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      mRegisterValues = mRegisterValuesQ35;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__,
        HostBridgeDevId
        ));
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
  }

  //
  // Install the Legacy Region Protocol on a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiLegacyRegion2ProtocolGuid,
                  &mLegacyRegion2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
