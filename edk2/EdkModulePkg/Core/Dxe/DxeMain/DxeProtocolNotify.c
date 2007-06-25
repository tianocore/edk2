/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DxeProtocolNotify.c

Abstract:

  This file deals with Architecture Protocol (AP) registration in
  the Dxe Core. The mArchProtocols[] array represents a list of
  events that represent the Architectural Protocols.

--*/

#include <DxeMain.h>


//
// DXE Core Global Variables for all of the Architectural Protocols.
// If a protocol is installed mArchProtocols[].Present will be TRUE.
//
// CoreNotifyOnArchProtocolInstallation () fills in mArchProtocols[].Event
// and mArchProtocols[].Registration as it creates events for every array
// entry.
//

ARCHITECTURAL_PROTOCOL_ENTRY  mArchProtocols[] = {
  { &gEfiSecurityArchProtocolGuid,         (VOID **)&gSecurity,      NULL, NULL, FALSE },
  { &gEfiCpuArchProtocolGuid,              (VOID **)&gCpu,           NULL, NULL, FALSE },
  { &gEfiMetronomeArchProtocolGuid,        (VOID **)&gMetronome,     NULL, NULL, FALSE },
  { &gEfiTimerArchProtocolGuid,            (VOID **)&gTimer,         NULL, NULL, FALSE },
  { &gEfiBdsArchProtocolGuid,              (VOID **)&gBds,           NULL, NULL, FALSE },
  { &gEfiWatchdogTimerArchProtocolGuid,    (VOID **)&gWatchdogTimer, NULL, NULL, FALSE },
  { &gEfiRuntimeArchProtocolGuid,          (VOID **)&gRuntime,       NULL, NULL, FALSE },
  { &gEfiVariableArchProtocolGuid,         (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiVariableWriteArchProtocolGuid,    (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiCapsuleArchProtocolGuid,          (VOID **)NULL,            NULL, NULL, FALSE},
  { &gEfiMonotonicCounterArchProtocolGuid, (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiResetArchProtocolGuid,            (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiRealTimeClockArchProtocolGuid,    (VOID **)NULL,            NULL, NULL, FALSE },
  { NULL,                                  (VOID **)NULL,            NULL, NULL, FALSE }
};


EFI_STATUS
CoreAllEfiServicesAvailable (
  VOID
  )
/*++

Routine Description:
  Return TRUE if all AP services are availible.

Arguments:
  NONE

Returns:
  EFI_SUCCESS   - All AP services are available
  EFI_NOT_FOUND - At least one AP service is not available

--*/
{
  ARCHITECTURAL_PROTOCOL_ENTRY  *Entry;

  for (Entry = mArchProtocols; Entry->ProtocolGuid != NULL; Entry++) {
    if (!Entry->Present) {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
GenericArchProtocolNotify (
  IN	EFI_EVENT       Event,
  IN	VOID            *Context
  )
/*++

Routine Description:
  Notification event handler registered by CoreNotifyOnArchProtocolInstallation ().
  This notify function is registered for every architectural protocol. This handler
  updates mArchProtocol[] array entry with protocol instance data and sets it's
  present flag to TRUE. If any constructor is required it is executed. The EFI
  System Table headers are updated.

Arguments:

  Event   - The Event that is being processed, not used.

  Context - Event Context, not used.

Returns:

  None

--*/
{
  EFI_STATUS                      Status;
  ARCHITECTURAL_PROTOCOL_ENTRY    *Entry;
  VOID                            *Protocol;
  BOOLEAN                         Found;
  LIST_ENTRY                      *Link;
  LIST_ENTRY                      TempLinkNode;

  Found = FALSE;
  for (Entry = mArchProtocols; Entry->ProtocolGuid != NULL; Entry++) {

    Status = CoreLocateProtocol (Entry->ProtocolGuid, Entry->Registration, &Protocol);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Found = TRUE;
    Entry->Present = TRUE;

    //
    // Update protocol global variable if one exists. Entry->Protocol points to a global variable
    // if one exists in the DXE core for this Architectural Protocol
    //
    if (Entry->Protocol != NULL) {
      *(Entry->Protocol) = Protocol;
    }

    if (CompareGuid (Entry->ProtocolGuid, &gEfiTimerArchProtocolGuid)) {
      //
      // Register the Core timer tick handler with the Timer AP
      //
      gTimer->RegisterHandler (gTimer, CoreTimerTick);
    }

    if (CompareGuid (Entry->ProtocolGuid, &gEfiRuntimeArchProtocolGuid)) {
      //
      // When runtime architectural protocol is available, updates CRC32 in the Debug Table
      //
      CoreUpdateDebugTableCrc32 ();

      //
      // Update the Runtime Architectural protocol with the template that the core was
      // using so there would not need to be a dependency on the Runtime AP
      //

      //
      // Copy all the registered Image to new gRuntime protocol
      //
      for (Link = gRuntimeTemplate.ImageHead.ForwardLink; Link != &gRuntimeTemplate.ImageHead; Link = TempLinkNode.ForwardLink) {
        CopyMem (&TempLinkNode, Link, sizeof(LIST_ENTRY));
        InsertTailList (&gRuntime->ImageHead, Link);
      }
      //
      // Copy all the registered Event to new gRuntime protocol
      //
      for (Link = gRuntimeTemplate.EventHead.ForwardLink; Link != &gRuntimeTemplate.EventHead; Link = TempLinkNode.ForwardLink) {
        CopyMem (&TempLinkNode, Link, sizeof(LIST_ENTRY));
        InsertTailList (&gRuntime->EventHead, Link);
      }

      //
      // Clean up gRuntimeTemplate
      //
      gRuntimeTemplate.ImageHead.ForwardLink = &gRuntimeTemplate.ImageHead;
      gRuntimeTemplate.ImageHead.BackLink    = &gRuntimeTemplate.ImageHead;
      gRuntimeTemplate.EventHead.ForwardLink = &gRuntimeTemplate.EventHead;
      gRuntimeTemplate.EventHead.BackLink    = &gRuntimeTemplate.EventHead;
    }
  }

  //
  // It's over kill to do them all every time, but it saves a lot of code.
  //
  if (Found) {
    CalculateEfiHdrCrc (&gDxeCoreRT->Hdr);
    CalculateEfiHdrCrc (&gDxeCoreBS->Hdr);
    CalculateEfiHdrCrc (&gDxeCoreST->Hdr);
    CalculateEfiHdrCrc (&gDxeCoreDS->Hdr);
  }
}



VOID
CoreNotifyOnArchProtocolInstallation (
  VOID
  )
/*++

Routine Description:
  Creates an event that is fired everytime a Protocol of a specific type is installed

Arguments:
  NONE

Returns:
  NONE

--*/
{
  EFI_STATUS                      Status;
  ARCHITECTURAL_PROTOCOL_ENTRY    *Entry;

  for (Entry = mArchProtocols; Entry->ProtocolGuid != NULL; Entry++) {

    //
    // Create the event
    //
    Status = CoreCreateEvent (
              EVT_NOTIFY_SIGNAL,
              TPL_CALLBACK,
              GenericArchProtocolNotify,
              NULL,
              &Entry->Event
              );
    ASSERT_EFI_ERROR(Status);

    //
    // Register for protocol notifactions on this event
    //
    Status = CoreRegisterProtocolNotify (
              Entry->ProtocolGuid,
              Entry->Event,
              &Entry->Registration
              );
    ASSERT_EFI_ERROR(Status);

  }
}

//
// Following is needed to display missing architectural protocols in debug builds
//
typedef struct {
  EFI_GUID                    *ProtocolGuid;
  CHAR16                       *GuidString;
} GUID_TO_STRING_PROTOCOL_ENTRY;

static const GUID_TO_STRING_PROTOCOL_ENTRY MissingProtocols[] = {
  { &gEfiSecurityArchProtocolGuid,         (CHAR16 *)L"Security"           },
  { &gEfiCpuArchProtocolGuid,              (CHAR16 *)L"CPU"                },
  { &gEfiMetronomeArchProtocolGuid,        (CHAR16 *)L"Metronome"          },
  { &gEfiTimerArchProtocolGuid,            (CHAR16 *)L"Timer"              },
  { &gEfiBdsArchProtocolGuid,              (CHAR16 *)L"Bds"                },
  { &gEfiWatchdogTimerArchProtocolGuid,    (CHAR16 *)L"Watchdog Timer"     },
  { &gEfiRuntimeArchProtocolGuid,          (CHAR16 *)L"Runtime"            },
  { &gEfiVariableArchProtocolGuid,         (CHAR16 *)L"Variable"           },
  { &gEfiVariableWriteArchProtocolGuid,    (CHAR16 *)L"Variable Write"     },
  { &gEfiCapsuleArchProtocolGuid,          (CHAR16 *)L"Capsule"            },
  { &gEfiMonotonicCounterArchProtocolGuid, (CHAR16 *)L"Monotonic Counter"  },
  { &gEfiResetArchProtocolGuid,            (CHAR16 *)L"Reset"              },
//  { &gEfiStatusCodeRuntimeProtocolGuid,       (CHAR16 *)L"Status Code"        },
  { &gEfiRealTimeClockArchProtocolGuid,    (CHAR16 *)L"Real Time Clock"    }
};

VOID
CoreDisplayMissingArchProtocols (
  VOID
  )
/*++

Routine Description:
  Displays Architectural protocols that were not loaded and are required for DXE core to function
  Only used in Debug Builds

Arguments:
  NONE

Returns:
  NONE

--*/
{
  const GUID_TO_STRING_PROTOCOL_ENTRY  *MissingEntry;
  ARCHITECTURAL_PROTOCOL_ENTRY         *Entry;

  for (Entry = mArchProtocols; Entry->ProtocolGuid != NULL; Entry++) {
    if (!Entry->Present) {
      MissingEntry = MissingProtocols;
      for (MissingEntry = MissingProtocols; TRUE ; MissingEntry++) {
        if (CompareGuid (Entry->ProtocolGuid, MissingEntry->ProtocolGuid)) {
          DEBUG ((EFI_D_ERROR, "\n%s Arch Protocol not present!!\n", MissingEntry->GuidString));
          break;
        }
      }
    }
  }
}
