/** @file
  Header file for the SPI flash module.

  Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_COMMON_LIB_H_
#define SPI_COMMON_LIB_H_

#include <PiDxe.h>
#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Pci30.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SpiFlashLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/TimerLib.h>
#include <Guid/SpiFlashInfoGuid.h>
#include "RegsSpi.h"

///
/// Maximum time allowed while waiting the SPI cycle to complete
///  Wait Time = 6 seconds = 6000000 microseconds
///  Wait Period = 10 microseconds
///
#define WAIT_TIME   6000000     ///< Wait Time = 6 seconds = 6000000 microseconds
#define WAIT_PERIOD 10          ///< Wait Period = 10 microseconds

///
/// Flash cycle Type
///
typedef enum {
  FlashCycleRead,
  FlashCycleWrite,
  FlashCycleErase,
  FlashCycleReadSfdp,
  FlashCycleReadJedecId,
  FlashCycleWriteStatus,
  FlashCycleReadStatus,
  FlashCycleMax
} FLASH_CYCLE_TYPE;

///
/// Flash Component Number
///
typedef enum {
  FlashComponent0,
  FlashComponent1,
  FlashComponentMax
} FLASH_COMPONENT_NUM;

///
/// Private data structure definitions for the driver
///
#define SC_SPI_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('P', 'S', 'P', 'I')

typedef struct {
  UINTN                 Signature;
  EFI_HANDLE            Handle;
  UINT32                AcpiTmrReg;
  UINTN                 PchSpiBase;
  UINT16                RegionPermission;
  UINT32                SfdpVscc0Value;
  UINT32                SfdpVscc1Value;
  UINT32                StrapBaseAddress;
  UINT8                 NumberOfComponents;
  UINT16                Flags;
  UINT32                Component1StartAddr;
} SPI_INSTANCE;


/**
  Acquire SPI MMIO BAR

  @param[in] PchSpiBase           PCH SPI PCI Base Address

  @retval                         Return SPI BAR Address

**/
UINT32
AcquireSpiBar0 (
  IN  UINTN         PchSpiBase
  );


/**
  Release SPI MMIO BAR. Do nothing.

  @param[in] PchSpiBase           PCH SPI PCI Base Address

  @retval None

**/
VOID
ReleaseSpiBar0 (
  IN  UINTN         PchSpiBase
  );


/**
  This function is a hook for Spi to disable BIOS Write Protect

  @param[in] PchSpiBase           PCH SPI PCI Base Address
  @param[in] CpuSmmBwp            Need to disable CPU SMM Bios write protection or not

  @retval EFI_SUCCESS             The protocol instance was properly initialized
  @retval EFI_ACCESS_DENIED       The BIOS Region can only be updated in SMM phase

**/
EFI_STATUS
EFIAPI
DisableBiosWriteProtect (
  IN  UINTN         PchSpiBase,
  IN  UINT8         CpuSmmBwp
  );

/**
  This function is a hook for Spi to enable BIOS Write Protect

  @param[in] PchSpiBase           PCH SPI PCI Base Address
  @param[in] CpuSmmBwp            Need to disable CPU SMM Bios write protection or not

  @retval None

**/
VOID
EFIAPI
EnableBiosWriteProtect (
  IN  UINTN         PchSpiBase,
  IN  UINT8         CpuSmmBwp
  );


/**
  This function disables SPI Prefetching and caching,
  and returns previous BIOS Control Register value before disabling.

  @param[in] PchSpiBase           PCH SPI PCI Base Address

  @retval                         Previous BIOS Control Register value

**/
UINT8
SaveAndDisableSpiPrefetchCache (
  IN  UINTN         PchSpiBase
  );

/**
  This function updates BIOS Control Register with the given value.

  @param[in] PchSpiBase           PCH SPI PCI Base Address
  @param[in] BiosCtlValue         BIOS Control Register Value to be updated

  @retval None

**/
VOID
SetSpiBiosControlRegister (
  IN  UINTN         PchSpiBase,
  IN  UINT8         BiosCtlValue
  );


/**
  This function sends the programmed SPI command to the slave device.

  @param[in] SpiRegionType        The SPI Region type for flash cycle which is listed in the Descriptor
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
  );

/**
  Wait execution cycle to complete on the SPI interface.

  @param[in] PchSpiBar0           Spi MMIO base address
  @param[in] ErrorCheck           TRUE if the SpiCycle needs to do the error check

  @retval TRUE                    SPI cycle completed on the interface.
  @retval FALSE                   Time out while waiting the SPI cycle to complete.
                                  It's not safe to program the next command on the SPI interface.
**/
BOOLEAN
WaitForSpiCycleComplete (
  IN     UINT32             PchSpiBar0,
  IN     BOOLEAN            ErrorCheck
  );

#endif
