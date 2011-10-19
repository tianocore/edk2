/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    EfiSmmDriverLib.h

Abstract:

  Light weight lib to support EFI Smm drivers.

--*/

#ifndef _EFI_SMM_DRIVER_LIB_H_
#define _EFI_SMM_DRIVER_LIB_H_

#include "Tiano.h"
#include "GetImage.h"
#include "EfiCommonLib.h"
#include EFI_GUID_DEFINITION (EventLegacyBios)
#include EFI_GUID_DEFINITION (EventGroup)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume2)
#include EFI_PROTOCOL_DEFINITION (SmmBase)
#include EFI_PROTOCOL_DEFINITION (SmmStatusCode)
//
// Driver Lib Globals.
//
extern EFI_BOOT_SERVICES            *gBS;
extern EFI_SYSTEM_TABLE             *gST;
extern EFI_RUNTIME_SERVICES         *gRT;
extern EFI_SMM_BASE_PROTOCOL        *gSMM;
extern EFI_SMM_STATUS_CODE_PROTOCOL *mSmmDebug;
extern UINTN                        gErrorLevel;

#define EfiCopyMem  EfiCommonLibCopyMem
#define EfiSetMem   EfiCommonLibSetMem
#define EfiZeroMem  EfiCommonLibZeroMem

EFI_STATUS
EfiInitializeSmmDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable,
  IN OUT BOOLEAN          *InSmm
  )
/*++

Routine Description:

  Intialize Smm Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.
  
  InSmm           - If InSmm is NULL, it will not register Image to SMM.
                    If InSmm is not NULL, it will register Image to SMM and
                      return information on currently in SMM mode or not.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

VOID
EfiDebugAssert (
  IN CHAR8    *FileName,
  IN INTN     LineNumber,
  IN CHAR8    *Description
  )
/*++

Routine Description:

  Worker function for ASSERT (). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded DEADLOOP ().
  
Arguments:

  FileName    - File name of failing routine.

  LineNumber  - Line number of failing ASSERT().

  Description - Description, usually the assertion,
  
Returns:
  
  None

--*/
;

VOID
EfiDebugVPrint (
  IN  UINTN   ErrorLevel,
  IN  CHAR8   *Format,
  IN  VA_LIST Marker
  )
/*++

Routine Description:

  Worker function for DEBUG(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.
  
Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  Marker     - VarArgs
  
Returns:
  
  None

--*/
;

VOID
EfiDebugPrint (
  IN  UINTN                   ErrorLevel,
  IN  CHAR8                   *Format,
  ...
  )
/*++

Routine Description:

  Worker function for DEBUG(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.

Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  ...        - VAR args for Format

Returns:
  
  None

--*/
;

EFI_STATUS
EFIAPI
SmmEfiCreateEventLegacyBoot (
  IN EFI_TPL                      NotifyTpl,
  IN EFI_EVENT_NOTIFY             NotifyFunction,
  IN VOID                         *NotifyContext,
  OUT EFI_EVENT                   *LegacyBootEvent
  )
/*++

Routine Description:
  Create a Legacy Boot Event.  
  Tiano extended the CreateEvent Type enum to add a legacy boot event type. 
  This was bad as Tiano did not own the enum. In UEFI 2.0 CreateEventEx was
  added and now it's possible to not voilate the UEFI specification by 
  declaring a GUID for the legacy boot event class. This library supports
  the EFI 1.10 form and UEFI 2.0 form and allows common code to work both ways.

Arguments:
  LegacyBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex)

Returns:
  EFI_SUCCESS   Event was created.
  Other         Event was not created.

--*/
;

EFI_STATUS
EFIAPI
SmmEfiCreateEventReadyToBoot (
  IN EFI_TPL                      NotifyTpl,
  IN EFI_EVENT_NOTIFY             NotifyFunction,
  IN VOID                         *NotifyContext,
  OUT EFI_EVENT                   *ReadyToBootEvent
  )
/*++

Routine Description:
  Create a Read to Boot Event.  
  
  Tiano extended the CreateEvent Type enum to add a ready to boot event type. 
  This was bad as Tiano did not own the enum. In UEFI 2.0 CreateEventEx was
  added and now it's possible to not voilate the UEFI specification and use 
  the ready to boot event class defined in UEFI 2.0. This library supports
  the EFI 1.10 form and UEFI 2.0 form and allows common code to work both ways.

Arguments:
  ReadyToBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex)

Return:
  EFI_SUCCESS   - Event was created.
  Other         - Event was not created.

--*/
;

#endif
