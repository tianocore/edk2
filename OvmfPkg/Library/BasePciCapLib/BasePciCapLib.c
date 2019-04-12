/** @file
  Work with PCI capabilities in PCI config space.

  Provides functions to parse capabilities lists, and to locate, describe, read
  and write capabilities. PCI config space access is abstracted away.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/PciExpress21.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "BasePciCapLib.h"


/**
  Compare a standalone PCI_CAP_KEY against a PCI_CAP containing an embedded
  PCI_CAP_KEY.

  @param[in] PciCapKey  Pointer to the bare PCI_CAP_KEY.

  @param[in] PciCap     Pointer to the PCI_CAP with the embedded PCI_CAP_KEY.

  @retval <0  If PciCapKey compares less than PciCap->Key.

  @retval  0  If PciCapKey compares equal to PciCap->Key.

  @retval >0  If PciCapKey compares greater than PciCap->Key.
**/
STATIC
INTN
EFIAPI
ComparePciCapKey (
  IN CONST VOID *PciCapKey,
  IN CONST VOID *PciCap
  )
{
  CONST PCI_CAP_KEY *Key1;
  CONST PCI_CAP_KEY *Key2;

  Key1 = PciCapKey;
  Key2 = &((CONST PCI_CAP *)PciCap)->Key;

  if (Key1->Domain < Key2->Domain) {
    return -1;
  }
  if (Key1->Domain > Key2->Domain) {
    return 1;
  }
  if (Key1->CapId < Key2->CapId) {
    return -1;
  }
  if (Key1->CapId > Key2->CapId) {
    return 1;
  }
  if (Key1->Instance < Key2->Instance) {
    return -1;
  }
  if (Key1->Instance > Key2->Instance) {
    return 1;
  }
  return 0;
}


/**
  Compare two PCI_CAP objects based on PCI_CAP.Key.

  @param[in] PciCap1  Pointer to the first PCI_CAP.

  @param[in] PciCap2  Pointer to the second PCI_CAP.

  @retval <0  If PciCap1 compares less than PciCap2.

  @retval  0  If PciCap1 compares equal to PciCap2.

  @retval >0  If PciCap1 compares greater than PciCap2.
**/
STATIC
INTN
EFIAPI
ComparePciCap (
  IN CONST VOID *PciCap1,
  IN CONST VOID *PciCap2
  )
{
  CONST PCI_CAP_KEY *PciCap1Key;

  PciCap1Key = &((CONST PCI_CAP *)PciCap1)->Key;
  return ComparePciCapKey (PciCap1Key, PciCap2);
}


/**
  Compare the standalone UINT16 config space offset of a capability header
  against a PCI_CAP containing an embedded Offset.

  @param[in] CapHdrOffset  Pointer to the bare UINT16 config space offset.

  @param[in] PciCap        Pointer to the PCI_CAP with the embedded Offset.

  @retval <0  If CapHdrOffset compares less than PciCap->Offset.

  @retval  0  If CapHdrOffset compares equal to PciCap->Offset.

  @retval >0  If CapHdrOffset compares greater than PciCap->Offset.
**/
STATIC
INTN
EFIAPI
ComparePciCapOffsetKey (
  IN CONST VOID *CapHdrOffset,
  IN CONST VOID *PciCap
  )
{
  UINT16 Offset1;
  UINT16 Offset2;

  Offset1 = *(CONST UINT16 *)CapHdrOffset;
  Offset2 = ((CONST PCI_CAP *)PciCap)->Offset;
  //
  // Note: both Offset1 and Offset2 are promoted to INT32 below, and the
  // subtraction takes place between INT32 values.
  //
  return Offset1 - Offset2;
}


/**
  Compare two PCI_CAP objects based on PCI_CAP.Offset.

  @param[in] PciCap1  Pointer to the first PCI_CAP.

  @param[in] PciCap2  Pointer to the second PCI_CAP.

  @retval <0  If PciCap1 compares less than PciCap2.

  @retval  0  If PciCap1 compares equal to PciCap2.

  @retval >0  If PciCap1 compares greater than PciCap2.
**/
STATIC
INTN
EFIAPI
ComparePciCapOffset (
  IN CONST VOID *PciCap1,
  IN CONST VOID *PciCap2
  )
{
  UINT16 Offset1;
  UINT16 Offset2;

  Offset1 = ((CONST PCI_CAP *)PciCap1)->Offset;
  Offset2 = ((CONST PCI_CAP *)PciCap2)->Offset;
  //
  // Note: both Offset1 and Offset2 are promoted to INT32 below, and the
  // subtraction takes place between INT32 values.
  //
  return Offset1 - Offset2;
}


/**
  Insert a new instance of the PCI capability given by (Domain, CapId) in
  CapList.

  @param[in,out] CapList        The PCI_CAP_LIST into which the new PCI_CAP
                                should be inserted. CapList will own the new
                                PCI_CAP structure.

  @param[in,out] CapHdrOffsets  Link the new PCI_CAP structure into the
                                (non-owning) CapHdrOffsets collection as well.
                                CapHdrOffsets orders the PCI_CAP structures
                                based on the PCI_CAP.Offset member, and enables
                                the calculation of PCI_CAP.MaxSizeHint.

  @param[in] Domain             Whether the capability is normal or extended.

  @param[in] CapId              Capability ID (specific to Domain).

  @param[in] Offset             Config space offset at which the standard
                                header of the capability starts. The caller is
                                responsible for ensuring that Offset be DWORD
                                aligned. The caller is also responsible for
                                ensuring that Offset be within the config space
                                identified by Domain.

  @param[in] Version            The version number of the capability. The
                                caller is responsible for passing 0 as Version
                                if Domain is PciCapNormal.

  @retval RETURN_SUCCESS           Insertion successful.

  @retval RETURN_OUT_OF_RESOURCES  Memory allocation failed.

  @retval RETURN_DEVICE_ERROR      A PCI_CAP with Offset is already linked by
                                   CapHdrOffsets. This indicates a loop in the
                                   capabilities list being parsed.
**/
STATIC
RETURN_STATUS
InsertPciCap (
  IN OUT PCI_CAP_LIST       *CapList,
  IN OUT ORDERED_COLLECTION *CapHdrOffsets,
  IN     PCI_CAP_DOMAIN     Domain,
  IN     UINT16             CapId,
  IN     UINT16             Offset,
  IN     UINT8              Version
  )
{
  PCI_CAP                  *PciCap;
  RETURN_STATUS            Status;
  ORDERED_COLLECTION_ENTRY *PciCapEntry;
  PCI_CAP                  *InstanceZero;

  ASSERT ((Offset & 0x3) == 0);
  ASSERT (Offset < (Domain == PciCapNormal ?
                    PCI_MAX_CONFIG_OFFSET : PCI_EXP_MAX_CONFIG_OFFSET));
  ASSERT (Domain == PciCapExtended || Version == 0);

  //
  // Set InstanceZero to suppress incorrect compiler/analyzer warnings.
  //
  InstanceZero = NULL;

  //
  // Allocate PciCap, and populate it assuming it is the first occurrence of
  // (Domain, CapId). Note that PciCap->MaxSizeHint is not assigned the final
  // value just yet.
  //
  PciCap = AllocatePool (sizeof *PciCap);
  if (PciCap == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  PciCap->Key.Domain                     = Domain;
  PciCap->Key.CapId                      = CapId;
  PciCap->Key.Instance                   = 0;
  PciCap->NumInstancesUnion.NumInstances = 1;
  PciCap->Offset                         = Offset;
  PciCap->MaxSizeHint                    = 0;
  PciCap->Version                        = Version;

  //
  // Add PciCap to CapList.
  //
  Status = OrderedCollectionInsert (CapList->Capabilities, &PciCapEntry,
             PciCap);
  if (RETURN_ERROR (Status)) {
    if (Status == RETURN_OUT_OF_RESOURCES) {
      goto FreePciCap;
    }
    ASSERT (Status == RETURN_ALREADY_STARTED);
    //
    // PciCap is not the first instance of (Domain, CapId). Add it as a new
    // instance, taking the current instance count from Instance#0. Note that
    // we don't bump the instance count maintained in Instance#0 just yet, to
    // keep rollback on errors simple.
    //
    InstanceZero = OrderedCollectionUserStruct (PciCapEntry);
    PciCap->Key.Instance = InstanceZero->NumInstancesUnion.NumInstances;
    PciCap->NumInstancesUnion.InstanceZero = InstanceZero;

    ASSERT (PciCap->Key.Instance > 0);
    Status = OrderedCollectionInsert (CapList->Capabilities, &PciCapEntry,
               PciCap);
    if (Status == RETURN_OUT_OF_RESOURCES) {
      goto FreePciCap;
    }
  }
  //
  // At this point, PciCap has been inserted in CapList->Capabilities, either
  // with Instance==0 or with Instance>0. PciCapEntry is the iterator that
  // links PciCap.
  //
  ASSERT_RETURN_ERROR (Status);

  //
  // Link PciCap into CapHdrOffsets too, to order it globally based on config
  // space offset. Note that partial overlaps between capability headers is not
  // possible: Offset is DWORD aligned, normal capability headers are 16-bit
  // wide, and extended capability headers are 32-bit wide. Therefore any two
  // capability headers either are distinct or start at the same offset
  // (implying a loop in the respective capabilities list).
  //
  Status = OrderedCollectionInsert (CapHdrOffsets, NULL, PciCap);
  if (RETURN_ERROR (Status)) {
    if (Status == RETURN_ALREADY_STARTED) {
      //
      // Loop found; map return status accordingly.
      //
      Status = RETURN_DEVICE_ERROR;
    }
    goto DeletePciCapFromCapList;
  }

  //
  // Now we can bump the instance count maintained in Instance#0, if PciCap is
  // not the first instance of (Domain, CapId).
  //
  if (PciCap->Key.Instance > 0) {
    //
    // Suppress invalid "nullptr dereference" compiler/analyzer warnings: the
    // only way for "PciCap->Key.Instance" to be positive here is for it to
    // have been assigned *from* dereferencing "InstanceZero" above.
    //
    ASSERT (InstanceZero != NULL);

    InstanceZero->NumInstancesUnion.NumInstances++;
  }
  return RETURN_SUCCESS;

DeletePciCapFromCapList:
  OrderedCollectionDelete (CapList->Capabilities, PciCapEntry, NULL);

FreePciCap:
  FreePool (PciCap);

  return Status;
}


/**
  Calculate the MaxSizeHint member for a PCI_CAP object.

  CalculatePciCapMaxSizeHint() may only be called once all capability instances
  have been successfully processed by InsertPciCap().

  @param[in,out] PciCap  The PCI_CAP object for which to calculate the
                         MaxSizeHint member. The caller is responsible for
                         passing a PCI_CAP object that has been created by a
                         successful invocation of InsertPciCap().

  @param[in] NextPciCap  If NextPciCap is NULL, then the caller is responsible
                         for PciCap to represent the capability instance with
                         the highest header offset in all config space. If
                         NextPciCap is not NULL, then the caller is responsible
                         for (a) having created NextPciCap with a successful
                         invocation of InsertPciCap(), and (b) NextPciCap being
                         the direct successor of PciCap in config space offset
                         order, as ordered by ComparePciCapOffset().
**/
STATIC
VOID
CalculatePciCapMaxSizeHint (
  IN OUT PCI_CAP *PciCap,
  IN     PCI_CAP *NextPciCap OPTIONAL
  )
{
  UINT16 ConfigSpaceSize;

  ConfigSpaceSize = (PciCap->Key.Domain == PciCapNormal ?
                     PCI_MAX_CONFIG_OFFSET : PCI_EXP_MAX_CONFIG_OFFSET);
  //
  // The following is guaranteed by the interface contract on
  // CalculatePciCapMaxSizeHint().
  //
  ASSERT (NextPciCap == NULL || PciCap->Offset < NextPciCap->Offset);
  //
  // The following is guaranteed by the interface contract on InsertPciCap().
  //
  ASSERT (PciCap->Offset < ConfigSpaceSize);
  //
  // Thus we can safely subtract PciCap->Offset from either of
  // - ConfigSpaceSize
  // - and NextPciCap->Offset (if NextPciCap is not NULL).
  //
  // PciCap extends from PciCap->Offset to NextPciCap->Offset (if any), except
  // it cannot cross config space boundary.
  //
  if (NextPciCap == NULL || NextPciCap->Offset >= ConfigSpaceSize) {
    PciCap->MaxSizeHint = ConfigSpaceSize - PciCap->Offset;
    return;
  }
  PciCap->MaxSizeHint = NextPciCap->Offset - PciCap->Offset;
}


/**
  Debug dump a PCI_CAP_LIST object at the DEBUG_VERBOSE level.

  @param[in] CapList  The PCI_CAP_LIST object to dump.
**/
STATIC
VOID
EFIAPI
DebugDumpPciCapList (
  IN PCI_CAP_LIST *CapList
  )
{
  DEBUG_CODE_BEGIN ();
  ORDERED_COLLECTION_ENTRY *PciCapEntry;

  for (PciCapEntry = OrderedCollectionMin (CapList->Capabilities);
       PciCapEntry != NULL;
       PciCapEntry = OrderedCollectionNext (PciCapEntry)) {
    PCI_CAP       *PciCap;
    RETURN_STATUS Status;
    PCI_CAP_INFO  Info;

    PciCap = OrderedCollectionUserStruct (PciCapEntry);
    Status = PciCapGetInfo (PciCap, &Info);
    //
    // PciCapGetInfo() cannot fail in this library instance.
    //
    ASSERT_RETURN_ERROR (Status);

    DEBUG ((DEBUG_VERBOSE,
      "%a:%a: %a 0x%04x %03u/%03u v0x%x @0x%03x+0x%03x\n", gEfiCallerBaseName,
      __FUNCTION__, (Info.Domain == PciCapNormal ? "Norm" : "Extd"),
      Info.CapId, Info.Instance, Info.NumInstances, Info.Version, Info.Offset,
      Info.MaxSizeHint));
  }
  DEBUG_CODE_END ();
}


/**
  Empty a collection of PCI_CAP structures, optionally releasing the referenced
  PCI_CAP structures themselves. Release the collection at last.

  @param[in,out] PciCapCollection  The collection to empty and release.

  @param[in] FreePciCap            TRUE if the PCI_CAP structures linked by
                                   PciCapCollection should be released. When
                                   FALSE, the caller is responsible for
                                   retaining at least one reference to each
                                   PCI_CAP structure originally linked by
                                   PciCapCollection.
**/
STATIC
VOID
EmptyAndUninitPciCapCollection (
  IN OUT ORDERED_COLLECTION *PciCapCollection,
  IN     BOOLEAN            FreePciCap
  )
{
  ORDERED_COLLECTION_ENTRY *PciCapEntry;
  ORDERED_COLLECTION_ENTRY *NextEntry;

  for (PciCapEntry = OrderedCollectionMin (PciCapCollection);
       PciCapEntry != NULL;
       PciCapEntry = NextEntry) {
    PCI_CAP *PciCap;

    NextEntry = OrderedCollectionNext (PciCapEntry);
    OrderedCollectionDelete (PciCapCollection, PciCapEntry, (VOID **)&PciCap);
    if (FreePciCap) {
      FreePool (PciCap);
    }
  }
  OrderedCollectionUninit (PciCapCollection);
}


/**
  Parse the capabilities lists (both normal and extended, as applicable) of a
  PCI device.

  If the PCI device has no capabilities, that per se will not fail
  PciCapListInit(); an empty capabilities list will be represented.

  If the PCI device is found to be PCI Express, then an attempt will be made to
  parse the extended capabilities list as well. If the first extended config
  space access -- via PciDevice->ReadConfig() with SourceOffset=0x100 and
  Size=4 -- fails, that per se will not fail PciCapListInit(); the device will
  be assumed to have no extended capabilities.

  @param[in] PciDevice  Implementation-specific unique representation of the
                        PCI device in the PCI hierarchy.

  @param[out] CapList   Opaque data structure that holds an in-memory
                        representation of the parsed capabilities lists of
                        PciDevice.

  @retval RETURN_SUCCESS           The capabilities lists have been parsed from
                                   config space.

  @retval RETURN_OUT_OF_RESOURCES  Memory allocation failed.

  @retval RETURN_DEVICE_ERROR      A loop or some other kind of invalid pointer
                                   was detected in the capabilities lists of
                                   PciDevice.

  @return                          Error codes propagated from
                                   PciDevice->ReadConfig().
**/
RETURN_STATUS
EFIAPI
PciCapListInit (
  IN  PCI_CAP_DEV  *PciDevice,
  OUT PCI_CAP_LIST **CapList
  )
{
  PCI_CAP_LIST             *OutCapList;
  RETURN_STATUS            Status;
  ORDERED_COLLECTION       *CapHdrOffsets;
  UINT16                   PciStatusReg;
  BOOLEAN                  DeviceIsExpress;
  ORDERED_COLLECTION_ENTRY *OffsetEntry;

  //
  // Allocate the output structure.
  //
  OutCapList = AllocatePool (sizeof *OutCapList);
  if (OutCapList == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // The OutCapList->Capabilities collection owns the PCI_CAP structures and
  // orders them based on PCI_CAP.Key.
  //
  OutCapList->Capabilities = OrderedCollectionInit (ComparePciCap,
                               ComparePciCapKey);
  if (OutCapList->Capabilities == NULL) {
    Status = RETURN_OUT_OF_RESOURCES;
    goto FreeOutCapList;
  }

  //
  // The (temporary) CapHdrOffsets collection only references PCI_CAP
  // structures, and orders them based on PCI_CAP.Offset.
  //
  CapHdrOffsets = OrderedCollectionInit (ComparePciCapOffset,
                    ComparePciCapOffsetKey);
  if (CapHdrOffsets == NULL) {
    Status = RETURN_OUT_OF_RESOURCES;
    goto FreeCapabilities;
  }

  //
  // Whether the device is PCI Express depends on the normal capability with
  // identifier EFI_PCI_CAPABILITY_ID_PCIEXP.
  //
  DeviceIsExpress = FALSE;

  //
  // Check whether a normal capabilities list is present. If there's none,
  // that's not an error; we'll just return OutCapList->Capabilities empty.
  //
  Status = PciDevice->ReadConfig (PciDevice, PCI_PRIMARY_STATUS_OFFSET,
                        &PciStatusReg, sizeof PciStatusReg);
  if (RETURN_ERROR (Status)) {
    goto FreeCapHdrOffsets;
  }
  if ((PciStatusReg & EFI_PCI_STATUS_CAPABILITY) != 0) {
    UINT8 NormalCapHdrOffset;

    //
    // Fetch the start offset of the normal capabilities list.
    //
    Status = PciDevice->ReadConfig (PciDevice, PCI_CAPBILITY_POINTER_OFFSET,
                          &NormalCapHdrOffset, sizeof NormalCapHdrOffset);
    if (RETURN_ERROR (Status)) {
      goto FreeCapHdrOffsets;
    }

    //
    // Traverse the normal capabilities list.
    //
    NormalCapHdrOffset &= 0xFC;
    while (NormalCapHdrOffset > 0) {
      EFI_PCI_CAPABILITY_HDR NormalCapHdr;

      Status = PciDevice->ReadConfig (PciDevice, NormalCapHdrOffset,
                            &NormalCapHdr, sizeof NormalCapHdr);
      if (RETURN_ERROR (Status)) {
        goto FreeCapHdrOffsets;
      }

      Status = InsertPciCap (OutCapList, CapHdrOffsets, PciCapNormal,
                 NormalCapHdr.CapabilityID, NormalCapHdrOffset, 0);
      if (RETURN_ERROR (Status)) {
        goto FreeCapHdrOffsets;
      }

      if (NormalCapHdr.CapabilityID == EFI_PCI_CAPABILITY_ID_PCIEXP) {
        DeviceIsExpress = TRUE;
      }
      NormalCapHdrOffset = NormalCapHdr.NextItemPtr & 0xFC;
    }
  }

  //
  // If the device has been found PCI Express, attempt to traverse the extended
  // capabilities list. It starts right after the normal config space.
  //
  if (DeviceIsExpress) {
    UINT16 ExtendedCapHdrOffset;

    ExtendedCapHdrOffset = PCI_MAX_CONFIG_OFFSET;
    while (ExtendedCapHdrOffset > 0) {
      PCI_EXPRESS_EXTENDED_CAPABILITIES_HEADER ExtendedCapHdr;

      Status = PciDevice->ReadConfig (PciDevice, ExtendedCapHdrOffset,
                            &ExtendedCapHdr, sizeof ExtendedCapHdr);
      //
      // If the first extended config space access fails, assume the device has
      // no extended capabilities. If the first extended config space access
      // succeeds but we read an "all bits zero" extended capability header,
      // that means (by spec) the device has no extended capabilities.
      //
      if (ExtendedCapHdrOffset == PCI_MAX_CONFIG_OFFSET &&
          (RETURN_ERROR (Status) ||
           IsZeroBuffer (&ExtendedCapHdr, sizeof ExtendedCapHdr))) {
        break;
      }
      if (RETURN_ERROR (Status)) {
        goto FreeCapHdrOffsets;
      }

      Status = InsertPciCap (OutCapList, CapHdrOffsets, PciCapExtended,
                 (UINT16)ExtendedCapHdr.CapabilityId, ExtendedCapHdrOffset,
                 (UINT8)ExtendedCapHdr.CapabilityVersion);
      if (RETURN_ERROR (Status)) {
        goto FreeCapHdrOffsets;
      }

      ExtendedCapHdrOffset = ExtendedCapHdr.NextCapabilityOffset & 0xFFC;
      if (ExtendedCapHdrOffset > 0 &&
          ExtendedCapHdrOffset < PCI_MAX_CONFIG_OFFSET) {
        //
        // Invalid capability pointer.
        //
        Status = RETURN_DEVICE_ERROR;
        goto FreeCapHdrOffsets;
      }
    }
  }

  //
  // Both capabilities lists have been parsed; compute the PCI_CAP.MaxSizeHint
  // members if at least one capability has been found. In parallel, evacuate
  // the CapHdrOffsets collection.
  //
  // At first, set OffsetEntry to the iterator of the PCI_CAP object with the
  // lowest Offset (if such exists).
  //
  OffsetEntry = OrderedCollectionMin (CapHdrOffsets);
  if (OffsetEntry != NULL) {
    ORDERED_COLLECTION_ENTRY *NextOffsetEntry;
    PCI_CAP                  *PciCap;

    //
    // Initialize NextOffsetEntry to the iterator of the PCI_CAP object with
    // the second lowest Offset (if such exists).
    //
    NextOffsetEntry = OrderedCollectionNext (OffsetEntry);
    //
    // Calculate MaxSizeHint for all PCI_CAP objects except the one with the
    // highest Offset.
    //
    while (NextOffsetEntry != NULL) {
      PCI_CAP *NextPciCap;

      OrderedCollectionDelete (CapHdrOffsets, OffsetEntry, (VOID **)&PciCap);
      NextPciCap = OrderedCollectionUserStruct (NextOffsetEntry);
      CalculatePciCapMaxSizeHint (PciCap, NextPciCap);

      OffsetEntry = NextOffsetEntry;
      NextOffsetEntry = OrderedCollectionNext (OffsetEntry);
    }
    //
    // Calculate MaxSizeHint for the PCI_CAP object with the highest Offset.
    //
    OrderedCollectionDelete (CapHdrOffsets, OffsetEntry, (VOID **)&PciCap);
    CalculatePciCapMaxSizeHint (PciCap, NULL);
  }
  ASSERT (OrderedCollectionIsEmpty (CapHdrOffsets));
  OrderedCollectionUninit (CapHdrOffsets);

  DebugDumpPciCapList (OutCapList);
  *CapList = OutCapList;
  return RETURN_SUCCESS;

FreeCapHdrOffsets:
  EmptyAndUninitPciCapCollection (CapHdrOffsets, FALSE);

FreeCapabilities:
  EmptyAndUninitPciCapCollection (OutCapList->Capabilities, TRUE);

FreeOutCapList:
  FreePool (OutCapList);

  ASSERT (RETURN_ERROR (Status));
  DEBUG ((DEBUG_ERROR, "%a:%a: %r\n", gEfiCallerBaseName, __FUNCTION__,
    Status));
  return Status;
}


/**
  Free the resources used by CapList.

  @param[in] CapList  The PCI_CAP_LIST object to free, originally produced by
                      PciCapListInit().
**/
VOID
EFIAPI
PciCapListUninit (
  IN PCI_CAP_LIST *CapList
  )
{
  EmptyAndUninitPciCapCollection (CapList->Capabilities, TRUE);
  FreePool (CapList);
}


/**
  Locate a capability instance in the parsed capabilities lists.

  @param[in] CapList   The PCI_CAP_LIST object produced by PciCapListInit().

  @param[in] Domain    Distinguishes whether CapId is 8-bit wide and
                       interpreted in normal config space, or 16-bit wide and
                       interpreted in extended config space. Capability ID
                       definitions are relative to domain.

  @param[in] CapId     Capability identifier to look up.

  @param[in] Instance  Domain and CapId may identify a multi-instance
                       capability. When Instance is zero, the first instance of
                       the capability is located (in list traversal order --
                       which may not mean increasing config space offset
                       order). Higher Instance values locate subsequent
                       instances of the same capability (in list traversal
                       order).

  @param[out] Cap      The capability instance that matches the search
                       criteria. Cap is owned by CapList and becomes invalid
                       when CapList is freed with PciCapListUninit().
                       PciCapListFindCap() may be called with Cap set to NULL,
                       in order to test the existence of a specific capability
                       instance.

  @retval RETURN_SUCCESS    The capability instance identified by (Domain,
                            CapId, Instance) has been found.

  @retval RETURN_NOT_FOUND  The requested (Domain, CapId, Instance) capability
                            instance does not exist.
**/
RETURN_STATUS
EFIAPI
PciCapListFindCap (
  IN  PCI_CAP_LIST   *CapList,
  IN  PCI_CAP_DOMAIN Domain,
  IN  UINT16         CapId,
  IN  UINT16         Instance,
  OUT PCI_CAP        **Cap    OPTIONAL
  )
{
  PCI_CAP_KEY              Key;
  ORDERED_COLLECTION_ENTRY *PciCapEntry;

  Key.Domain   = Domain;
  Key.CapId    = CapId;
  Key.Instance = Instance;

  PciCapEntry = OrderedCollectionFind (CapList->Capabilities, &Key);
  if (PciCapEntry == NULL) {
    return RETURN_NOT_FOUND;
  }
  if (Cap != NULL) {
    *Cap = OrderedCollectionUserStruct (PciCapEntry);
  }
  return RETURN_SUCCESS;
}


/**
  Locate the first instance of the capability given by (Domain, CapId) such
  that the instance's Version is greater than or equal to MinVersion.

  This is a convenience function that may save client code calls to
  PciCapListFindCap() and PciCapGetInfo().

  @param[in] CapList     The PCI_CAP_LIST object produced by PciCapListInit().

  @param[in] Domain      Distinguishes whether CapId is 8-bit wide and
                         interpreted in normal config space, or 16-bit wide and
                         interpreted in extended config space. Capability ID
                         definitions are relative to domain.

  @param[in] CapId       Capability identifier to look up.

  @param[in] MinVersion  The minimum version that the capability instance is
                         required to have. Note that all capability instances
                         in Domain=PciCapNormal have Version=0.

  @param[out] Cap        The first capability instance that matches the search
                         criteria. Cap is owned by CapList and becomes invalid
                         when CapList is freed with PciCapListUninit().
                         PciCapListFindCapVersion() may be called with Cap set
                         to NULL, in order just to test whether the search
                         criteria are satisfiable.

  @retval RETURN_SUCCESS    The first capability instance matching (Domain,
                            CapId, MinVersion) has been located.

  @retval RETURN_NOT_FOUND  No capability instance matches (Domain, CapId,
                            MinVersion).
**/
RETURN_STATUS
EFIAPI
PciCapListFindCapVersion (
  IN  PCI_CAP_LIST   *CapList,
  IN  PCI_CAP_DOMAIN Domain,
  IN  UINT16         CapId,
  IN  UINT8          MinVersion,
  OUT PCI_CAP        **Cap      OPTIONAL
  )
{
  PCI_CAP_KEY              Key;
  ORDERED_COLLECTION_ENTRY *PciCapEntry;

  //
  // Start the version checks at Instance#0 of (Domain, CapId).
  //
  Key.Domain   = Domain;
  Key.CapId    = CapId;
  Key.Instance = 0;

  for (PciCapEntry = OrderedCollectionFind (CapList->Capabilities, &Key);
       PciCapEntry != NULL;
       PciCapEntry = OrderedCollectionNext (PciCapEntry)) {
    PCI_CAP *PciCap;

    PciCap = OrderedCollectionUserStruct (PciCapEntry);
    //
    // PCI_CAP.Key ordering keeps instances of the same (Domain, CapId)
    // adjacent to each other, so stop searching if either Domain or CapId
    // changes.
    //
    if (PciCap->Key.Domain != Domain || PciCap->Key.CapId != CapId) {
      break;
    }
    if (PciCap->Version >= MinVersion) {
      //
      // Match found.
      //
      if (Cap != NULL) {
        *Cap = PciCap;
      }
      return RETURN_SUCCESS;
    }
  }
  return RETURN_NOT_FOUND;
}


/**
  Get information about a PCI Capability instance.

  @param[in] Cap    The capability instance to get info about, located with
                    PciCapListFindCap*().

  @param[out] Info  A PCI_CAP_INFO structure that describes the properties of
                    Cap.

  @retval RETURN_SUCCESS  Fields of Info have been set.

  @return                 Unspecified error codes, if filling in Info failed
                          for some reason.
**/
RETURN_STATUS
EFIAPI
PciCapGetInfo (
  IN  PCI_CAP      *Cap,
  OUT PCI_CAP_INFO *Info
  )
{
  PCI_CAP *InstanceZero;

  ASSERT (Info != NULL);

  InstanceZero = (Cap->Key.Instance == 0 ? Cap :
                  Cap->NumInstancesUnion.InstanceZero);

  Info->Domain       = Cap->Key.Domain;
  Info->CapId        = Cap->Key.CapId;
  Info->NumInstances = InstanceZero->NumInstancesUnion.NumInstances;
  Info->Instance     = Cap->Key.Instance;
  Info->Offset       = Cap->Offset;
  Info->MaxSizeHint  = Cap->MaxSizeHint;
  Info->Version      = Cap->Version;

  return RETURN_SUCCESS;
}


/**
  Read a slice of a capability instance.

  The function performs as few config space accesses as possible (without
  attempting 64-bit wide accesses). PciCapRead() performs bounds checking on
  SourceOffsetInCap and Size, and only invokes PciDevice->ReadConfig() if the
  requested transfer falls within Cap.

  @param[in] PciDevice           Implementation-specific unique representation
                                 of the PCI device in the PCI hierarchy.

  @param[in] Cap                 The capability instance to read, located with
                                 PciCapListFindCap*().

  @param[in] SourceOffsetInCap   Source offset relative to the capability
                                 header to start reading from. A zero value
                                 refers to the first byte of the capability
                                 header.

  @param[out] DestinationBuffer  Buffer to store the read data to.

  @param[in] Size                The number of bytes to transfer.

  @retval RETURN_SUCCESS          Size bytes have been transferred from Cap to
                                  DestinationBuffer.

  @retval RETURN_BAD_BUFFER_SIZE  Reading Size bytes starting from
                                  SourceOffsetInCap would not (entirely) be
                                  contained within Cap, as suggested by
                                  PCI_CAP_INFO.MaxSizeHint. No bytes have been
                                  read.

  @return                         Error codes propagated from
                                  PciDevice->ReadConfig(). Fewer than Size
                                  bytes may have been read.
**/
RETURN_STATUS
EFIAPI
PciCapRead (
  IN  PCI_CAP_DEV *PciDevice,
  IN  PCI_CAP     *Cap,
  IN  UINT16      SourceOffsetInCap,
  OUT VOID        *DestinationBuffer,
  IN  UINT16      Size
  )
{
  //
  // Note: all UINT16 values are promoted to INT32 below, and addition and
  // comparison take place between INT32 values.
  //
  if (SourceOffsetInCap + Size > Cap->MaxSizeHint) {
    return RETURN_BAD_BUFFER_SIZE;
  }
  return PciDevice->ReadConfig (PciDevice, Cap->Offset + SourceOffsetInCap,
                      DestinationBuffer, Size);
}


/**
  Write a slice of a capability instance.

  The function performs as few config space accesses as possible (without
  attempting 64-bit wide accesses). PciCapWrite() performs bounds checking on
  DestinationOffsetInCap and Size, and only invokes PciDevice->WriteConfig() if
  the requested transfer falls within Cap.

  @param[in] PciDevice               Implementation-specific unique
                                     representation of the PCI device in the
                                     PCI hierarchy.

  @param[in] Cap                     The capability instance to write, located
                                     with PciCapListFindCap*().

  @param[in] DestinationOffsetInCap  Destination offset relative to the
                                     capability header to start writing at. A
                                     zero value refers to the first byte of the
                                     capability header.

  @param[in] SourceBuffer            Buffer to read the data to be stored from.

  @param[in] Size                    The number of bytes to transfer.

  @retval RETURN_SUCCESS          Size bytes have been transferred from
                                  SourceBuffer to Cap.

  @retval RETURN_BAD_BUFFER_SIZE  Writing Size bytes starting at
                                  DestinationOffsetInCap would not (entirely)
                                  be contained within Cap, as suggested by
                                  PCI_CAP_INFO.MaxSizeHint. No bytes have been
                                  written.

  @return                         Error codes propagated from
                                  PciDevice->WriteConfig(). Fewer than Size
                                  bytes may have been written.
**/
RETURN_STATUS
EFIAPI
PciCapWrite (
  IN PCI_CAP_DEV *PciDevice,
  IN PCI_CAP     *Cap,
  IN UINT16      DestinationOffsetInCap,
  IN VOID        *SourceBuffer,
  IN UINT16      Size
  )
{
  //
  // Note: all UINT16 values are promoted to INT32 below, and addition and
  // comparison take place between INT32 values.
  //
  if (DestinationOffsetInCap + Size > Cap->MaxSizeHint) {
    return RETURN_BAD_BUFFER_SIZE;
  }
  return PciDevice->WriteConfig (PciDevice,
                      Cap->Offset + DestinationOffsetInCap, SourceBuffer,
                      Size);
}
