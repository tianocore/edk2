/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/QemuFwCfg.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/TdxLib.h>
#include <Library/SynchronizationLib.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>

#define ALIGNED_2MB_MASK                0x1fffff
#define EFI_RESOURCE_MEMORY_UNACCEPTED  7

/**
  This function will be called to accept pages. Only BSP accepts pages.

  TDCALL(ACCEPT_PAGE) supports the accept page size of 4k and 2M. To
  simplify the implementation, the Memory to be accpeted is splitted
  into 3 parts:
  -----------------  <-- StartAddress1 (not 2M aligned)
  |  part 1       |      Length1 < 2M
  |---------------|  <-- StartAddress2 (2M aligned)
  |               |      Length2 = Integer multiples of 2M
  |  part 2       |
  |               |
  |---------------|  <-- StartAddress3
  |  part 3       |      Length3 < 2M
  |---------------|

  @param[in] PhysicalAddress   Start physical adress
  @param[in] PhysicalEnd       End physical address

  @retval    EFI_SUCCESS       Accept memory successfully
  @retval    Others            Other errors as indicated
**/
EFI_STATUS
EFIAPI
BspAcceptMemoryResourceRange (
  IN EFI_PHYSICAL_ADDRESS  PhysicalAddress,
  IN EFI_PHYSICAL_ADDRESS  PhysicalEnd
  )
{
  EFI_STATUS  Status;
  UINT32      AcceptPageSize;
  UINT64      StartAddress1;
  UINT64      StartAddress2;
  UINT64      StartAddress3;
  UINT64      TotalLength;
  UINT64      Length1;
  UINT64      Length2;
  UINT64      Length3;
  UINT64      Pages;

  AcceptPageSize = FixedPcdGet32 (PcdTdxAcceptPageSize);
  TotalLength    = PhysicalEnd - PhysicalAddress;
  StartAddress1  = 0;
  StartAddress2  = 0;
  StartAddress3  = 0;
  Length1        = 0;
  Length2        = 0;
  Length3        = 0;

  if (TotalLength == 0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "TdAccept: 0x%llx - 0x%llx\n", PhysicalAddress, TotalLength));

  if (ALIGN_VALUE (PhysicalAddress, SIZE_2MB) != PhysicalAddress) {
    StartAddress1 = PhysicalAddress;
    Length1       = ALIGN_VALUE (PhysicalAddress, SIZE_2MB) - PhysicalAddress;
    if (Length1 >= TotalLength) {
      Length1 = TotalLength;
    }

    PhysicalAddress += Length1;
    TotalLength     -= Length1;
  }

  if (TotalLength > SIZE_2MB) {
    StartAddress2    = PhysicalAddress;
    Length2          = TotalLength & ~(UINT64)ALIGNED_2MB_MASK;
    PhysicalAddress += Length2;
    TotalLength     -= Length2;
  }

  if (TotalLength) {
    StartAddress3 = PhysicalAddress;
    Length3       = TotalLength;
  }

  DEBUG ((DEBUG_INFO, "   Part1: 0x%llx - 0x%llx\n", StartAddress1, Length1));
  DEBUG ((DEBUG_INFO, "   Part2: 0x%llx - 0x%llx\n", StartAddress2, Length2));
  DEBUG ((DEBUG_INFO, "   Part3: 0x%llx - 0x%llx\n", StartAddress3, Length3));
  DEBUG ((DEBUG_INFO, "   Page : 0x%x\n", AcceptPageSize));

  Status = EFI_SUCCESS;
  if (Length1 > 0) {
    Pages  = Length1 / SIZE_4KB;
    Status = TdAcceptPages (StartAddress1, Pages, SIZE_4KB);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Length2 > 0) {
    Pages  = Length2 / AcceptPageSize;
    Status = TdAcceptPages (StartAddress2, Pages, AcceptPageSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Length3 > 0) {
    Pages  = Length3 / SIZE_4KB;
    Status = TdAcceptPages (StartAddress3, Pages, SIZE_4KB);
    ASSERT (!EFI_ERROR (Status));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}

/**
  Check the value whether in the valid list.

  @param[in] Value             A value
  @param[in] ValidList         A pointer to valid list
  @param[in] ValidListLength   Length of valid list

  @retval  TRUE   The value is in valid list.
  @retval  FALSE  The value is not in valid list.

**/
BOOLEAN
EFIAPI
IsInValidList (
  IN UINT32  Value,
  IN UINT32  *ValidList,
  IN UINT32  ValidListLength
  )
{
  UINT32  index;

  if (ValidList == NULL) {
    return FALSE;
  }

  for (index = 0; index < ValidListLength; index++) {
    if (ValidList[index] == Value) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Check the integrity of VMM Hob List.

  @param[in] VmmHobList   A pointer to Hob List

  @retval  TRUE     The Hob List is valid.
  @retval  FALSE    The Hob List is invalid.

**/
BOOLEAN
EFIAPI
ValidateHobList (
  IN CONST VOID  *VmmHobList
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINT32                EFI_BOOT_MODE_LIST[] = {
    BOOT_WITH_FULL_CONFIGURATION,
    BOOT_WITH_MINIMAL_CONFIGURATION,
    BOOT_ASSUMING_NO_CONFIGURATION_CHANGES,
    BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS,
    BOOT_WITH_DEFAULT_SETTINGS,
    BOOT_ON_S4_RESUME,
    BOOT_ON_S5_RESUME,
    BOOT_WITH_MFG_MODE_SETTINGS,
    BOOT_ON_S2_RESUME,
    BOOT_ON_S3_RESUME,
    BOOT_ON_FLASH_UPDATE,
    BOOT_IN_RECOVERY_MODE
  };

  UINT32  EFI_RESOURCE_TYPE_LIST[] = {
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    EFI_RESOURCE_IO,
    EFI_RESOURCE_FIRMWARE_DEVICE,
    EFI_RESOURCE_MEMORY_MAPPED_IO_PORT,
    EFI_RESOURCE_MEMORY_RESERVED,
    EFI_RESOURCE_IO_RESERVED,
    EFI_RESOURCE_MEMORY_UNACCEPTED
  };

  if (VmmHobList == NULL) {
    DEBUG ((DEBUG_ERROR, "HOB: HOB data pointer is NULL\n"));
    return FALSE;
  }

  Hob.Raw = (UINT8 *)VmmHobList;

  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->Reserved != (UINT32)0) {
      DEBUG ((DEBUG_ERROR, "HOB: Hob header Reserved filed should be zero\n"));
      return FALSE;
    }

    if (Hob.Header->HobLength == 0) {
      DEBUG ((DEBUG_ERROR, "HOB: Hob header LEANGTH should not be zero\n"));
      return FALSE;
    }

    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_HANDOFF:
        if (Hob.Header->HobLength != sizeof (EFI_HOB_HANDOFF_INFO_TABLE)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_HANDOFF));
          return FALSE;
        }

        if (IsInValidList (Hob.HandoffInformationTable->BootMode, EFI_BOOT_MODE_LIST, ARRAY_SIZE (EFI_BOOT_MODE_LIST)) == FALSE) {
          DEBUG ((DEBUG_ERROR, "HOB: Unknow HandoffInformationTable BootMode type. Type: 0x%08x\n", Hob.HandoffInformationTable->BootMode));
          return FALSE;
        }

        if ((Hob.HandoffInformationTable->EfiFreeMemoryTop % 4096) != 0) {
          DEBUG ((DEBUG_ERROR, "HOB: HandoffInformationTable EfiFreeMemoryTop address must be 4-KB aligned to meet page restrictions of UEFI.\
                               Address: 0x%016lx\n", Hob.HandoffInformationTable->EfiFreeMemoryTop));
          return FALSE;
        }

        break;

      case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
        if (Hob.Header->HobLength != sizeof (EFI_HOB_RESOURCE_DESCRIPTOR)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_RESOURCE_DESCRIPTOR));
          return FALSE;
        }

        if (IsInValidList (Hob.ResourceDescriptor->ResourceType, EFI_RESOURCE_TYPE_LIST, ARRAY_SIZE (EFI_RESOURCE_TYPE_LIST)) == FALSE) {
          DEBUG ((DEBUG_ERROR, "HOB: Unknow ResourceDescriptor ResourceType type. Type: 0x%08x\n", Hob.ResourceDescriptor->ResourceType));
          return FALSE;
        }

        if ((Hob.ResourceDescriptor->ResourceAttribute & (~(EFI_RESOURCE_ATTRIBUTE_PRESENT |
                                                            EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                                                            EFI_RESOURCE_ATTRIBUTE_TESTED |
                                                            EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED |
                                                            EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED |
                                                            EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED |
                                                            EFI_RESOURCE_ATTRIBUTE_PERSISTENT |
                                                            EFI_RESOURCE_ATTRIBUTE_SINGLE_BIT_ECC |
                                                            EFI_RESOURCE_ATTRIBUTE_MULTIPLE_BIT_ECC |
                                                            EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_1 |
                                                            EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_2 |
                                                            EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_16_BIT_IO |
                                                            EFI_RESOURCE_ATTRIBUTE_32_BIT_IO |
                                                            EFI_RESOURCE_ATTRIBUTE_64_BIT_IO |
                                                            EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED |
                                                            EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_PERSISTABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED |
                                                            EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE |
                                                            EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE))) != 0)
        {
          DEBUG ((DEBUG_ERROR, "HOB: Unknow ResourceDescriptor ResourceAttribute type. Type: 0x%08x\n", Hob.ResourceDescriptor->ResourceAttribute));
          return FALSE;
        }

        break;

      // EFI_HOB_GUID_TYPE is variable length data, so skip check
      case EFI_HOB_TYPE_GUID_EXTENSION:
        break;

      case EFI_HOB_TYPE_FV:
        if (Hob.Header->HobLength != sizeof (EFI_HOB_FIRMWARE_VOLUME)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_FV));
          return FALSE;
        }

        break;

      case EFI_HOB_TYPE_FV2:
        if (Hob.Header->HobLength != sizeof (EFI_HOB_FIRMWARE_VOLUME2)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_FV2));
          return FALSE;
        }

        break;

      case EFI_HOB_TYPE_FV3:
        if (Hob.Header->HobLength != sizeof (EFI_HOB_FIRMWARE_VOLUME3)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_FV3));
          return FALSE;
        }

        break;

      case EFI_HOB_TYPE_CPU:
        if (Hob.Header->HobLength != sizeof (EFI_HOB_CPU)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_CPU));
          return FALSE;
        }

        for (UINT32 index = 0; index < 6; index++) {
          if (Hob.Cpu->Reserved[index] != 0) {
            DEBUG ((DEBUG_ERROR, "HOB: Cpu Reserved field will always be set to zero.\n"));
            return FALSE;
          }
        }

        break;

      default:
        DEBUG ((DEBUG_ERROR, "HOB: Hob type is not know. Type: 0x%04x\n", Hob.Header->HobType));
        return FALSE;
    }

    // Get next HOB
    Hob.Raw = (UINT8 *)(Hob.Raw + Hob.Header->HobLength);
  }

  return TRUE;
}

/**
  Processing the incoming HobList for the TDX

  Firmware must parse list, and accept the pages of memory before their can be
  use by the guest.

  @param[in] VmmHobList    The Hoblist pass the firmware

  @retval  EFI_SUCCESS     Process the HobList successfully
  @retval  Others          Other errors as indicated

**/
EFI_STATUS
EFIAPI
ProcessHobList (
  IN CONST VOID  *VmmHobList
  )
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  PhysicalEnd;

  Status = EFI_SUCCESS;
  ASSERT (VmmHobList != NULL);
  Hob.Raw = (UINT8 *)VmmHobList;

  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      DEBUG ((DEBUG_INFO, "\nResourceType: 0x%x\n", Hob.ResourceDescriptor->ResourceType));

      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_UNACCEPTED) {
        DEBUG ((DEBUG_INFO, "ResourceAttribute: 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
        DEBUG ((DEBUG_INFO, "PhysicalStart: 0x%llx\n", Hob.ResourceDescriptor->PhysicalStart));
        DEBUG ((DEBUG_INFO, "ResourceLength: 0x%llx\n", Hob.ResourceDescriptor->ResourceLength));
        DEBUG ((DEBUG_INFO, "Owner: %g\n\n", &Hob.ResourceDescriptor->Owner));

        PhysicalEnd = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;

        Status = BspAcceptMemoryResourceRange (
                   Hob.ResourceDescriptor->PhysicalStart,
                   PhysicalEnd
                   );
        if (EFI_ERROR (Status)) {
          break;
        }
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return Status;
}

/**
  In Tdx guest, some information need to be passed from host VMM to guest
  firmware. For example, the memory resource, etc. These information are
  prepared by host VMM and put in HobList which is described in TdxMetadata.

  Information in HobList is treated as external input. From the security
  perspective before it is consumed, it should be validated.

  @retval   EFI_SUCCESS   Successfully process the hoblist
  @retval   Others        Other error as indicated
**/
EFI_STATUS
EFIAPI
ProcessTdxHobList (
  VOID
  )
{
  EFI_STATUS      Status;
  VOID            *TdHob;
  TD_RETURN_DATA  TdReturnData;

  TdHob  = (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  Status = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "Intel Tdx Started with (GPAW: %d, Cpus: %d)\n",
    TdReturnData.TdInfo.Gpaw,
    TdReturnData.TdInfo.NumVcpus
    ));

  //
  // Validate HobList
  //
  if (ValidateHobList (TdHob) == FALSE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Process Hoblist to accept memory
  //
  Status = ProcessHobList (TdHob);

  return Status;
}

/**
  Transfer the incoming HobList for the TD to the final HobList for Dxe.
  The Hobs transferred in this function are ResourceDescriptor hob and
  MemoryAllocation hob.

  @param[in] VmmHobList    The Hoblist pass the firmware

**/
VOID
EFIAPI
TransferTdxHobList (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_RESOURCE_TYPE            ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;

  //
  // PcdOvmfSecGhcbBase is used as the TD_HOB in Tdx guest.
  //
  Hob.Raw = (UINT8 *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  while (!END_OF_HOB_LIST (Hob)) {
    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
        ResourceType      = Hob.ResourceDescriptor->ResourceType;
        ResourceAttribute = Hob.ResourceDescriptor->ResourceAttribute;

        if (ResourceType == EFI_RESOURCE_MEMORY_UNACCEPTED) {
          ResourceType       = EFI_RESOURCE_SYSTEM_MEMORY;
          ResourceAttribute |= (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED);
        }

        BuildResourceDescriptorHob (
          ResourceType,
          ResourceAttribute,
          Hob.ResourceDescriptor->PhysicalStart,
          Hob.ResourceDescriptor->ResourceLength
          );
        break;
      case EFI_HOB_TYPE_MEMORY_ALLOCATION:
        BuildMemoryAllocationHob (
          Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
          Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
          Hob.MemoryAllocation->AllocDescriptor.MemoryType
          );
        break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
}

/**
  In Tdx guest, the system memory is passed in TdHob by host VMM. So
  the major task of PlatformTdxPublishRamRegions is to walk thru the
  TdHob list and transfer the ResourceDescriptorHob and MemoryAllocationHob
  to the hobs in DXE phase.

  MemoryAllocationHob should also be created for Mailbox and Ovmf work area.
**/
VOID
EFIAPI
PlatformTdxPublishRamRegions (
  VOID
  )
{
  if (!TdIsEnabled ()) {
    return;
  }

  TransferTdxHobList ();

  //
  // The memory region defined by PcdOvmfSecGhcbBackupBase is pre-allocated by
  // host VMM and used as the td mailbox at the beginning of system boot.
  //
  BuildMemoryAllocationHob (
    FixedPcdGet32 (PcdOvmfSecGhcbBackupBase),
    FixedPcdGet32 (PcdOvmfSecGhcbBackupSize),
    EfiACPIMemoryNVS
    );

  if (FixedPcdGet32 (PcdOvmfWorkAreaSize) != 0) {
    //
    // Reserve the work area.
    //
    // Since this memory range will be used by the Reset Vector on S3
    // resume, it must be reserved as ACPI NVS.
    //
    // If S3 is unsupported, then various drivers might still write to the
    // work area. We ought to prevent DXE from serving allocation requests
    // such that they would overlap the work area.
    //
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaBase),
      (UINT64)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaSize),
      EfiBootServicesData
      );
  }
}
