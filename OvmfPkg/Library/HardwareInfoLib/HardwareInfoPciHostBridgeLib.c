/**@file
  Hardware info library with types and accessors to parse information about
  PCI host bridges.

  Copyright 2021 - 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>

#include "HardwareInfoPciHostBridgeLib.h"

#define IS_RANGE_INVALID(Start, Size, MaxValue)  (Start >= MaxValue || Size == 0)

/**
  Calculate the last (inclusive) address of a range.

  @param[in] Start  First address of the range
  @param[in] Size   Size of the range
  @return Last address of the range
**/
STATIC
UINT64
GetRangeEnd (
  IN  UINT64  Start,
  IN  UINT64  Size,
  IN  UINT64  MaxValue
  )
{
  if (IS_RANGE_INVALID (Start, Size, MaxValue)) {
    return 0;
  }

  return Start + Size - 1;
}

/**
  Internal helper to update LastAddress if the Limit address
  of the Mem aperture is higher than the provided value.

  @param[in]  Mem           Pointer to aperture whose limit is
                            to be compared against accumulative
                            last address.
  @param[out] LastAddress   Pointer to accumulative last address
                            to be updated if Limit is higher
**/
STATIC
VOID
UpdateLastAddressIfHigher (
  IN  PCI_ROOT_BRIDGE_APERTURE  *Mem,
  OUT UINT64                    *LastAddress
  )
{
  if (Mem->Limit > *LastAddress) {
    *LastAddress = Mem->Limit;
  }
}

EFI_STATUS
HardwareInfoPciHostBridgeLastMmioAddress (
  IN CONST  HOST_BRIDGE_INFO  *HostBridge,
  IN        UINTN             DataSize,
  IN        BOOLEAN           HighMem,
  OUT       UINT64            *LastMmioAddress
  )
{
  EFI_STATUS                Status;
  PCI_ROOT_BRIDGE_APERTURE  Mem;
  PCI_ROOT_BRIDGE_APERTURE  MemAbove4G;
  PCI_ROOT_BRIDGE_APERTURE  PMem;
  PCI_ROOT_BRIDGE_APERTURE  PMemAbove4G;

  if (LastMmioAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set the output to the lowest possible value so that if some step fails
  // the overall outcome reflects no information found
  //
  *LastMmioAddress = 0;

  Status = HardwareInfoPciHostBridgeGetApertures (
             HostBridge,
             DataSize,
             NULL,
             &Mem,
             &MemAbove4G,
             &PMem,
             &PMemAbove4G,
             NULL
             );

  //
  // Forward error to caller but ignore warnings given that, very likely,
  // the host bridge will have a PIO aperture we are explicitly
  // ignoring here since we care only about MMIO resources.
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (HighMem) {
    UpdateLastAddressIfHigher (&MemAbove4G, LastMmioAddress);
    UpdateLastAddressIfHigher (&PMemAbove4G, LastMmioAddress);
  } else {
    UpdateLastAddressIfHigher (&Mem, LastMmioAddress);
    UpdateLastAddressIfHigher (&PMem, LastMmioAddress);
  }

  return EFI_SUCCESS;
}

/**
  Set the boundaries of an aperture to invalid values having
  size zero and start MaxValue (yields Start > Limit which
  depicts an invalid range)

  @param[in]  MaxValue    Max value of the aperture's range (depends
                          on the data type)
  @param[out] Aperture    Aperture object to invalidate
**/
STATIC
VOID
InvalidateRootBridgeAperture (
  OUT PCI_ROOT_BRIDGE_APERTURE  *Aperture
  )
{
  if (Aperture == NULL) {
    return;
  }

  Aperture->Base  = MAX_UINT64;
  Aperture->Limit = 0;
}

/**
  Fill a PCI ROOT BRIDGE APERTURE with the proper values calculated
  from the provided start and size.

  @param[in]  Start       Start address of the aperture
  @param[in]  Size        Size, in bytes, of the aperture
  @param[in]  MaxValue    Max value a valid address could take and which
                          represents an invalid start address.
  @param[out] Aperture    Pointer to the aperture to be filled

  @retval EFI_SUCCESS                 Aperture was filled successfully
  @retval EFI_INVALID_PARAMETER       Range depicted by Start and Size is
                                      valid but ignored because aperture
                                      pointer is NULL
  @retval EFI_WARN_BUFFER_TOO_SMALL   Aperture pointer is invalid but the
                                      range also is so no harm.
**/
STATIC
EFI_STATUS
FillHostBridgeAperture (
  IN  UINT64                    Start,
  IN  UINT64                    Size,
  IN  UINT64                    MaxValue,
  OUT PCI_ROOT_BRIDGE_APERTURE  *Aperture
  )
{
  UINT64  End;

  End = GetRangeEnd (Start, Size, MaxValue);

  if (Aperture == NULL) {
    if (!IS_RANGE_INVALID (Start, Size, MaxValue)) {
      //
      // Report an error to the caller since the range specified in
      // the host bridge's resources is non-empty but the provided
      // aperture pointer is null, thus the valid range is ignored.
      //
      return EFI_INVALID_PARAMETER;
    }

    return EFI_WARN_BUFFER_TOO_SMALL;
  }

  if (IS_RANGE_INVALID (Start, Size, MaxValue)) {
    //
    // Fill Aperture with invalid range values to signal the
    // absence of an address space (empty range)
    //
    InvalidateRootBridgeAperture (Aperture);
  } else {
    Aperture->Base  = Start;
    Aperture->Limit = End;
  }

  return EFI_SUCCESS;
}

/**
  Merge 2 ranges (normal and prefetchable) into a single aperture
  comprehending the addresses encompassed by both of them. If both
  ranges are not empty they must be contiguous for correctness.

  @param[in]  Start       Range start address
  @param[in]  Size        Range size in bytes
  @param[in]  PStart      Prefetchable range start address
  @param[in]  PSize       Prefetchable range size in bytes
  @param[in]  MaxValue    Max value a valid address could take and which
                          represents an invalid start address.
  @param[out] Aperture    Pointer to the aperture to be filled

  @retval EFI_SUCCESS                 Aperture was filled successfully
  @retval EFI_INVALID_PARAMETER       Either range depicted by Start, Size
                                      or PStart, PSize or both are valid
                                      but ignored because aperture pointer
                                      is NULL
  @retval EFI_WARN_BUFFER_TOO_SMALL   Aperture pointer is invalid but both
                                      ranges are too so no harm.
**/
STATIC
EFI_STATUS
MergeHostBridgeApertures (
  IN  UINT64                    Start,
  IN  UINT64                    Size,
  IN  UINT64                    PStart,
  IN  UINT64                    PSize,
  IN  UINT64                    MaxValue,
  OUT PCI_ROOT_BRIDGE_APERTURE  *Aperture
  )
{
  UINT64  PEnd;

  if (Aperture == NULL) {
    if (!IS_RANGE_INVALID (Start, Size, MaxValue) ||
        !IS_RANGE_INVALID (PStart, PSize, MaxValue))
    {
      //
      // Report an error to the caller since the range specified in
      // the host bridge's resources is non-empty but the provided
      // aperture pointer is null, thus the valid range is ignored.
      //
      return EFI_INVALID_PARAMETER;
    }

    return EFI_WARN_BUFFER_TOO_SMALL;
  }

  //
  // Start from an empty range (Limit < Base)
  //
  InvalidateRootBridgeAperture (Aperture);

  if (!IS_RANGE_INVALID (Start, Size, MaxValue)) {
    Aperture->Base  = Start;
    Aperture->Limit = Start + Size - 1;
  }

  if (!IS_RANGE_INVALID (PStart, PSize, MaxValue)) {
    PEnd = PStart + PSize - 1;

    if (PStart < Aperture->Base) {
      Aperture->Base = PStart;
    }

    if (PEnd > Aperture->Limit) {
      Aperture->Limit = PEnd;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HardwareInfoPciHostBridgeGetBusNrRange (
  IN CONST  HOST_BRIDGE_INFO  *HostBridge,
  IN        UINTN             DataSize,
  OUT       UINTN             *BusNrStart,
  OUT       UINTN             *BusNrLast
  )
{
  if ((HostBridge == NULL) || (DataSize == 0) ||
      (BusNrStart == NULL) || (BusNrLast == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For now only version 0 is supported
  //
  if (HostBridge->Version != 0) {
    return EFI_INCOMPATIBLE_VERSION;
  }

  *BusNrStart = HostBridge->BusNrStart;
  *BusNrLast  = HostBridge->BusNrLast;

  return EFI_SUCCESS;
}

EFI_STATUS
HardwareInfoPciHostBridgeGetApertures (
  IN CONST  HOST_BRIDGE_INFO          *HostBridge,
  IN        UINTN                     DataSize,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *Io,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *Mem,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PMem,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G,
  OUT       PCI_ROOT_BRIDGE_APERTURE  *PcieConfig
  )
{
  EFI_STATUS  Status;
  BOOLEAN     StickyError;

  StickyError = FALSE;
  if ((HostBridge == NULL) || (DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For now only version 0 is supported
  //
  if (HostBridge->Version != 0) {
    return EFI_INCOMPATIBLE_VERSION;
  }

  Status = FillHostBridgeAperture (
             HostBridge->IoStart,
             HostBridge->IoSize,
             MAX_UINT32,
             Io
             );
  StickyError |= EFI_ERROR (Status);

  Status = FillHostBridgeAperture (
             HostBridge->PcieConfigStart,
             HostBridge->PcieConfigSize,
             MAX_UINT64,
             PcieConfig
             );
  StickyError |= EFI_ERROR (Status);

  if (HostBridge->Flags.Bits.CombineMemPMem) {
    Status = MergeHostBridgeApertures (
               HostBridge->MemStart,
               HostBridge->MemSize,
               HostBridge->PMemStart,
               HostBridge->PMemSize,
               MAX_UINT32,
               Mem
               );
    StickyError |= EFI_ERROR (Status);

    Status = MergeHostBridgeApertures (
               HostBridge->MemAbove4GStart,
               HostBridge->MemAbove4GSize,
               HostBridge->PMemAbove4GStart,
               HostBridge->PMemAbove4GSize,
               MAX_UINT64,
               MemAbove4G
               );
    StickyError |= EFI_ERROR (Status);

    //
    // Invalidate unused apertures
    //
    InvalidateRootBridgeAperture (PMem);
    InvalidateRootBridgeAperture (PMemAbove4G);
  } else {
    Status = FillHostBridgeAperture (
               HostBridge->MemStart,
               HostBridge->MemSize,
               MAX_UINT32,
               Mem
               );
    StickyError |= EFI_ERROR (Status);

    Status = FillHostBridgeAperture (
               HostBridge->PMemStart,
               HostBridge->PMemSize,
               MAX_UINT32,
               PMem
               );
    StickyError |= EFI_ERROR (Status);

    Status = FillHostBridgeAperture (
               HostBridge->MemAbove4GStart,
               HostBridge->MemAbove4GSize,
               MAX_UINT64,
               MemAbove4G
               );
    StickyError |= EFI_ERROR (Status);

    Status = FillHostBridgeAperture (
               HostBridge->PMemAbove4GStart,
               HostBridge->PMemAbove4GSize,
               MAX_UINT64,
               PMem
               );
    StickyError |= EFI_ERROR (Status);
  }

  if (StickyError) {
    //
    // If any function returned an error it is due to a valid range
    // specified in the host bridge that was ignored due to a NULL
    // pointer. Translate it to a warning to allow for calling with
    // only a subset of the apertures.
    //
    return EFI_WARN_STALE_DATA;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HardwareInfoPciHostBridgeGetFlags (
  IN CONST  HOST_BRIDGE_INFO  *HostBridge,
  IN        UINTN             DataSize,
  OUT       UINT64            *Attributes               OPTIONAL,
  OUT       BOOLEAN           *DmaAbove4G               OPTIONAL,
  OUT       BOOLEAN           *NoExtendedConfigSpace    OPTIONAL,
  OUT       BOOLEAN           *CombineMemPMem           OPTIONAL
  )
{
  if ((HostBridge == NULL) || (DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For now only version 0 is supported
  //
  if (HostBridge->Version != 0) {
    return EFI_INCOMPATIBLE_VERSION;
  }

  if (Attributes) {
    *Attributes = HostBridge->Attributes;
  }

  if (DmaAbove4G) {
    *DmaAbove4G = !!HostBridge->Flags.Bits.DmaAbove4G;
  }

  if (NoExtendedConfigSpace) {
    *NoExtendedConfigSpace = !!HostBridge->Flags.Bits.NoExtendedConfigSpace;
  }

  if (CombineMemPMem) {
    *CombineMemPMem = !!HostBridge->Flags.Bits.CombineMemPMem;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS  Status;

  Status = HardwareInfoPciHostBridgeGetBusNrRange (
             HostBridge,
             DataSize,
             BusNrStart,
             BusNrLast
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HardwareInfoPciHostBridgeGetFlags (
             HostBridge,
             DataSize,
             Attributes,
             DmaAbove4G,
             NoExtendedConfigSpace,
             CombineMemPMem
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HardwareInfoPciHostBridgeGetApertures (
             HostBridge,
             DataSize,
             Io,
             Mem,
             MemAbove4G,
             PMem,
             PMemAbove4G,
             PcieConfig
             );

  return Status;
}
