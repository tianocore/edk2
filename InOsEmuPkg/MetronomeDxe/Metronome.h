/*++ @file
  Emu Emulation Metronome Architectural Protocol Driver as defined in DXE CIS

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _EMU_THUNK_METRONOME_H_
#define _EMU_THUNK_METRONOME_H_

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/EmuThunkLib.h>

#include <Protocol/Metronome.h>



//
// Period of on tick in 100 nanosecond units
//
#define TICK_PERIOD 2000

//
// Function Prototypes
//

EFI_STATUS
EFIAPI
EmuMetronomeDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
EFIAPI
EmuMetronomeDriverWaitForTick (
  IN EFI_METRONOME_ARCH_PROTOCOL  *This,
  IN UINT32                       TickNumber
  );

#endif
