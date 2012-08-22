/** @file
  This file deals with Architecture Protocol (AP) registration in
  the Dxe Core. The mArchProtocols[] array represents a list of
  events that represent the Architectural Protocols.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"

//
// DXE Core Global Variables for all of the Architectural Protocols.
// If a protocol is installed mArchProtocols[].Present will be TRUE.
//
// CoreNotifyOnArchProtocolInstallation () fills in mArchProtocols[].Event
// and mArchProtocols[].Registration as it creates events for every array
// entry.
//
EFI_CORE_PROTOCOL_NOTIFY_ENTRY  mArchProtocols[] = {
  { &gEfiSecurityArchProtocolGuid,         (VOID **)&gSecurity,      NULL, NULL, FALSE },
  { &gEfiCpuArchProtocolGuid,              (VOID **)&gCpu,           NULL, NULL, FALSE },
  { &gEfiMetronomeArchProtocolGuid,        (VOID **)&gMetronome,     NULL, NULL, FALSE },
  { &gEfiTimerArchProtocolGuid,            (VOID **)&gTimer,         NULL, NULL, FALSE },
  { &gEfiBdsArchProtocolGuid,              (VOID **)&gBds,           NULL, NULL, FALSE },
  { &gEfiWatchdogTimerArchProtocolGuid,    (VOID **)&gWatchdogTimer, NULL, NULL, FALSE },
  { &gEfiRuntimeArchProtocolGuid,          (VOID **)&gRuntime,       NULL, NULL, FALSE },
  { &gEfiVariableArchProtocolGuid,         (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiVariableWriteArchProtocolGuid,    (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiCapsuleArchProtocolGuid,          (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiMonotonicCounterArchProtocolGuid, (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiResetArchProtocolGuid,            (VOID **)NULL,            NULL, NULL, FALSE },
  { &gEfiRealTimeClockArchProtocolGuid,    (VOID **)NULL,            NULL, NULL, FALSE },
  { NULL,                                  (VOID **)NULL,            NULL, NULL, FALSE }
};

//
// Optional protocols that the DXE Core will use if they are present
//
EFI_CORE_PROTOCOL_NOTIFY_ENTRY  mOptionalProtocols[] = {
  { &gEfiSecurity2ArchProtocolGuid,        (VOID **)&gSecurity2,     NULL, NULL, FALSE },
  { &gEfiSmmBase2ProtocolGuid,             (VOID **)&gSmmBase2,      NULL, NULL, FALSE },
  { NULL,                                  (VOID **)NULL,            NULL, NULL, FALSE }
};

//
// Following is needed to display missing architectural protocols in debug builds
//
typedef struct {
  EFI_GUID  *ProtocolGuid;
  CHAR8     *GuidString;
} GUID_TO_STRING_PROTOCOL_ENTRY;

GLOBAL_REMOVE_IF_UNREFERENCED CONST GUID_TO_STRING_PROTOCOL_ENTRY mMissingProtocols[] = {
  { &gEfiSecurityArchProtocolGuid,         "Security"           },
  { &gEfiCpuArchProtocolGuid,              "CPU"                },
  { &gEfiMetronomeArchProtocolGuid,        "Metronome"          },
  { &gEfiTimerArchProtocolGuid,            "Timer"              },
  { &gEfiBdsArchProtocolGuid,              "Bds"                },
  { &gEfiWatchdogTimerArchProtocolGuid,    "Watchdog Timer"     },
  { &gEfiRuntimeArchProtocolGuid,          "Runtime"            },
  { &gEfiVariableArchProtocolGuid,         "Variable"           },
  { &gEfiVariableWriteArchProtocolGuid,    "Variable Write"     },
  { &gEfiCapsuleArchProtocolGuid,          "Capsule"            },
  { &gEfiMonotonicCounterArchProtocolGuid, "Monotonic Counter"  },
  { &gEfiResetArchProtocolGuid,            "Reset"              },
  { &gEfiRealTimeClockArchProtocolGuid,    "Real Time Clock"    },
  { NULL,                                  ""                   }
};

/**
  Return TRUE if all AP services are availible.

  @retval EFI_SUCCESS    All AP services are available
  @retval EFI_NOT_FOUND  At least one AP service is not available

**/
EFI_STATUS
CoreAllEfiServicesAvailable (
  VOID
  )
{
  EFI_CORE_PROTOCOL_NOTIFY_ENTRY  *Entry;

  for (Entry = mArchProtocols; Entry->ProtocolGuid != NULL; Entry++) {
    if (!Entry->Present) {
      return EFI_NOT_FOUND;
    }
  }
  return EFI_SUCCESS;
}


/**
  Notification event handler registered by CoreNotifyOnArchProtocolInstallation ().
  This notify function is registered for every architectural protocol. This handler
  updates mArchProtocol[] array entry with protocol instance data and sets it's
  present flag to TRUE. If any constructor is required it is executed. The EFI
  System Table headers are updated.

  @param  Event          The Event that is being processed, not used.
  @param  Context        Event Context, not used.

**/
VOID
EFIAPI
GenericProtocolNotify (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                      Status;
  EFI_CORE_PROTOCOL_NOTIFY_ENTRY  *Entry;
  VOID                            *Protocol;
  LIST_ENTRY                      *Link;
  LIST_ENTRY                      TempLinkNode;

  //
  // Get Entry from Context
  //
  Entry = (EFI_CORE_PROTOCOL_NOTIFY_ENTRY *)Context;

  //
  // See if the expected protocol is present in the handle database
  //
  Status = CoreLocateProtocol (Entry->ProtocolGuid, Entry->Registration, &Protocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Mark the protocol as present
  //
  Entry->Present = TRUE;

  //
  // Update protocol global variable if one exists. Entry->Protocol points to a global variable
  // if one exists in the DXE core for this Architectural Protocol
  //
  if (Entry->Protocol != NULL) {
    *(Entry->Protocol) = Protocol;
  }

  //
  // Do special operations for Architectural Protocols
  //

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

  //
  // It's over kill to do them all every time, but it saves a lot of code.
  //
  CalculateEfiHdrCrc (&gDxeCoreRT->Hdr);
  CalculateEfiHdrCrc (&gBS->Hdr);
  CalculateEfiHdrCrc (&gDxeCoreST->Hdr);
  CalculateEfiHdrCrc (&gDxeCoreDS->Hdr);
}

/**
  Creates an event for each entry in a table that is fired everytime a Protocol 
  of a specific type is installed.

  @param Entry  Pointer to EFI_CORE_PROTOCOL_NOTIFY_ENTRY.

**/
VOID
CoreNotifyOnProtocolEntryTable (
  EFI_CORE_PROTOCOL_NOTIFY_ENTRY  *Entry
  )
{
  EFI_STATUS  Status;

  for (; Entry->ProtocolGuid != NULL; Entry++) {
    //
    // Create the event
    //
    Status = CoreCreateEvent (
              EVT_NOTIFY_SIGNAL,
              TPL_CALLBACK,
              GenericProtocolNotify,
              Entry,
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

/**
  Creates an events for the Architectural Protocols and the optional protocols 
  that are fired everytime a Protocol of a specific type is installed.

**/
VOID
CoreNotifyOnProtocolInstallation (
  VOID
  )
{
  CoreNotifyOnProtocolEntryTable (mArchProtocols);
  CoreNotifyOnProtocolEntryTable (mOptionalProtocols);
}


/**
  Displays Architectural protocols that were not loaded and are required for DXE
  core to function.  Only used in Debug Builds.

**/
VOID
CoreDisplayMissingArchProtocols (
  VOID
  )
{
  EFI_CORE_PROTOCOL_NOTIFY_ENTRY       *Entry;
  CONST GUID_TO_STRING_PROTOCOL_ENTRY  *MissingEntry;

  for (Entry = mArchProtocols; Entry->ProtocolGuid != NULL; Entry++) {
    if (!Entry->Present) {
      for (MissingEntry = mMissingProtocols; MissingEntry->ProtocolGuid != NULL; MissingEntry++) {
        if (CompareGuid (Entry->ProtocolGuid, MissingEntry->ProtocolGuid)) {
          DEBUG ((DEBUG_ERROR, "\n%a Arch Protocol not present!!\n", MissingEntry->GuidString));
          break;
        }
      }
    }
  }
}
