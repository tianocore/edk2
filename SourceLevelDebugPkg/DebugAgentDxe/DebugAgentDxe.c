/** @file
  Initialize Debug Agent in DXE by invoking Debug Agent Library.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiDxe.h>
#include <Guid/EventGroup.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugAgentLib.h>

EFI_EVENT       mExitBootServiceEvent; 

/**
  One notified function to disable Debug Timer interrupt when gBS->ExitBootServices() called.

  @param[in]  Event              Pointer to this event
  @param[in]  Context            Event hanlder private data

**/
VOID
EFIAPI
DisableDebugTimerExitBootService (
  EFI_EVENT                      Event,
  VOID                           *Context
  )

{
  SaveAndSetDebugTimerInterrupt (FALSE);
}

/**
  The Entry Point for Debug Agent Dxe driver.

  It will invoke Debug Agent Library to enable source debugging feature in DXE phase.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when initialzed Debug Agent.

**/
EFI_STATUS
EFIAPI
DebugAgentDxeInitialize(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS      Status;

  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_LOAD, &Status, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Create event to disable Debug Timer interrupt when exit boot service.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DisableDebugTimerExitBootService,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mExitBootServiceEvent
                  );
  return Status;
}

/**
  This is the unload handle for Debug Agent Dxe driver.

  It will invoke Debug Agent Library to disable source debugging feature.

  @param[in]  ImageHandle       The drivers' driver image.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
DebugAgentDxeUnload (
  IN EFI_HANDLE           ImageHandle
  )
{
  EFI_STATUS          Status;

  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_UNLOAD, &Status, NULL);

  return Status;
}
