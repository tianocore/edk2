/** @file
  TdxHelper Functions which are used in SEC phase

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/TdxLib.h>
#include <Library/TdxMailboxLib.h>
#include <Library/SynchronizationLib.h>
#include <Pi/PiHob.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>
#include <Library/TdxHelperLib.h>

#define ALIGNED_2MB_MASK  0x1fffff
#define MEGABYTE_SHIFT    20

#define ACCEPT_CHUNK_SIZE  SIZE_32MB
#define AP_STACK_SIZE      SIZE_16KB
#define APS_STACK_SIZE(CpusNum)  (ALIGN_VALUE(CpusNum*AP_STACK_SIZE, SIZE_2MB))

/**
  Build the GuidHob for tdx measurements which were done in SEC phase.
  The measurement values are stored in WorkArea.

  @retval EFI_SUCCESS  The GuidHob is built successfully
  @retval Others       Other errors as indicated
**/
EFI_STATUS
InternalBuildGuidHobForTdxMeasurement (
  VOID
  );

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
STATIC
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
 * This function is called by BSP and APs to accept memory.
 * Note:
 * The input PhysicalStart/PhysicalEnd indicates the whole memory region
 * to be accepted. BSP or AP only accepts one piece in the whole memory region.
 *
 * @param CpuIndex        vCPU index
 * @param CpusNum         Total vCPU number of a Tdx guest
 * @param PhysicalStart   Start address of a memory region which is to be accepted
 * @param PhysicalEnd     End address of a memory region which is to be accepted
 *
 * @retval EFI_SUCCESS    Successfully accept the memory
 * @retval Other          Other errors as indicated
 */
STATIC
EFI_STATUS
EFIAPI
BspApAcceptMemoryResourceRange (
  UINT32                CpuIndex,
  UINT32                CpusNum,
  EFI_PHYSICAL_ADDRESS  PhysicalStart,
  EFI_PHYSICAL_ADDRESS  PhysicalEnd
  )
{
  UINT64                Status;
  UINT64                Pages;
  UINT64                Stride;
  UINT64                AcceptPageSize;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  AcceptPageSize = (UINT64)(UINTN)FixedPcdGet32 (PcdTdxAcceptPageSize);

  Status          = EFI_SUCCESS;
  Stride          = (UINTN)CpusNum * ACCEPT_CHUNK_SIZE;
  PhysicalAddress = PhysicalStart + ACCEPT_CHUNK_SIZE * (UINTN)CpuIndex;

  while (!EFI_ERROR (Status) && PhysicalAddress < PhysicalEnd) {
    Pages  = MIN (ACCEPT_CHUNK_SIZE, PhysicalEnd - PhysicalAddress) / AcceptPageSize;
    Status = TdAcceptPages (PhysicalAddress, Pages, (UINT32)(UINTN)AcceptPageSize);
    ASSERT (!EFI_ERROR (Status));
    PhysicalAddress += Stride;
  }

  return EFI_SUCCESS;
}

/**
 * This function is called by APs to accept memory.
 *
 * @param CpuIndex        vCPU index of an AP
 * @param PhysicalStart   Start address of a memory region which is to be accepted
 * @param PhysicalEnd     End address of a memory region which is to be accepted
 *
 * @retval EFI_SUCCESS    Successfully accept the memory
 * @retval Others         Other errors as indicated
 */
STATIC
EFI_STATUS
EFIAPI
ApAcceptMemoryResourceRange (
  UINT32                CpuIndex,
  EFI_PHYSICAL_ADDRESS  PhysicalStart,
  EFI_PHYSICAL_ADDRESS  PhysicalEnd
  )
{
  UINT64          Status;
  TD_RETURN_DATA  TdReturnData;

  Status = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
  if (Status != TDX_EXIT_REASON_SUCCESS) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  if ((CpuIndex == 0) || (CpuIndex >= TdReturnData.TdInfo.NumVcpus)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  return BspApAcceptMemoryResourceRange (CpuIndex, TdReturnData.TdInfo.NumVcpus, PhysicalStart, PhysicalEnd);
}

/**
 * This function is called by BSP. It coordinates BSP/APs to accept memory together.
 *
 * @param PhysicalStart     Start address of a memory region which is to be accepted
 * @param PhysicalEnd       End address of a memory region which is to be accepted
 * @param APsStackAddress   APs stack address
 * @param CpusNum           Total vCPU number of the Tdx guest
 *
 * @retval EFI_SUCCESS      Successfully accept the memory
 * @retval Others           Other errors as indicated
 */
STATIC
EFI_STATUS
EFIAPI
MpAcceptMemoryResourceRange (
  IN EFI_PHYSICAL_ADDRESS      PhysicalStart,
  IN EFI_PHYSICAL_ADDRESS      PhysicalEnd,
  IN OUT EFI_PHYSICAL_ADDRESS  APsStackAddress,
  IN UINT32                    CpusNum
  )
{
  UINT64      Length;
  EFI_STATUS  Status;

  Length = PhysicalEnd - PhysicalStart;

  DEBUG ((DEBUG_INFO, "MpAccept : 0x%llx - 0x%llx (0x%llx)\n", PhysicalStart, PhysicalEnd, Length));

  if (Length == 0) {
    return EFI_SUCCESS;
  }

  //
  // The start address is not 2M aligned. BSP first accept the part which is not 2M aligned.
  //
  if (ALIGN_VALUE (PhysicalStart, SIZE_2MB) != PhysicalStart) {
    Length = MIN (ALIGN_VALUE (PhysicalStart, SIZE_2MB) - PhysicalStart, Length);
    Status = BspAcceptMemoryResourceRange (PhysicalStart, PhysicalStart + Length);
    ASSERT (Status == EFI_SUCCESS);

    PhysicalStart += Length;
    Length         = PhysicalEnd - PhysicalStart;
  }

  if (Length == 0) {
    return EFI_SUCCESS;
  }

  //
  // BSP will accept the memory by itself if the memory is not big enough compared with a chunk.
  //
  if (Length <= ACCEPT_CHUNK_SIZE) {
    return BspAcceptMemoryResourceRange (PhysicalStart, PhysicalEnd);
  }

  //
  // Now APs are asked to accept the memory together.
  //
  MpSerializeStart ();

  MpSendWakeupCommand (
    MpProtectedModeWakeupCommandAcceptPages,
    (UINT64)(UINTN)ApAcceptMemoryResourceRange,
    PhysicalStart,
    PhysicalEnd,
    APsStackAddress,
    AP_STACK_SIZE
    );

  //
  // Now BSP does its job.
  //
  BspApAcceptMemoryResourceRange (0, CpusNum, PhysicalStart, PhysicalEnd);

  MpSerializeEnd ();

  return EFI_SUCCESS;
}

/**
  BSP accept a small piece of memory which will be used as APs stack.

  @param[in] VmmHobList    The Hoblist pass the firmware
  @param[in] APsStackSize  APs stack size
  @param[out] PhysicalAddressEnd    The physical end address of accepted memory in phase-1

  @retval  EFI_SUCCESS     Process the HobList successfully
  @retval  Others          Other errors as indicated
**/
STATIC
EFI_STATUS
EFIAPI
AcceptMemoryForAPsStack (
  IN CONST VOID             *VmmHobList,
  IN UINT32                 APsStackSize,
  OUT EFI_PHYSICAL_ADDRESS  *PhysicalAddressEnd
  )
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  PhysicalEnd;
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  UINT64                ResourceLength;
  BOOLEAN               MemoryRegionFound;

  ASSERT (VmmHobList != NULL);

  Status            = EFI_SUCCESS;
  Hob.Raw           = (UINT8 *)VmmHobList;
  MemoryRegionFound = FALSE;

  DEBUG ((DEBUG_INFO, "AcceptMemoryForAPsStack with APsStackSize=0x%x\n", APsStackSize));

  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob) && !MemoryRegionFound) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      DEBUG ((DEBUG_INFO, "\nResourceType: 0x%x\n", Hob.ResourceDescriptor->ResourceType));

      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_UNACCEPTED) {
        ResourceLength = Hob.ResourceDescriptor->ResourceLength;
        PhysicalStart  = Hob.ResourceDescriptor->PhysicalStart;
        PhysicalEnd    = PhysicalStart + ResourceLength;

        DEBUG ((DEBUG_INFO, "ResourceAttribute: 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
        DEBUG ((DEBUG_INFO, "PhysicalStart: 0x%llx\n", PhysicalStart));
        DEBUG ((DEBUG_INFO, "ResourceLength: 0x%llx\n", ResourceLength));
        DEBUG ((DEBUG_INFO, "Owner: %g\n\n", &Hob.ResourceDescriptor->Owner));

        if (ResourceLength >= APsStackSize) {
          MemoryRegionFound = TRUE;
          if (ResourceLength > ACCEPT_CHUNK_SIZE) {
            PhysicalEnd = Hob.ResourceDescriptor->PhysicalStart + APsStackSize;
          }
        }

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

  ASSERT (MemoryRegionFound);
  *PhysicalAddressEnd = PhysicalEnd;

  return Status;
}

/**
  BSP and APs work togeter to accept memory which is under the address of 4G.

  @param[in] VmmHobList           The Hoblist pass the firmware
  @param[in] CpusNum              Number of vCPUs
  @param[in] APsStackStartAddres  Start address of APs stack
  @param[in] PhysicalAddressStart Start physical address which to be accepted

  @retval  EFI_SUCCESS     Process the HobList successfully
  @retval  Others          Other errors as indicated
**/
STATIC
EFI_STATUS
EFIAPI
AcceptMemory (
  IN CONST VOID            *VmmHobList,
  IN UINT32                CpusNum,
  IN EFI_PHYSICAL_ADDRESS  APsStackStartAddress,
  IN EFI_PHYSICAL_ADDRESS  PhysicalAddressStart
  )
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  EFI_PHYSICAL_ADDRESS  PhysicalEnd;
  EFI_PHYSICAL_ADDRESS  AcceptMemoryEndAddress;

  Status                 = EFI_SUCCESS;
  AcceptMemoryEndAddress = BASE_4GB;

  ASSERT (VmmHobList != NULL);
  Hob.Raw = (UINT8 *)VmmHobList;

  DEBUG ((DEBUG_INFO, "AcceptMemory under address of 4G\n"));

  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_UNACCEPTED) {
        PhysicalStart = Hob.ResourceDescriptor->PhysicalStart;
        PhysicalEnd   = PhysicalStart + Hob.ResourceDescriptor->ResourceLength;

        if (PhysicalEnd <= PhysicalAddressStart) {
          // this memory region has been accepted. Skipped it.
          Hob.Raw = GET_NEXT_HOB (Hob);
          continue;
        }

        if (PhysicalStart >= AcceptMemoryEndAddress) {
          // this memory region is not to be accepted. And we're done.
          break;
        }

        if (PhysicalStart >= PhysicalAddressStart) {
          // this memory region has not been acceted.
        } else if ((PhysicalStart < PhysicalAddressStart) && (PhysicalEnd > PhysicalAddressStart)) {
          // part of the memory region has been accepted.
          PhysicalStart = PhysicalAddressStart;
        }

        // then compare the PhysicalEnd with AcceptMemoryEndAddress
        if (PhysicalEnd >= AcceptMemoryEndAddress) {
          PhysicalEnd = AcceptMemoryEndAddress;
        }

        DEBUG ((DEBUG_INFO, "ResourceAttribute: 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
        DEBUG ((DEBUG_INFO, "PhysicalStart: 0x%llx\n", Hob.ResourceDescriptor->PhysicalStart));
        DEBUG ((DEBUG_INFO, "ResourceLength: 0x%llx\n", Hob.ResourceDescriptor->ResourceLength));
        DEBUG ((DEBUG_INFO, "Owner: %g\n\n", &Hob.ResourceDescriptor->Owner));

        // Now we're ready to accept memory [PhysicalStart, PhysicalEnd)
        if (CpusNum == 1) {
          Status = BspAcceptMemoryResourceRange (PhysicalStart, PhysicalEnd);
        } else {
          Status = MpAcceptMemoryResourceRange (
                     PhysicalStart,
                     PhysicalEnd,
                     APsStackStartAddress,
                     CpusNum
                     );
        }

        if (EFI_ERROR (Status)) {
          ASSERT (FALSE);
          break;
        }

        if (PhysicalEnd == AcceptMemoryEndAddress) {
          break;
        }
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
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
STATIC
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
STATIC
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
                                                            EFI_RESOURCE_ATTRIBUTE_ENCRYPTED|
                                                            EFI_RESOURCE_ATTRIBUTE_SPECIAL_PURPOSE |
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
STATIC
EFI_STATUS
EFIAPI
ProcessHobList (
  IN CONST VOID  *VmmHobList
  )
{
  EFI_STATUS            Status;
  UINT32                CpusNum;
  EFI_PHYSICAL_ADDRESS  PhysicalEnd;
  EFI_PHYSICAL_ADDRESS  APsStackStartAddress;

  CpusNum = GetCpusNum ();

  //
  // If there are mutli-vCPU in a TDX guest, accept memory is split into 2 phases.
  // Phase-1 accepts a small piece of memory by BSP. This piece of memory
  // is used to setup AP's stack.
  // After that phase-2 accepts a big piece of memory by BSP/APs.
  //
  // TDVF supports 4K and 2M accept-page-size. The memory which can be accpeted
  // in 2M accept-page-size must be 2M aligned and multiple 2M. So we align
  // APsStackSize to 2M size aligned.
  //
  if (CpusNum > 1) {
    Status = AcceptMemoryForAPsStack (VmmHobList, APS_STACK_SIZE (CpusNum), &PhysicalEnd);
    ASSERT (Status == EFI_SUCCESS);
    APsStackStartAddress = PhysicalEnd - APS_STACK_SIZE (CpusNum);
  } else {
    PhysicalEnd          = 0;
    APsStackStartAddress = 0;
  }

  Status = AcceptMemory (VmmHobList, CpusNum, APsStackStartAddress, PhysicalEnd);
  ASSERT (Status == EFI_SUCCESS);

  return Status;
}

/**
  In Tdx guest, some information need to be passed from host VMM to guest
  firmware. For example, the memory resource, etc. These information are
  prepared by host VMM and put in TdHob which is described in TdxMetadata.
  TDVF processes the TdHob to accept memories.

  @retval   EFI_SUCCESS   Successfully process the TdHob
  @retval   Others        Other error as indicated
**/
EFI_STATUS
EFIAPI
TdxHelperProcessTdHob (
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
 * Calculate the sha384 of input Data and extend it to RTMR register.
 *
 * @param RtmrIndex       Index of the RTMR register
 * @param DataToHash      Data to be hashed
 * @param DataToHashLen   Length of the data
 * @param Digest          Hash value of the input data
 * @param DigestLen       Length of the hash value
 *
 * @retval EFI_SUCCESS    Successfully hash and extend to RTMR
 * @retval Others         Other errors as indicated
 */
STATIC
EFI_STATUS
HashAndExtendToRtmr (
  IN UINT32  RtmrIndex,
  IN VOID    *DataToHash,
  IN UINTN   DataToHashLen,
  OUT UINT8  *Digest,
  IN  UINTN  DigestLen
  )
{
  EFI_STATUS  Status;

  if ((DataToHash == NULL) || (DataToHashLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Digest == NULL) || (DigestLen != SHA384_DIGEST_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the sha384 of the data
  //
  if (!Sha384HashAll (DataToHash, DataToHashLen, Digest)) {
    return EFI_ABORTED;
  }

  //
  // Extend to RTMR
  //
  Status = TdExtendRtmr (
             (UINT32 *)Digest,
             SHA384_DIGEST_SIZE,
             (UINT8)RtmrIndex
             );

  ASSERT (!EFI_ERROR (Status));
  return Status;
}

/**
  In Tdx guest, TdHob is passed from host VMM to guest firmware and it contains
  the information of the memory resource. From the security perspective before
  it is consumed, it should be measured and extended.
 *
 * @retval EFI_SUCCESS Successfully measure the TdHob
 * @retval Others      Other error as indicated
 */
EFI_STATUS
EFIAPI
TdxHelperMeasureTdHob (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_STATUS            Status;
  UINT8                 Digest[SHA384_DIGEST_SIZE];
  OVMF_WORK_AREA        *WorkArea;
  VOID                  *TdHob;

  TdHob   = (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  Hob.Raw = (UINT8 *)TdHob;

  //
  // Walk thru the TdHob list until end of list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  Status = HashAndExtendToRtmr (
             0,
             (UINT8 *)TdHob,
             (UINTN)((UINT8 *)Hob.Raw - (UINT8 *)TdHob),
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // This function is called in SEC phase and at that moment the Hob service
  // is not available. So the TdHob measurement value is stored in workarea.
  //
  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);
  if (WorkArea == NULL) {
    return EFI_DEVICE_ERROR;
  }

  WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.MeasurementsBitmap |= TDX_MEASUREMENT_TDHOB_BITMASK;
  CopyMem (WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.TdHobHashValue, Digest, SHA384_DIGEST_SIZE);

  return EFI_SUCCESS;
}

/**
 * In Tdx guest, Configuration FV (CFV) is treated as external input because it
 * may contain the data provided by VMM. From the sucurity perspective Cfv image
 * should be measured before it is consumed.
 *
 * @retval EFI_SUCCESS Successfully measure the CFV image
 * @retval Others      Other error as indicated
 */
EFI_STATUS
EFIAPI
TdxHelperMeasureCfvImage (
  VOID
  )
{
  EFI_STATUS      Status;
  UINT8           Digest[SHA384_DIGEST_SIZE];
  OVMF_WORK_AREA  *WorkArea;

  Status = HashAndExtendToRtmr (
             0,
             (UINT8 *)(UINTN)PcdGet32 (PcdOvmfFlashNvStorageVariableBase),
             (UINT64)PcdGet32 (PcdCfvRawDataSize),
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // This function is called in SEC phase and at that moment the Hob service
  // is not available. So CfvImage measurement value is stored in workarea.
  //
  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);
  if (WorkArea == NULL) {
    return EFI_DEVICE_ERROR;
  }

  WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.MeasurementsBitmap |= TDX_MEASUREMENT_CFVIMG_BITMASK;
  CopyMem (WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.CfvImgHashValue, Digest, SHA384_DIGEST_SIZE);

  return EFI_SUCCESS;
}

/**
  Build the GuidHob for tdx measurements which were done in SEC phase.
  The measurement values are stored in WorkArea.

  @retval EFI_SUCCESS  The GuidHob is built successfully
  @retval Others       Other errors as indicated
**/
EFI_STATUS
EFIAPI
TdxHelperBuildGuidHobForTdxMeasurement (
  VOID
  )
{
 #ifdef TDX_PEI_LESS_BOOT
  return InternalBuildGuidHobForTdxMeasurement ();
 #else
  return EFI_UNSUPPORTED;
 #endif
}
