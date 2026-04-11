/** @file
  Map positions of extra PCI root buses to bus numbers.

  Copyright (C) 2015, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OrderedCollectionLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciRootBridgeIo.h>

#include "ExtraRootBusMap.h"

//
// The BusNumbers field is an array with Count elements. The elements increase
// strictry monotonically. Zero is not an element (because the zero bus number
// belongs to the "main" root bus, never to an extra root bus). Offset N in the
// array maps the extra root bus with position (N+1) to its bus number (because
// the root bus with position 0 is always the main root bus, therefore we don't
// store it).
//
// If there are no extra root buses in the system, then Count is 0, and
// BusNumbers is NULL.
//
struct EXTRA_ROOT_BUS_MAP_STRUCT {
  UINT32    *BusNumbers;
  UINTN     Count;
};

/**
  An ORDERED_COLLECTION_USER_COMPARE function that compares root bridge
  protocol device paths based on UID.

  @param[in] UserStruct1  Pointer to the first ACPI_HID_DEVICE_PATH.

  @param[in] UserStruct2  Pointer to the second ACPI_HID_DEVICE_PATH.

  @retval <0  If UserStruct1 compares less than UserStruct2.

  @retval  0  If UserStruct1 compares equal to UserStruct2.

  @retval >0  If UserStruct1 compares greater than UserStruct2.
**/
STATIC
INTN
EFIAPI
RootBridgePathCompare (
  IN CONST VOID  *UserStruct1,
  IN CONST VOID  *UserStruct2
  )
{
  CONST ACPI_HID_DEVICE_PATH  *Acpi1;
  CONST ACPI_HID_DEVICE_PATH  *Acpi2;

  Acpi1 = UserStruct1;
  Acpi2 = UserStruct2;

  return Acpi1->UID < Acpi2->UID ? -1 :
         Acpi1->UID > Acpi2->UID ?  1 :
         0;
}

/**
  An ORDERED_COLLECTION_KEY_COMPARE function that compares a root bridge
  protocol device path against a UID.

  @param[in] StandaloneKey  Pointer to the bare UINT32 UID.

  @param[in] UserStruct     Pointer to the ACPI_HID_DEVICE_PATH with the
                            embedded UINT32 UID.

  @retval <0  If StandaloneKey compares less than UserStruct's key.

  @retval  0  If StandaloneKey compares equal to UserStruct's key.

  @retval >0  If StandaloneKey compares greater than UserStruct's key.
**/
STATIC
INTN
EFIAPI
RootBridgePathKeyCompare (
  IN CONST VOID  *StandaloneKey,
  IN CONST VOID  *UserStruct
  )
{
  CONST UINT32                *Uid;
  CONST ACPI_HID_DEVICE_PATH  *Acpi;

  Uid  = StandaloneKey;
  Acpi = UserStruct;

  return *Uid < Acpi->UID ? -1 :
         *Uid > Acpi->UID ?  1 :
         0;
}

/**
  Create a structure that maps the relative positions of PCI root buses to bus
  numbers.

  In the "bootorder" fw_cfg file, QEMU refers to extra PCI root buses by their
  positions, in relative root bus number order, not by their actual PCI bus
  numbers. The ACPI HID device path nodes however that are associated with
  PciRootBridgeIo protocol instances in the system have their UID fields set to
  the bus numbers. Create a map that gives, for each extra PCI root bus's
  position (ie. "serial number") its actual PCI bus number.

  @param[out] ExtraRootBusMap  The data structure implementing the map.

  @retval EFI_SUCCESS           ExtraRootBusMap has been populated.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.

  @retval EFI_ALREADY_STARTED   A duplicate root bus number has been found in
                                the system. (This should never happen.)

  @return                       Error codes returned by
                                gBS->LocateHandleBuffer() and
                                gBS->HandleProtocol().

**/
EFI_STATUS
CreateExtraRootBusMap (
  OUT EXTRA_ROOT_BUS_MAP  **ExtraRootBusMap
  )
{
  EFI_STATUS                Status;
  UINTN                     NumHandles;
  EFI_HANDLE                *Handles;
  ORDERED_COLLECTION        *Collection;
  EXTRA_ROOT_BUS_MAP        *Map;
  UINTN                     Idx;
  ORDERED_COLLECTION_ENTRY  *Entry, *Entry2;

  //
  // Handles and Collection are temporary / helper variables, while in Map we
  // build the return value.
  //

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL /* SearchKey */,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Collection = OrderedCollectionInit (
                 RootBridgePathCompare,
                 RootBridgePathKeyCompare
                 );
  if (Collection == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeHandles;
  }

  Map = AllocateZeroPool (sizeof *Map);
  if (Map == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeCollection;
  }

  //
  // Collect the ACPI device path protocols of the root bridges.
  //
  for (Idx = 0; Idx < NumHandles; ++Idx) {
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

    Status = gBS->HandleProtocol (
                    Handles[Idx],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      goto FreeMap;
    }

    //
    // Examine if the device path is an ACPI HID one, and if so, if UID is
    // nonzero (ie. the root bridge that the bus number belongs to is "extra",
    // not the main one). In that case, link the device path into Collection.
    //
    if ((DevicePathType (DevicePath) == ACPI_DEVICE_PATH) &&
        (DevicePathSubType (DevicePath) == ACPI_DP) &&
        (((ACPI_HID_DEVICE_PATH *)DevicePath)->HID == EISA_PNP_ID (0x0A03)) &&
        (((ACPI_HID_DEVICE_PATH *)DevicePath)->UID > 0))
    {
      Status = OrderedCollectionInsert (Collection, NULL, DevicePath);
      if (EFI_ERROR (Status)) {
        goto FreeMap;
      }

      ++Map->Count;
    }
  }

  if (Map->Count > 0) {
    //
    // At least one extra PCI root bus exists.
    //
    Map->BusNumbers = AllocatePool (Map->Count * sizeof *Map->BusNumbers);
    if (Map->BusNumbers == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeMap;
    }
  }

  //
  // Now collect the bus numbers of the extra PCI root buses into Map.
  //
  Idx   = 0;
  Entry = OrderedCollectionMin (Collection);
  while (Idx < Map->Count) {
    ACPI_HID_DEVICE_PATH  *Acpi;

    ASSERT (Entry != NULL);
    Acpi                 = OrderedCollectionUserStruct (Entry);
    Map->BusNumbers[Idx] = Acpi->UID;
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: extra bus position 0x%Lx maps to bus number (UID) 0x%x\n",
      __func__,
      (UINT64)(Idx + 1),
      Acpi->UID
      ));
    ++Idx;
    Entry = OrderedCollectionNext (Entry);
  }

  ASSERT (Entry == NULL);

  *ExtraRootBusMap = Map;
  Status           = EFI_SUCCESS;

  //
  // Fall through in order to release temporaries.
  //

FreeMap:
  if (EFI_ERROR (Status)) {
    if (Map->BusNumbers != NULL) {
      FreePool (Map->BusNumbers);
    }

    FreePool (Map);
  }

FreeCollection:
  for (Entry = OrderedCollectionMin (Collection); Entry != NULL;
       Entry = Entry2)
  {
    Entry2 = OrderedCollectionNext (Entry);
    OrderedCollectionDelete (Collection, Entry, NULL);
  }

  OrderedCollectionUninit (Collection);

FreeHandles:
  FreePool (Handles);

  return Status;
}

/**
  Release a map created with CreateExtraRootBusMap().

  @param[in] ExtraRootBusMap  The map to release.
*/
VOID
DestroyExtraRootBusMap (
  IN EXTRA_ROOT_BUS_MAP  *ExtraRootBusMap
  )
{
  if (ExtraRootBusMap->BusNumbers != NULL) {
    FreePool (ExtraRootBusMap->BusNumbers);
  }

  FreePool (ExtraRootBusMap);
}

/**
  Map the position (serial number) of an extra PCI root bus to its bus number.

  @param[in]  ExtraRootBusMap  The map created with CreateExtraRootBusMap();

  @param[in]  RootBusPos       The extra PCI root bus position to map.

  @param[out] RootBusNr        The bus number belonging to the extra PCI root
                               bus identified by RootBusPos.

  @retval EFI_INVALID_PARAMETER  RootBusPos is zero. The zero position
                                 identifies the main root bus, whose bus number
                                 is always zero, and is therefore never
                                 maintained in ExtraRootBusMap.

  @retval EFI_NOT_FOUND          RootBusPos is not found in ExtraRootBusMap.

  @retval EFI_SUCCESS            Mapping successful.
**/
EFI_STATUS
MapRootBusPosToBusNr (
  IN  CONST EXTRA_ROOT_BUS_MAP  *ExtraRootBusMap,
  IN  UINT64                    RootBusPos,
  OUT UINT32                    *RootBusNr
  )
{
  if (RootBusPos == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (RootBusPos > ExtraRootBusMap->Count) {
    return EFI_NOT_FOUND;
  }

  *RootBusNr = ExtraRootBusMap->BusNumbers[(UINTN)RootBusPos - 1];
  return EFI_SUCCESS;
}
