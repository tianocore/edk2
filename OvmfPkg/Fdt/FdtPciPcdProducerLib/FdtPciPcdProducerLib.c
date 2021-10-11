/** @file
  FDT client library for consumers of PCI related dynamic PCDs

  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

//
// We expect the "ranges" property of "pci-host-ecam-generic" to consist of
// records like this.
//
#pragma pack (1)
typedef struct {
  UINT32 Type;
  UINT64 ChildBase;
  UINT64 CpuBase;
  UINT64 Size;
} DTB_PCI_HOST_RANGE_RECORD;
#pragma pack ()

#define DTB_PCI_HOST_RANGE_RELOCATABLE  BIT31
#define DTB_PCI_HOST_RANGE_PREFETCHABLE BIT30
#define DTB_PCI_HOST_RANGE_ALIASED      BIT29
#define DTB_PCI_HOST_RANGE_MMIO32       BIT25
#define DTB_PCI_HOST_RANGE_MMIO64       (BIT25 | BIT24)
#define DTB_PCI_HOST_RANGE_IO           BIT24
#define DTB_PCI_HOST_RANGE_TYPEMASK     (BIT31 | BIT30 | BIT29 | BIT25 | BIT24)

STATIC
RETURN_STATUS
GetPciIoTranslation (
  IN  FDT_CLIENT_PROTOCOL *FdtClient,
  IN  INT32               Node,
  OUT UINT64              *IoTranslation
  )
{
  UINT32        RecordIdx;
  CONST VOID    *Prop;
  UINT32        Len;
  EFI_STATUS    Status;
  UINT64        IoBase;

  //
  // Iterate over "ranges".
  //
  Status = FdtClient->GetNodeProperty (FdtClient, Node, "ranges", &Prop, &Len);
  if (EFI_ERROR (Status) || Len == 0 ||
      Len % sizeof (DTB_PCI_HOST_RANGE_RECORD) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: 'ranges' not found or invalid\n", __FUNCTION__));
    return RETURN_PROTOCOL_ERROR;
  }

  for (RecordIdx = 0; RecordIdx < Len / sizeof (DTB_PCI_HOST_RANGE_RECORD);
       ++RecordIdx) {
    CONST DTB_PCI_HOST_RANGE_RECORD *Record;
    UINT32                          Type;

    Record = (CONST DTB_PCI_HOST_RANGE_RECORD *)Prop + RecordIdx;
    Type = SwapBytes32 (Record->Type) & DTB_PCI_HOST_RANGE_TYPEMASK;
    if (Type == DTB_PCI_HOST_RANGE_IO) {
      IoBase = SwapBytes64 (Record->ChildBase);
      *IoTranslation = SwapBytes64 (Record->CpuBase) - IoBase;

      return RETURN_SUCCESS;
    }
  }
  return RETURN_NOT_FOUND;
}

RETURN_STATUS
EFIAPI
FdtPciPcdProducerLibConstructor (
  VOID
  )
{
  UINT64              PciExpressBaseAddress;
  FDT_CLIENT_PROTOCOL *FdtClient;
  CONST UINT64        *Reg;
  UINT32              RegSize;
  EFI_STATUS          Status;
  INT32               Node;
  RETURN_STATUS       RetStatus;
  UINT64              IoTranslation;
  RETURN_STATUS       PcdStatus;

  PciExpressBaseAddress = PcdGet64 (PcdPciExpressBaseAddress);
  if (PciExpressBaseAddress != MAX_UINT64) {
    //
    // Assume that the fact that PciExpressBaseAddress has been changed from
    // its default value of MAX_UINT64 implies that this code has been
    // executed already, in the context of another module. That means we can
    // assume that PcdPciIoTranslation has been discovered from the DT node
    // as well.
    //
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  PciExpressBaseAddress = 0;
  Status = FdtClient->FindCompatibleNode (FdtClient, "pci-host-ecam-generic",
                        &Node);

  if (!EFI_ERROR (Status)) {
    Status = FdtClient->GetNodeProperty (FdtClient, Node, "reg",
                          (CONST VOID **)&Reg, &RegSize);

    if (!EFI_ERROR (Status) && RegSize == 2 * sizeof (UINT64)) {
      PciExpressBaseAddress = SwapBytes64 (*Reg);

      PcdStatus = PcdSetBoolS (PcdPciDisableBusEnumeration, FALSE);
      ASSERT_RETURN_ERROR (PcdStatus);

      IoTranslation = 0;
      RetStatus = GetPciIoTranslation (FdtClient, Node, &IoTranslation);
      if (!RETURN_ERROR (RetStatus)) {
          PcdStatus = PcdSet64S (PcdPciIoTranslation, IoTranslation);
          ASSERT_RETURN_ERROR (PcdStatus);
      } else {
        //
        // Support for I/O BARs is not mandatory, and so it does not make sense
        // to abort in the general case. So leave it up to the actual driver to
        // complain about this if it wants to, and just issue a warning here.
        //
        DEBUG ((EFI_D_WARN,
          "%a: 'pci-host-ecam-generic' device encountered with no I/O range\n",
          __FUNCTION__));
      }
    }
  }

  PcdStatus = PcdSet64S (PcdPciExpressBaseAddress, PciExpressBaseAddress);
  ASSERT_RETURN_ERROR (PcdStatus);

  return RETURN_SUCCESS;
}
