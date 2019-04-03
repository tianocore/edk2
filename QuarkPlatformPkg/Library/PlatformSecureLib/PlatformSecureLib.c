/** @file
Provides a secure platform-specific method to detect physically present user.

Copyright (c) 2013 - 2016 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/PlatformHelperLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/I2cLib.h>

#include <PlatformBoards.h>
#include <Pcal9555.h>
#include <QNCAccess.h>

//
// Global variable to cache pointer to I2C protocol.
//
EFI_PLATFORM_TYPE mPlatformType = TypeUnknown;

BOOLEAN
CheckResetButtonState (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_I2C_DEVICE_ADDRESS  I2CSlaveAddress;
  UINTN                   Length;
  UINTN                   ReadLength;
  UINT8                   Buffer[2];

  DEBUG ((EFI_D_INFO, "CheckResetButtonState(): mPlatformType == %d\n", mPlatformType));
  if (mPlatformType == GalileoGen2) {
    //
    // Read state of Reset Button - EXP2.P1_7
    // This GPIO is pulled high when the button is not pressed
    // This GPIO reads low when button is pressed
    //
    return PlatformPcal9555GpioGetState (
      GALILEO_GEN2_IOEXP2_7BIT_SLAVE_ADDR,  // IO Expander 2.
      15                                    // P1-7.
      );
  }
  if (mPlatformType == Galileo) {
    //
    // Detect the I2C Slave Address of the GPIO Expander
    //
    if (PlatformLegacyGpioGetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, GALILEO_DETERMINE_IOEXP_SLA_RESUMEWELL_GPIO)) {
      I2CSlaveAddress.I2CDeviceAddress = GALILEO_IOEXP_J2HI_7BIT_SLAVE_ADDR;
    } else {
      I2CSlaveAddress.I2CDeviceAddress = GALILEO_IOEXP_J2LO_7BIT_SLAVE_ADDR;
    }
    DEBUG ((EFI_D_INFO, "Galileo GPIO Expender Slave Address = %02x\n", I2CSlaveAddress.I2CDeviceAddress));

    //
    // Read state of RESET_N_SHLD (GPORT5_BIT0)
    //
    Buffer[1] = 5;
    Length = 1;
    ReadLength = 1;
    Status = I2cReadMultipleByte (
               I2CSlaveAddress,
               EfiI2CSevenBitAddrMode,
               &Length,
               &ReadLength,
               &Buffer[1]
               );
    ASSERT_EFI_ERROR (Status);

    //
    // Return the state of GPORT5_BIT0
    //
    return ((Buffer[1] & BIT0) != 0);
  }
  return TRUE;
}

/**

  This function provides a platform-specific method to detect whether the platform
  is operating by a physically present user.

  Programmatic changing of platform security policy (such as disable Secure Boot,
  or switch between Standard/Custom Secure Boot mode) MUST NOT be possible during
  Boot Services or after exiting EFI Boot Services. Only a physically present user
  is allowed to perform these operations.

  NOTE THAT: This function cannot depend on any EFI Variable Service since they are
  not available when this function is called in AuthenticateVariable driver.

  @retval  TRUE       The platform is operated by a physically present user.
  @retval  FALSE      The platform is NOT operated by a physically present user.

**/
BOOLEAN
EFIAPI
UserPhysicalPresent (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // If user has already been detected as present, then return TRUE
  //
  if (PcdGetBool (PcdUserIsPhysicallyPresent)) {
    return TRUE;
  }

  //
  // Check to see if user is present now
  //
  if (CheckResetButtonState ()) {
    //
    // User is still not present, then return FALSE
    //
    return FALSE;
  }

  //
  // User has gone from not present to present state, so set
  // PcdUserIsPhysicallyPresent to TRUE
  //
  Status = PcdSetBoolS (PcdUserIsPhysicallyPresent, TRUE);
  ASSERT_EFI_ERROR (Status);

  return TRUE;
}

/**
  Determines if a user is physically present by reading the reset button state.

  @param  ImageHandle  The image handle of this driver.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS   Install the Secure Boot Helper Protocol successfully.

**/
EFI_STATUS
EFIAPI
PlatformSecureLibInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Get the platform type
  //
  mPlatformType = (EFI_PLATFORM_TYPE)PcdGet16 (PcdPlatformType);

  //
  // Read the state of the reset button when the library is initialized
  //
  Status = PcdSetBoolS (PcdUserIsPhysicallyPresent, !CheckResetButtonState ());
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
