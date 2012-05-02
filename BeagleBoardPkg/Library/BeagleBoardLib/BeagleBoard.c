/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
*  
*  This program and the accompanying materials                          
*  are licensed and made available under the terms and conditions of the BSD License         
*  which accompanies this distribution.  The full text of the license may be found at        
*  http://opensource.org/licenses/bsd-license.php                                            
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
*
**/

#include <Library/IoLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Omap3530/Omap3530.h>
#include <BeagleBoard.h>

VOID
PadConfiguration (
  BEAGLEBOARD_REVISION Revision
  );

VOID
ClockInit (
  VOID
  );

/**
  Detect board revision

  @return Board revision
**/
BEAGLEBOARD_REVISION
BeagleBoardGetRevision (
  VOID
  )
{
  UINT32 OldPinDir;
  UINT32 Revision;

  // Read GPIO 171, 172, 173
  OldPinDir = MmioRead32 (GPIO6_BASE + GPIO_OE);
  MmioWrite32(GPIO6_BASE + GPIO_OE, (OldPinDir | BIT11 | BIT12 | BIT13));
  Revision = MmioRead32 (GPIO6_BASE + GPIO_DATAIN);
  
  // Restore I/O settings
  MmioWrite32 (GPIO6_BASE + GPIO_OE, OldPinDir);
  
  return (BEAGLEBOARD_REVISION)((Revision >> 11) & 0x7);
}

/**
  Return if Trustzone is supported by your platform

  A non-zero value must be returned if you want to support a Secure World on your platform.
  ArmPlatformTrustzoneInit() will later set up the secure regions.
  This function can return 0 even if Trustzone is supported by your processor. In this case,
  the platform will continue to run in Secure World.

  @return   A non-zero value if Trustzone supported.

**/
UINTN
ArmPlatformTrustzoneSupported (
  VOID
  )
{
  // The BeagleBoard starts in Normal World (Non Secure World)
	return FALSE;
}

/**
  Remap the memory at 0x0

  Some platform requires or gives the ability to remap the memory at the address 0x0.
  This function can do nothing if this feature is not relevant to your platform.

**/
VOID
ArmPlatformBootRemapping (
  VOID
  )
{
  // Do nothing for the BeagleBoard
}

/**
  Return the current Boot Mode

  This function returns the boot reason on the platform

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  Initialize controllers that must setup at the early stage

  Some peripherals must be initialized in Secure World.
  For example, some L2x0 requires to be initialized in Secure World

**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN                     MpId
  )
{
  BEAGLEBOARD_REVISION Revision;

  Revision = BeagleBoardGetRevision();

  // Set up Pin muxing.
  PadConfiguration (Revision);

  // Set up system clocking
  ClockInit ();

  // Turn off the functional clock for Timer 3
  MmioAnd32 (CM_FCLKEN_PER, 0xFFFFFFFF ^ CM_ICLKEN_PER_EN_GPT3_ENABLE );
  ArmDataSyncronizationBarrier ();

  // Clear IRQs
  MmioWrite32 (INTCPS_CONTROL, INTCPS_CONTROL_NEWIRQAGR);
  ArmDataSyncronizationBarrier ();

  return RETURN_SUCCESS;
}

/**
  Initialize the system (or sometimes called permanent) memory

  This memory is generally represented by the DRAM.

**/
VOID
ArmPlatformInitializeSystemMemory (
  VOID
  )
{
  // We do not need to initialize the System Memory on RTSM
}
