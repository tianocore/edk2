/*++

Copyright (c) 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  LegacyMetronome.c

Abstract:

  This contains the installation function for the driver.

--*/

#include "Metronome.h"

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
VOID
IoWrite8 (
  UINT16  Port,
  UINT8   Data
  )
/*++

Routine Description:

  Write an 8 bit value to an I/O port and save it to the S3 script

Arguments:

Returns: 

  None.

--*/
// TODO:    Port - add argument and description to function comment
// TODO:    Data - add argument and description to function comment
{
  mCpuIo->Io.Write (
              mCpuIo,
              EfiCpuIoWidthUint8,
              Port,
              1,
              &Data
              );

}

UINT8
ReadRefresh (
  VOID
  )
/*++

Routine Description:

  Read the refresh bit from the REFRESH_PORT

Arguments:

Returns: 

  None.

--*/
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

EFI_STATUS
EFIAPI
WaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  )
/*++

Routine Description:

  Waits for the TickNumber of ticks from a known platform time source.

Arguments:

  This                Pointer to the protocol instance.

Returns: 

  EFI_SUCCESS         If number of ticks occurred.
  EFI_NOT_FOUND       Could not locate CPU IO protocol

--*/
// TODO:    TickNumber - add argument and description to function comment
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

EFI_STATUS
EFIAPI
InstallMetronome (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:
  
  Install the LegacyMetronome driver.  Loads a Metronome Arch Protocol based
  on the Port 61 timer.

Arguments:

  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCCESS - Metronome Architectural Protocol Installed

--*/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
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
  Status = gBS->LocateProtocol (&gEfiCpuIoProtocolGuid, NULL, &mCpuIo);
  ASSERT_EFI_ERROR (Status);

  //
  // Program port 61 timer 1 as refresh timer. We could use ACPI timer in the
  // future.
  //
  IoWrite8 (TIMER1_CONTROL_PORT, LOAD_COUNTER1_LSB);
  IoWrite8 (TIMER1_COUNT_PORT, COUNTER1_COUNT);

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
