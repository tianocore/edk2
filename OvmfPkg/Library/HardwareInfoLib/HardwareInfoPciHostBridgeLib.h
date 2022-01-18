/**@file
  Hardware info library with types and accessors to parse information about
  PCI host bridges.

  Copyright 2021 - 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HARDWARE_INFO_PCI_HOST_BRIDGE_LIB_H__
#define __HARDWARE_INFO_PCI_HOST_BRIDGE_LIB_H__

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Library/PciHostBridgeLib.h>

//
// Host Bridge resources information
//
#pragma pack(1)
typedef struct {
  //
  // Feature tracking, initially 0
  //
  UINT64    Version;

  //
  // Host bridge enabled attributes (EFI_PCI_ATTRIBUTE_*)
  //
  UINT64    Attributes;

  union {
    UINT32    Uint32;
    struct {
      UINT32    DmaAbove4G            : 1;
      UINT32    NoExtendedConfigSpace : 1;
      UINT32    CombineMemPMem        : 1;
      UINT32    Reserved              : 29;
    } Bits;
  } Flags;

  //
  // Bus number range
  //
  UINT8     BusNrStart;
  UINT8     BusNrLast;

  UINT8     Padding[2];

  //
  // IO aperture
  //
  UINT64    IoStart;
  UINT64    IoSize;

  //
  // 32-bit MMIO aperture
  //
  UINT64    MemStart;
  UINT64    MemSize;

  //
  // 32-bit prefetchable MMIO aperture
  //
  UINT64    PMemStart;
  UINT64    PMemSize;

  //
  // 64-bit MMIO aperture
  //
  UINT64    MemAbove4GStart;
  UINT64    MemAbove4GSize;

  //
  // 64-bit prefetchable MMIO aperture
  //
  UINT64    PMemAbove4GStart;
  UINT64    PMemAbove4GSize;

  //
  // MMIO accessible PCIe config space (ECAM)
  //
  UINT64    PcieConfigStart;
  UINT64    PcieConfigSize;
} HOST_BRIDGE_INFO;
#pragma pack()

/**
  Extract the last MMIO address, either from high (64-bit) or low (32-bit)
  memory used by the HostBridge's apertures.

  @param[in]  HostBridge      Root bridge's resources specification
  @param[in]  DataSize        Size in bytes of the actually filled
                              data available in the HostBridge object
  @param[in]  HighMem         64-bit (true) or 32-bit (false) MMIO
                              address
  @param[out] LastMmioAddress Pointer to last MMIO address

  @retval EFI_SUCCESS               Operation succeeded
  @retval EFI_INVALID_PARAMETER     One or more pointer parameters are
                                    invalid
  @retval EFI_INCOMPATIBLE_VERSION  HostBridge information belongs to
                                    an unsupported version
**/
EFI_STATUS
HardwareInfoPciHostBridgeLastMmioAddress (
  IN CONST  HOST_BRIDGE_INFO  *HostBridge,
  IN        UINTN             DataSize,
  IN        BOOLEAN           HighMem,
  OUT       UINT64            *LastMmioAddress
  );

/**
  Interpret the HostBridge resources and extact the bus number
  range.

  @param[in]  HostBridge   Root bridge's resources specification
  @param[in]  DataSize     Size in bytes of the actually filled
                           data available in the HostBridge object
  @param[out] BusNrStart   Pointer to the Bus Number range start
  @param[out] BusNrLast    Pointer to the Bus Number range end

  @retval EFI_SUCCESS               Retrieved the bus number range
                                    without any issues.
  @retval EFI_INVALID_PARAMETER     One of the parameters is invalid,
                                    either NULL pointer or size 0
  @retval EFI_INCOMPATIBLE_VERSION  HostBridge data of unsupported
                                    version
**/
EFI_STATUS
HardwareInfoPciHostBridgeGetBusNrRange (
  IN CONST  HOST_BRIDGE_INFO  *HostBridge,
  IN        UINTN             DataSize,
  OUT       UINTN             *BusNrStart,
  OUT       UINTN             *BusNrLast
  );

/**
  Interpret the MMIO resources in HostBridge and set the apertures
  in 32-bit space (Mem), 64-bit space (MemAbove4G), PIO (IO) and
  ECAM (PcieConfig) accordingly.

  The 2 types of apertures in each MMIO space (prefetchable and
  non-prefetchable) may be merged into a single window, hence if both
  types of apertures are defined while the CombineMemPMem flag is set,
  the ranges must be contiguous.

  @param[in]  HostBridge   Root bridge's resources specification
  @param[in]  DataSize     Size in bytes of the actually filled
                           data available in the HostBridge object
  @param[out] Mem          Pointer to 32-bit MMIO aperture
  @param[out] MemAbove4G   Pointer to 64-bit MMIO aperture
  @param[out] PMem         Pointer to the 32-bit prefetchable MMIO aperture
  @param[out] PMemAbove4G  Pointer to the 64-bit prefetchable MMIO aperture
  @param[out] PcieConfig   Pointer to MMIO mapped PCIe config aperture (ECAM)

  @retval EFI_INVALID_PARAMETER     HostBridge object is invalid
  @retval EFI_INCOMPATIBLE_VERSION  HostBridge information belongs to
                                    an unsupported version
  @retval EFI_WARN_STALE_DATA       One or more valid aperture in the
                                    HostBridge's resources were ignored
                                    because corresponding aperture pointer
                                    is NULL.
  @retval EFI_SUCCESS               Operation executed cleanly, all valid
                                    ranges were parsed into the corresponding
                                    aperture object.
**/
EFI_STATUS
HardwareInfoPciHostBridgeGetApertures (
  IN  CONST HOST_BRIDGE_INFO          *HostBridge,
  IN        UINTN                     DataSize,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *Io,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *Mem,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PMem,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PcieConfig
  );

/**
  Retrieve all flags and attributes of a host bridge describing the
  resources and capabilities.

  @param[in]  HostBridge            Host bridge information object
  @param[in]  DataSize              Size in bytes of the actually filled
                                    data available in the HostBridge object
  @param[out] Attributes            Pointer to the host bridge's attributes
  @param[out] DmaAbove4G            Pointer to the DMA Above 4G flag
  @param[out] NoExtendedConfigSpace Pointer to the Extended Config Space flag
  @param[out] CombineMemPMem        Pointer to the Combine Mem and PMem flag

  @retval EFI_INVALID_PARAMETER     HostBridge object is invalid
  @retval EFI_INCOMPATIBLE_VERSION  HostBridge information belongs to
                                    an unsupported version
  @retval EFI_SUCCESS               Operation executed cleanly
**/
EFI_STATUS
HardwareInfoPciHostBridgeGetFlags (
  IN CONST  HOST_BRIDGE_INFO  *HostBridge,
  IN        UINTN             DataSize,
  OUT       UINT64            *Attributes               OPTIONAL,
  OUT       BOOLEAN           *DmaAbove4G               OPTIONAL,
  OUT       BOOLEAN           *NoExtendedConfigSpace    OPTIONAL,
  OUT       BOOLEAN           *CombineMemPMem           OPTIONAL
  );

/**
  Getter that parses information from a HOST_BRIDGE_INFO object
  into smaller chunks of types handled by the PciHostBridgeLib.

  @param[in]  HostBridge            Host bridge information object
  @param[in]  DataSize              Size in bytes of the actually filled
                                    data available in the HostBridge object
  @param[out] BusNrStart            Pointer to the Bus Number range start
  @param[out] BusNrLast             Pointer to the Bus Number range end
  @param[out] Attributes            Pointer to the host bridge's attributes
  @param[out] DmaAbove4G            Pointer to the DMA Above 4G flag
  @param[out] NoExtendedConfigSpace Pointer to the Extended Config Space flag
  @param[out] CombineMemPMem        Pointer to the Combine Mem and PMem flag
  @param[out] Io                    Pointer to the PIO aperture object
  @param[out] Mem                   Pointer to the 32-bit MMIO aperture object
  @param[out] MemAbove4G            Pointer to the 64-bit MMIO aperture object
  @param[out] PMem                  Pointer to the 32-bit prefetchable MMIO
                                    aperture object
  @param[out] PMemAbove4G           Pointer to the 64-bit prefetchable MMIO
                                    aperture object
  @param[out] PcieConfig            MMIO mapped PCIe config aperture (ECAM)

  @retval EFI_SUCCESS               Whole operation succeeded
  @retval EFI_INVALID_PARAMETER     HostBridge object and/or non-optional
                                    output parameters are invalid
  @retval EFI_INCOMPATIBLE_VERSION  HostBridge information provided belongs to
                                    and unsupported version
  @retval EFI_WARN_STALE_DATA       One or more apertures having valid ranges
                                    in the HostBridge info were ignored because
                                    the correspnding aperture pointer is NULL
**/
EFI_STATUS
HardwareInfoPciHostBridgeGet (
  IN CONST  HOST_BRIDGE_INFO          *HostBridge,
  IN        UINTN                     DataSize,
  OUT       UINTN                     *BusNrStart,
  OUT       UINTN                     *BusNrLast,
  OUT       UINT64                    *Attributes               OPTIONAL,
  OUT       BOOLEAN                   *DmaAbove4G               OPTIONAL,
  OUT       BOOLEAN                   *NoExtendedConfigSpace    OPTIONAL,
  OUT       BOOLEAN                   *CombineMemPMem           OPTIONAL,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *Io                       OPTIONAL,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *Mem                      OPTIONAL,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G               OPTIONAL,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PMem                     OPTIONAL,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G              OPTIONAL,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PcieConfig               OPTIONAL
  );

#endif // __HARDWARE_INFO_PCI_HOST_BRIDGE_LIB_H__
