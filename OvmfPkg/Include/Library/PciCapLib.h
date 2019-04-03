/** @file
  Library class to work with PCI capabilities in PCI config space.

  Provides functions to parse capabilities lists, and to locate, describe, read
  and write capabilities. PCI config space access is abstracted away.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __PCI_CAP_LIB_H__
#define __PCI_CAP_LIB_H__

#include <Uefi/UefiBaseType.h>

//
// Base structure for representing a PCI device -- down to the PCI function
// level -- for the purposes of this library class. This is a forward
// declaration that is completed below. Concrete implementations are supposed
// to inherit and extend this type.
//
typedef struct PCI_CAP_DEV PCI_CAP_DEV;

/**
  Read the config space of a given PCI device (both normal and extended).

  PCI_CAP_DEV_READ_CONFIG performs as few config space accesses as possible
  (without attempting 64-bit wide accesses).

  PCI_CAP_DEV_READ_CONFIG returns an unspecified error if accessing Size bytes
  from SourceOffset exceeds the config space limit of the PCI device. Fewer
  than Size bytes may have been read in this case.

  @param[in] PciDevice           Implementation-specific unique representation
                                 of the PCI device in the PCI hierarchy.

  @param[in] SourceOffset        Source offset in the config space of the PCI
                                 device to start reading from.

  @param[out] DestinationBuffer  Buffer to store the read data to.

  @param[in] Size                The number of bytes to transfer.

  @retval RETURN_SUCCESS  Size bytes have been transferred from config space to
                          DestinationBuffer.

  @return                 Unspecified error codes. Fewer than Size bytes may
                          have been read.
**/
typedef
RETURN_STATUS
(EFIAPI *PCI_CAP_DEV_READ_CONFIG) (
  IN  PCI_CAP_DEV *PciDevice,
  IN  UINT16      SourceOffset,
  OUT VOID        *DestinationBuffer,
  IN  UINT16      Size
  );

/**
  Write the config space of a given PCI device (both normal and extended).

  PCI_CAP_DEV_WRITE_CONFIG performs as few config space accesses as possible
  (without attempting 64-bit wide accesses).

  PCI_CAP_DEV_WRITE_CONFIG returns an unspecified error if accessing Size bytes
  at DestinationOffset exceeds the config space limit of the PCI device. Fewer
  than Size bytes may have been written in this case.

  @param[in] PciDevice          Implementation-specific unique representation
                                of the PCI device in the PCI hierarchy.

  @param[in] DestinationOffset  Destination offset in the config space of the
                                PCI device to start writing at.

  @param[in] SourceBuffer       Buffer to read the data to be stored from.

  @param[in] Size               The number of bytes to transfer.

  @retval RETURN_SUCCESS  Size bytes have been transferred from SourceBuffer to
                          config space.

  @return                 Unspecified error codes. Fewer than Size bytes may
                          have been written.
**/
typedef
RETURN_STATUS
(EFIAPI *PCI_CAP_DEV_WRITE_CONFIG) (
  IN PCI_CAP_DEV *PciDevice,
  IN UINT16      DestinationOffset,
  IN VOID        *SourceBuffer,
  IN UINT16      Size
  );

//
// Complete the PCI_CAP_DEV type here. The base abstraction only requires
// config space accessors.
//
struct PCI_CAP_DEV {
  PCI_CAP_DEV_READ_CONFIG  ReadConfig;
  PCI_CAP_DEV_WRITE_CONFIG WriteConfig;
};

//
// Opaque data structure representing parsed PCI Capabilities Lists.
//
typedef struct PCI_CAP_LIST PCI_CAP_LIST;

//
// Opaque data structure representing a PCI Capability in a parsed Capability
// List.
//
typedef struct PCI_CAP PCI_CAP;

//
// Distinguishes whether a Capability ID is 8-bit wide and interpreted in
// normal config space, or 16-bit wide and interpreted in extended config
// space. Capability ID definitions are relative to domain.
//
typedef enum {
  PciCapNormal,
  PciCapExtended
} PCI_CAP_DOMAIN;

//
// Public data structure that PciCapGetInfo() fills in about a PCI_CAP object.
//
typedef struct {
  PCI_CAP_DOMAIN Domain;
  UINT16         CapId;
  //
  // The capability identified by Domain and CapId may have multiple instances
  // in config space. NumInstances provides the total count of occurrences of
  // the capability. It is always positive.
  //
  UINT16 NumInstances;
  //
  // Instance is the serial number, in capabilities list traversal order (not
  // necessarily config space offset order), of the one capability instance
  // that PciCapGetInfo() is reporting about. Instance is always smaller than
  // NumInstances.
  //
  UINT16 Instance;
  //
  // The offset in config space at which the capability header of the
  // capability instance starts.
  //
  UINT16 Offset;
  //
  // The deduced maximum size of the capability instance, including the
  // capability header. This hint is an upper bound, calculated -- without
  // regard to the internal structure of the capability -- from (a) the next
  // lowest offset in configuration space that is known to be used by another
  // capability, and (b) from the end of the config space identified by Domain,
  // whichever is lower.
  //
  UINT16 MaxSizeHint;
  //
  // The version number of the capability instance. Always zero when Domain is
  // PciCapNormal.
  //
  UINT8 Version;
} PCI_CAP_INFO;


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
  );


/**
  Free the resources used by CapList.

  @param[in] CapList  The PCI_CAP_LIST object to free, originally produced by
                      PciCapListInit().
**/
VOID
EFIAPI
PciCapListUninit (
  IN PCI_CAP_LIST *CapList
  );


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
  );


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
  );


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
  );


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
  );


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
  );

#endif // __PCI_CAP_LIB_H__
