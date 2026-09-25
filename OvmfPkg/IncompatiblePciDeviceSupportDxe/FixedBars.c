/** @file
  Fixed PCI BAR placement using metadata provided by QEMU through the
  "etc/fixed-bars" fw_cfg blob.

  Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "FixedBars.h"

/* byte-for-byte identical to QEMU's structs in hw/pci/pci-fixed-bar.h */
#pragma pack (1)

typedef struct {
  UINT32    Version;
  UINT32    NumDevices;
} QEMU_FIXED_BARS_HDR;                       /* 8 bytes */

typedef struct {
  UINT16    VendorId;
  UINT16    DeviceId;
  UINT8     DevFlags;
  UINT8     RpBus;
  UINT8     NumBars;
  UINT8     Reserved;
  CHAR8     FwPath[128];                     /* qdev_get_fw_dev_path(), unused here */
} QEMU_FIXED_BARS_DEVICE;                    /* 136 bytes */

typedef struct {
  UINT8     Bar;
  UINT8     Reserved[3];
  UINT32    Flags;
  UINT64    Address;
  UINT64    Size;
} QEMU_FIXED_BARS_BAR;                       /* 24 bytes */

#pragma pack ()

#define QEMU_FIXED_BARS_VERSION      1
#define QEMU_FIXED_BAR_F_MEM64       BIT0
#define QEMU_FIXED_BAR_F_PREF        BIT1
#define QEMU_FIXED_BARS_DEV_F_FIXED  BIT0

/*
 * ACPI general-flag bits (ACPI 6.x 6.4.3.5.1):
 *   BIT2 = _MIF (minimum address fixed)
 *   BIT3 = _MAF (maximum address fixed)
 * Both set means AddrRangeMin carries the exact required base address.
 */
#define ACPI_ADDR_FLAG_FIXED  (BIT2 | BIT3)

/* Raw blob kept in memory; device entries point into it. */
STATIC UINT8              *mBlobData;
STATIC QEMU_FIXED_BARS_HDR mHdr;

/* Pre-parsed per-device index built from the blob at startup. */
typedef struct {
  UINT16                VendorId;
  UINT16                DeviceId;
  UINT8                 DevFlags;
  UINT8                 RpBus;    /* primary bus of the root port */
  UINT8                 NumBars;
  QEMU_FIXED_BARS_BAR  *Bars;    /* pointer into mBlobData */
} QFBD_DEVICE;

/*
 * Per-(VendorId, DeviceId) class. FixedBarsCheckDevice() is called once per
 * PCI function and once per scan pass. Different VID:DID classes must
 * advance their counters independently so that one class cannot disturb
 * the position of another across scan passes.
 *
 * Assumptions:
 *   - QEMU emits blob entries in the same order PciBusDxe discovers devices.
 *   - For identical VID:DID devices, discovery order is stable across passes.
 *   - The counter is maintained per VID:DID class, not globally.
 */
typedef struct {
  UINT16    VendorId;
  UINT16    DeviceId;
  UINTN     Count;       /* number of devices with this VID:DID in the blob */
  UINTN     CurrentIdx;  /* next entry to serve; wraps at Count */
} QFBD_DEVICE_CLASS;

STATIC QFBD_DEVICE        *mDevices;
STATIC UINTN               mNumDevices;
STATIC QFBD_DEVICE_CLASS  *mClasses;   /* heap-allocated; mNumDevices entries max */
STATIC UINTN               mNumClasses;

STATIC VOID
ReserveFixedBarRanges (
  VOID
  )
{
  UINTN       DevIdx;
  UINTN       BarIdx;
  UINT64      Min64;
  UINT64      Max64;
  UINT64      Min32;
  UINT64      Max32;
  UINT64      Gran;
  UINT64      Base;
  UINT64      Size;
  EFI_STATUS  Status;

  Min64 = MAX_UINT64;
  Max64 = 0;
  Min32 = MAX_UINT64;
  Max32 = 0;
  Gran  = SIZE_1MB;

  for (DevIdx = 0; DevIdx < mNumDevices; DevIdx++) {
    QFBD_DEVICE  *Dev = &mDevices[DevIdx];

    if (!(Dev->DevFlags & QEMU_FIXED_BARS_DEV_F_FIXED)) {
      continue;
    }
    for (BarIdx = 0; BarIdx < Dev->NumBars; BarIdx++) {
      QEMU_FIXED_BARS_BAR  *Bar = &Dev->Bars[BarIdx];

      if (Bar->Flags & QEMU_FIXED_BAR_F_MEM64) {
        if (Bar->Address < Min64) {
          Min64 = Bar->Address;
        }
        if (Bar->Address + Bar->Size > Max64) {
          Max64 = Bar->Address + Bar->Size;
        }
      } else {
        if (Bar->Address < Min32) {
          Min32 = Bar->Address;
        }
        if (Bar->Address + Bar->Size > Max32) {
          Max32 = Bar->Address + Bar->Size;
        }
      }
    }
  }

  if (Min64 < Max64) {
    Base   = Min64 & ~(Gran - 1);
    Size   = ALIGN_VALUE (Max64 - Base, Gran);
    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    EfiGcdMemoryTypeMemoryMappedIo,
                    0, Size, &Base, gImageHandle, NULL
                    );
    DEBUG ((EFI_ERROR (Status) ? DEBUG_ERROR : DEBUG_INFO,
            "QFBD: reserve 64-bit 0x%016Lx+0x%016Lx: %r\n",
            Base, Size, Status));
  }

  if (Min32 < Max32) {
    Base   = Min32 & ~(Gran - 1);
    Size   = ALIGN_VALUE (Max32 - Base, Gran);
    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    EfiGcdMemoryTypeMemoryMappedIo,
                    0, Size, &Base, gImageHandle, NULL
                    );
    DEBUG ((EFI_ERROR (Status) ? DEBUG_ERROR : DEBUG_INFO,
            "QFBD: reserve 32-bit 0x%016Lx+0x%016Lx: %r\n",
            Base, Size, Status));
  }
}

/* Walk the blob once, build the mDevices[] index and the mClasses[] table. */
STATIC VOID
InitDevices (
  IN UINTN  BlobSize
  )
{
  UINT8  *Ptr = mBlobData + sizeof (QEMU_FIXED_BARS_HDR);
  UINT8  *End = mBlobData + BlobSize;
  UINTN   DevIdx, ClassIdx;

  for (DevIdx = 0; DevIdx < mHdr.NumDevices; DevIdx++) {
    QEMU_FIXED_BARS_DEVICE  *Dev;
    UINT8                   *Next;

    if (Ptr + sizeof (QEMU_FIXED_BARS_DEVICE) > End) {
      DEBUG ((DEBUG_ERROR, "QFBD: blob truncated at device %u\n", (UINT32)DevIdx));
      break;
    }

    Dev  = (QEMU_FIXED_BARS_DEVICE *)Ptr;
    Next = Ptr
         + sizeof (QEMU_FIXED_BARS_DEVICE)
         + Dev->NumBars * sizeof (QEMU_FIXED_BARS_BAR);

    if (Next > End) {
      DEBUG ((DEBUG_ERROR, "QFBD: blob truncated at device %u\n", (UINT32)DevIdx));
      break;
    }

    mDevices[mNumDevices].VendorId  = Dev->VendorId;
    mDevices[mNumDevices].DeviceId  = Dev->DeviceId;
    mDevices[mNumDevices].DevFlags  = Dev->DevFlags;
    mDevices[mNumDevices].RpBus     = Dev->RpBus;
    mDevices[mNumDevices].NumBars   = Dev->NumBars;
    mDevices[mNumDevices].Bars      = (QEMU_FIXED_BARS_BAR *)(
      Ptr + sizeof (QEMU_FIXED_BARS_DEVICE)
      );
    mNumDevices++;

    /* Find or create the class entry for this VID:DID. */
    for (ClassIdx = 0; ClassIdx < mNumClasses; ClassIdx++) {
      if (mClasses[ClassIdx].VendorId == Dev->VendorId &&
          mClasses[ClassIdx].DeviceId == Dev->DeviceId)
      {
        mClasses[ClassIdx].Count++;
        break;
      }
    }
    if (ClassIdx == mNumClasses) {
      mClasses[mNumClasses].VendorId   = Dev->VendorId;
      mClasses[mNumClasses].DeviceId   = Dev->DeviceId;
      mClasses[mNumClasses].Count      = 1;
      mClasses[mNumClasses].CurrentIdx = 0;
      mNumClasses++;
    }

    DEBUG ((DEBUG_INFO,
            "QFBD: device %04x:%04x num_bars=%u\n",
            Dev->VendorId, Dev->DeviceId, Dev->NumBars));

    Ptr = Next;
  }

  for (ClassIdx = 0; ClassIdx < mNumClasses; ClassIdx++) {
    DEBUG ((DEBUG_INFO,
            "QFBD: class %04x:%04x count=%u\n",
            mClasses[ClassIdx].VendorId, mClasses[ClassIdx].DeviceId,
            (UINT32)mClasses[ClassIdx].Count));
  }
}

EFI_STATUS
FixedBarsCheckDevice (
  IN  UINTN  VendorId,
  IN  UINTN  DeviceId,
  OUT VOID   **Descriptors,
  OUT UINTN  *DescriptorsSize
  )
{
  QFBD_DEVICE_CLASS                  *Class;
  QFBD_DEVICE                        *Dev;
  UINTN                               BarIdx, DevIdx, MatchCount, ClassIdx;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;

  *Descriptors     = NULL;
  *DescriptorsSize = 0;

  if (mNumClasses == 0) {
    return EFI_SUCCESS;
  }

  /* Look up the per-VID:DID class. */
  Class = NULL;
  for (ClassIdx = 0; ClassIdx < mNumClasses; ClassIdx++) {
    if (mClasses[ClassIdx].VendorId == (UINT16)VendorId &&
        mClasses[ClassIdx].DeviceId == (UINT16)DeviceId)
    {
      Class = &mClasses[ClassIdx];
      break;
    }
  }
  if (Class == NULL) {
    return EFI_SUCCESS;  /* not a fixed-bar device */
  }

  /* Find the CurrentIdx-th device in mDevices[] matching this VID:DID. */
  Dev        = NULL;
  MatchCount = 0;
  for (DevIdx = 0; DevIdx < mNumDevices; DevIdx++) {
    if (mDevices[DevIdx].VendorId == (UINT16)VendorId &&
        mDevices[DevIdx].DeviceId == (UINT16)DeviceId)
    {
      if (MatchCount == Class->CurrentIdx) {
        Dev = &mDevices[DevIdx];
        break;
      }
      MatchCount++;
    }
  }
  if (Dev == NULL) {
    return EFI_SUCCESS;
  }

  /* Always advance the counter so ordering stays in sync with the blob. */
  Class->CurrentIdx++;
  if (Class->CurrentIdx >= Class->Count) {
    Class->CurrentIdx = 0;
  }

  if (!(Dev->DevFlags & QEMU_FIXED_BARS_DEV_F_FIXED)) {
    return EFI_SUCCESS;
  }

  Desc = AllocateZeroPool (Dev->NumBars * sizeof (*Desc));
  if (Desc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Descriptors     = Desc;
  *DescriptorsSize = Dev->NumBars * sizeof (*Desc);

  for (BarIdx = 0; BarIdx < Dev->NumBars; BarIdx++) {
    QEMU_FIXED_BARS_BAR  *Bar = &Dev->Bars[BarIdx];

    Desc->Desc                   = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Desc->Len                    = (UINT16)(sizeof (*Desc) - 3);
    Desc->ResType                = ACPI_ADDRESS_SPACE_TYPE_MEM;
    Desc->GenFlag                = ACPI_ADDR_FLAG_FIXED;
    Desc->SpecificFlag           = 0;
    Desc->AddrSpaceGranularity   = (Bar->Flags & QEMU_FIXED_BAR_F_MEM64) ? 64 : 32;
    Desc->AddrRangeMin           = Bar->Address;
    Desc->AddrRangeMax           = Bar->Size - 1;
    Desc->AddrTranslationOffset  = BarIdx;
    Desc->AddrLen                = Bar->Size;

    DEBUG ((DEBUG_INFO,
            "QFBD: %04x:%04x BAR%u -> 0x%016Lx size 0x%016Lx\n",
            Dev->VendorId, Dev->DeviceId,
            (UINT32)Bar->Bar, Bar->Address, Bar->Size));

    Desc++;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
FixedBarsInit (
  OUT BOOLEAN  *HasFixedBarsDevices
  )
{
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  EFI_STATUS            Status;

  *HasFixedBarsDevices = FALSE;

  Status = QemuFwCfgFindFile ("etc/fixed-bars", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "QFBD: etc/fixed-bars absent, nothing to do\n"));
    return EFI_SUCCESS;
  }

  if (FwCfgSize < sizeof (mHdr)) {
    DEBUG ((DEBUG_ERROR, "QFBD: blob too small (%Lu bytes)\n",
            (UINT64)FwCfgSize));
    return EFI_UNSUPPORTED;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof (mHdr), &mHdr);

  if (mHdr.Version != QEMU_FIXED_BARS_VERSION) {
    DEBUG ((DEBUG_ERROR,
            "QFBD: unsupported blob version %u (expected %u)\n",
            mHdr.Version, QEMU_FIXED_BARS_VERSION));
    return EFI_UNSUPPORTED;
  }

  if (mHdr.NumDevices == 0) {
    DEBUG ((DEBUG_INFO, "QFBD: zero devices, nothing to do\n"));
    return EFI_SUCCESS;
  }

  /* Read the full blob into a persistent buffer; device entries point into it. */
  mBlobData = AllocatePool (FwCfgSize);
  if (mBlobData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, mBlobData);
  CopyMem (&mHdr, mBlobData, sizeof (mHdr));

  mDevices = AllocateZeroPool (mHdr.NumDevices * sizeof (QFBD_DEVICE));
  if (mDevices == NULL) {
    FreePool (mBlobData);
    return EFI_OUT_OF_RESOURCES;
  }

  /* Worst case: every device has a unique VID:DID — allocate mNumDevices slots. */
  mClasses = AllocateZeroPool (mHdr.NumDevices * sizeof (QFBD_DEVICE_CLASS));
  if (mClasses == NULL) {
    FreePool (mDevices);
    FreePool (mBlobData);
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "QFBD: %u device(s)\n", mHdr.NumDevices));

  mNumDevices = 0;
  mNumClasses = 0;
  InitDevices (FwCfgSize);
  ReserveFixedBarRanges ();

  *HasFixedBarsDevices = TRUE;

  return EFI_SUCCESS;
}
