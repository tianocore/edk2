/** @file
System reset Library Services.  This library class provides a set of
methods to reset whole system with manipulate QNC.

Copyright (c) 2013-2019 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <IntelQNCBase.h>
#include <QNCAccess.h>

#include <Uefi/UefiBaseType.h>

#include <Library/ResetSystemLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/CpuLib.h>
#include <Library/QNCAccessLib.h>

//
// Amount of time (seconds) before RTC alarm fires
// This must be < BCD_BASE
//
#define PLATFORM_WAKE_SECONDS_BUFFER 0x06

//
// RTC 'seconds' above which we will not read to avoid potential rollover
//
#define PLATFORM_RTC_ROLLOVER_LIMIT 0x47

//
// BCD is base 10
//
#define BCD_BASE 0x0A

#define PCAT_RTC_ADDRESS_REGISTER 0x70
#define PCAT_RTC_DATA_REGISTER    0x71

//
// Dallas DS12C887 Real Time Clock
//
#define RTC_ADDRESS_SECONDS           0   // R/W  Range 0..59
#define RTC_ADDRESS_SECONDS_ALARM     1   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES           2   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES_ALARM     3   // R/W  Range 0..59
#define RTC_ADDRESS_HOURS             4   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_HOURS_ALARM       5   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_DAY_OF_THE_WEEK   6   // R/W  Range 1..7
#define RTC_ADDRESS_DAY_OF_THE_MONTH  7   // R/W  Range 1..31
#define RTC_ADDRESS_MONTH             8   // R/W  Range 1..12
#define RTC_ADDRESS_YEAR              9   // R/W  Range 0..99
#define RTC_ADDRESS_REGISTER_A        10  // R/W[0..6]  R0[7]
#define RTC_ADDRESS_REGISTER_B        11  // R/W
#define RTC_ADDRESS_REGISTER_C        12  // RO
#define RTC_ADDRESS_REGISTER_D        13  // RO
#define RTC_ADDRESS_CENTURY           50  // R/W  Range 19..20 Bit 8 is R/W

/**
  Wait for an RTC update to happen

**/
VOID
EFIAPI
WaitForRTCUpdate (
VOID
)
{
  UINT8   Data8;

  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_A);
  Data8 = IoRead8 (PCAT_RTC_DATA_REGISTER);
  if ((Data8 & BIT7) == BIT7) {
    while ((Data8 & BIT7) == BIT7) {
      IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_A);
      Data8 = IoRead8 (PCAT_RTC_DATA_REGISTER);
    }

  } else {
    while ((Data8 & BIT7) == 0) {
      IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_A);
      Data8 = IoRead8 (PCAT_RTC_DATA_REGISTER);
    }

    while ((Data8 & BIT7) == BIT7) {
      IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_A);
      Data8 = IoRead8 (PCAT_RTC_DATA_REGISTER);
    }
  }
}

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  System reset should not return, if it returns, it means the system does
  not support cold reset.
**/
VOID
EFIAPI
ResetCold (
VOID
)
{
  //
  // Reference to QuarkNcSocId BWG
  // Setting bit 1 will generate a warm reset, driving only RSTRDY# low
  //
  IoWrite8 (RST_CNT, B_RST_CNT_COLD_RST);
}

/**
  Calling this function causes a system-wide initialization. The processors
  are set to their initial state, and pending cycles are not corrupted.

  System reset should not return, if it returns, it means the system does
  not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
VOID
)
{
  //
  // Reference to QuarkNcSocId BWG
  // Setting bit 1 will generate a warm reset, driving only RSTRDY# low
  //
  IoWrite8 (RST_CNT, B_RST_CNT_WARM_RST);
}

/**
  Calling this function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  System shutdown should not return, if it returns, it means the system does
  not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
VOID
)
{
  //
  // Reference to QuarkNcSocId BWG
  //  Disable RTC Alarm :  (RTC Enable at PM1BLK + 02h[10]))
  //
  IoWrite16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1E, 0);

  //
  // Firstly, GPE0_EN should be disabled to
  // avoid any GPI waking up the system from S5
  //
  IoWrite32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_GPE0E, 0);

  //
  // Reference to QuarkNcSocId BWG
  //  Disable Resume Well GPIO :  (GPIO bits in GPIOBASE + 34h[8:0])
  //
  IoWrite32 (PcdGet16 (PcdGbaIoBaseAddress) + R_QNC_GPIO_RGGPE_RESUME_WELL, 0);

  //
  // No power button status bit to clear for our platform, go to next step.
  //

  //
  // Finally, transform system into S5 sleep state
  //
  IoAndThenOr32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C, 0xffffc3ff, B_QNC_PM1BLK_PM1C_SLPEN | V_S5);
}

/**
  Calling this function causes the system to enter a power state for capsule
  update.

  Reset update should not return, if it returns, it means the system does
  not support capsule update.

**/
VOID
EFIAPI
EnterS3WithImmediateWake (
VOID
)
{
  UINT8     Data8;
  UINT16    Data16;
  UINT32    Data32;
  UINTN     Eflags;
  UINTN     RegCr0;
  EFI_TIME  EfiTime;
  UINT32    SmiEnSave;

  Eflags  = AsmReadEflags ();
  if ( (Eflags & 0x200) ) {
     DisableInterrupts ();
  }

  //
  //  Write all cache data to memory because processor will lost power
  //
  AsmWbinvd();
  RegCr0 = AsmReadCr0();
  AsmWriteCr0 (RegCr0 | 0x060000000);

  SmiEnSave = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, (SmiEnSave & ~SMI_EN));

  //
  // Pogram RTC alarm for immediate WAKE
  //

  //
  // Disable SMI sources
  //
  IoWrite16 (PcdGet16 (PcdGpe0blkIoBaseAddress) + R_QNC_GPE0BLK_SMIE, 0);

  //
  // Disable RTC alarm interrupt
  //
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_B);
  Data8 = IoRead8 (PCAT_RTC_DATA_REGISTER);
  IoWrite8 (PCAT_RTC_DATA_REGISTER, (Data8 & ~BIT5));

  //
  // Clear RTC alarm if already set
  //
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_C);
  Data8 = IoRead8 (PCAT_RTC_DATA_REGISTER);              // Read clears alarm status

  //
  // Disable all WAKE events
  //
  IoWrite16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1E, B_QNC_PM1BLK_PM1E_PWAKED);

  //
  // Clear all WAKE status bits
  //
  IoWrite16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1S, B_QNC_PM1BLK_PM1S_ALL);

  //
  // Avoid RTC rollover
  //
  do {
    WaitForRTCUpdate();
    IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_SECONDS);
    EfiTime.Second = IoRead8 (PCAT_RTC_DATA_REGISTER);
  } while (EfiTime.Second > PLATFORM_RTC_ROLLOVER_LIMIT);

  //
  // Read RTC time
  //
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_HOURS);
  EfiTime.Hour = IoRead8 (PCAT_RTC_DATA_REGISTER);
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_MINUTES);
  EfiTime.Minute = IoRead8 (PCAT_RTC_DATA_REGISTER);
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_SECONDS);
  EfiTime.Second = IoRead8 (PCAT_RTC_DATA_REGISTER);

  //
  // Set RTC alarm
  //

  //
  // Add PLATFORM_WAKE_SECONDS_BUFFER to current EfiTime.Second
  // The maths is to allow for the fact we are adding to a BCD number and require the answer to be BCD (EfiTime.Second)
  //
  if ((BCD_BASE - (EfiTime.Second & 0x0F)) <= PLATFORM_WAKE_SECONDS_BUFFER) {
    Data8 = (((EfiTime.Second & 0xF0) + 0x10) + (PLATFORM_WAKE_SECONDS_BUFFER - (BCD_BASE - (EfiTime.Second & 0x0F))));
  } else {
    Data8 = EfiTime.Second + PLATFORM_WAKE_SECONDS_BUFFER;
  }

  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_HOURS_ALARM);
  IoWrite8 (PCAT_RTC_DATA_REGISTER, EfiTime.Hour);
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_MINUTES_ALARM);
  IoWrite8 (PCAT_RTC_DATA_REGISTER, EfiTime.Minute);
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_SECONDS_ALARM);
  IoWrite8 (PCAT_RTC_DATA_REGISTER, Data8);

  //
  // Enable RTC alarm interrupt
  //
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, RTC_ADDRESS_REGISTER_B);
  Data8 = IoRead8 (PCAT_RTC_DATA_REGISTER);
  IoWrite8 (PCAT_RTC_DATA_REGISTER, (Data8 | BIT5));

  //
  // Enable RTC alarm as WAKE event
  //
  Data16 = IoRead16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1E);
  IoWrite16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1E, (Data16 | B_QNC_PM1BLK_PM1E_RTC));

  //
  // Enter S3
  //
  Data32 = IoRead32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C);
  Data32  = (UINT32) ((Data32 & 0xffffc3fe) | V_S3 | B_QNC_PM1BLK_PM1C_SCIEN);
  IoWrite32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C, Data32);
  Data32 = Data32 | B_QNC_PM1BLK_PM1C_SLPEN;
  IoWrite32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C, Data32);

  //
  // Enable Interrupt if it's enabled before
  //
  if ( (Eflags & 0x200) ) {
     EnableInterrupts ();
  }
}

/**
  This function causes a systemwide reset. The exact type of the reset is
  defined by the EFI_GUID that follows the Null-terminated Unicode string passed
  into ResetData. If the platform does not recognize the EFI_GUID in ResetData
  the platform must pick a supported reset type to perform.The platform may
  optionally log the parameters from any non-normal reset that occurs.

  @param[in]  DataSize   The size, in bytes, of ResetData.
  @param[in]  ResetData  The data buffer starts with a Null-terminated string,
                         followed by the EFI_GUID.
**/
VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN   DataSize,
  IN VOID    *ResetData
  )
{
  ResetCold ();
}

/**
  The ResetSystem function resets the entire platform.

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
ResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN VOID                         *ResetData OPTIONAL
  )
{
  switch (ResetType) {
  case EfiResetWarm:
    ResetWarm ();
    break;

  case EfiResetCold:
    ResetCold ();
    break;

  case EfiResetShutdown:
    ResetShutdown ();
    return;

  case EfiResetPlatformSpecific:
    ResetPlatformSpecific (DataSize, ResetData);
    return;

  default:
    return;
  }
}
