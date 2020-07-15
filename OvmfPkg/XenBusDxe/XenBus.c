/** @file
  XenBus Bus driver implementation.

  This file implement the necessary to discover and enumerate Xen PV devices
  through XenStore.

  Copyright (C) 2010 Spectra Logic Corporation
  Copyright (C) 2008 Doug Rabson
  Copyright (C) 2005 Rusty Russell, IBM Corporation
  Copyright (C) 2005 Mike Wray, Hewlett-Packard
  Copyright (C) 2005 XenSource Ltd
  Copyright (C) 2014, Citrix Ltd.

  This file may be distributed separately from the Linux kernel, or
  incorporated into other software packages, subject to the following license:

  SPDX-License-Identifier: MIT
**/

#include <Library/PrintLib.h>

#include "XenBus.h"
#include "GrantTable.h"
#include "XenStore.h"
#include "EventChannel.h"

#include <IndustryStandard/Xen/io/xenbus.h>

STATIC XENBUS_PRIVATE_DATA gXenBusPrivateData;

STATIC XENBUS_DEVICE_PATH gXenBusDevicePathTemplate = {
  {                                                 // Vendor
    {                                               // Vendor.Header
      HARDWARE_DEVICE_PATH,                         // Vendor.Header.Type
      HW_VENDOR_DP,                                 // Vendor.Header.SubType
      {
        (UINT8) (sizeof (XENBUS_DEVICE_PATH)),      // Vendor.Header.Length[0]
        (UINT8) (sizeof (XENBUS_DEVICE_PATH) >> 8), // Vendor.Header.Length[1]
      }
    },
    XENBUS_PROTOCOL_GUID,                           // Vendor.Guid
  },
  0,                                                // Type
  0                                                 // DeviceId
};


/**
  Search our internal record of configured devices (not the XenStore) to
  determine if the XenBus device indicated by Node is known to the system.

  @param Dev   The XENBUS_DEVICE instance to search for device children.
  @param Node  The XenStore node path for the device to find.

  @return  The XENBUS_PRIVATE_DATA of the found device if any, or NULL.
 */
STATIC
XENBUS_PRIVATE_DATA *
XenBusDeviceInitialized (
  IN XENBUS_DEVICE *Dev,
  IN CONST CHAR8 *Node
  )
{
  LIST_ENTRY *Entry;
  XENBUS_PRIVATE_DATA *Child;
  XENBUS_PRIVATE_DATA *Result;

  if (IsListEmpty (&Dev->ChildList)) {
    return NULL;
  }

  Result = NULL;
  for (Entry = GetFirstNode (&Dev->ChildList);
       !IsNodeAtEnd (&Dev->ChildList, Entry);
       Entry = GetNextNode (&Dev->ChildList, Entry)) {
    Child = XENBUS_PRIVATE_DATA_FROM_LINK (Entry);
    if (!AsciiStrCmp (Child->XenBusIo.Node, Node)) {
      Result = Child;
      break;
    }
  }

  return (Result);
}

STATIC
XenbusState
XenBusReadDriverState (
  IN CONST CHAR8 *Path
  )
{
  XenbusState State;
  CHAR8 *Ptr = NULL;
  XENSTORE_STATUS Status;

  Status = XenStoreRead (XST_NIL, Path, "state", NULL, (VOID **)&Ptr);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    State = XenbusStateClosed;
  } else {
    State = AsciiStrDecimalToUintn (Ptr);
  }

  if (Ptr != NULL) {
    FreePool (Ptr);
  }

  return State;
}

//
// Callers should ensure that they are only one calling XenBusAddDevice.
//
STATIC
EFI_STATUS
XenBusAddDevice (
  XENBUS_DEVICE *Dev,
  CONST CHAR8 *Type,
  CONST CHAR8 *Id)
{
  CHAR8 DevicePath[XENSTORE_ABS_PATH_MAX];
  XENSTORE_STATUS StatusXenStore;
  XENBUS_PRIVATE_DATA *Private;
  EFI_STATUS Status;
  XENBUS_DEVICE_PATH *TempXenBusPath;
  VOID *ChildXenIo;

  AsciiSPrint (DevicePath, sizeof (DevicePath),
               "device/%a/%a", Type, Id);

  if (XenStorePathExists (XST_NIL, DevicePath, "")) {
    XENBUS_PRIVATE_DATA *Child;
    enum xenbus_state State;
    CHAR8 *BackendPath;

    Child = XenBusDeviceInitialized (Dev, DevicePath);
    if (Child != NULL) {
      /*
       * We are already tracking this node
       */
      Status = EFI_SUCCESS;
      goto out;
    }

    State = XenBusReadDriverState (DevicePath);
    if (State != XenbusStateInitialising) {
      /*
       * Device is not new, so ignore it. This can
       * happen if a device is going away after
       * switching to Closed.
       */
      DEBUG ((DEBUG_INFO, "XenBus: Device %a ignored. "
              "State %d\n", DevicePath, State));
      Status = EFI_SUCCESS;
      goto out;
    }

    StatusXenStore = XenStoreRead (XST_NIL, DevicePath, "backend",
                                   NULL, (VOID **) &BackendPath);
    if (StatusXenStore != XENSTORE_STATUS_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "xenbus: %a no backend path.\n", DevicePath));
      Status = EFI_NOT_FOUND;
      goto out;
    }

    Private = AllocateCopyPool (sizeof (*Private), &gXenBusPrivateData);
    Private->XenBusIo.Type = AsciiStrDup (Type);
    Private->XenBusIo.Node = AsciiStrDup (DevicePath);
    Private->XenBusIo.Backend = BackendPath;
    Private->XenBusIo.DeviceId = (UINT16)AsciiStrDecimalToUintn (Id);
    Private->Dev = Dev;

    TempXenBusPath = AllocateCopyPool (sizeof (XENBUS_DEVICE_PATH),
                                       &gXenBusDevicePathTemplate);
    if (!AsciiStrCmp (Private->XenBusIo.Type, "vbd")) {
      TempXenBusPath->Type = XENBUS_DEVICE_PATH_TYPE_VBD;
    }
    TempXenBusPath->DeviceId = Private->XenBusIo.DeviceId;
    Private->DevicePath = (XENBUS_DEVICE_PATH *)AppendDevicePathNode (
                            Dev->DevicePath,
                            &TempXenBusPath->Vendor.Header);
    FreePool (TempXenBusPath);

    InsertTailList (&Dev->ChildList, &Private->Link);

    Status = gBS->InstallMultipleProtocolInterfaces (
               &Private->Handle,
               &gEfiDevicePathProtocolGuid, Private->DevicePath,
               &gXenBusProtocolGuid, &Private->XenBusIo,
               NULL);
    if (EFI_ERROR (Status)) {
      goto ErrorInstallProtocol;
    }

    Status = gBS->OpenProtocol (Dev->ControllerHandle,
               &gXenIoProtocolGuid,
               &ChildXenIo, Dev->This->DriverBindingHandle,
               Private->Handle,
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "open by child controller fail (%r)\n",
              Status));
      goto ErrorOpenProtocolByChild;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "XenBus: does not exist: %a\n", DevicePath));
    Status = EFI_NOT_FOUND;
  }

  return Status;

ErrorOpenProtocolByChild:
  gBS->UninstallMultipleProtocolInterfaces (
    Private->Handle,
    &gEfiDevicePathProtocolGuid, Private->DevicePath,
    &gXenBusProtocolGuid, &Private->XenBusIo,
    NULL);
ErrorInstallProtocol:
  RemoveEntryList (&Private->Link);
  FreePool (Private->DevicePath);
  FreePool ((VOID *) Private->XenBusIo.Backend);
  FreePool ((VOID *) Private->XenBusIo.Node);
  FreePool ((VOID *) Private->XenBusIo.Type);
  FreePool (Private);
out:
  return Status;
}

/**
  Enumerate all devices of the given type on this bus.

  @param Dev   A XENBUS_DEVICE instance.
  @param Type  String indicating the device sub-tree (e.g. "vfb", "vif")
               to enumerate.

  Devices that are found are been initialize via XenBusAddDevice ().
  XenBusAddDevice () ignores duplicate detects and ignores duplicate devices,
  so it can be called unconditionally for any device found in the XenStore.
 */
STATIC
VOID
XenBusEnumerateDeviceType (
  XENBUS_DEVICE *Dev,
  CONST CHAR8 *Type
  )
{
  CONST CHAR8 **Directory;
  UINTN Index;
  UINT32 Count;
  XENSTORE_STATUS Status;

  Status = XenStoreListDirectory (XST_NIL,
                                  "device", Type,
                                  &Count, &Directory);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    return;
  }
  for (Index = 0; Index < Count; Index++) {
    XenBusAddDevice (Dev, Type, Directory[Index]);
  }

  FreePool ((VOID*)Directory);
}


/**
  Enumerate the devices on a XenBus bus and install a XenBus Protocol instance.

  Caller should ensure that it is the only one to call this function. This
  function cannot be called concurrently.

  @param Dev   A XENBUS_DEVICE instance.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
 */
XENSTORE_STATUS
XenBusEnumerateBus (
  XENBUS_DEVICE *Dev
  )
{
  CONST CHAR8 **Types;
  UINTN Index;
  UINT32 Count;
  XENSTORE_STATUS Status;

  Status = XenStoreListDirectory (XST_NIL,
                                  "device", "",
                                  &Count, &Types);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    return Status;
  }

  for (Index = 0; Index < Count; Index++) {
    XenBusEnumerateDeviceType (Dev, Types[Index]);
  }

  FreePool ((VOID*)Types);

  return XENSTORE_STATUS_SUCCESS;
}

STATIC
XENSTORE_STATUS
EFIAPI
XenBusSetState (
  IN XENBUS_PROTOCOL      *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN enum xenbus_state    NewState
  )
{
  enum xenbus_state CurrentState;
  XENSTORE_STATUS Status;
  CHAR8 *Temp;

  DEBUG ((DEBUG_INFO, "XenBus: Set state to %d\n", NewState));

  Status = XenStoreRead (Transaction, This->Node, "state", NULL, (VOID **)&Temp);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    goto Out;
  }
  CurrentState = AsciiStrDecimalToUintn (Temp);
  FreePool (Temp);
  if (CurrentState == NewState) {
    goto Out;
  }

  do {
    Status = XenStoreSPrint (Transaction, This->Node, "state", "%d", NewState);
  } while (Status == XENSTORE_STATUS_EAGAIN);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "XenBus: failed to write new state\n"));
    goto Out;
  }
  DEBUG ((DEBUG_INFO, "XenBus: Set state to %d, done\n", NewState));

Out:
  return Status;
}

STATIC XENBUS_PRIVATE_DATA gXenBusPrivateData = {
  XENBUS_PRIVATE_DATA_SIGNATURE,    // Signature
  { NULL, NULL },                   // Link
  NULL,                             // Handle
  {                                 // XenBusIo
    XenBusXenStoreRead,             // XenBusIo.XsRead
    XenBusXenStoreBackendRead,      // XenBusIo.XsBackendRead
    XenBusXenStoreSPrint,           // XenBusIo.XsPrintf
    XenBusXenStoreRemove,           // XenBusIo.XsRemove
    XenBusXenStoreTransactionStart, // XenBusIo.XsTransactionStart
    XenBusXenStoreTransactionEnd,   // XenBusIo.XsTransactionEnd
    XenBusSetState,                 // XenBusIo.SetState
    XenBusGrantAccess,              // XenBusIo.GrantAccess
    XenBusGrantEndAccess,           // XenBusIo.GrantEndAccess
    XenBusEventChannelAllocate,     // XenBusIo.EventChannelAllocate
    XenBusEventChannelNotify,       // XenBusIo.EventChannelNotify
    XenBusEventChannelClose,        // XenBusIo.EventChannelClose
    XenBusRegisterWatch,            // XenBusIo.RegisterWatch
    XenBusRegisterWatchBackend,     // XenBusIo.RegisterWatchBackend
    XenBusUnregisterWatch,          // XenBusIo.UnregisterWatch
    XenBusWaitForWatch,             // XenBusIo.WaitForWatch

    NULL,                           // XenBusIo.Type
    0,                              // XenBusIo.DeviceId
    NULL,                           // XenBusIo.Node
    NULL,                           // XenBusIo.Backend
  },

  NULL,                             // Dev
  NULL                              // DevicePath
};
