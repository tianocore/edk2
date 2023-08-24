/** @file
  Realm Aperture Management Protocol Dxe

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - IPA   - Intermediate Physical Address
    - RAMP  - Realm Aperture Management Protocol
    - RIPAS - Realm IPA state
    - RSI   - Realm Service Interface
**/

#include <Base.h>
#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/RealmApertureManagementProtocol.h>
#include <Protocol/ResetNotification.h>

/**
  A macro defining the signature for the aperture information structure.
*/
#define APERTURE_INFO_SIG  SIGNATURE_64 ('A', 'P', 'E', 'R', 'T', 'U', 'R', 'E')

/**
  A structure describing the aperture.
*/
typedef struct {
  /// Signature for identifying this structure.
  UINT64                  Signature;

  /// The linked list entry.
  LIST_ENTRY              Link;

  /// The base address for the start of the aperture.
  EFI_PHYSICAL_ADDRESS    BaseAddress;

  /// The number of pages covered by the aperture.
  UINTN                   Pages;

  /// The bit mask of attributes for the memory region. The
  /// bit mask of available attributes is defined in GetMemoryMap().
  UINT64                  MemoryAttributes;

  /// The RIPAS for the aperture.
  RIPAS                   Ripas;
} APERTURE_INFO;

/**
  List of the APERTURE_INFO structures that have been set up by OpenAperture()
  and not yet torn down by CloseAperture(). The list represents the full set
  of open apertures currently in effect.
*/
STATIC
LIST_ENTRY  mApertureInfos = INITIALIZE_LIST_HEAD_VARIABLE (mApertureInfos);

/**
  A local variable to store the IPA width of the Realm. The IPA width
  of the Realm is required to configure the protection attribute of
  memory regions.
*/
STATIC UINT64  mIpaWidth;

/** Checks if an open aperture is overlapping the memory region.

  @param [in]  MemStart           Pointer to the page start address.
  @param [in]  Pages              Number of pages to share.

  @retval TRUE  If memory region overlaps an open aperture.
  @retval FALSE Memory region does not overlap any open apertures.
**/
STATIC
BOOLEAN
EFIAPI
IsApertureOverlapping (
  IN  CONST EFI_PHYSICAL_ADDRESS  MemStart,
  IN  CONST UINTN                 Pages
  )
{
  LIST_ENTRY            *Node;
  LIST_ENTRY            *NextNode;
  APERTURE_INFO         *ApertureInfo;
  EFI_PHYSICAL_ADDRESS  MemEnd;
  EFI_PHYSICAL_ADDRESS  ApertureStart;
  EFI_PHYSICAL_ADDRESS  ApertureEnd;

  MemEnd = MemStart + (EFI_PAGE_SIZE * Pages) - 1;

  // All drivers that had opened the apertures have halted their respective
  // controllers by now; close all the apertures.
  for (
       Node = GetFirstNode (&mApertureInfos);
       Node != &mApertureInfos;
       Node = NextNode
       )
  {
    NextNode      = GetNextNode (&mApertureInfos, Node);
    ApertureInfo  = CR (Node, APERTURE_INFO, Link, APERTURE_INFO_SIG);
    ApertureStart = ApertureInfo->BaseAddress;
    ApertureEnd   = ApertureStart + (EFI_PAGE_SIZE * ApertureInfo->Pages) - 1;

    if (((ApertureStart >= MemStart) && (ApertureStart <= MemEnd)) ||
        ((ApertureEnd >= MemStart) && (ApertureEnd <= MemEnd))     ||
        ((MemStart >= ApertureStart) && (MemStart <= ApertureEnd)) ||
        ((MemEnd >= ApertureStart) && (MemEnd <= ApertureEnd)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/** Enables sharing of the memory buffers with the host.

  @param [in]  Memory             Pointer to the page start address.
  @param [in]  Pages              Number of pages to share.
  @param [out] ApertureReference  Reference to the opened aperture.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
  @retval EFI_ACCESS_DENIED       Aperture already open over memory region.
**/
STATIC
EFI_STATUS
EFIAPI
RampOpenAperture (
  IN  CONST EFI_PHYSICAL_ADDRESS                    Memory,
  IN  CONST UINTN                                   Pages,
  OUT       EFI_HANDLE                      *CONST  ApertureReference
  )
{
  EFI_STATUS                       Status;
  EFI_STATUS                       Status1;
  APERTURE_INFO                    *ApertInfo;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdDescriptor;
  EFI_PHYSICAL_ADDRESS             MemRangeAddr;
  UINTN                            Index;

  if ((Memory == 0) ||
      (Pages == 0)  ||
      (ApertureReference == NULL) ||
      ((Memory & (EFI_PAGE_SIZE - 1)) != 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // The pages size must be aligned to the Realm Granule size.
  STATIC_ASSERT ((EFI_PAGE_SIZE & (REALM_GRANULE_SIZE - 1)) == 0);

  // Checks if we already have an open aperture that overlaps the
  // memory region. If so return the request as invalid.
  if (IsApertureOverlapping (Memory, Pages)) {
    return EFI_INVALID_PARAMETER;
  }

  MemRangeAddr = Memory;
  for (Index = 0; Index < Pages; Index++) {
    Status = gDS->GetMemorySpaceDescriptor (MemRangeAddr, &GcdDescriptor);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: Memory = 0x%lx, MemType = %a\n",
      __func__,
      MemRangeAddr,
      ((GcdDescriptor.Attributes & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME) ?
      "Runtime Services Memory" : "Boot Services Memory"
      ));

    // We currently do not have a usecase where we would want to open apertures
    // for runtime services memory
    if ((GcdDescriptor.Attributes & EFI_MEMORY_RUNTIME) == EFI_MEMORY_RUNTIME) {
      return EFI_UNSUPPORTED;
    }

    MemRangeAddr += EFI_PAGE_SIZE;
  } // for

  Status = ArmCcaSetMemoryProtectAttribute (
             Memory,
             EFI_PAGES_TO_SIZE (Pages),
             mIpaWidth,
             TRUE
             );
  if (RETURN_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to update page tables for Protected EMPTY page mapping, "
      "Address = %p, Pages = 0x%lx, Status = %r\n",
      Memory,
      Pages,
      Status
      ));
    return Status;
  }

  // Allocate a APERTURE_INFO structure to remember the apertures opened.
  ApertInfo = AllocateZeroPool (sizeof (APERTURE_INFO));
  if (ApertInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto error_handler1;
  }

  InitializeListHead (&ApertInfo->Link);
  ApertInfo->Signature        = APERTURE_INFO_SIG;
  ApertInfo->BaseAddress      = Memory;
  ApertInfo->Pages            = Pages;
  ApertInfo->MemoryAttributes = GcdDescriptor.Attributes;
  ApertInfo->Ripas            = RipasEmpty;

  DEBUG ((
    DEBUG_INFO,
    "%a: ApertRef = 0x%p, Memory = 0x%lx, Pages = 0x%x, "
    "MemoryAttributes = 0x%x, Ripas = 0x%x\n",
    __func__,
    ApertInfo,
    ApertInfo->BaseAddress,
    ApertInfo->Pages,
    ApertInfo->MemoryAttributes,
    ApertInfo->Ripas
    ));

  // Set the Realm IPA state to Empty to open the Aperture
  Status = RsiSetIpaState (
             (UINT64 *)Memory,
             (Pages * EFI_PAGE_SIZE),
             RipasEmpty,
             RIPAS_CHANGE_FLAGS_RSI_CHANGE_DESTROYED
             );
  if (RETURN_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RSI Set IPA State failed, Address = %p, Pages = 0x%lx, "
      "Status = %r\n",
      Memory,
      Pages,
      Status
      ));
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "SUCCESS: RSI Set IPA State complete, Address = %p, Pages = 0x%lx, "
    "Status = %r\n",
    Memory,
    Pages,
    Status
    ));

  InsertHeadList (&mApertureInfos, &ApertInfo->Link);
  *ApertureReference = (EFI_HANDLE *)&ApertInfo->Link;

  return Status;

error_handler:

  FreePool (ApertInfo);

error_handler1:
  Status1 = ArmCcaSetMemoryProtectAttribute (
              Memory,
              EFI_PAGES_TO_SIZE (Pages),
              mIpaWidth,
              FALSE
              );
  if (RETURN_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to update page tables to Protected page mapping, "
      "Address = %p, Pages = 0x%lx, Status = %r\n",
      Memory,
      Pages,
      Status1
      ));
  }

  *ApertureReference = NULL;
  // return the first error code
  return Status;
}

/** Disables the sharing of the buffers.

  @param [in] ApertureReference   Reference to the aperture for closing.
  @param [in] MemoryMapLocked     The function is executing on the stack of
                                  gBS->ExitBootServices(); changes to the UEFI
                                  memory map are forbidden.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required buffer information is not found.
**/
STATIC
EFI_STATUS
EFIAPI
RampCloseAperture (
  IN  CONST EFI_HANDLE  ApertureReference,
  IN        BOOLEAN     MemoryMapLocked
  )
{
  EFI_STATUS     Status;
  APERTURE_INFO  *ApertInfo;

  if (ApertureReference == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ApertInfo = NULL;
  ApertInfo = CR (ApertureReference, APERTURE_INFO, Link, APERTURE_INFO_SIG);
  if (ApertInfo == NULL) {
    return EFI_NOT_FOUND;
  }

  if (!IsNodeInList (&mApertureInfos, (LIST_ENTRY *)ApertureReference)) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: ApertRef = 0x%p, Memory = 0x%lx, Pages = 0x%x, "
    "MemoryAttributes = 0x%x, Ripas = 0x%x\n",
    __func__,
    ApertInfo,
    ApertInfo->BaseAddress,
    ApertInfo->Pages,
    ApertInfo->MemoryAttributes,
    ApertInfo->Ripas
    ));

  // Set the Realm IPA state to RAM to close the Aperture
  Status = RsiSetIpaState (
             (UINT64 *)ApertInfo->BaseAddress,
             (ApertInfo->Pages * EFI_PAGE_SIZE),
             RipasRam,
             RIPAS_CHANGE_FLAGS_RSI_CHANGE_DESTROYED
             );
  if (RETURN_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RSI Set IPA State failed, Address = %p, Pages = 0x%lx, "
      "Status = %r\n",
      ApertInfo->BaseAddress,
      ApertInfo->Pages,
      Status
      ));
    return Status;
  }

  RemoveEntryList (&ApertInfo->Link);

  if (!MemoryMapLocked) {
    Status = ArmCcaSetMemoryProtectAttribute (
               ApertInfo->BaseAddress,
               EFI_PAGES_TO_SIZE (ApertInfo->Pages),
               mIpaWidth,
               FALSE
               );
    if (RETURN_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to update page tables for Protected RAM page mapping,"
        "Address = %p, Pages = 0x%lx, Status = %r\n",
        ApertInfo->BaseAddress,
        ApertInfo->Pages,
        Status
        ));
    }

    ZeroMem (ApertInfo, sizeof (APERTURE_INFO));
    FreePool (ApertInfo);
  } else {
    // Changes to the UEFI memory map are forbidden.
    // So just zero out the memory.
    ZeroMem (ApertInfo, sizeof (APERTURE_INFO));
  }

  return Status;
}

/** Closes all open apertures.
  @param [in] MemoryMapLocked     The function is executing on the stack of
                                  gBS->ExitBootServices(); changes to the UEFI
                                  memory map are forbidden.

**/
STATIC
VOID
EFIAPI
RampCloseAllApertures (
  IN  BOOLEAN  MemoryMapLocked
  )
{
  LIST_ENTRY     *Node;
  LIST_ENTRY     *NextNode;
  APERTURE_INFO  *ApertureInfo;

  // All drivers that had opened the apertures have halted their respective
  // controllers by now; close all the apertures.
  for (
       Node = GetFirstNode (&mApertureInfos);
       Node != &mApertureInfos;
       Node = NextNode
       )
  {
    NextNode     = GetNextNode (&mApertureInfos, Node);
    ApertureInfo = CR (Node, APERTURE_INFO, Link, APERTURE_INFO_SIG);
    RampCloseAperture (&ApertureInfo->Link, MemoryMapLocked);
  }
}

/**
  Notification function that is queued after the notification functions of all
  events in the EFI_EVENT_GROUP_EXIT_BOOT_SERVICES event group.

  This function invokes the closing of all open apertures.

  @param[in] Event    Event whose notification function is being invoked. Event
                      is permitted to request the queueing of this function
                      only at TPL_CALLBACK task priority level.

  @param[in] Context  Ignored.
**/
STATIC
VOID
EFIAPI
OnRampExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  RampCloseAllApertures (TRUE);
}

/**
  Notification function that is queued when gBS->ExitBootServices() signals the
  EFI_EVENT_GROUP_EXIT_BOOT_SERVICES event group. This function signals another
  event, received as Context, and returns.

  Signaling an event in this context is safe. The UEFI spec allows
  gBS->SignalEvent() to return EFI_SUCCESS only; EFI_OUT_OF_RESOURCES is not
  listed, hence memory is not allocated.

  @param[in] Event          Event whose notification function is being invoked.
                            Event is permitted to request the queueing of this
                            function at TPL_CALLBACK or TPL_NOTIFY task
                            priority level.

  @param[in] EventToSignal  Identifies the EFI_EVENT to signal. EventToSignal
                            is permitted to request the queueing of its
                            notification function only at TPL_CALLBACK level.
**/
STATIC
VOID
EFIAPI
RampExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *EventToSignal
  )
{
  // (1) The NotifyFunctions of all the events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES will have been queued before
  //     RampExitBootServices() is entered.
  //
  // (2) RampExitBootServices() is executing minimally at TPL_CALLBACK.
  //
  // (3) RampExitBootServices() has been queued in unspecified order relative
  //      to the NotifyFunctions of all the other events in
  //     EFI_EVENT_GROUP_EXIT_BOOT_SERVICES whose NotifyTpl is the same as
  //     Event's.
  //
  // Consequences:
  //
  // - If Event's NotifyTpl is TPL_CALLBACK, then some other NotifyFunctions
  //   queued at TPL_CALLBACK may be invoked after RampExitBootServices()
  //   returns.
  //
  // - If Event's NotifyTpl is TPL_NOTIFY, then some other NotifyFunctions
  //   queued at TPL_NOTIFY may be invoked after RampExitBootServices()
  //   returns; plus *all* NotifyFunctions queued at TPL_CALLBACK will be
  //   invoked strictly after all NotifyFunctions queued at TPL_NOTIFY,
  //   including RampExitBootServices(), have been invoked.
  //
  // - By signaling EventToSignal here, whose NotifyTpl is TPL_CALLBACK, we
  //   queue EventToSignal's NotifyFunction after the NotifyFunctions of *all*
  //   events in EFI_EVENT_GROUP_EXIT_BOOT_SERVICES.
  gBS->SignalEvent (EventToSignal);
}

/** A structure describing the Realm Aperture Management protocol.
*/
STATIC
CONST
EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL  Ramp = {
  EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL_REVISION,
  RampOpenAperture,
  RampCloseAperture
};

/**
  This routine is called to close all apertures before system reset.

  @param[in]  ResetType    The type of reset to perform.
  @param[in]  ResetStatus  The status code for the reset.
  @param[in]  DataSize     The size, in bytes, of ResetData.
  @param[in]  ResetData    For a ResetType of EfiResetCold, EfiResetWarm, or
                           EfiResetShutdown the data buffer starts with a Null-
                           terminated string, optionally followed by additional
                           binary data. The string is a description that the
                           caller may use to further indicate the reason for
                           the system reset. ResetData is only valid if
                           ResetStatus is something other than EFI_SUCCESS
                           unless the ResetType is EfiResetPlatformSpecific
                           where a minimum amount of ResetData is always
                           required.
                           For a ResetType of EfiResetPlatformSpecific the data
                           buffer also starts with a Null-terminated string
                           that is followed by an EFI_GUID that describes the
                           specific type of reset to perform.
**/
STATIC
VOID
EFIAPI
OnResetEvent (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  RampCloseAllApertures (FALSE);
}

/**
  Hook the system reset to close all apertures.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
STATIC
VOID
EFIAPI
OnResetNotificationInstall (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                       Status;
  EFI_RESET_NOTIFICATION_PROTOCOL  *ResetNotify;

  Status = gBS->LocateProtocol (
                  &gEfiResetNotificationProtocolGuid,
                  NULL,
                  (VOID **)&ResetNotify
                  );
  if (!EFI_ERROR (Status)) {
    Status = ResetNotify->RegisterResetNotify (ResetNotify, OnResetEvent);
    ASSERT_EFI_ERROR (Status);
    DEBUG ((DEBUG_INFO, "RAMP: Hook system reset to close all apertures.\n"));
    gBS->CloseEvent (Event);
  }
}

/** Entry point for Realm Aperture Management Protocol Dxe

  @param [in]  ImageHandle  Handle for this image.
  @param [in]  SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS             When executing in a Realm the RAMP was
                                  installed successfully.
                                  When execution context is not a Realm, this
                                  function returns success indicating nothing
                                  needs to be done and allow other modules to
                                  run.
  @retval EFI_OUT_OF_RESOURCES    There was not enough memory to install the
                                  protocols.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.

**/
EFI_STATUS
EFIAPI
RealmApertureManagementProtocolDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  EFI_EVENT   CloseAllAperturesEvent;
  EFI_EVENT   ExitBootEvent;
  VOID        *Registration;

  // When the execution context is a Realm, install the Realm Aperture
  // Management protocol otherwise return success so that other modules
  // can run.
  if (!IsRealm ()) {
    return EFI_SUCCESS;
  }

  // Retrieve the IPA Width of the Realm for subsequent use to configure
  // the protection attribute of memory regions.
  Status = GetIpaWidth (&mIpaWidth);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Ipa Width, Status = %r\n",
      Status
      ));
    ASSERT (0);
    return Status;
  }

  /*
    Create the "late" event whose notification function will close all
    apertures.
  */
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,            // Type
                  TPL_CALLBACK,                 // NotifyTpl
                  OnRampExitBootServicesEvent,  // NotifyFunction
                  NULL,                         // NotifyContext
                  &CloseAllAperturesEvent       // Event
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /*
    Create the event whose notification function will be queued by
    gBS->ExitBootServices() and will signal the event created above.
  */
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES, // Type
                  TPL_CALLBACK,                  // NotifyTpl
                  RampExitBootServices,          // NotifyFunction
                  CloseAllAperturesEvent,        // NotifyContext
                  &ExitBootEvent                 // Event
                  );
  if (EFI_ERROR (Status)) {
    goto error_handler1;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiRealmApertureManagementProtocolGuid,
                  &Ramp,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    // RAMP Protocol installed successfully
    // Hook the system reset to close all apertures.
    EfiCreateProtocolNotifyEvent (
      &gEfiResetNotificationProtocolGuid,
      TPL_CALLBACK,
      OnResetNotificationInstall,
      NULL,
      &Registration
      );
    return Status;
  }

  // cleanup on error
  gBS->CloseEvent (ExitBootEvent);

error_handler1:
  gBS->CloseEvent (CloseAllAperturesEvent);
  return Status;
}
