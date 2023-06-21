/** @file
  Temporarily enable IO and MMIO decoding for all PCI devices while QEMU
  regenerates the ACPI tables.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/AcpiPlatformLib.h>
#include <Library/DebugLib.h>                  // DEBUG()
#include <Library/MemoryAllocationLib.h>       // AllocatePool()
#include <Library/UefiBootServicesTableLib.h>  // gBS

/**
  Collect all PciIo protocol instances in the system. Save their original
  attributes, and enable IO and MMIO decoding for each.

  This is a best effort function; it doesn't return status codes. Its
  caller is supposed to proceed even if this function fails.

  @param[out] OriginalAttributes  On output, a dynamically allocated array of
                                  ORIGINAL_ATTRIBUTES elements. The array lists
                                  the PciIo protocol instances found in the
                                  system at the time of the call, plus the
                                  original PCI attributes for each.

                                  Before returning, the function enables IO and
                                  MMIO decoding for each PciIo instance it
                                  finds.

                                  On error, or when no such instances are
                                  found, OriginalAttributes is set to NULL.

  @param[out] Count               On output, the number of elements in
                                  OriginalAttributes. On error it is set to
                                  zero.
**/
VOID
EnablePciDecoding (
  OUT ORIGINAL_ATTRIBUTES  **OriginalAttributes,
  OUT UINTN                *Count
  )
{
  EFI_STATUS           Status;
  UINTN                NoHandles;
  EFI_HANDLE           *Handles;
  ORIGINAL_ATTRIBUTES  *OrigAttrs;
  UINTN                Idx;

  *OriginalAttributes = NULL;
  *Count              = 0;

  if (PcdGetBool (PcdPciDisableBusEnumeration)) {
    //
    // The platform downloads ACPI tables from QEMU in general, but there are
    // no root bridges in this execution. We're done.
    //
    return;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL /* SearchKey */,
                  &NoHandles,
                  &Handles
                  );
  if (Status == EFI_NOT_FOUND) {
    //
    // No PCI devices were found on either of the root bridges. We're done.
    //
    return;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: LocateHandleBuffer(): %r\n",
      __func__,
      Status
      ));
    return;
  }

  OrigAttrs = AllocatePool (NoHandles * sizeof *OrigAttrs);
  if (OrigAttrs == NULL) {
    DEBUG ((
      DEBUG_WARN,
      "%a: AllocatePool(): out of resources\n",
      __func__
      ));
    goto FreeHandles;
  }

  for (Idx = 0; Idx < NoHandles; ++Idx) {
    EFI_PCI_IO_PROTOCOL  *PciIo;
    UINT64               Attributes;

    //
    // Look up PciIo on the handle and stash it
    //
    Status = gBS->HandleProtocol (
                    Handles[Idx],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    ASSERT_EFI_ERROR (Status);
    OrigAttrs[Idx].PciIo = PciIo;

    //
    // Stash the current attributes
    //
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationGet,
                      0,
                      &OrigAttrs[Idx].PciAttributes
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "%a: EfiPciIoAttributeOperationGet: %r\n",
        __func__,
        Status
        ));
      goto RestoreAttributes;
    }

    //
    // Retrieve supported attributes
    //
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationSupported,
                      0,
                      &Attributes
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "%a: EfiPciIoAttributeOperationSupported: %r\n",
        __func__,
        Status
        ));
      goto RestoreAttributes;
    }

    //
    // Enable IO and MMIO decoding
    //
    Attributes &= EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_MEMORY;
    Status      = PciIo->Attributes (
                           PciIo,
                           EfiPciIoAttributeOperationEnable,
                           Attributes,
                           NULL
                           );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "%a: EfiPciIoAttributeOperationEnable: %r\n",
        __func__,
        Status
        ));
      goto RestoreAttributes;
    }
  }

  //
  // Success
  //
  FreePool (Handles);
  *OriginalAttributes = OrigAttrs;
  *Count              = NoHandles;
  return;

RestoreAttributes:
  while (Idx > 0) {
    --Idx;
    OrigAttrs[Idx].PciIo->Attributes (
                            OrigAttrs[Idx].PciIo,
                            EfiPciIoAttributeOperationSet,
                            OrigAttrs[Idx].PciAttributes,
                            NULL
                            );
  }

  FreePool (OrigAttrs);

FreeHandles:
  FreePool (Handles);
}

/**
  Restore the original PCI attributes saved with EnablePciDecoding().

  @param[in] OriginalAttributes  The array allocated and populated by
                                 EnablePciDecoding(). This parameter may be
                                 NULL. If OriginalAttributes is NULL, then the
                                 function is a no-op; otherwise the PciIo
                                 attributes will be restored, and the
                                 OriginalAttributes array will be freed.

  @param[in] Count               The Count value stored by EnablePciDecoding(),
                                 the number of elements in OriginalAttributes.
                                 Count may be zero if and only if
                                 OriginalAttributes is NULL.
**/
VOID
RestorePciDecoding (
  IN ORIGINAL_ATTRIBUTES  *OriginalAttributes,
  IN UINTN                Count
  )
{
  UINTN  Idx;

  ASSERT ((OriginalAttributes == NULL) == (Count == 0));
  if (OriginalAttributes == NULL) {
    return;
  }

  for (Idx = 0; Idx < Count; ++Idx) {
    OriginalAttributes[Idx].PciIo->Attributes (
                                     OriginalAttributes[Idx].PciIo,
                                     EfiPciIoAttributeOperationSet,
                                     OriginalAttributes[Idx].PciAttributes,
                                     NULL
                                     );
  }

  FreePool (OriginalAttributes);
}
