/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  LegacyMetronome.c

Abstract:

  This contains the installation function for the driver.

--*/

#include "LegacyMetronome.h"

//
// Handle for the Metronome Architectural Protocol instance produced by this driver
//
EFI_HANDLE                  mMetronomeHandle = NULL;

//
// The Metronome Architectural Protocol instance produced by this driver
//
EFI_METRONOME_ARCH_PROTOCOL mMetronome = {
  WaitForTick,
  TICK_PERIOD
};

//
// The CPU I/O Protocol used to access system hardware
//
EFI_CPU_IO_PROTOCOL         *mCpuIo = NULL;

//
// Worker Functions
//

/**
  Write an 8 bit value to an I/O port and save it to the S3 script

  @param Port  IO Port
  @param Data  Data in IO Port

  @retval None.

**/
VOID
ScriptWriteIo8 (
  UINT16  Port,
  UINT8   Data
  )
{
  mCpuIo->Io.Write (
               mCpuIo,
               EfiCpuIoWidthUint8,
               Port,
               1,
               &Data
               );

}

/**

  Read the refresh bit from the REFRESH_PORT

  @param None.

  @retval Refresh bit.

**/
UINT8
ReadRefresh (
  VOID
  )
{
  UINT8 Data;

  mCpuIo->Io.Read (
               mCpuIo,
               EfiCpuIoWidthUint8,
               REFRESH_PORT,
               1,
               &Data
               );
  return (UINT8) (Data & REFRESH_ON);
}

/**

  Waits for the TickNumber of ticks from a known platform time source.

  @param This                Pointer to the protocol instance.
  @param TickNumber          Tick Number to be waited


  @retval EFI_SUCCESS         If number of ticks occurred.
  @retval EFI_NOT_FOUND       Could not locate CPU IO protocol

**/
EFI_STATUS
EFIAPI
WaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  )
{
  //
  // Wait for TickNumber toggles of the Refresh bit
  //
  for (; TickNumber != 0x00; TickNumber--) {
    while (ReadRefresh () == REFRESH_ON)
      ;
    while (ReadRefresh () == REFRESH_OFF)
      ;
  }

  return EFI_SUCCESS;
}

//
// Driver Entry Point
//
/**
  Install the LegacyMetronome driver.  Loads a Metronome Arch Protocol based
  on the Port 61 timer.

  @param ImageHandle      Handle for the image of this driver
  @param SystemTable      Pointer to the EFI System Table

  @retval EFI_SUCCESS     Metronome Architectural Protocol Installed

**/
EFI_STATUS
EFIAPI
InstallLegacyMetronome (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Make sure the Metronome Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiMetronomeArchProtocolGuid);

  //
  // Get the CPU I/O Protocol that this driver requires
  // If the CPU I/O Protocol is not found, then ASSERT because the dependency expression
  // should guarantee that it is present in the handle database.
  //
  Status = gBS->LocateProtocol (
                  &gEfiCpuIoProtocolGuid,
                  NULL,
                  (void **)&mCpuIo
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Program port 61 timer 1 as refresh timer. We could use ACPI timer in the
  // future.
  //
  ScriptWriteIo8 (TIMER1_CONTROL_PORT, LOAD_COUNTER1_LSB);
  ScriptWriteIo8 (TIMER1_COUNT_PORT, COUNTER1_COUNT);

  //
  // Install on a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mMetronomeHandle,
                  &gEfiMetronomeArchProtocolGuid,
                  &mMetronome,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
