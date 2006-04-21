/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    SetWatchdogTimer.c

Abstract:

    Tiano Miscellaneous Services SetWatchdogTimer service implementation

--*/

#include <DxeMain.h>

#define WATCHDOG_TIMER_CALIBRATE_PER_SECOND 10000000


EFI_STATUS
EFIAPI
CoreSetWatchdogTimer (
  IN UINTN    Timeout,
  IN UINT64   WatchdogCode,
  IN UINTN    DataSize,
  IN CHAR16   *WatchdogData OPTIONAL
  )
/*++

Routine Description:

  Sets the system's watchdog timer.

Arguments:

  Timeout         The number of seconds.  Zero disables the timer.

  ///////following  three parameters are left for platform specific using  
  
  WatchdogCode    The numberic code to log.  0x0 to 0xffff are firmware
  DataSize        Size of the optional data
  WatchdogData    Optional Null terminated unicode string followed by binary 
                  data.

Returns:

  EFI_SUCCESS               Timeout has been set
  EFI_NOT_AVAILABLE_YET     WatchdogTimer is not available yet 
  EFI_UNSUPPORTED           System does not have a timer (currently not used)
  EFI_DEVICE_ERROR          Could not complete due to hardware error

--*/
{
  EFI_STATUS  Status;

  //
  // Check our architectural protocol
  //
  if (gWatchdogTimer == NULL) {
    return EFI_NOT_AVAILABLE_YET;
  }

  //
  // Attempt to set the timeout
  //
  Status = gWatchdogTimer->SetTimerPeriod (gWatchdogTimer, MultU64x32 (Timeout, WATCHDOG_TIMER_CALIBRATE_PER_SECOND));

  //
  // Check for errors
  //
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
