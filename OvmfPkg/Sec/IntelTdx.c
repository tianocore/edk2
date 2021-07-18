/** @file

  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TdxLib.h>
#include <Library/TdxMailboxLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include "IntelTdx.h"

#define ALIGNED_2MB_MASK 0x1fffff

/**
  BSP call this function to accept memory in a range.

  @param[in]  StartAddress      Start address of the memory region
  @param[in]  Length            Length of the memory region
  @param[in]  AcceptChunkSize   Accept chunk size
  @param[in]  AcceptPageSize    Accept page size
  @retval     EFI_SUCCESS       Successfully accept the memory region
  @retval     Others            Indicate the other errors
**/
EFI_STATUS
EFIAPI
BspAcceptMemoryResourceRange (
  IN EFI_PHYSICAL_ADDRESS   StartAddress,
  IN UINT64                 Length,
  IN UINT64                 AcceptChunkSize,
  IN UINT64                 AcceptPageSize
  )
{
  EFI_STATUS                  Status;
  UINT64                      Pages;
  UINT64                      Stride;
  EFI_PHYSICAL_ADDRESS        PhysicalAddress;
  volatile MP_WAKEUP_MAILBOX  *MailBox;

  Status = EFI_SUCCESS;
  PhysicalAddress = StartAddress;
  Stride = GetCpusNum () * AcceptChunkSize;
  MailBox = (volatile MP_WAKEUP_MAILBOX *) GetTdxMailBox ();

  while (!EFI_ERROR(Status) && PhysicalAddress < StartAddress + Length) {
    //
    // Decrease size of near end of resource if needed.
    //
    Pages = MIN (AcceptChunkSize, StartAddress + Length - PhysicalAddress) / AcceptPageSize;

    MailBox->Tallies[0] += (UINT32)Pages;

    Status = TdAcceptPages (PhysicalAddress, Pages, AcceptPageSize);
    //
    // Bump address to next chunk this cpu is responisble for
    //
    PhysicalAddress += Stride;
  }

  return Status;
}

/**
  This function will be called to accept pages. BSP and APs are invokded
  to do the task together.

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

  part 1) will be accepted in 4k and by BSP.
  Part 2) will be accepted in 2M and by BSP/AP.
  Part 3) will be accepted in 4k and by BSP.

  @param[in] PhysicalAddress   Start physical adress
  @param[in] PhysicalEnd       End physical address

  @retval    EFI_SUCCESS       Accept memory successfully
  @retval    Others            Other errors as indicated
**/
EFI_STATUS
EFIAPI
MpAcceptMemoryResourceRange (
  IN EFI_PHYSICAL_ADDRESS        PhysicalAddress,
  IN EFI_PHYSICAL_ADDRESS        PhysicalEnd
  )
{
  EFI_STATUS                  Status;
  UINT64                      AcceptChunkSize;
  UINT64                      AcceptPageSize;
  UINT64                      StartAddress1;
  UINT64                      StartAddress2;
  UINT64                      StartAddress3;
  UINT64                      TotalLength;
  UINT64                      Length1;
  UINT64                      Length2;
  UINT64                      Length3;
  UINT32                      Index;
  UINT32                      CpusNum;
  volatile MP_WAKEUP_MAILBOX  *MailBox;

  AcceptChunkSize = FixedPcdGet64 (PcdTdxAcceptChunkSize);
  AcceptPageSize = FixedPcdGet64 (PcdTdxAcceptPageSize);
  TotalLength = PhysicalEnd - PhysicalAddress;
  StartAddress1 = 0;
  StartAddress2 = 0;
  StartAddress3 = 0;
  Length1 = 0;
  Length2 = 0;
  Length3 = 0;

  if (AcceptPageSize == SIZE_4KB || TotalLength <= SIZE_2MB) {
    //
    // if total length is less than 2M, then we accept pages in 4k
    //
    StartAddress1 = 0;
    Length1 = 0;
    StartAddress2 = PhysicalAddress;
    Length2 = PhysicalEnd - PhysicalAddress;
    StartAddress3 = 0;
    Length3 = 0;
    AcceptPageSize = SIZE_4KB;
  } else if (AcceptPageSize == SIZE_2MB) {
    //
    // Total length is bigger than 2M and Page Accept size 2M is supported.
    //
    if ((PhysicalAddress & ALIGNED_2MB_MASK) == 0) {
      //
      // Start address is 2M aligned
      //
      StartAddress1 = 0;
      Length1 = 0;
      StartAddress2 = PhysicalAddress;
      Length2 = TotalLength & ~(UINT64)ALIGNED_2MB_MASK;

      if (TotalLength > Length2) {
        //
        // There is remaining part 3)
        //
        StartAddress3 = StartAddress2 + Length2;
        Length3 = TotalLength - Length2;
        ASSERT (Length3 < SIZE_2MB);
      }
    } else {
      //
      // Start address is not 2M aligned and total length is bigger than 2M.
      //
      StartAddress1 = PhysicalAddress;
      ASSERT (TotalLength > SIZE_2MB);
      Length1 = SIZE_2MB - (PhysicalAddress & ALIGNED_2MB_MASK);
      if (TotalLength - Length1 < SIZE_2MB) {
        //
        // The Part 2) length is less than 2MB, so let's accept all the
        // memory in 4K
        //
        Length1 = TotalLength;

      } else {
        StartAddress2 = PhysicalAddress + Length1;
        Length2 = (TotalLength - Length1) & ~(UINT64)ALIGNED_2MB_MASK;
        Length3 = TotalLength - Length1 - Length2;
        StartAddress3 = Length3 > 0 ? StartAddress2 + Length2 : 0;
        ASSERT (Length3 < SIZE_2MB);
      }
    }
  }

  DEBUG ((DEBUG_INFO, "TdAccept: 0x%llx - 0x%llx\n", PhysicalAddress, TotalLength));
  DEBUG ((DEBUG_INFO, "   Part1: 0x%llx - 0x%llx\n", StartAddress1, Length1));
  DEBUG ((DEBUG_INFO, "   Part2: 0x%llx - 0x%llx\n", StartAddress2, Length2));
  DEBUG ((DEBUG_INFO, "   Part3: 0x%llx - 0x%llx\n", StartAddress3, Length3));
  DEBUG ((DEBUG_INFO, "   Chunk: 0x%llx, Page : 0x%llx\n", AcceptChunkSize, AcceptPageSize));

  MpSerializeStart ();

  if (Length2 > 0) {
    MpSendWakeupCommand (
      MpProtectedModeWakeupCommandAcceptPages,
      0,
      StartAddress2,
      StartAddress2 + Length2,
      AcceptChunkSize,
      AcceptPageSize);

    Status = BspAcceptMemoryResourceRange (
                StartAddress2,
                Length2,
                AcceptChunkSize,
                AcceptPageSize);
    ASSERT (!EFI_ERROR (Status));
  }

  if (Length1 > 0) {
    Status = BspAcceptMemoryResourceRange (
                StartAddress1,
                Length1,
                AcceptChunkSize,
                SIZE_4KB);
    ASSERT (!EFI_ERROR (Status));
  }

  if (Length3 > 0) {
    Status = BspAcceptMemoryResourceRange (
                StartAddress3,
                Length3,
                AcceptChunkSize,
                SIZE_4KB);
    ASSERT (!EFI_ERROR (Status));
  }

  MpSerializeEnd ();

  CpusNum = GetCpusNum ();
  MailBox = (volatile MP_WAKEUP_MAILBOX *) GetTdxMailBox ();

  DEBUG ((DEBUG_INFO, "AcceptPage Tallies:\n"));
  DEBUG ((DEBUG_INFO, "  "));
  for (Index = 0; Index < CpusNum; Index++) {
    DEBUG ((DEBUG_INFO, "%8d", MailBox->Tallies[Index]));
    if (Index % 8 == 7) {
      DEBUG ((DEBUG_INFO, "\n"));
      DEBUG ((DEBUG_INFO, "  "));
    }
  }
  DEBUG ((DEBUG_INFO, "\n"));

  for (Index = 0; Index < CpusNum; Index++) {
    if (MailBox->Errors[Index] > 0) {
      Status = EFI_DEVICE_ERROR;
      DEBUG ((DEBUG_ERROR, "Error(%d) of CPU-%d when accepting memory\n",
        MailBox->Errors[Index], Index));
    }
  }

  return Status;
}

/**
  Dump out the hob list

  @param[in]  HobStart    Start address of the hob list
**/
VOID
EFIAPI
DEBUG_HOBLIST (
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    DEBUG ((DEBUG_INFO, "HOB(%p) : %x %x\n", Hob, Hob.Header->HobType, Hob.Header->HobLength));
    switch (Hob.Header->HobType) {
    case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
      DEBUG ((DEBUG_INFO, "\t: %x %x %llx %llx\n",
        Hob.ResourceDescriptor->ResourceType,
        Hob.ResourceDescriptor->ResourceAttribute,
        Hob.ResourceDescriptor->PhysicalStart,
        Hob.ResourceDescriptor->ResourceLength));

      break;
    case EFI_HOB_TYPE_MEMORY_ALLOCATION:
      DEBUG ((DEBUG_INFO, "\t: %llx %llx %x\n",
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
        Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
        Hob.MemoryAllocation->AllocDescriptor.MemoryType));
      break;
    default:
      break;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
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
  IN UINT32    Value,
  IN UINT32    *ValidList,
  IN UINT32    ValidListLength
) {
  UINT32 index;

  if (ValidList == NULL) {
    return FALSE;
  }

  for (index = 0; index < ValidListLength; index ++) {
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
  IN CONST VOID             *VmmHobList
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINT32 EFI_BOOT_MODE_LIST[12] = { BOOT_WITH_FULL_CONFIGURATION,
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

  UINT32 EFI_RESOURCE_TYPE_LIST[8] = { EFI_RESOURCE_SYSTEM_MEMORY,
                                       EFI_RESOURCE_MEMORY_MAPPED_IO,
                                       EFI_RESOURCE_IO,
                                       EFI_RESOURCE_FIRMWARE_DEVICE,
                                       EFI_RESOURCE_MEMORY_MAPPED_IO_PORT,
                                       EFI_RESOURCE_MEMORY_RESERVED,
                                       EFI_RESOURCE_IO_RESERVED,
                                       EFI_RESOURCE_MAX_MEMORY_TYPE
                                     };

  if (VmmHobList == NULL) {
    DEBUG ((DEBUG_ERROR, "HOB: HOB data pointer is NULL\n"));
    return FALSE;
  }

  Hob.Raw = (UINT8 *) VmmHobList;

  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->Reserved != (UINT32) 0) {
      DEBUG ((DEBUG_ERROR, "HOB: Hob header Reserved filed should be zero\n"));
      return FALSE;
    }

    if (Hob.Header->HobLength == 0) {
        DEBUG ((DEBUG_ERROR, "HOB: Hob header LEANGTH should not be zero\n"));
        return FALSE;
    }

    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_HANDOFF:
        if (Hob.Header->HobLength != sizeof(EFI_HOB_HANDOFF_INFO_TABLE)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_HANDOFF));
          return FALSE;
        }

        if (IsInValidList (Hob.HandoffInformationTable->BootMode, EFI_BOOT_MODE_LIST, 12) == FALSE) {
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
        if (Hob.Header->HobLength != sizeof(EFI_HOB_RESOURCE_DESCRIPTOR)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_RESOURCE_DESCRIPTOR));
          return FALSE;
        }

        if (IsInValidList (Hob.ResourceDescriptor->ResourceType, EFI_RESOURCE_TYPE_LIST, 8) == FALSE) {
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
                                                          EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE |
                                                          EFI_RESOURCE_ATTRIBUTE_ENCRYPTED))) != 0) {
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
        if (Hob.Header->HobLength != sizeof(EFI_HOB_FIRMWARE_VOLUME2)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_FV2));
          return FALSE;
        }
        break;

      case EFI_HOB_TYPE_FV3:
        if (Hob.Header->HobLength != sizeof(EFI_HOB_FIRMWARE_VOLUME3)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_FV3));
          return FALSE;
        }
        break;

      case EFI_HOB_TYPE_CPU:
        if (Hob.Header->HobLength != sizeof(EFI_HOB_CPU)) {
          DEBUG ((DEBUG_ERROR, "HOB: Hob length is not equal corresponding hob structure. Type: 0x%04x\n", EFI_HOB_TYPE_CPU));
          return FALSE;
        }

        for (UINT32 index = 0; index < 6; index ++) {
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
    Hob.Raw = (UINT8 *) (Hob.Raw + Hob.Header->HobLength);
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
  IN CONST VOID             *VmmHobList
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PHYSICAL_ADDRESS        PhysicalEnd;

  Status = EFI_SUCCESS;
  ASSERT (VmmHobList != NULL);
  Hob.Raw = (UINT8 *) VmmHobList;

  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {

    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      DEBUG ((DEBUG_INFO, "\nResourceType: 0x%x\n", Hob.ResourceDescriptor->ResourceType));

      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        DEBUG ((DEBUG_INFO, "ResourceAttribute: 0x%x\n", Hob.ResourceDescriptor->ResourceAttribute));
        DEBUG ((DEBUG_INFO, "PhysicalStart: 0x%llx\n", Hob.ResourceDescriptor->PhysicalStart));
        DEBUG ((DEBUG_INFO, "ResourceLength: 0x%llx\n", Hob.ResourceDescriptor->ResourceLength));
        DEBUG ((DEBUG_INFO, "Owner: %g\n\n", &Hob.ResourceDescriptor->Owner));

        PhysicalEnd = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;

        Status = MpAcceptMemoryResourceRange (
            Hob.ResourceDescriptor->PhysicalStart,
            PhysicalEnd);
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
  EFI_STATUS                  Status;
  VOID                        *TdHob;
  TD_RETURN_DATA              TdReturnData;

  TdHob = (VOID *) (UINTN) FixedPcdGet32 (PcdOvmfSecGhcbBase);
  Status = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO,
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

