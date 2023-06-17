/** @file
  SPI NOR Flash JEDEC Serial Flash Discoverable Parameters (SFDP)
  common functions.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - JEDEC Standard, JESD216F.02
      https://www.jedec.org/document_search?search_api_views_fulltext=JESD216

  @par Glossary:
    - SFDP - Serial Flash Discoverable Parameters
    - PTP  - Parameter Table Pointer
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include "SpiNorFlash.h"
#include "SpiNorFlashJedecSfdpInternal.h"

/**
  Build up the Fast Read capability entry and link it to
  the linked list.

  @param[in]  Instance              SPI Nor Flash Instance data with pointer to
                                    EFI_SPI_NOR_FLASH_PROTOCOL and
                                    EFI_SPI_IO_PROTOCOL.
  @param[in]  FastReadInstruction   The string of fast read instruction.
  @param[in]  FastReadModeClk       The string of fast read mode clock.
  @param[in]  FastReadDummyClk      The string of fast read dummy clock.

**/
VOID
CreateSpiFastReadTableEntry (
  IN SPI_NOR_FLASH_INSTANCE  *Instance,
  IN UINT32                  FastReadInstruction,
  IN UINT32                  FastReadModeClk,
  IN UINT32                  FastReadDummyClk
  )
{
  SFPD_FAST_READ_CAPBILITY_RECORD  *CapabilityEntry;

  CapabilityEntry = AllocateZeroPool (sizeof (SFPD_FAST_READ_CAPBILITY_RECORD));
  if (CapabilityEntry == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create fast read table\n", __func__));
    ASSERT (FALSE);
    return;
  }

  InitializeListHead (&CapabilityEntry->NextFastReadCap);
  CapabilityEntry->FastReadInstruction = (UINT8)FastReadInstruction;
  CapabilityEntry->ModeClocks          = (UINT8)FastReadModeClk;
  CapabilityEntry->WaitStates          = (UINT8)FastReadDummyClk;
  InsertTailList (&Instance->FastReadTableList, &CapabilityEntry->NextFastReadCap);
  DEBUG ((DEBUG_VERBOSE, "%a: Create and link table.\n", __func__));
  DEBUG ((DEBUG_VERBOSE, "  Instruction               : 0x%x\n", FastReadInstruction));
  DEBUG ((DEBUG_VERBOSE, "  Mode bits                 : 0x%x\n", FastReadModeClk));
  DEBUG ((DEBUG_VERBOSE, "  Wait States (Dummy Clocks): 0x%x\n", FastReadDummyClk));
}

/**
  Calculate erase type typical time.

  @param[in]  SfdpEraseTypicalTime      Erase type typical time indicated in
                                        Basic Flash Parameter Table.
                                        EraseTypicalTime [0:4] - Count
                                        EraseTypicalTime [5:6] - Unit
                                                                  00b: 1ms
                                                                  01b: 16ms
                                                                  10b: 128ms
                                                                  11b: 1s
  @param[in]  SfdpEraseTimeMultiplier   Multiplier from erase typical time.
  @param[out] EraseTypicalTime          Pointer to receive Erase typical time in milliseconds.
  @param[out] EraseTimeout              Pointer to receive Erase timeout in milliseconds.

**/
VOID
CalculateEraseTiming (
  IN  UINT32  SfdpEraseTypicalTime,
  IN  UINT32  SfdpEraseTimeMultiplier,
  OUT UINT32  *EraseTypicalTime,
  OUT UINT64  *EraseTimeout
  )
{
  UINT32  UnitInMs;

  UnitInMs = (SfdpEraseTypicalTime & ERASE_TYPICAL_TIME_UNITS_MASK) >> ERASE_TYPICAL_TIME_BIT_POSITION;
  switch (UnitInMs) {
    case ERASE_TYPICAL_TIME_UNIT_1_MS_BITMAP:
      UnitInMs = ERASE_TYPICAL_TIME_UNIT_1_MS;
      break;

    case ERASE_TYPICAL_TIME_UNIT_16_MS_BITMAP:
      UnitInMs = ERASE_TYPICAL_TIME_UNIT_16_MS;
      break;

    case ERASE_TYPICAL_TIME_UNIT_128_MS_BITMAP:
      UnitInMs = ERASE_TYPICAL_TIME_UNIT_128_MS;
      break;

    case ERASE_TYPICAL_TIME_UNIT_1000_MS_BITMAP:
      UnitInMs = ERASE_TYPICAL_TIME_UNIT_1000_MS;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Unsupported Erase Typical time.\n", __func__));
      ASSERT (FALSE);
  }

  *EraseTypicalTime = UnitInMs * ((SfdpEraseTypicalTime & ERASE_TYPICAL_TIME_COUNT_MASK) + 1);
  *EraseTimeout     = 2 * (SfdpEraseTimeMultiplier + 1) * *EraseTypicalTime;
  return;
}

/**
  Print out the erase type information.

  @param[in]  SupportedEraseType    Pointer to SFDP_SUPPORTED_ERASE_TYPE_RECORD.
**/
VOID
DebugPrintEraseType (
  IN SFDP_SUPPORTED_ERASE_TYPE_RECORD  *SupportedEraseType
  )
{
  DEBUG ((DEBUG_VERBOSE, "  Erase Type %d\n", SupportedEraseType->EraseType));
  DEBUG ((DEBUG_VERBOSE, "  Erase Type instruction: 0x%x\n", SupportedEraseType->EraseInstruction));
  DEBUG ((DEBUG_VERBOSE, "  Erase size: 0x%x bytes\n", SupportedEraseType->EraseSizeInByte));
  DEBUG ((DEBUG_VERBOSE, "  Erase time: %d Milliseconds\n", SupportedEraseType->EraseTypicalTime));
  DEBUG ((DEBUG_VERBOSE, "  Erase timeout:  %d Milliseconds:\n", SupportedEraseType->EraseTimeout));
}

/**
  Insert supported erase type entry.

  @param[in]  Instance              SPI Nor Flash Instance data with pointer to
                                    EFI_SPI_NOR_FLASH_PROTOCOL and
                                    EFI_SPI_IO_PROTOCOL.
  @param[in]  SupportedEraseType    Pointer to SFDP_SUPPORTED_ERASE_TYPE_RECORD.
**/
VOID
CreateEraseTypeEntry (
  IN  SPI_NOR_FLASH_INSTANCE            *Instance,
  IN  SFDP_SUPPORTED_ERASE_TYPE_RECORD  *SupportedEraseType
  )
{
  InitializeListHead (&SupportedEraseType->NextEraseType);
  InsertTailList (&Instance->SupportedEraseTypes, &SupportedEraseType->NextEraseType);

  DEBUG ((DEBUG_VERBOSE, "%a: Erase Type 0x%x is supported:\n", __func__, SupportedEraseType->EraseType));
  DebugPrintEraseType (SupportedEraseType);
}

/**
  Build up the erase type tables.

  @param[in]  Instance    SPI Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and
                          EFI_SPI_IO_PROTOCOL.

**/
VOID
BuildUpEraseTypeTable (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  SFDP_SUPPORTED_ERASE_TYPE_RECORD  *SupportedEraseType;

  // Build up erase type 1 entry.
  if (Instance->SfdpBasicFlash->Erase1Size != 0) {
    SupportedEraseType = AllocateZeroPool (sizeof (SFDP_SUPPORTED_ERASE_TYPE_RECORD));
    if (SupportedEraseType != NULL) {
      SupportedEraseType->EraseType        = SFDP_ERASE_TYPE_1;
      SupportedEraseType->EraseInstruction = (UINT8)Instance->SfdpBasicFlash->Erase1Instr;
      SupportedEraseType->EraseSizeInByte  = (UINT32)1 << Instance->SfdpBasicFlash->Erase1Size;
      CalculateEraseTiming (
        Instance->SfdpBasicFlash->Erase1Time,
        Instance->SfdpBasicFlash->EraseMultiplier,
        &SupportedEraseType->EraseTypicalTime,
        &SupportedEraseType->EraseTimeout
        );
      CreateEraseTypeEntry (Instance, SupportedEraseType);
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Memory allocated failed for SFDP_SUPPORTED_ERASE_TYPE_RECORD (Type 1).\n", __func__));
      ASSERT (FALSE);
    }
  }

  // Build up erase type 2 entry.
  if (Instance->SfdpBasicFlash->Erase2Size != 0) {
    SupportedEraseType = AllocateZeroPool (sizeof (SFDP_SUPPORTED_ERASE_TYPE_RECORD));
    if (SupportedEraseType != NULL) {
      SupportedEraseType->EraseType        = SFDP_ERASE_TYPE_2;
      SupportedEraseType->EraseInstruction = (UINT8)Instance->SfdpBasicFlash->Erase2Instr;
      SupportedEraseType->EraseSizeInByte  = (UINT32)1 << Instance->SfdpBasicFlash->Erase2Size;
      CalculateEraseTiming (
        Instance->SfdpBasicFlash->Erase2Time,
        Instance->SfdpBasicFlash->EraseMultiplier,
        &SupportedEraseType->EraseTypicalTime,
        &SupportedEraseType->EraseTimeout
        );
      CreateEraseTypeEntry (Instance, SupportedEraseType);
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Memory allocated failed for SFDP_SUPPORTED_ERASE_TYPE_RECORD (Type 2).\n", __func__));
      ASSERT (FALSE);
    }
  }

  // Build up erase type 3 entry.
  if (Instance->SfdpBasicFlash->Erase3Size != 0) {
    SupportedEraseType = AllocateZeroPool (sizeof (SFDP_SUPPORTED_ERASE_TYPE_RECORD));
    if (SupportedEraseType != NULL) {
      SupportedEraseType->EraseType        = SFDP_ERASE_TYPE_3;
      SupportedEraseType->EraseInstruction = (UINT8)Instance->SfdpBasicFlash->Erase3Instr;
      SupportedEraseType->EraseSizeInByte  = (UINT32)1 << Instance->SfdpBasicFlash->Erase3Size;
      CalculateEraseTiming (
        Instance->SfdpBasicFlash->Erase3Time,
        Instance->SfdpBasicFlash->EraseMultiplier,
        &SupportedEraseType->EraseTypicalTime,
        &SupportedEraseType->EraseTimeout
        );
      CreateEraseTypeEntry (Instance, SupportedEraseType);
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Memory allocated failed for SFDP_SUPPORTED_ERASE_TYPE_RECORD (Type 3).\n", __func__));
      ASSERT (FALSE);
    }
  }

  // Build up erase type 4 entry.
  if (Instance->SfdpBasicFlash->Erase4Size != 0) {
    SupportedEraseType = AllocateZeroPool (sizeof (SFDP_SUPPORTED_ERASE_TYPE_RECORD));
    if (SupportedEraseType != NULL) {
      SupportedEraseType->EraseType        = SFDP_ERASE_TYPE_4;
      SupportedEraseType->EraseInstruction = (UINT8)Instance->SfdpBasicFlash->Erase4Instr;
      SupportedEraseType->EraseSizeInByte  = (UINT32)1 << Instance->SfdpBasicFlash->Erase4Size;
      CalculateEraseTiming (
        Instance->SfdpBasicFlash->Erase4Time,
        Instance->SfdpBasicFlash->EraseMultiplier,
        &SupportedEraseType->EraseTypicalTime,
        &SupportedEraseType->EraseTimeout
        );
      CreateEraseTypeEntry (Instance, SupportedEraseType);
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Memory allocated failed for SFDP_SUPPORTED_ERASE_TYPE_RECORD (Type 4).\n", __func__));
      ASSERT (FALSE);
    }
  }
}

/**
  This function check if the erase type is one of the target erase types.

  @param[in]   EraseType       The erase type.
  @param[in]   TargetTypeNum   Number of target search types.
  @param[in]   TargetTypes     Target types.


  @retval     TRUE    Yes, this is the target erase type.
  @retval     FALSE   No, this is not the target erase type.

**/
BOOLEAN
IsTargetEraseType (
  IN UINT16  EraseType,
  IN UINT8   TargetTypeNum,
  IN UINT8   *TargetTypes
  )
{
  UINT8  Index;

  for (Index = 0; Index < TargetTypeNum; Index++) {
    if (EraseType == *(TargetTypes + Index)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Search the erase type record according to the given search type and value.

  @param[in]   Instance                SPI Nor Flash Instance data with pointer to
                                       EFI_SPI_NOR_FLASH_PROTOCOL and
                                       EFI_SPI_IO_PROTOCOL.
  @param[in]   SearchType              Search type.
  @param[in]   SearchValue             The value of according to search type.
                                       - For SearchEraseTypeByCommand:
                                         SearchValue is the erase instruction.
                                       - For SearchEraseTypeBySize:
                                          SearchValue is the erase block size.
                                       - For SearchEraseTypeBySmallestSize:
                                         SearchValue is not used.
                                       - For SearchEraseTypeByBiggestSize:
                                         SearchValue is not used.
  @param[in]   SupportedTypeTargetNum  Only search the specific erase types.
  @param[in]   SupportedTypeTarget     Pointer to SupportedTypeTargetNum of
                                       supported erase types.
  @param[out]  EraseTypeRecord         Pointer to receive the erase type record.

  @retval     EFI_SUCCESS              Pointer to erase type record is returned.
              EFI_INVALID_PARAMETER    Invalid SearchType.
              EFI_NOT_FOUND            Erase type not found.
**/
EFI_STATUS
GetEraseTypeRecord (
  IN   SPI_NOR_FLASH_INSTANCE            *Instance,
  IN   SFDP_SEARCH_ERASE_TYPE            SearchType,
  IN   UINT32                            SearchValue,
  IN   UINT8                             SupportedTypeTargetNum,
  IN   UINT8                             *SupportedTypeTarget OPTIONAL,
  OUT  SFDP_SUPPORTED_ERASE_TYPE_RECORD  **EraseTypeRecord
  )
{
  SFDP_SUPPORTED_ERASE_TYPE_RECORD  *EraseType;
  UINT32                            ValueToCompare;
  BOOLEAN                           ExitSearching;

  if (IsListEmpty (&Instance->SupportedEraseTypes)) {
    return EFI_NOT_FOUND;
  }

  *EraseTypeRecord = NULL;

  //
  // Initial the comapre value.
  //
  switch (SearchType) {
    case SearchEraseTypeByType:
    case SearchEraseTypeByCommand:
    case SearchEraseTypeBySize:
      break;
    case SearchEraseTypeBySmallestSize:
      ValueToCompare = (UINT32)-1;
      break;
    case SearchEraseTypeByBiggestSize:
      ValueToCompare = 0;
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  ExitSearching = FALSE;
  EraseType     = (SFDP_SUPPORTED_ERASE_TYPE_RECORD *)GetFirstNode (&Instance->SupportedEraseTypes);
  while (TRUE) {
    if ((SupportedTypeTarget == NULL) || IsTargetEraseType (EraseType->EraseType, SupportedTypeTargetNum, SupportedTypeTarget)) {
      switch (SearchType) {
        case SearchEraseTypeByType:
          if (EraseType->EraseType == SearchValue) {
            *EraseTypeRecord = EraseType;
            ExitSearching    = TRUE;
          }

          break;

        case SearchEraseTypeBySize:
          if (EraseType->EraseSizeInByte == SearchValue) {
            *EraseTypeRecord = EraseType;
            ExitSearching    = TRUE;
          }

          break;

        case SearchEraseTypeByCommand:
          if (EraseType->EraseInstruction == (UINT8)SearchValue) {
            *EraseTypeRecord = EraseType;
            ExitSearching    = TRUE;
          }

          break;

        case SearchEraseTypeBySmallestSize:
          if (EraseType->EraseSizeInByte < ValueToCompare) {
            ValueToCompare   = EraseType->EraseSizeInByte;
            *EraseTypeRecord = EraseType;
          }

          break;

        case SearchEraseTypeByBiggestSize:
          if (EraseType->EraseSizeInByte > ValueToCompare) {
            ValueToCompare   = EraseType->EraseSizeInByte;
            *EraseTypeRecord = EraseType;
          }

          break;

        default:
          return EFI_INVALID_PARAMETER;
      }
    }

    if (IsNodeAtEnd (&Instance->SupportedEraseTypes, &EraseType->NextEraseType) || ExitSearching) {
      break;
    }

    EraseType = (SFDP_SUPPORTED_ERASE_TYPE_RECORD *)GetNextNode (&Instance->SupportedEraseTypes, &EraseType->NextEraseType);
  }

  if (*EraseTypeRecord == NULL) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Get the erase block attribute for the target address.

  @param[in]      Instance              Spi Nor Flash Instance data with pointer to
                                        EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]      FlashRegion           The region the flash address belong.
  @param[in]      FlashAddress          The target flash address.
  @param[in]      RemainingSize         Remaining size to erase.
  @param[in, out] BlockSizeToErase      Input  - The block erase size for this continious blocks.
                                        Output - The determined block size for erasing.
  @param[in, out] BlockCountToErase     Input  - The expected blocks to erase.
                                        Output - The determined number of blocks to erase.
  @param[out]     BlockEraseCommand     The erase command used for this continious blocks.
  @param[out]     TypicalTime           Pointer to receive the typical time in millisecond
                                        to erase this erase type size.
  @param[out]     MaximumTimeout        Pointer to receive the maximum timeout in millisecond
                                        to erase this erase type size.

  @retval EFI_SUCCESS          The erase block attribute is returned.
  @retval EFI_DEVICE_ERROR     No valid SFDP discovered.
  @retval EFI_NOT_FOUND        No valud erase block attribute found.

**/
EFI_STATUS
GetEraseBlockAttribute (
  IN     SPI_NOR_FLASH_INSTANCE     *Instance,
  IN     SFDP_SECTOR_REGION_RECORD  *FlashRegion,
  IN     UINT32                     FlashAddress,
  IN     UINT32                     RemainingSize,
  IN OUT UINT32                     *BlockSizeToErase,
  IN OUT UINT32                     *BlockCountToErase,
  OUT    UINT8                      *BlockEraseCommand,
  OUT    UINT32                     *TypicalTime,
  OUT    UINT64                     *MaximumTimeout
  )
{
  EFI_STATUS                        Status;
  SFDP_SUPPORTED_ERASE_TYPE_RECORD  *EraseType;
  UINT32                            EraseSize;

  DEBUG ((DEBUG_VERBOSE, "%a: Entry\n", __func__));

  for (EraseSize = SIZE_2GB; EraseSize != 0; EraseSize = EraseSize >> 1) {
    Status = GetEraseTypeRecord (Instance, SearchEraseTypeBySize, EraseSize, 0, NULL, &EraseType);
    if (!EFI_ERROR (Status)) {
      // Validate this erase type.
      if (((FlashAddress & (EraseType->EraseSizeInByte - 1)) == 0) &&
          (RemainingSize >= EraseType->EraseSizeInByte))
      {
        *BlockSizeToErase  = EraseType->EraseSizeInByte;
        *BlockCountToErase = 1;
        *BlockEraseCommand = EraseType->EraseInstruction;
        *TypicalTime       = EraseType->EraseTypicalTime;
        *MaximumTimeout    = EraseType->EraseTimeout;
        Status             = EFI_SUCCESS;
        break;
      }
    }
  }

  if (EraseType == NULL) {
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_VERBOSE, "  Erase address at 0x%08x.\n", FlashAddress));
  DEBUG ((DEBUG_VERBOSE, "    - Erase block size   : 0x%08x.\n", *BlockSizeToErase));
  DEBUG ((DEBUG_VERBOSE, "    - Erase block count  : 0x%08x.\n", *BlockCountToErase));
  DEBUG ((DEBUG_VERBOSE, "    - Erase block command: 0x%02x.\n", *BlockEraseCommand));
  DEBUG ((DEBUG_VERBOSE, "    - Remaining size to erase: 0x%08x.\n", RemainingSize));
  DEBUG ((DEBUG_VERBOSE, "    - Erase typical time: %d milliseconds.\n", *TypicalTime));
  DEBUG ((DEBUG_VERBOSE, "    - Erase timeout: %d milliseconds.\n", *MaximumTimeout));
  return EFI_SUCCESS;
}

/**
  Get the erase block attribute for the target address.

  @param[in]   Instance                Spi Nor Flash Instance data with pointer to
                                       EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]   FlashAddress            The target flash address.
  @param[out]  FlashRegion             The target flash address.

  @retval EFI_SUCCESS             The region is returned.
  @retval EFI_INVALID_PARAMETER   FlashAddress is not belong to any region.
  @retval Otherwise               Other errors.

**/
EFI_STATUS
GetRegionByFlashAddress (
  IN  SPI_NOR_FLASH_INSTANCE      *Instance,
  IN  UINT32                      FlashAddress,
  OUT  SFDP_SECTOR_REGION_RECORD  **FlashRegion
  )
{
  SFDP_SECTOR_MAP_RECORD     *SectorMapRecord;
  SFDP_SECTOR_REGION_RECORD  *RegionRecord;

  DEBUG ((DEBUG_VERBOSE, "%a: Entry\n", __func__));

  SectorMapRecord = Instance->CurrentSectorMap;
  if (SectorMapRecord == NULL) {
    return EFI_DEVICE_ERROR;
  }

  RegionRecord = (SFDP_SECTOR_REGION_RECORD *)GetFirstNode (&SectorMapRecord->RegionList);
  while (TRUE) {
    if ((FlashAddress >= RegionRecord->RegionAddress) &&
        (FlashAddress < RegionRecord->RegionAddress + RegionRecord->RegionTotalSize))
    {
      *FlashRegion = RegionRecord;
      return EFI_SUCCESS;
    }

    if (IsNodeAtEnd (&SectorMapRecord->RegionList, &RegionRecord->NextRegion)) {
      break;
    }

    RegionRecord = (SFDP_SECTOR_REGION_RECORD *)GetNextNode (&SectorMapRecord->RegionList, &RegionRecord->NextRegion);
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Build up the Fast Read capability tables. The earlier linked table
  in the linked list has the faster transfer.
  NOTE: 1. The Quad input instructions mentioned in 21th DWOWRD
           are not considered yet.
        2. Maximum speed options for certain Fast Read modes are
           not considered yet. (e.g., 8D-8D-8D or 4S-4D-4D)

  @param[in]  Instance    SPI Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and
                          EFI_SPI_IO_PROTOCOL.

**/
VOID
BuildUpFastReadTable (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  // Build up the standard Fast Read
  // This will be first picked for the ReadData.
  // TODO: The mechanism to choose the advance fast read
  //       is not determined yet in this version of
  //       SpiNorFlash driver.
  CreateSpiFastReadTableEntry (
    Instance,
    SPI_FLASH_FAST_READ,
    0,
    SPI_FLASH_FAST_READ_DUMMY * 8
    );

  // Build up Fast Read table 1S-1S-4S
  if (Instance->SfdpBasicFlash->FastRead114 != 0) {
    CreateSpiFastReadTableEntry (
      Instance,
      Instance->SfdpBasicFlash->FastRead114Instr,
      Instance->SfdpBasicFlash->FastRead114ModeClk,
      Instance->SfdpBasicFlash->FastRead114Dummy
      );
  }

  // Build up Fast Read table 1S-2S-2S
  if (Instance->SfdpBasicFlash->FastRead122 != 0) {
    CreateSpiFastReadTableEntry (
      Instance,
      Instance->SfdpBasicFlash->FastRead122Instr,
      Instance->SfdpBasicFlash->FastRead122ModeClk,
      Instance->SfdpBasicFlash->FastRead122Dummy
      );
  }

  // Build up Fast Read table 2S-2S-2S
  if (Instance->SfdpBasicFlash->FastRead222 != 0) {
    CreateSpiFastReadTableEntry (
      Instance,
      Instance->SfdpBasicFlash->FastRead222Instr,
      Instance->SfdpBasicFlash->FastRead222ModeClk,
      Instance->SfdpBasicFlash->FastRead222Dummy
      );
  }

  // Build up Fast Read table 1S-4S-4S
  if (Instance->SfdpBasicFlash->FastRead144 != 0) {
    CreateSpiFastReadTableEntry (
      Instance,
      Instance->SfdpBasicFlash->FastRead144Instr,
      Instance->SfdpBasicFlash->FastRead144ModeClk,
      Instance->SfdpBasicFlash->FastRead144Dummy
      );
  }

  // Build up Fast Read table 4S-4S-4S
  if (Instance->SfdpBasicFlash->FastRead444 != 0) {
    CreateSpiFastReadTableEntry (
      Instance,
      Instance->SfdpBasicFlash->FastRead444Instr,
      Instance->SfdpBasicFlash->FastRead444ModeClk,
      Instance->SfdpBasicFlash->FastRead444Dummy
      );
  }

  // Build up Fast Read table 1S-1S-8S
  if (Instance->SfdpBasicFlash->FastRead118Instr != 0) {
    CreateSpiFastReadTableEntry (
      Instance,
      Instance->SfdpBasicFlash->FastRead118Instr,
      Instance->SfdpBasicFlash->FastRead118ModeClk,
      Instance->SfdpBasicFlash->FastRead118Dummy
      );
  }

  // Build up Fast Read table 1S-8S-8S
  if (Instance->SfdpBasicFlash->FastRead188Instr != 0) {
    CreateSpiFastReadTableEntry (
      Instance,
      Instance->SfdpBasicFlash->FastRead188Instr,
      Instance->SfdpBasicFlash->FastRead188ModeClk,
      Instance->SfdpBasicFlash->FastRead188Dummy
      );
  }
}

/**
  This function sets up the erase types supported
  by this region.

  @param[in]  Instance          SPI Nor Flash Instance data with pointer to
                                EFI_SPI_NOR_FLASH_PROTOCOL and
                                EFI_SPI_IO_PROTOCOL.
  @param[in]  RegionRecord      Pointer to SFDP_SECTOR_REGION_RECORD of this
                                regions.
  @retval     EFI_SUCCESS       Current sector map configuration is determined.
              EFI_DEVICE_ERROR  Current sector map configuration is not found.

**/
EFI_STATUS
SetupRegionEraseInfo (
  IN  SPI_NOR_FLASH_INSTANCE     *Instance,
  IN  SFDP_SECTOR_REGION_RECORD  *RegionRecord
  )
{
  SFDP_SUPPORTED_ERASE_TYPE_RECORD  *SupportedEraseType;
  UINT32                            MinimumEraseSize;

  if (IsListEmpty (&Instance->SupportedEraseTypes)) {
    DEBUG ((DEBUG_ERROR, "%a: No erase type suppoted on the flash device.\n", __func__));
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  MinimumEraseSize   = (UINT32)-1;
  SupportedEraseType = (SFDP_SUPPORTED_ERASE_TYPE_RECORD *)GetFirstNode (&Instance->SupportedEraseTypes);
  while (TRUE) {
    RegionRecord->SupportedEraseType[RegionRecord->SupportedEraseTypeNum] = (UINT8)SupportedEraseType->EraseType;
    RegionRecord->SupportedEraseTypeNum++;
    RegionRecord->EraseTypeBySizeBitmap |= SupportedEraseType->EraseSizeInByte;
    if (MinimumEraseSize > SupportedEraseType->EraseSizeInByte) {
      MinimumEraseSize = SupportedEraseType->EraseSizeInByte;
    }

    if (IsNodeAtEnd (&Instance->SupportedEraseTypes, &SupportedEraseType->NextEraseType)) {
      break;
    }

    SupportedEraseType = (SFDP_SUPPORTED_ERASE_TYPE_RECORD *)GetNextNode (&Instance->SupportedEraseTypes, &SupportedEraseType->NextEraseType);
  }

  RegionRecord->SectorSize      = MinimumEraseSize;
  RegionRecord->RegionTotalSize = Instance->FlashDeviceSize;
  RegionRecord->RegionSectors   = RegionRecord->RegionTotalSize / RegionRecord->SectorSize;
  return EFI_SUCCESS;
}

/**
  Create a single flash sector map.

  @param[in]  Instance          SPI Nor Flash Instance data with pointer to
                                EFI_SPI_NOR_FLASH_PROTOCOL and
                                EFI_SPI_IO_PROTOCOL.
  @retval     EFI_SUCCESS         Current sector map configuration is determined.
              EFI_DEVICE_ERROR    Current sector map configuration is not found.

**/
EFI_STATUS
CreateSingleFlashSectorMap (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  SFDP_SECTOR_MAP_RECORD     *SectorMapRecord;
  SFDP_SECTOR_REGION_RECORD  *RegionRecord;
  UINTN                      EraseIndex;

  DEBUG ((DEBUG_VERBOSE, "%a: Entry:\n", __func__));
  SectorMapRecord = (SFDP_SECTOR_MAP_RECORD *)AllocateZeroPool (sizeof (SFDP_SECTOR_MAP_RECORD));
  if (SectorMapRecord == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No memory resource for SFDP_SECTOR_MAP_DETECTION_RECORD.\n", __func__));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  // Create SFDP_SECTOR_MAP_RECORD.
  InitializeListHead (&SectorMapRecord->NextDescriptor);
  InitializeListHead (&SectorMapRecord->RegionList);
  SectorMapRecord->ConfigurationId = 0;
  SectorMapRecord->RegionCount     = 1;
  InsertTailList (&Instance->ConfigurationMapList, &SectorMapRecord->NextDescriptor);
  DEBUG ((DEBUG_VERBOSE, "    Sector map configurations ID     : 0x%x\n", SectorMapRecord->ConfigurationId));
  DEBUG ((DEBUG_VERBOSE, "    Sector map configurations regions: %d\n", SectorMapRecord->RegionCount));

  // Create SFDP_SECTOR_MAP_RECORD region record.
  RegionRecord = (SFDP_SECTOR_REGION_RECORD *)AllocateZeroPool (sizeof (SFDP_SECTOR_REGION_RECORD));
  if (RegionRecord == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No memory resource for SFDP_SECTOR_REGION_RECORD.\n", __func__));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&RegionRecord->NextRegion);

  RegionRecord->RegionAddress = 0;
  //
  // Setup erase information in the region record.
  //
  SetupRegionEraseInfo (Instance, RegionRecord);

  InsertTailList (&SectorMapRecord->RegionList, &RegionRecord->NextRegion);

  Instance->CurrentSectorMap = SectorMapRecord;

  DEBUG ((DEBUG_VERBOSE, "    Region totoal size         : 0x%x\n", RegionRecord->RegionTotalSize));
  DEBUG ((DEBUG_VERBOSE, "    Region sector size         : 0x%x\n", RegionRecord->SectorSize));
  DEBUG ((DEBUG_VERBOSE, "    Region sectors             : 0x%x\n", RegionRecord->RegionSectors));

  for (EraseIndex = 0; EraseIndex < RegionRecord->SupportedEraseTypeNum; EraseIndex++) {
    DEBUG ((DEBUG_VERBOSE, "    Region erase type supported: 0x%x\n", RegionRecord->SupportedEraseType[EraseIndex]));
  }

  return EFI_SUCCESS;
}

/**
  Set EraseBlockBytes in SPI NOR Flash Protocol.

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS     The erase block size is returned.
  @retval Otherwise       Failed to get erase block size.

**/
EFI_STATUS
SetSectorEraseBlockSize (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS                        Status;
  SFDP_SUPPORTED_ERASE_TYPE_RECORD  *EraseTypeRecord;

  // Use the smallest size for the sector erase.
  Status = GetEraseTypeRecord (Instance, SearchEraseTypeBySmallestSize, 0, 0, NULL, &EraseTypeRecord);
  if (!EFI_ERROR (Status)) {
    Instance->Protocol.EraseBlockBytes = EraseTypeRecord->EraseSizeInByte;
    DEBUG ((DEBUG_VERBOSE, "  Erase block size = 0x%08x\n", EraseTypeRecord->EraseSizeInByte));
  }

  return Status;
}

/**
  Get the current sector map configuration.

  @param[in]  Instance    SPI Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and
                          EFI_SPI_IO_PROTOCOL.

  @retval     EFI_SUCCESS         Current sector map configuration is determined.
              EFI_DEVICE_ERROR    Current sector map configuration is not found.

**/
EFI_STATUS
GetCurrentSectorMapConfiguration (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS                        Status;
  UINT32                            TransactionBufferLength;
  UINT8                             AddressLength;
  BOOLEAN                           UseAddress;
  UINT32                            DummyBytes;
  UINT8                             ReturnByte;
  UINT8                             ConfigurationId;
  SFDP_SECTOR_MAP_RECORD            *SectorMap;
  SFDP_SECTOR_MAP_DETECTION_RECORD  *CommandEntry;

  Instance->CurrentSectorMap = NULL;
  if (!Instance->ConfigurationCommandsNeeded) {
    // No command needed measn only one configuration for the flash device sector map.
    Instance->CurrentSectorMap = (SFDP_SECTOR_MAP_RECORD *)GetFirstNode (&Instance->ConfigurationMapList);
    return EFI_SUCCESS;
  }

  //
  // Send the command to collect interest bit.
  //
  ConfigurationId = 0;
  CommandEntry    = (SFDP_SECTOR_MAP_DETECTION_RECORD *)GetFirstNode (&Instance->ConfigurationCommandList);
  while (TRUE) {
    // Check not WIP
    Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));

    // Read configuration byte.
    AddressLength = SPI_ADDR_3BYTE_ONLY;
    DummyBytes    = 1;
    if (CommandEntry->CommandAddressLength == SpdfConfigurationCommandAddress4Byte) {
      AddressLength = SPI_ADDR_4BYTE_ONLY;
      DummyBytes    = 0;
    }

    UseAddress = TRUE;
    if (CommandEntry->CommandAddress == SpdfConfigurationCommandAddressNone) {
      UseAddress = FALSE;
    }

    TransactionBufferLength = FillWriteBuffer (
                                Instance,
                                CommandEntry->CommandInstruction,
                                DummyBytes,
                                AddressLength,
                                UseAddress,
                                CommandEntry->CommandAddress,
                                0,
                                NULL
                                );
    Status = Instance->SpiIo->Transaction (
                                Instance->SpiIo,
                                SPI_TRANSACTION_WRITE_THEN_READ,
                                FALSE,
                                0,
                                1,
                                8,
                                TransactionBufferLength,
                                Instance->SpiTransactionWriteBuffer,
                                1,
                                &ReturnByte
                                );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fails to read the configuration byte.\n", __func__));
      ASSERT (FALSE);
      return EFI_DEVICE_ERROR;
    }

    //
    // Retrieve the interest bit.
    //
    if ((ReturnByte & CommandEntry->ConfigurationBitMask) != 0) {
      ConfigurationId |= 0x01;
    }

    if (IsNodeAtEnd (&Instance->ConfigurationCommandList, &CommandEntry->NextCommand)) {
      break;
    }

    CommandEntry    = (SFDP_SECTOR_MAP_DETECTION_RECORD *)GetNextNode (&Instance->ConfigurationCommandList, &CommandEntry->NextCommand);
    ConfigurationId = ConfigurationId << 1;
  }

  //
  // Now we have current activated configuration ID in ConfigurationId.
  // Walk through ConfigurationMapList to record the activated flash sector
  // map configuration.
  //
  SectorMap = (SFDP_SECTOR_MAP_RECORD *)GetFirstNode (&Instance->ConfigurationMapList);
  while (TRUE) {
    if (SectorMap->ConfigurationId == ConfigurationId) {
      Instance->CurrentSectorMap = SectorMap;
      break;
    }

    if (IsNodeAtEnd (&Instance->ConfigurationMapList, &SectorMap->NextDescriptor)) {
      break;
    }

    SectorMap = (SFDP_SECTOR_MAP_RECORD *)GetNextNode (&Instance->ConfigurationMapList, &SectorMap->NextDescriptor);
  }

  if (Instance->CurrentSectorMap == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Activated flash sector map is not found!\n", __func__));
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Build sector map configurations.

  @param[in]  Instance    SPI Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and
                          EFI_SPI_IO_PROTOCOL.

  @retval     EFI_SUCCESS           Records of sector map configuration command and map
                                    descriptor are built up successfully.
              EFI_OUT_OF_RESOURCES  Not enough memory resource.
              EFI_DEVICE_ERROR      SFDP Sector Map Parameter is not
                                    constructed correctly.

**/
EFI_STATUS
BuildSectorMapCommandAndMap (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  SFDP_SECTOR_MAP_TABLE              *SfdpSectorMapTable;
  SFDP_SECTOR_CONFIGURATION_COMMAND  *SfdpDetectionCommand;
  SFDP_SECTOR_MAP_DETECTION_RECORD   *CommandEntry;
  SFDP_SECTOR_CONFIGURATION_MAP      *SfdpConfigurationMap;
  SFDP_SECTOR_MAP_RECORD             *SectorMapRecord;
  SFDP_SECTOR_REGION                 *SpdfSectorRegion;
  SFDP_SECTOR_REGION_RECORD          *RegionRecord;
  SFDP_SUPPORTED_ERASE_TYPE_RECORD   *SupportedEraseType;
  UINT8                              RegionCount;
  UINT8                              EraseTypeCount;
  UINT32                             MinimumEraseSize;
  UINT32                             RegionAddress;

  SfdpSectorMapTable   = Instance->SfdpFlashSectorMap;
  SfdpConfigurationMap = &SfdpSectorMapTable->ConfigurationMap;
  SfdpDetectionCommand = &SfdpSectorMapTable->ConfigurationCommand;

  if (SfdpSectorMapTable->GenericHeader.DescriptorType == SFDP_SECTOR_MAP_TABLE_ENTRY_TYPE_MAP) {
    // No configuration detection commands are needs.
    Instance->ConfigurationCommandsNeeded = FALSE;
  } else {
    DEBUG ((DEBUG_VERBOSE, "%a: Sector map configuration detection command is needed\n", __func__));
    Instance->ConfigurationCommandsNeeded = TRUE;

    // Go through the section map detection commands.
    while (TRUE) {
      CommandEntry = (SFDP_SECTOR_MAP_DETECTION_RECORD *)AllocateZeroPool (sizeof (SFDP_SECTOR_MAP_DETECTION_RECORD));
      if (CommandEntry == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: No memory resource for SFDP_SECTOR_MAP_DETECTION_RECORD.\n", __func__));
        ASSERT (FALSE);
        return EFI_OUT_OF_RESOURCES;
      }

      InitializeListHead (&CommandEntry->NextCommand);
      CommandEntry->CommandAddress       = SfdpDetectionCommand->CommandAddress;
      CommandEntry->CommandAddressLength = (SPDF_CONFIGURATION_COMMAND_ADDR_LENGTH)SfdpDetectionCommand->DetectionCommandAddressLen;
      CommandEntry->CommandInstruction   = (UINT8)SfdpDetectionCommand->DetectionInstruction;
      CommandEntry->ConfigurationBitMask = (UINT8)SfdpDetectionCommand->ReadDataMask;
      CommandEntry->LatencyInClock       = (UINT8)SfdpDetectionCommand->DetectionLatency;
      InsertTailList (&Instance->ConfigurationCommandList, &CommandEntry->NextCommand);
      DEBUG ((DEBUG_VERBOSE, "  Command instruction   : 0x%x\n", CommandEntry->CommandInstruction));
      DEBUG ((DEBUG_VERBOSE, "  Bit selection         : 0x%x\n", CommandEntry->ConfigurationBitMask));
      DEBUG ((DEBUG_VERBOSE, "  Command address       : 0x%x\n", CommandEntry->CommandAddress));
      DEBUG ((DEBUG_VERBOSE, "  Command address length: %d\n", CommandEntry->CommandAddressLength));
      DEBUG ((DEBUG_VERBOSE, "  Command latency clocks: %d\n\n", CommandEntry->LatencyInClock));
      if (SfdpDetectionCommand->DescriptorEnd == SFDP_SECTOR_MAP_TABLE_ENTRY_LAST) {
        break;
      }

      SfdpDetectionCommand++;
    }

    SfdpConfigurationMap = (SFDP_SECTOR_CONFIGURATION_MAP *)SfdpDetectionCommand++;
  }

  //
  // Go through the region table pointed in SfdpConfigurationMap.
  //
  if (SfdpConfigurationMap->DescriptorType != SFDP_SECTOR_MAP_TABLE_ENTRY_TYPE_MAP) {
    DEBUG ((DEBUG_ERROR, "%a: Incorrect format of Sector Map Parameter.\n", __func__));
    ASSERT (FALSE);
    return EFI_DEVICE_ERROR;
  }

  while (TRUE) {
    DEBUG ((DEBUG_VERBOSE, "%a: Sector map configurations:\n", __func__));
    SectorMapRecord = (SFDP_SECTOR_MAP_RECORD *)AllocateZeroPool (sizeof (SFDP_SECTOR_MAP_RECORD));
    if (SectorMapRecord == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: No memory resource for SFDP_SECTOR_MAP_DETECTION_RECORD.\n", __func__));
      ASSERT (FALSE);
      return EFI_OUT_OF_RESOURCES;
    }

    InitializeListHead (&SectorMapRecord->NextDescriptor);
    InitializeListHead (&SectorMapRecord->RegionList);
    SectorMapRecord->ConfigurationId = (UINT8)SfdpConfigurationMap->ConfigurationID;
    SectorMapRecord->RegionCount     = (UINT8)SfdpConfigurationMap->RegionCount;
    InsertTailList (&Instance->ConfigurationMapList, &SectorMapRecord->NextDescriptor);
    DEBUG ((DEBUG_VERBOSE, "    Sector map configurations ID     : 0x%x\n", SectorMapRecord->ConfigurationId));
    DEBUG ((DEBUG_VERBOSE, "    Sector map configurations regions: %d\n", SectorMapRecord->RegionCount));
    SpdfSectorRegion = (SFDP_SECTOR_REGION *)SfdpConfigurationMap + 1;
    RegionAddress    = 0;
    for (RegionCount = 0; RegionCount < SectorMapRecord->RegionCount; RegionCount++) {
      RegionRecord = (SFDP_SECTOR_REGION_RECORD *)AllocateZeroPool (sizeof (SFDP_SECTOR_REGION_RECORD));
      if (RegionRecord == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: No memory resource for SFDP_SECTOR_MAP_DETECTION_RECORD.\n", __func__));
        ASSERT (FALSE);
        return EFI_OUT_OF_RESOURCES;
      }

      InitializeListHead (&RegionRecord->NextRegion);
      RegionRecord->RegionTotalSize = (SpdfSectorRegion->RegionSize + 1) * SFDP_SECTOR_REGION_SIZE_UNIT;
      //
      // Construct erase type supported for this region.
      //
      if (SpdfSectorRegion->EraseType1 != 0) {
        RegionRecord->SupportedEraseType[RegionRecord->SupportedEraseTypeNum] = SFDP_ERASE_TYPE_1;
        RegionRecord->SupportedEraseTypeNum++;
      }

      if (SpdfSectorRegion->EraseType2 != 0) {
        RegionRecord->SupportedEraseType[RegionRecord->SupportedEraseTypeNum] = SFDP_ERASE_TYPE_2;
        RegionRecord->SupportedEraseTypeNum++;
      }

      if (SpdfSectorRegion->EraseType3 != 0) {
        RegionRecord->SupportedEraseType[RegionRecord->SupportedEraseTypeNum] = SFDP_ERASE_TYPE_3;
        RegionRecord->SupportedEraseTypeNum++;
      }

      if (SpdfSectorRegion->EraseType4 != 0) {
        RegionRecord->SupportedEraseType[RegionRecord->SupportedEraseTypeNum] = SFDP_ERASE_TYPE_4;
        RegionRecord->SupportedEraseTypeNum++;
      }

      //
      // Calculate the sector size and total sectors.
      //
      if (IsListEmpty (&Instance->SupportedEraseTypes)) {
        DEBUG ((DEBUG_ERROR, "%a: No erase type suppoted on the flash device.\n", __func__));
        ASSERT (FALSE);
        return EFI_DEVICE_ERROR;
      }

      MinimumEraseSize = (UINT32)-1;
      for (EraseTypeCount = 0; EraseTypeCount < RegionRecord->SupportedEraseTypeNum++; EraseTypeCount++) {
        //
        // Walk through Instance->SupportedEraseTypes to find the matching erase type and
        // Use the minimum erase size as the sector size;
        //
        SupportedEraseType = (SFDP_SUPPORTED_ERASE_TYPE_RECORD *)GetFirstNode (&Instance->SupportedEraseTypes);
        while (TRUE) {
          if (RegionRecord->SupportedEraseType[EraseTypeCount] == SupportedEraseType->EraseType) {
            // Set erase size bitmap.
            RegionRecord->EraseTypeBySizeBitmap |= SupportedEraseType->EraseSizeInByte;

            if (MinimumEraseSize > SupportedEraseType->EraseSizeInByte) {
              MinimumEraseSize = SupportedEraseType->EraseSizeInByte;
              break;
            }
          }

          if (IsNodeAtEnd (&Instance->SupportedEraseTypes, &SupportedEraseType->NextEraseType)) {
            break;
          }

          SupportedEraseType = (SFDP_SUPPORTED_ERASE_TYPE_RECORD *)GetNextNode (&Instance->SupportedEraseTypes, &SupportedEraseType->NextEraseType);
        }
      }

      RegionRecord->SectorSize    = MinimumEraseSize;
      RegionRecord->RegionSectors = RegionRecord->RegionTotalSize / RegionRecord->SectorSize;
      RegionRecord->RegionAddress = RegionAddress;

      // Insert to link.
      InsertTailList (&SectorMapRecord->RegionList, &RegionRecord->NextRegion);
      DEBUG ((DEBUG_VERBOSE, "        Region: %d\n", RegionCount));
      DEBUG ((DEBUG_VERBOSE, "          Region totoal size: 0x%x\n", RegionRecord->RegionTotalSize));
      DEBUG ((DEBUG_VERBOSE, "          Region sector size: 0x%x\n", RegionRecord->SectorSize));
      DEBUG ((DEBUG_VERBOSE, "          Region sectors    : 0x%x\n", RegionRecord->RegionSectors));
      DEBUG ((DEBUG_VERBOSE, "          Region erase supported bitmap: 0x%x\n", RegionRecord->EraseTypeBySizeBitmap));

      SpdfSectorRegion++;
      RegionAddress += RegionRecord->RegionTotalSize;
    }

    if (SfdpConfigurationMap->DescriptorEnd == SFDP_SECTOR_MAP_TABLE_ENTRY_LAST) {
      break;
    }

    SfdpConfigurationMap = (SFDP_SECTOR_CONFIGURATION_MAP *)SpdfSectorRegion;
  }

  return EFI_SUCCESS;
}

/**
  This routine get Write Enable latch command.

  @param[in]  Instance    SPI Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and
                          EFI_SPI_IO_PROTOCOL.

**/
VOID
GetWriteEnableCommand  (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  //
  // Set Wrtie Enable command.
  //
  Instance->WriteEnableLatchRequired = TRUE;
  Instance->WriteEnableLatchCommand  = SPI_FLASH_WREN;
  if (Instance->SfdpBasicFlash->VolatileStatusBlockProtect == 1) {
    if (Instance->SfdpBasicFlash->WriteEnableVolatileStatus == 0) {
      Instance->WriteEnableLatchCommand = SPI_FLASH_WREN_50H;
    }
  }

  DEBUG ((DEBUG_ERROR, "%a: Use Write Enable Command 0x%x.\n", __func__, Instance->WriteEnableLatchCommand));
}

/**
  This routine returns the desired Fast Read mode.

  @param[in]           Instance                 Spi Nor Flash Instance data with pointer to
                                                EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in,out]       FastReadInstruction      Fast Read instruction, the input is
                                                the default value.
  @param[in,out]       FastReadModeBits         The operational mode bits.
  @param[in,out]       FastReadDummyClocks      Fast Read wait state (Dummy clocks), the
                                                input is the default value.

  @retval EFI_SUCCESS     The parameters are updated.
  @retval EFI_NOT_FOUND   No desired Fas Read mode found.

**/
EFI_STATUS
GetFastReadParameter (
  IN     SPI_NOR_FLASH_INSTANCE  *Instance,
  IN OUT UINT8                   *FastReadInstruction,
  IN OUT UINT8                   *FastReadModeBits,
  IN OUT UINT8                   *FastReadDummyClocks
  )
{
  SFPD_FAST_READ_CAPBILITY_RECORD  *FastReadEntry;

  if (IsListEmpty (&Instance->FastReadTableList)) {
    return EFI_NOT_FOUND;
  }

  FastReadEntry        = (SFPD_FAST_READ_CAPBILITY_RECORD *)GetFirstNode (&Instance->FastReadTableList);
  *FastReadInstruction = FastReadEntry->FastReadInstruction;
  *FastReadDummyClocks = FastReadEntry->WaitStates;
  *FastReadModeBits    = FastReadEntry->ModeClocks;

  //
  // *FastReadOperationClock may be replaced by 8D-8D-8D or 4S-4D-4D Fast Read
  // mode clock operation mode. Which is not cosidered in the implementation yet.
  //
  return EFI_SUCCESS;
}

/**
  Return the flash device size from SFDP Basic Flash Parameter Table DWORD 2.

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and
                          EFI_SPI_IO_PROTOCOL.

  @retval   UINT32        Flash device size in byte, zero indicates error.

**/
UINT32
SfdpGetFlashSize (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  if (Instance == NULL) {
    return 0;
  }

  if ((Instance->SfdpBasicFlash->Density & SFDP_FLASH_MEMORY_DENSITY_4GBIT) == 0) {
    //
    // The flash device size is <= 256MB.
    //
    return (Instance->SfdpBasicFlash->Density + 1) / 8;
  }

  //
  // The flash deivce size is >= 512MB.
  // Bit [0:30] defines 'N' where the density is computed as 2^N bits.
  // N must be >=32 according to the SFDP specification.
  //
  if ((Instance->SfdpBasicFlash->Density & ~SFDP_FLASH_MEMORY_DENSITY_4GBIT) < 32) {
    return 0;
  }

  return (UINT32)RShiftU64 (LShiftU64 (1, Instance->SfdpBasicFlash->Density & ~SFDP_FLASH_MEMORY_DENSITY_4GBIT), 3);
}

/**
  Read SFDP Header

  This routine reads the JEDEC SPI Flash Discoverable Parameter header from the
  SPI chip.  Fails if Major Revision is not = 1

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            Header is filled in
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
EFIAPI
ReadSfdpHeader (
  IN      SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS  Status;
  UINT32      TransactionBufferLength;

  // Check not WIP
  Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));

  // Read SFDP Header
  TransactionBufferLength = FillWriteBuffer (
                              Instance,
                              SPI_FLASH_RDSFDP,
                              SPI_FLASH_RDSFDP_DUMMY,
                              SPI_FLASH_RDSFDP_ADDR_BYTES,
                              TRUE,
                              0,
                              0,
                              NULL
                              );
  Status = Instance->SpiIo->Transaction (
                              Instance->SpiIo,
                              SPI_TRANSACTION_WRITE_THEN_READ,
                              FALSE,
                              0,
                              1,
                              8,
                              TransactionBufferLength,
                              Instance->SpiTransactionWriteBuffer,
                              sizeof (SFDP_HEADER),
                              (UINT8 *)&Instance->SfdpHeader
                              );
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    // Read Basic Flash Parameter Header
    if ((Instance->SfdpHeader.Signature != SFDP_HEADER_SIGNATURE) ||
        (Instance->SfdpHeader.MajorRev != SFDP_SUPPORTED_MAJOR_REVISION))
    {
      Status = EFI_DEVICE_ERROR;
    } else {
      DEBUG ((DEBUG_VERBOSE, "Total %d parameter headers\n", Instance->SfdpHeader.NumParameterHeaders + 1));
    }
  }

  return Status;
}

/**
  Read SFDP
  This routine reads the JEDEC SPI Flash Discoverable Parameters. We just
  read the necessary tables in this routine.

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
                          EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            Header is filled in
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
ReadSfdp (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS                        Status;
  SFDP_SUPPORTED_ERASE_TYPE_RECORD  *EraseTypeRecord;

  InitializeListHead (&Instance->FastReadTableList);
  InitializeListHead (&Instance->SupportedEraseTypes);
  InitializeListHead (&Instance->ConfigurationCommandList);
  InitializeListHead (&Instance->ConfigurationMapList);

  DEBUG ((DEBUG_VERBOSE, "%a: Entry\n", __func__));

  Status = ReadSfdpHeader (Instance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read SFDP header\n", __func__));
    ASSERT (FALSE);
    return Status;
  }

  Status = ReadSfdpBasicParameterTable (Instance);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read SFDP Basic Parameter Table\n", __func__));
    ASSERT (FALSE);
    return Status;
  }

  Instance->FlashDeviceSize = SfdpGetFlashSize (Instance);
  DEBUG ((DEBUG_VERBOSE, "%a: Flash Size=0x%X\n", __func__, Instance->FlashDeviceSize));
  if (Instance->FlashDeviceSize == 0) {
    ASSERT (FALSE);
    return Status;
  }

  Status = ReadSfdpSectorMapParameterTable (Instance);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to read SFDP Sector Map Parameter Table\n", __func__));
    ASSERT (FALSE);
  } else if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_VERBOSE, "%a: The SPI NOR flash device doesn't have SFDP Sector Map Parameter Table implemented:\n", __func__));

    //
    // No SFDP Sector Map Parameter Table exist.
    // Check if device support the uniform 4K erase size.
    //
    Instance->Uniform4KEraseSupported = FALSE;
    if (Instance->SfdpBasicFlash->EraseSizes == SPI_UNIFORM_4K_ERASE_SUPPORTED) {
      DEBUG ((DEBUG_VERBOSE, "%a: The SPI NOR flash device supports uniform 4K erase.\n", __func__));

      // Check if 4K erase type supported?
      Status = GetEraseTypeRecord (Instance, SearchEraseTypeBySize, SIZE_4KB, 0, NULL, &EraseTypeRecord);
      if (Status == EFI_NOT_FOUND) {
        DEBUG ((DEBUG_ERROR, "However, no corresponding 4K size erase type found.\n"));
        ASSERT (FALSE);
      }

      Instance->Uniform4KEraseSupported = TRUE;
    } else {
      // Uniform 4K erase unsupported, get the smallest erase block size.
      DEBUG ((DEBUG_VERBOSE, "%a: The SPI NOR flash device doesn't support uniform 4K erase.\n", __func__));
    }

    //
    // Build flash map
    // Instance->ConfigurationMapList is an empty list because no FDP Sector Map Parameter Table.
    //
    CreateSingleFlashSectorMap (Instance);
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Read SFDP Specific Parameter Header.

  This routine reads the JEDEC SPI Flash Discoverable Parameter header from the
  SPI chip.  Fails if Major Revision is not = SFDP_SUPPORTED_MAJOR_REVISION.

  @param[in]  Instance             Spi Nor Flash Instance data with pointer to
                                   EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]  SfdpParameterHeader  SFDP Header Buffer Pointer
  @param[in]  ParameterIdMsb       Most significant byte of parameter ID.
  @param[in]  ParameterIdLsb       Lowest significant byte of parameter ID.

  @retval EFI_SUCCESS            Header is filled in
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.
  @retval EFI_NOT_FOUND          Unsupported Parameter Header.

**/
EFI_STATUS
EFIAPI
ReadSfdpParameterHeader (
  IN      SPI_NOR_FLASH_INSTANCE  *Instance,
  IN      SFDP_PARAMETER_HEADER   *SfdpParameterHeader,
  IN      UINT8                   ParameterIdMsb,
  IN      UINT8                   ParameterIdLsb
  )
{
  EFI_STATUS             Status;
  UINT32                 Index;
  SFDP_PARAMETER_HEADER  LocalSfdpParameterHeader;
  UINT32                 TransactionBufferLength;

  DEBUG ((DEBUG_VERBOSE, "%a: Entry\n", __func__));
  DEBUG ((DEBUG_VERBOSE, "  Looking for Parameter Header %02x:%02x\n", ParameterIdMsb, ParameterIdLsb));

  //
  // Parse Parameter Headers Starting at size eof SFDP_HEADER.
  // SfdpHeader.NumParameterHeaders is zero based, 0 means 1 parameter header.
  //
  ZeroMem (SfdpParameterHeader, sizeof (SFDP_PARAMETER_HEADER));
  for (Index = 0; Index < Instance->SfdpHeader.NumParameterHeaders + 1; Index++) {
    // Check not WIP
    Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
    if (!EFI_ERROR (Status)) {
      TransactionBufferLength = FillWriteBuffer (
                                  Instance,
                                  SPI_FLASH_RDSFDP,
                                  SPI_FLASH_RDSFDP_DUMMY,
                                  SPI_FLASH_RDSFDP_ADDR_BYTES,
                                  TRUE,
                                  sizeof (SFDP_HEADER) + Index * 8, // Parameter Header Index
                                  0,
                                  NULL
                                  );
      Status = Instance->SpiIo->Transaction (
                                  Instance->SpiIo,
                                  SPI_TRANSACTION_WRITE_THEN_READ,
                                  FALSE,
                                  0,
                                  1,
                                  8,
                                  TransactionBufferLength,
                                  Instance->SpiTransactionWriteBuffer,
                                  sizeof (LocalSfdpParameterHeader),
                                  (UINT8 *)&LocalSfdpParameterHeader
                                  );
      ASSERT_EFI_ERROR (Status);
      if (!EFI_ERROR (Status)) {
        // Break if SfdParamHeader is Type 0, Basic SPI Protocol Parameters
        DEBUG ((
          DEBUG_VERBOSE,
          "  #%d Parameter Header: %02x:%02x, revision: %d.%d\n",
          Index,
          LocalSfdpParameterHeader.IdMsb,
          LocalSfdpParameterHeader.IdLsb,
          LocalSfdpParameterHeader.MajorRev,
          LocalSfdpParameterHeader.MinorRev >= SfdpParameterHeader->MinorRev
          ));
        if ((LocalSfdpParameterHeader.IdLsb == ParameterIdLsb) &&
            (LocalSfdpParameterHeader.IdMsb == ParameterIdMsb) &&
            (LocalSfdpParameterHeader.MajorRev == (UINT32)SFDP_SUPPORTED_MAJOR_REVISION) &&
            (LocalSfdpParameterHeader.MinorRev >= SfdpParameterHeader->MinorRev))
        {
          CopyMem (
            (VOID **)SfdpParameterHeader,
            (VOID **)&LocalSfdpParameterHeader,
            sizeof (SFDP_PARAMETER_HEADER)
            );
        }
      } else {
        break;
      }
    } else {
      break;
    }
  }

  if (Status != EFI_DEVICE_ERROR) {
    if ((SfdpParameterHeader->IdLsb != ParameterIdLsb) ||
        (SfdpParameterHeader->IdMsb != ParameterIdMsb))
    {
      DEBUG ((DEBUG_ERROR, "  Parameter Header: %02x:%02x is not found.\n", ParameterIdMsb, ParameterIdLsb));
      Status =  EFI_NOT_FOUND;
    }
  }

  return Status;
}

/**
  Read from SFDP table pointer.

  This routine sends SPI_FLASH_RDSFDP command and reads parameter from the
  given TablePointer.

  @param[in]  Instance           Spi Nor Flash Instance data with pointer to
                                 EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL
  @param[in]  TablePointer       Pointer to read data from SFDP.
  @param[in]  DestBuffer         Destination buffer.
  @param[in]  LengthInBytes      Length to read.

  @retval EFI_SUCCESS            The SPI part size is filled.
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.
  @retval Other errors

**/
EFI_STATUS
EFIAPI
SpiReadSfdpPtp (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance,
  IN  UINT32                  TablePointer,
  IN  VOID                    *DestBuffer,
  IN  UINT32                  LengthInBytes
  )
{
  EFI_STATUS  Status;
  UINT32      Length;
  UINT8       *CurrentBuffer;
  UINT32      ByteCounter;
  UINT32      CurrentAddress;
  UINT32      MaximumTransferBytes;
  UINT32      TransactionBufferLength;

  Length               = 0;
  MaximumTransferBytes = Instance->SpiIo->MaximumTransferBytes;
  CurrentBuffer        = (UINT8 *)DestBuffer;
  for (ByteCounter = 0; ByteCounter < LengthInBytes; ByteCounter += Length) {
    CurrentAddress = TablePointer + ByteCounter;
    Length         = LengthInBytes - ByteCounter;

    // Length must be MaximumTransferBytes or less
    if (Length > MaximumTransferBytes) {
      Length = MaximumTransferBytes;
    }

    // Check not WIP
    Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));

    //  Read Data
    if (!EFI_ERROR (Status)) {
      TransactionBufferLength = FillWriteBuffer (
                                  Instance,
                                  SPI_FLASH_RDSFDP,
                                  SPI_FLASH_RDSFDP_DUMMY,
                                  SPI_FLASH_RDSFDP_ADDR_BYTES,
                                  TRUE,
                                  CurrentAddress,
                                  0,
                                  NULL
                                  );
      Status = Instance->SpiIo->Transaction (
                                  Instance->SpiIo,
                                  SPI_TRANSACTION_WRITE_THEN_READ,
                                  FALSE,
                                  0,
                                  1,
                                  8,
                                  TransactionBufferLength,
                                  Instance->SpiTransactionWriteBuffer,
                                  Length,
                                  CurrentBuffer
                                  );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Fails to read SFDP parameter.\n", __func__));
        ASSERT_EFI_ERROR (Status);
      }

      CurrentBuffer += Length;
    } else {
      break;
    }
  }

  return Status;
}

/**
  Read SFDP Sector Map Parameter into buffer.

  This routine reads the JEDEC SPI Flash Discoverable Parameters from the SPI
  chip.

  @param[in]  Instance           Spi Nor Flash Instance data with pointer to
                                 EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            The SPI part size is filled.
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
ReadSfdpSectorMapParameterTable (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS             Status;
  SFDP_PARAMETER_HEADER  SfdpParamHeader;

  Status = ReadSfdpParameterHeader (
             Instance,
             &SfdpParamHeader,
             SFDP_SECTOR_MAP_PARAMETER_ID_MSB,
             SFDP_SECTOR_MAP_PARAMETER_ID_LSB
             );
  if (!EFI_ERROR (Status)) {
    // Read Sector Map Parameters.  Already know it is MajorRev = SFDP_SUPPORTED_MAJOR_REVISION
    Instance->SfdpSectorMapByteCount = SfdpParamHeader.Length * sizeof (UINT32);
    Instance->SfdpFlashSectorMap     = AllocateZeroPool (Instance->SfdpSectorMapByteCount);
    if (Instance->SfdpFlashSectorMap != NULL) {
      // Read from SFDP Parameter Table Pointer (PTP).
      Status = SpiReadSfdpPtp (
                 Instance,
                 SfdpParamHeader.TablePointer,
                 (VOID *)Instance->SfdpFlashSectorMap,
                 Instance->SfdpSectorMapByteCount
                 );
      if (!EFI_ERROR (Status)) {
        Status = BuildSectorMapCommandAndMap (Instance);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Fails to build sector map command and descriptor.\n", __func__));
          ASSERT (FALSE);
          Status = GetCurrentSectorMapConfiguration (Instance);
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "%a: Fails to get current sector map configuration.\n", __func__));
            ASSERT (FALSE);
          }
        }
      } else {
        FreePool (Instance->SfdpFlashSectorMap);
        Instance->SfdpFlashSectorMap = NULL;
        DEBUG ((DEBUG_ERROR, "%a: Fails to read SFDP Sector Map Parameter.\n", __func__));
        ASSERT (FALSE);
      }
    } else {
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Fails to allocate memory for reading SFDP Sector Map Parameter.\n", __func__));
        ASSERT (FALSE);
      }
    }
  }

  return Status;
}

/**
  Read SFDP Basic Parameters into buffer.

  This routine reads the JEDEC SPI Flash Discoverable Parameters from the SPI
  chip.

  @param[in]  Instance    Spi Nor Flash Instance data with pointer to
  EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS            The SPI part size is filled.
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.
  @retval EFI_NOT_FOUND          Parameter header is not found.

**/
EFI_STATUS
ReadSfdpBasicParameterTable (
  IN  SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS             Status;
  SFDP_PARAMETER_HEADER  SfdpBasicFlashParamHeader;

  Status = ReadSfdpParameterHeader (
             Instance,
             &SfdpBasicFlashParamHeader,
             SFDP_BASIC_PARAMETER_ID_MSB,
             SFDP_BASIC_PARAMETER_ID_LSB
             );
  if (!EFI_ERROR (Status)) {
    // Read Basic Flash Parameters.  Already know it is MajorRev = SFDP_SUPPORTED_MAJOR_REVISION
    Instance->SfdpBasicFlashByteCount = SfdpBasicFlashParamHeader.Length * sizeof (UINT32);
    Instance->SfdpBasicFlash          = AllocateZeroPool (Instance->SfdpBasicFlashByteCount);
    if (Instance->SfdpBasicFlash != NULL) {
      // Read from SFDP Parameter Table Pointer (PTP).
      Status = SpiReadSfdpPtp (
                 Instance,
                 SfdpBasicFlashParamHeader.TablePointer,
                 (VOID *)Instance->SfdpBasicFlash,
                 Instance->SfdpBasicFlashByteCount
                 );
      if (!EFI_ERROR (Status)) {
        GetWriteEnableCommand (Instance);
        //
        // Build the Fast Read capability table according to
        // the Basic Flash Parameter Table.
        //
        BuildUpFastReadTable (Instance);
        BuildUpEraseTypeTable (Instance); // Build up erase type and size.

        // Set current address bytes to 3-Bytes.
        Instance->CurrentAddressBytes = 3;
      } else {
        FreePool (Instance->SfdpBasicFlash);
        Instance->SfdpBasicFlash = NULL;
        DEBUG ((DEBUG_ERROR, "%a: Fails to read SFDP Basic Parameter.\n", __func__));
        ASSERT (FALSE);
      }
    } else {
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Fails to allocate memory for reading SFDP Basic Parameter.\n", __func__));
        ASSERT (FALSE);
      }
    }
  }

  return Status;
}

/**
  Initial SPI_NOR_FLASH_INSTANCE structure.

  @param[in]   Instance                Pointer to SPI_NOR_FLASH_INSTANCE.
                                       EFI_SPI_NOR_FLASH_PROTOCOL and EFI_SPI_IO_PROTOCOL

  @retval EFI_SUCCESS                  SPI_NOR_FLASH_INSTANCE is initialized according to
                                       SPI NOR Flash SFDP specification.
  @retval EFI_INVALID_PARAMETER        Instance = NULL or
                                       Instance->SpiIo == NULL or
                                       Instance->SpiIo->SpiPeripheral == NULL or
                                       Instance->SpiIo->SpiPeripheral->SpiBus == NULL or
                                       Instance->SpiIo->SpiPeripheral->SpiBus->ControllerPath.
  @retval Otherwise                    Failed to initial SPI_NOR_FLASH_INSTANCE structure.

**/
EFI_STATUS
InitialSpiNorFlashSfdpInstance (
  IN SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS                  Status;
  EFI_SPI_NOR_FLASH_PROTOCOL  *Protocol;

  if (Instance == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Instance is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((Instance->SpiIo == NULL) ||
      (Instance->SpiIo->SpiPeripheral == NULL) ||
      (Instance->SpiIo->SpiPeripheral->SpiBus == NULL)
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: One of SpiIo, SpiPeripheral and SpiBus is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Instance->Signature = SPI_NOR_FLASH_SIGNATURE;

  // Allocate write buffer for SPI IO transactions with extra room for Opcode
  // and Address with 10 bytes extra room.
  Instance->SpiTransactionWriteBuffer =
    AllocatePool (Instance->SpiIo->MaximumTransferBytes + 10);

  Protocol                = &Instance->Protocol;
  Protocol->SpiPeripheral = Instance->SpiIo->SpiPeripheral;
  Protocol->GetFlashid    = GetFlashId;
  Protocol->ReadData      = ReadData;      // Fast Read transfer
  Protocol->LfReadData    = LfReadData;    // Normal Read transfer
  Protocol->ReadStatus    = ReadStatus;
  Protocol->WriteStatus   = WriteStatus;
  Protocol->WriteData     = WriteData;
  Protocol->Erase         = Erase;
  Status                  = Protocol->GetFlashid (Protocol, (UINT8 *)&Protocol->Deviceid);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Flash ID: Manufacturer=0x%02X, Device=0x%02X%02X\n",
    __func__,
    Protocol->Deviceid[0],
    Protocol->Deviceid[1],
    Protocol->Deviceid[2]
    )
    );

  Status = ReadSfdp (Instance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to Read SFDP\n", __func__));
    ASSERT (FALSE);
  }

  // Get flash deivce size from SFDP.
  Protocol->FlashSize = SfdpGetFlashSize (Instance);
  DEBUG ((DEBUG_VERBOSE, "%a: Flash Size=0x%X\n", __func__, Protocol->FlashSize));
  if (Protocol->FlashSize == 0) {
    ASSERT_EFI_ERROR (Status);
  }

  // Set flash erase block size.
  Status = SetSectorEraseBlockSize (Instance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fails to get the smallest erase block size.\n", __func__));
    ASSERT (FALSE);
  }

  return Status;
}
