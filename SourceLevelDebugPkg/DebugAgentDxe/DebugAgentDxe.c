/** @file
  Initialize Debug Agent in DXE by invoking Debug Agent Library.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/EventGroup.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/UefiLib.h>

EFI_EVENT  mExitBootServiceEvent;

/**
  One notified function to disable Debug Timer interrupt when gBS->ExitBootServices() called.

  @param[in]  Event              Pointer to this event
  @param[in]  Context            Event handler private data

**/
VOID
EFIAPI
DisableDebugTimerExitBootService (
  EFI_EVENT  Event,
  VOID       *Context
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
  @retval other             Some error occurs when initialized Debug Agent.

**/
EFI_STATUS
EFIAPI
DebugAgentDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (gST->ConOut != NULL) {
    Print (L"If the Debug Port is serial port, please make sure this serial port isn't connected by");
    Print (L" ISA Serial driver\r\n");
    Print (L"You could do the following steps to disconnect the serial port:\r\n");
    Print (L"1: Shell> drivers\r\n");
    Print (L"   ...\r\n");
    Print (L"   V  VERSION  E G G #D #C DRIVER NAME                         IMAGE NAME\r\n");
    Print (L"   == ======== = = = == == =================================== ===================\r\n");
    Print (L"   8F 0000000A B - -  1 14 PCI Bus Driver                      PciBusDxe\r\n");
    Print (L"   91 00000010 ? - -  -  - ATA Bus Driver                      AtaBusDxe\r\n");
    Print (L"   ...\r\n");
    Print (L"   A7 0000000A B - -  1  1 ISA Serial Driver                   IsaSerialDxe\r\n");
    Print (L"   ...\r\n");
    Print (L"2: Shell> dh -d A7\r\n");
    Print (L"   A7: Image(IsaSerialDxe) ImageDevPath (..9FB3-11D4-9A3A-0090273FC14D))DriverBinding");
    Print (L" ComponentName ComponentName2\r\n");
    Print (L"        Driver Name    : ISA Serial Driver\r\n");
    Print (L"        Image Name     : FvFile(93B80003-9FB3-11D4-9A3A-0090273FC14D)\r\n");
    Print (L"        Driver Version : 0000000A\r\n");
    Print (L"        Driver Type    : BUS\r\n");
    Print (L"        Configuration  : NO\r\n");
    Print (L"        Diagnostics    : NO\r\n");
    Print (L"        Managing       :\r\n");
    Print (L"          Ctrl[EA] : PciRoot(0x0)/Pci(0x1F,0x0)/Serial(0x0)\r\n");
    Print (L"            Child[EB] : PciRoot(0x0)/Pci(0x1F,0x0)/Serial(0x0)/Uart(115200,8,N,1)\r\n");
    Print (L"3: Shell> disconnect EA\r\n");
    Print (L"4: Shell> load -nc DebugAgentDxe.efi\r\n\r\n");
  }

  Status = EFI_UNSUPPORTED;
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_LOAD, &Status, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (gST->ConOut != NULL) {
    Print (L"Debug Agent: Initialized successfully!\r\n\r\n");
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
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_UNLOAD, &Status, NULL);
  switch (Status) {
    case EFI_ACCESS_DENIED:
      Print (L"Debug Agent: Host is still connected, please de-attach TARGET firstly!\r\n");
      break;
    case EFI_NOT_STARTED:
      Print (L"Debug Agent: It hasn't been initialized, cannot unload it!\r\n");
      break;
  }

  return Status;
}
