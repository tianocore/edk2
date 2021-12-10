/** @file
  Generic driver using Hardware Sequencing registers.

  Copyright (c) 2017-2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "SpiCommon.h"

SPI_INSTANCE  *mSpiInstance = NULL;

/**
  Get SPI Instance from library global data..

  @retval SpiInstance       Return SPI instance
**/
SPI_INSTANCE *
GetSpiInstance (
  VOID
  )
{
  if (mSpiInstance == NULL) {
    mSpiInstance = AllocatePool (sizeof (SPI_INSTANCE));
    if (mSpiInstance == NULL) {
      return NULL;
    }

    ZeroMem (mSpiInstance, sizeof (SPI_INSTANCE));
  }

  return mSpiInstance;
}

/**
  Initialize an SPI library.

  @retval EFI_SUCCESS             The protocol instance was properly initialized
  @retval EFI_NOT_FOUND           The expected SPI info could not be found
**/
EFI_STATUS
EFIAPI
SpiConstructor (
  VOID
  )
{
  UINT32             ScSpiBar0;
  UINT8              Comp0Density;
  SPI_INSTANCE       *SpiInstance;
  EFI_HOB_GUID_TYPE  *GuidHob;
  SPI_FLASH_INFO     *SpiFlashInfo;

  //
  // Find SPI flash hob
  //
  GuidHob = GetFirstGuidHob (&gSpiFlashInfoGuid);
  if (GuidHob == NULL) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  SpiFlashInfo = (SPI_FLASH_INFO *)GET_GUID_HOB_DATA (GuidHob);

  //
  // Initialize the SPI instance
  //
  SpiInstance = GetSpiInstance ();
  if (SpiInstance == NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "SpiInstance = %08X\n", SpiInstance));

  SpiInstance->Signature = SC_SPI_PRIVATE_DATA_SIGNATURE;
  SpiInstance->Handle    = NULL;

  //
  // Check the SPI address
  //
  if ((SpiFlashInfo->SpiAddress.AddressSpaceId !=  EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE) ||
      (SpiFlashInfo->SpiAddress.RegisterBitWidth !=  32) ||
      (SpiFlashInfo->SpiAddress.RegisterBitOffset !=  0) ||
      (SpiFlashInfo->SpiAddress.AccessSize !=  EFI_ACPI_3_0_DWORD))
  {
    DEBUG ((DEBUG_ERROR, "SPI FLASH HOB is not expected. need check the hob or enhance SPI flash driver.\n"));
  }

  SpiInstance->PchSpiBase = (UINT32)(UINTN)SpiFlashInfo->SpiAddress.Address;
  SpiInstance->Flags      = SpiFlashInfo->Flags;
  DEBUG ((DEBUG_INFO, "PchSpiBase at 0x%x\n", SpiInstance->PchSpiBase));

  ScSpiBar0 = AcquireSpiBar0 (SpiInstance->PchSpiBase);
  DEBUG ((DEBUG_INFO, "ScSpiBar0 at 0x%08X\n", ScSpiBar0));

  if (ScSpiBar0 == 0) {
    ASSERT (FALSE);
  }

  if ((MmioRead32 (ScSpiBar0 + R_SPI_HSFS) & B_SPI_HSFS_FDV) == 0) {
    DEBUG ((DEBUG_ERROR, "SPI Flash descriptor invalid, cannot use Hardware Sequencing registers!\n"));
    ASSERT (FALSE);
  }

  MmioOr32 (SpiInstance->PchSpiBase + PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_MEMORY_SPACE);
  SpiInstance->RegionPermission = MmioRead16 (ScSpiBar0 + R_SPI_FRAP);
  SpiInstance->SfdpVscc0Value   = MmioRead32 (ScSpiBar0 + R_SPI_LVSCC);
  SpiInstance->SfdpVscc1Value   = MmioRead32 (ScSpiBar0 + R_SPI_UVSCC);

  //
  // Select to Flash Map 0 Register to get the number of flash Component
  //
  MmioAndThenOr32 (
    ScSpiBar0 + R_SPI_FDOC,
    (UINT32)(~(B_SPI_FDOC_FDSS_MASK | B_SPI_FDOC_FDSI_MASK)),
    (UINT32)(V_SPI_FDOC_FDSS_FSDM | R_SPI_FDBAR_FLASH_MAP0)
    );

  //
  // Copy Zero based Number Of Components
  //
  SpiInstance->NumberOfComponents = (UINT8)((MmioRead16 (ScSpiBar0 + R_SPI_FDOD) & B_SPI_FDBAR_NC) >> N_SPI_FDBAR_NC);

  MmioAndThenOr32 (
    ScSpiBar0 + R_SPI_FDOC,
    (UINT32)(~(B_SPI_FDOC_FDSS_MASK | B_SPI_FDOC_FDSI_MASK)),
    (UINT32)(V_SPI_FDOC_FDSS_COMP | R_SPI_FCBA_FLCOMP)
    );

  //
  // Copy Component 0 Density
  //
  Comp0Density                     = (UINT8)MmioRead32 (ScSpiBar0 + R_SPI_FDOD) & B_SPI_FLCOMP_COMP1_MASK;
  SpiInstance->Component1StartAddr = (UINT32)(SIZE_512KB << Comp0Density);

  //
  // Select FLASH_MAP1 to get Flash SC Strap Base Address
  //
  MmioAndThenOr32 (
    (ScSpiBar0 + R_SPI_FDOC),
    (UINT32)(~(B_SPI_FDOC_FDSS_MASK | B_SPI_FDOC_FDSI_MASK)),
    (UINT32)(V_SPI_FDOC_FDSS_FSDM | R_SPI_FDBAR_FLASH_MAP1)
    );

  SpiInstance->StrapBaseAddress = MmioRead32 (ScSpiBar0 + R_SPI_FDOD) & B_SPI_FDBAR_FPSBA;

  //
  // Align FPSBA with address bits for the SC Strap portion of flash descriptor
  //
  SpiInstance->StrapBaseAddress &= B_SPI_FDBAR_FPSBA;

  return EFI_SUCCESS;
}

/**
  Read data from the flash part.

  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[out] Buffer              The Pointer to caller-allocated buffer containing the data received.
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiFlashRead (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  OUT    UINT8              *Buffer
  )
{
  EFI_STATUS  Status;

  Status = SendSpiCmd (FlashRegionType, FlashCycleRead, Address, ByteCount, Buffer);
  return Status;
}

/**
  Write data to the flash part.

  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[in] Buffer               Pointer to caller-allocated buffer containing the data sent during the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiFlashWrite (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  IN     UINT8              *Buffer
  )
{
  EFI_STATUS  Status;

  Status = SendSpiCmd (FlashRegionType, FlashCycleWrite, Address, ByteCount, Buffer);
  return Status;
}

/**
  Erase some area on the flash part.

  @param[in] FlashRegionType      The Flash Region type for flash cycle which is listed in the Descriptor.
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiFlashErase (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount
  )
{
  EFI_STATUS  Status;

  Status = SendSpiCmd (FlashRegionType, FlashCycleErase, Address, ByteCount, NULL);
  return Status;
}

/**
  Read SFDP data from the flash part.

  @param[in] ComponentNumber      The Component Number for chip select
  @param[in] ByteCount            Number of bytes in SFDP data portion of the SPI cycle, the max number is 64
  @param[out] SfdpData            The Pointer to caller-allocated buffer containing the SFDP data received
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiFlashReadSfdp (
  IN     UINT8   ComponentNumber,
  IN     UINT32  ByteCount,
  OUT    UINT8   *SfdpData
  )
{
  EFI_STATUS    Status;
  UINT32        Address;
  SPI_INSTANCE  *SpiInstance;

  SpiInstance = GetSpiInstance ();
  if (SpiInstance == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if ((ByteCount > 64) || (ComponentNumber > SpiInstance->NumberOfComponents)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Address = 0;
  if (ComponentNumber == FlashComponent1) {
    Address = SpiInstance->Component1StartAddr;
  }

  Status = SendSpiCmd (0, FlashCycleReadSfdp, Address, ByteCount, SfdpData);
  return Status;
}

/**
  Read Jedec Id from the flash part.

  @param[in] ComponentNumber      The Component Number for chip select
  @param[in] ByteCount            Number of bytes in JedecId data portion of the SPI cycle, the data size is 3 typically
  @param[out] JedecId             The Pointer to caller-allocated buffer containing JEDEC ID received
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiFlashReadJedecId (
  IN     UINT8   ComponentNumber,
  IN     UINT32  ByteCount,
  OUT    UINT8   *JedecId
  )
{
  EFI_STATUS    Status;
  UINT32        Address;
  SPI_INSTANCE  *SpiInstance;

  SpiInstance = GetSpiInstance ();
  if (SpiInstance == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (ComponentNumber > SpiInstance->NumberOfComponents) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Address = 0;
  if (ComponentNumber == FlashComponent1) {
    Address = SpiInstance->Component1StartAddr;
  }

  Status = SendSpiCmd (0, FlashCycleReadJedecId, Address, ByteCount, JedecId);
  return Status;
}

/**
  Write the status register in the flash part.

  @param[in] ByteCount            Number of bytes in Status data portion of the SPI cycle, the data size is 1 typically
  @param[in] StatusValue          The Pointer to caller-allocated buffer containing the value of Status register writing

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiFlashWriteStatus (
  IN     UINT32  ByteCount,
  IN     UINT8   *StatusValue
  )
{
  EFI_STATUS  Status;

  Status = SendSpiCmd (0, FlashCycleWriteStatus, 0, ByteCount, StatusValue);
  return Status;
}

/**
  Read status register in the flash part.

  @param[in] ByteCount            Number of bytes in Status data portion of the SPI cycle, the data size is 1 typically
  @param[out] StatusValue         The Pointer to caller-allocated buffer containing the value of Status register received.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiFlashReadStatus (
  IN     UINT32  ByteCount,
  OUT    UINT8   *StatusValue
  )
{
  EFI_STATUS  Status;

  Status = SendSpiCmd (0, FlashCycleReadStatus, 0, ByteCount, StatusValue);
  return Status;
}

/**
  Read SC Soft Strap Values

  @param[in] SoftStrapAddr        SC Soft Strap address offset from FPSBA.
  @param[in] ByteCount            Number of bytes in SoftStrap data portion of the SPI cycle
  @param[out] SoftStrapValue      The Pointer to caller-allocated buffer containing SC Soft Strap Value.
                                  It is the caller's responsibility to make sure Buffer is large enough for the total number of bytes read.

  @retval EFI_SUCCESS             Command succeed.
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
  @retval EFI_DEVICE_ERROR        Device error, command aborts abnormally.
**/
EFI_STATUS
EFIAPI
SpiReadPchSoftStrap (
  IN     UINT32  SoftStrapAddr,
  IN     UINT32  ByteCount,
  OUT    UINT8   *SoftStrapValue
  )
{
  UINT32        StrapFlashAddr;
  EFI_STATUS    Status;
  SPI_INSTANCE  *SpiInstance;

  SpiInstance = GetSpiInstance ();
  if (SpiInstance == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ASSERT (SpiInstance->StrapBaseAddress != 0);
  //
  // SC Strap Flash Address = FPSBA + RamAddr
  //
  StrapFlashAddr = SpiInstance->StrapBaseAddress + SoftStrapAddr;

  Status = SendSpiCmd (FlashRegionDescriptor, FlashCycleRead, StrapFlashAddr, ByteCount, SoftStrapValue);
  return Status;
}

/**
  This function sends the programmed SPI command to the slave device.

  @param[in] FlashRegionType      The SPI Region type for flash cycle which is listed in the Descriptor
  @param[in] FlashCycleType       The Flash SPI cycle type list in HSFC (Hardware Sequencing Flash Control Register) register
  @param[in] Address              The Flash Linear Address must fall within a region for which BIOS has access permissions.
  @param[in] ByteCount            Number of bytes in the data portion of the SPI cycle.
  @param[in,out] Buffer           Pointer to caller-allocated buffer containing the data received or sent during the SPI cycle.

  @retval EFI_SUCCESS             SPI command completes successfully.
  @retval EFI_DEVICE_ERROR        Device error, the command aborts abnormally.
  @retval EFI_ACCESS_DENIED       Some unrecognized command encountered in hardware sequencing mode
  @retval EFI_INVALID_PARAMETER   The parameters specified are not valid.
**/
EFI_STATUS
SendSpiCmd (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  IN     FLASH_CYCLE_TYPE   FlashCycleType,
  IN     UINT32             Address,
  IN     UINT32             ByteCount,
  IN OUT UINT8              *Buffer
  )
{
  EFI_STATUS    Status;
  UINT32        Index;
  UINTN         SpiBaseAddress;
  UINT32        ScSpiBar0;
  UINT32        LimitAddress;
  UINT32        HardwareSpiAddr;
  UINT16        PermissionBit;
  UINT32        SpiDataCount;
  UINT32        FlashCycle;
  UINT8         BiosCtlSave;
  SPI_INSTANCE  *SpiInstance;
  UINT32        Data32;

  SpiInstance = GetSpiInstance ();
  if (SpiInstance == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Status                        = EFI_SUCCESS;
  SpiBaseAddress                = SpiInstance->PchSpiBase;
  ScSpiBar0                     = AcquireSpiBar0 (SpiBaseAddress);
  BiosCtlSave                   = 0;
  SpiInstance->RegionPermission = MmioRead16 (ScSpiBar0 + R_SPI_FRAP);

  //
  // If it's write cycle, disable Prefetching, Caching and disable BIOS Write Protect
  //
  if ((FlashCycleType == FlashCycleWrite) || (FlashCycleType == FlashCycleErase)) {
    Status = DisableBiosWriteProtect (SpiBaseAddress, mSpiInstance->Flags & FLAGS_SPI_DISABLE_SMM_WRITE_PROTECT);
    if (EFI_ERROR (Status)) {
      goto SendSpiCmdEnd;
    }

    BiosCtlSave = SaveAndDisableSpiPrefetchCache (SpiBaseAddress);
  }

  //
  // Make sure it's safe to program the command.
  //
  if (!WaitForSpiCycleComplete (ScSpiBar0, FALSE)) {
    Status = EFI_DEVICE_ERROR;
    goto SendSpiCmdEnd;
  }

  HardwareSpiAddr = Address;
  if ((FlashCycleType == FlashCycleRead) ||
      (FlashCycleType == FlashCycleWrite) ||
      (FlashCycleType == FlashCycleErase))
  {
    switch (FlashRegionType) {
      case FlashRegionDescriptor:
        if (FlashCycleType == FlashCycleRead) {
          PermissionBit = B_SPI_FRAP_BRRA_FLASHD;
        } else {
          PermissionBit = B_SPI_FRAP_BRWA_FLASHD;
        }

        Data32           = MmioRead32 (ScSpiBar0 + R_SPI_FREG0_FLASHD);
        HardwareSpiAddr += (Data32 & B_SPI_FREG0_BASE_MASK) << N_SPI_FREG0_BASE;
        LimitAddress     = (Data32 & B_SPI_FREG0_LIMIT_MASK) >> N_SPI_FREG0_LIMIT;
        break;

      case FlashRegionBios:
        if (FlashCycleType == FlashCycleRead) {
          PermissionBit = B_SPI_FRAP_BRRA_BIOS;
        } else {
          PermissionBit = B_SPI_FRAP_BRWA_BIOS;
        }

        Data32           = MmioRead32 (ScSpiBar0 + R_SPI_FREG1_BIOS);
        HardwareSpiAddr += (Data32 & B_SPI_FREG1_BASE_MASK) << N_SPI_FREG1_BASE;
        LimitAddress     = (Data32 & B_SPI_FREG1_LIMIT_MASK) >> N_SPI_FREG1_LIMIT;
        break;

      case FlashRegionMe:
        if (FlashCycleType == FlashCycleRead) {
          PermissionBit = B_SPI_FRAP_BRRA_SEC;
        } else {
          PermissionBit = B_SPI_FRAP_BRWA_SEC;
        }

        Data32           = MmioRead32 (ScSpiBar0 + R_SPI_FREG2_SEC);
        HardwareSpiAddr += (Data32 & B_SPI_FREG2_BASE_MASK) << N_SPI_FREG2_BASE;
        LimitAddress     = (Data32 & B_SPI_FREG2_LIMIT_MASK) >> N_SPI_FREG2_LIMIT;
        break;

      case FlashRegionGbE:
        if (FlashCycleType == FlashCycleRead) {
          PermissionBit = B_SPI_FRAP_BRRA_GBE;
        } else {
          PermissionBit = B_SPI_FRAP_BRWA_GBE;
        }

        Data32           = MmioRead32 (ScSpiBar0 + R_SPI_FREG3_GBE);
        HardwareSpiAddr += (Data32 & B_SPI_FREG3_BASE_MASK) << N_SPI_FREG3_BASE;
        LimitAddress     = (Data32 & B_SPI_FREG3_LIMIT_MASK) >> N_SPI_FREG3_LIMIT;
        break;

      case FlashRegionPlatformData:
        if (FlashCycleType == FlashCycleRead) {
          PermissionBit = B_SPI_FRAP_BRRA_PLATFORM;
        } else {
          PermissionBit = B_SPI_FRAP_BRWA_PLATFORM;
        }

        Data32           = MmioRead32 (ScSpiBar0 + R_SPI_FREG4_PLATFORM_DATA);
        HardwareSpiAddr += (Data32 & B_SPI_FREG4_BASE_MASK) << N_SPI_FREG4_BASE;
        LimitAddress     = (Data32 & B_SPI_FREG4_LIMIT_MASK) >> N_SPI_FREG4_LIMIT;
        break;

      case FlashRegionAll:
        //
        // FlashRegionAll indicates address is relative to flash device
        // No error checking for this case
        //
        LimitAddress  = 0;
        PermissionBit = 0;
        break;

      default:
        Status = EFI_UNSUPPORTED;
        goto SendSpiCmdEnd;
    }

    if ((LimitAddress != 0) && (Address > LimitAddress)) {
      Status = EFI_INVALID_PARAMETER;
      goto SendSpiCmdEnd;
    }

    //
    // If the operation is read, but the region attribute is not read allowed, return error.
    // If the operation is write, but the region attribute is not write allowed, return error.
    //
    if ((PermissionBit != 0) && ((SpiInstance->RegionPermission & PermissionBit) == 0)) {
      Status = EFI_ACCESS_DENIED;
      goto SendSpiCmdEnd;
    }
  }

  //
  // Check for SC SPI hardware sequencing required commands
  //
  FlashCycle = 0;
  switch (FlashCycleType) {
    case FlashCycleRead:
      FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_READ << N_SPI_HSFS_CYCLE);
      break;

    case FlashCycleWrite:
      FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_WRITE << N_SPI_HSFS_CYCLE);
      break;

    case FlashCycleErase:
      if (((ByteCount % SIZE_4KB) != 0) || ((HardwareSpiAddr % SIZE_4KB) != 0)) {
        DEBUG ((DEBUG_ERROR, "Erase and erase size must be 4KB aligned. \n"));
        ASSERT (FALSE);
        Status = EFI_INVALID_PARAMETER;
        goto SendSpiCmdEnd;
      }

      break;

    case FlashCycleReadSfdp:
      FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_READ_SFDP << N_SPI_HSFS_CYCLE);
      break;

    case FlashCycleReadJedecId:
      FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_READ_JEDEC_ID << N_SPI_HSFS_CYCLE);
      break;

    case FlashCycleWriteStatus:
      FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_WRITE_STATUS << N_SPI_HSFS_CYCLE);
      break;

    case FlashCycleReadStatus:
      FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_READ_STATUS << N_SPI_HSFS_CYCLE);
      break;

    default:
      //
      // Unrecognized Operation
      //
      ASSERT (FALSE);
      Status = EFI_INVALID_PARAMETER;
      goto SendSpiCmdEnd;
      break;
  }

  do {
    SpiDataCount = ByteCount;
    if ((FlashCycleType == FlashCycleRead) || (FlashCycleType == FlashCycleWrite)) {
      //
      // Trim at 256 byte boundary per operation,
      // - SC SPI controller requires trimming at 4KB boundary
      // - Some SPI chips require trimming at 256 byte boundary for write operation
      // - Trimming has limited performance impact as we can read / write at most 64 byte
      //   per operation
      //
      if (HardwareSpiAddr + ByteCount > ((HardwareSpiAddr + BIT8) &~(BIT8 - 1))) {
        SpiDataCount = (((UINT32)(HardwareSpiAddr) + BIT8) &~(BIT8 - 1)) - (UINT32)(HardwareSpiAddr);
      }

      //
      // Calculate the number of bytes to shift in/out during the SPI data cycle.
      // Valid settings for the number of bytes during each data portion of the
      // SC SPI cycles are: 0, 1, 2, 3, 4, 5, 6, 7, 8, 16, 24, 32, 40, 48, 56, 64
      //
      if (SpiDataCount >= 64) {
        SpiDataCount = 64;
      } else if ((SpiDataCount &~0x07) != 0) {
        SpiDataCount = SpiDataCount &~0x07;
      }
    }

    if (FlashCycleType == FlashCycleErase) {
      if (((ByteCount / SIZE_64KB) != 0) &&
          ((ByteCount % SIZE_64KB) == 0) &&
          ((HardwareSpiAddr % SIZE_64KB) == 0))
      {
        if (HardwareSpiAddr < SpiInstance->Component1StartAddr) {
          //
          // Check whether Component0 support 64k Erase
          //
          if ((SpiInstance->SfdpVscc0Value & B_SPI_LVSCC_EO_64K) != 0) {
            SpiDataCount = SIZE_64KB;
          } else {
            SpiDataCount = SIZE_4KB;
          }
        } else {
          //
          // Check whether Component1 support 64k Erase
          //
          if ((SpiInstance->SfdpVscc1Value & B_SPI_LVSCC_EO_64K) != 0) {
            SpiDataCount = SIZE_64KB;
          } else {
            SpiDataCount = SIZE_4KB;
          }
        }
      } else {
        SpiDataCount = SIZE_4KB;
      }

      if (SpiDataCount == SIZE_4KB) {
        FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_4K_ERASE << N_SPI_HSFS_CYCLE);
      } else {
        FlashCycle = (UINT32)(V_SPI_HSFS_CYCLE_64K_ERASE << N_SPI_HSFS_CYCLE);
      }
    }

    //
    // If it's write cycle, load data into the SPI data buffer.
    //
    if ((FlashCycleType == FlashCycleWrite) || (FlashCycleType == FlashCycleWriteStatus)) {
      if ((SpiDataCount & 0x07) != 0) {
        //
        // Use Byte write if Data Count is 0, 1, 2, 3, 4, 5, 6, 7
        //
        for (Index = 0; Index < SpiDataCount; Index++) {
          MmioWrite8 (ScSpiBar0 + R_SPI_FDATA00 + Index, Buffer[Index]);
        }
      } else {
        //
        // Use Dword write if Data Count is 8, 16, 24, 32, 40, 48, 56, 64
        //
        for (Index = 0; Index < SpiDataCount; Index += sizeof (UINT32)) {
          MmioWrite32 (ScSpiBar0 + R_SPI_FDATA00 + Index, *(UINT32 *)(Buffer + Index));
        }
      }
    }

    //
    // Set the Flash Address
    //
    MmioWrite32 (ScSpiBar0 + R_SPI_FADDR, (UINT32)(HardwareSpiAddr & B_SPI_FADDR_MASK));

    //
    // Set Data count, Flash cycle, and Set Go bit to start a cycle
    //
    MmioAndThenOr32 (
      ScSpiBar0 + R_SPI_HSFS,
      (UINT32)(~(B_SPI_HSFS_FDBC_MASK | B_SPI_HSFS_CYCLE_MASK)),
      (UINT32)(((SpiDataCount - 1) << N_SPI_HSFS_FDBC) | FlashCycle | B_SPI_HSFS_CYCLE_FGO)
      );

    //
    // Wait for command execution complete.
    //
    if (!WaitForSpiCycleComplete (ScSpiBar0, TRUE)) {
      Status = EFI_DEVICE_ERROR;
      goto SendSpiCmdEnd;
    }

    //
    // If it's read cycle, load data into the caller's buffer.
    //
    if ((FlashCycleType == FlashCycleRead) ||
        (FlashCycleType == FlashCycleReadSfdp) ||
        (FlashCycleType == FlashCycleReadJedecId) ||
        (FlashCycleType == FlashCycleReadStatus))
    {
      if ((SpiDataCount & 0x07) != 0) {
        //
        // Use Byte read if Data Count is 0, 1, 2, 3, 4, 5, 6, 7
        //
        for (Index = 0; Index < SpiDataCount; Index++) {
          Buffer[Index] = MmioRead8 (ScSpiBar0 + R_SPI_FDATA00 + Index);
        }
      } else {
        //
        // Use Dword read if Data Count is 8, 16, 24, 32, 40, 48, 56, 64
        //
        for (Index = 0; Index < SpiDataCount; Index += sizeof (UINT32)) {
          *(UINT32 *)(Buffer + Index) = MmioRead32 (ScSpiBar0 + R_SPI_FDATA00 + Index);
        }
      }
    }

    HardwareSpiAddr += SpiDataCount;
    Buffer          += SpiDataCount;
    ByteCount       -= SpiDataCount;
  } while (ByteCount > 0);

SendSpiCmdEnd:
  ///
  /// Restore the settings for SPI Prefetching and Caching and enable BIOS Write Protect
  ///
  if ((FlashCycleType == FlashCycleWrite) || (FlashCycleType == FlashCycleErase)) {
    EnableBiosWriteProtect (SpiBaseAddress, mSpiInstance->Flags & FLAGS_SPI_DISABLE_SMM_WRITE_PROTECT);
    SetSpiBiosControlRegister (SpiBaseAddress, BiosCtlSave);
  }

  ReleaseSpiBar0 (SpiBaseAddress);

  return Status;
}

/**
  Wait execution cycle to complete on the SPI interface.

  @param[in] ScSpiBar0            Spi MMIO base address
  @param[in] ErrorCheck           TRUE if the SpiCycle needs to do the error check

  @retval TRUE                    SPI cycle completed on the interface.
  @retval FALSE                   Time out while waiting the SPI cycle to complete.
                                  It's not safe to program the next command on the SPI interface.
**/
BOOLEAN
WaitForSpiCycleComplete (
  IN     UINT32   ScSpiBar0,
  IN     BOOLEAN  ErrorCheck
  )
{
  UINT64  WaitTicks;
  UINT64  WaitCount;
  UINT32  Data32;

  //
  // Convert the wait period allowed into to tick count
  //
  WaitCount = WAIT_TIME / WAIT_PERIOD;
  //
  // Wait for the SPI cycle to complete.
  //
  for (WaitTicks = 0; WaitTicks < WaitCount; WaitTicks++) {
    Data32 = MmioRead32 (ScSpiBar0 + R_SPI_HSFS);
    if ((Data32 & B_SPI_HSFS_SCIP) == 0) {
      MmioWrite32 (ScSpiBar0 + R_SPI_HSFS, B_SPI_HSFS_FCERR | B_SPI_HSFS_FDONE);
      if (((Data32 & B_SPI_HSFS_FCERR) != 0) && ErrorCheck) {
        return FALSE;
      } else {
        return TRUE;
      }
    }

    MicroSecondDelay (WAIT_PERIOD);
  }

  return FALSE;
}

/**
  Get the SPI region base and size, based on the enum type

  @param[in] FlashRegionType      The Flash Region type for for the base address which is listed in the Descriptor.
  @param[out] BaseAddress         The Flash Linear Address for the Region 'n' Base
  @param[out] RegionSize          The size for the Region 'n'

  @retval EFI_SUCCESS             Read success
  @retval EFI_INVALID_PARAMETER   Invalid region type given
  @retval EFI_DEVICE_ERROR        The region is not used
**/
EFI_STATUS
EFIAPI
SpiGetRegionAddress (
  IN     FLASH_REGION_TYPE  FlashRegionType,
  OUT    UINT32             *BaseAddress  OPTIONAL,
  OUT    UINT32             *RegionSize OPTIONAL
  )
{
  UINT32        ScSpiBar0;
  UINT32        ReadValue;
  UINT32        Base;
  SPI_INSTANCE  *SpiInstance;

  if (FlashRegionType >= FlashRegionMax) {
    return EFI_INVALID_PARAMETER;
  }

  SpiInstance = GetSpiInstance ();
  if (SpiInstance == NULL) {
    return EFI_DEVICE_ERROR;
  }

  if (FlashRegionType == FlashRegionAll) {
    if (BaseAddress != NULL) {
      *BaseAddress = 0;
    }

    if (RegionSize != NULL) {
      *RegionSize = SpiInstance->Component1StartAddr;
    }

    return EFI_SUCCESS;
  }

  ScSpiBar0 = AcquireSpiBar0 (SpiInstance->PchSpiBase);
  ReadValue = MmioRead32 (ScSpiBar0 + R_SPI_FREG0_FLASHD + S_SPI_FREGX * (UINT32)FlashRegionType);
  ReleaseSpiBar0 (SpiInstance->PchSpiBase);

  //
  // If the region is not used, the Region Base is 7FFFh and Region Limit is 0000h
  //
  if (ReadValue == B_SPI_FREGX_BASE_MASK) {
    return EFI_DEVICE_ERROR;
  }

  Base = (ReadValue & B_SPI_FREG1_BASE_MASK) << N_SPI_FREG1_BASE;
  if (BaseAddress != NULL) {
    *BaseAddress = Base;
  }

  if (RegionSize != NULL) {
    *RegionSize =  ((((ReadValue & B_SPI_FREGX_LIMIT_MASK) >> N_SPI_FREGX_LIMIT) + 1) <<
                    N_SPI_FREGX_LIMIT_REPR) - Base;
  }

  return EFI_SUCCESS;
}
