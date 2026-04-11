/** @file
  Rewrite the BootOrder NvVar based on QEMU's "bootorder" fw_cfg file.

  Copyright (C) 2012 - 2014, Red Hat, Inc.
  Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/QemuFwCfgLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Library/QemuBootOrderLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/GlobalVariable.h>
#include <Guid/VirtioMmioTransport.h>

#include "ExtraRootBusMap.h"

/**
  OpenFirmware to UEFI device path translation output buffer size in CHAR16's.
**/
#define TRANSLATION_OUTPUT_SIZE  0x100

/**
  Output buffer size for OpenFirmware to UEFI device path fragment translation,
  in CHAR16's, for a sequence of PCI bridges.
**/
#define BRIDGE_TRANSLATION_OUTPUT_SIZE  0x40

/**
  Numbers of nodes in OpenFirmware device paths that are required and examined.
**/
#define REQUIRED_PCI_OFW_NODES   2
#define REQUIRED_MMIO_OFW_NODES  1
#define EXAMINED_OFW_NODES       6

/**
  Simple character classification routines, corresponding to POSIX class names
  and ASCII encoding.
**/
STATIC
BOOLEAN
IsAlnum (
  IN  CHAR8  Chr
  )
{
  return (('0' <= Chr && Chr <= '9') ||
          ('A' <= Chr && Chr <= 'Z') ||
          ('a' <= Chr && Chr <= 'z')
          );
}

STATIC
BOOLEAN
IsDriverNamePunct (
  IN  CHAR8  Chr
  )
{
  return (Chr == ',' ||  Chr == '.' || Chr == '_' ||
          Chr == '+' || Chr == '-'
          );
}

STATIC
BOOLEAN
IsPrintNotDelim (
  IN  CHAR8  Chr
  )
{
  return (32 <= Chr && Chr <= 126 &&
          Chr != '/' && Chr != '@' && Chr != ':');
}

/**
  Utility types and functions.
**/
typedef struct {
  CONST CHAR8    *Ptr; // not necessarily NUL-terminated
  UINTN          Len;  // number of non-NUL characters
} SUBSTRING;

/**

  Check if Substring and String have identical contents.

  The function relies on the restriction that a SUBSTRING cannot have embedded
  NULs either.

  @param[in] Substring  The SUBSTRING input to the comparison.

  @param[in] String     The ASCII string input to the comparison.


  @return  Whether the inputs have identical contents.

**/
STATIC
BOOLEAN
SubstringEq (
  IN  SUBSTRING    Substring,
  IN  CONST CHAR8  *String
  )
{
  UINTN        Pos;
  CONST CHAR8  *Chr;

  Pos = 0;
  Chr = String;

  while (Pos < Substring.Len && Substring.Ptr[Pos] == *Chr) {
    ++Pos;
    ++Chr;
  }

  return (BOOLEAN)(Pos == Substring.Len && *Chr == '\0');
}

/**

  Parse a comma-separated list of hexadecimal integers into the elements of an
  UINT64 array.

  Whitespace, "0x" prefixes, leading or trailing commas, sequences of commas,
  or an empty string are not allowed; they are rejected.

  The function relies on ASCII encoding.

  @param[in]     UnitAddress  The substring to parse.

  @param[out]    Result       The array, allocated by the caller, to receive
                              the parsed values. This parameter may be NULL if
                              NumResults is zero on input.

  @param[in out] NumResults   On input, the number of elements allocated for
                              Result. On output, the number of elements it has
                              taken (or would have taken) to parse the string
                              fully.


  @retval RETURN_SUCCESS            UnitAddress has been fully parsed.
                                    NumResults is set to the number of parsed
                                    values; the corresponding elements have
                                    been set in Result. The rest of Result's
                                    elements are unchanged.

  @retval RETURN_BUFFER_TOO_SMALL   UnitAddress has been fully parsed.
                                    NumResults is set to the number of parsed
                                    values, but elements have been stored only
                                    up to the input value of NumResults, which
                                    is less than what has been parsed.

  @retval RETURN_INVALID_PARAMETER  Parse error. The contents of Results is
                                    indeterminate. NumResults has not been
                                    changed.

**/
STATIC
RETURN_STATUS
ParseUnitAddressHexList (
  IN      SUBSTRING  UnitAddress,
  OUT     UINT64     *Result,
  IN OUT  UINTN      *NumResults
  )
{
  UINTN          Entry;    // number of entry currently being parsed
  UINT64         EntryVal; // value being constructed for current entry
  CHAR8          PrevChr;  // UnitAddress character previously checked
  UINTN          Pos;      // current position within UnitAddress
  RETURN_STATUS  Status;

  Entry    = 0;
  EntryVal = 0;
  PrevChr  = ',';

  for (Pos = 0; Pos < UnitAddress.Len; ++Pos) {
    CHAR8  Chr;
    INT8   Val;

    Chr = UnitAddress.Ptr[Pos];
    Val = ('a' <= Chr && Chr <= 'f') ? (Chr - 'a' + 10) :
          ('A' <= Chr && Chr <= 'F') ? (Chr - 'A' + 10) :
          ('0' <= Chr && Chr <= '9') ? (Chr - '0') :
          -1;

    if (Val >= 0) {
      if (EntryVal > 0xFFFFFFFFFFFFFFFull) {
        return RETURN_INVALID_PARAMETER;
      }

      EntryVal = LShiftU64 (EntryVal, 4) | Val;
    } else if (Chr == ',') {
      if (PrevChr == ',') {
        return RETURN_INVALID_PARAMETER;
      }

      if (Entry < *NumResults) {
        Result[Entry] = EntryVal;
      }

      ++Entry;
      EntryVal = 0;
    } else {
      return RETURN_INVALID_PARAMETER;
    }

    PrevChr = Chr;
  }

  if (PrevChr == ',') {
    return RETURN_INVALID_PARAMETER;
  }

  if (Entry < *NumResults) {
    Result[Entry] = EntryVal;
    Status        = RETURN_SUCCESS;
  } else {
    Status = RETURN_BUFFER_TOO_SMALL;
  }

  ++Entry;

  *NumResults = Entry;
  return Status;
}

/**
  A simple array of Boot Option ID's.
**/
typedef struct {
  UINT16    *Data;
  UINTN     Allocated;
  UINTN     Produced;
} BOOT_ORDER;

/**
  Array element tracking an enumerated boot option that has the
  LOAD_OPTION_ACTIVE attribute.
**/
typedef struct {
  CONST EFI_BOOT_MANAGER_LOAD_OPTION    *BootOption; // reference only, no
                                                     //   ownership
  BOOLEAN                               Appended;    // has been added to a
                                                     //   BOOT_ORDER?
} ACTIVE_OPTION;

/**

  Append an active boot option to BootOrder, reallocating the latter if needed.

  @param[in out] BootOrder     The structure pointing to the array and holding
                               allocation and usage counters.

  @param[in]     ActiveOption  The active boot option whose ID should be
                               appended to the array.


  @retval RETURN_SUCCESS           ID of ActiveOption appended.

  @retval RETURN_OUT_OF_RESOURCES  Memory reallocation failed.

**/
STATIC
RETURN_STATUS
BootOrderAppend (
  IN OUT  BOOT_ORDER     *BootOrder,
  IN OUT  ACTIVE_OPTION  *ActiveOption
  )
{
  if (BootOrder->Produced == BootOrder->Allocated) {
    UINTN   AllocatedNew;
    UINT16  *DataNew;

    ASSERT (BootOrder->Allocated > 0);
    AllocatedNew = BootOrder->Allocated * 2;
    DataNew      = ReallocatePool (
                     BootOrder->Allocated * sizeof (*BootOrder->Data),
                     AllocatedNew         * sizeof (*DataNew),
                     BootOrder->Data
                     );
    if (DataNew == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }

    BootOrder->Allocated = AllocatedNew;
    BootOrder->Data      = DataNew;
  }

  BootOrder->Data[BootOrder->Produced++] =
    (UINT16)ActiveOption->BootOption->OptionNumber;
  ActiveOption->Appended = TRUE;
  return RETURN_SUCCESS;
}

/**

  Create an array of ACTIVE_OPTION elements for a boot option array.

  @param[in]  BootOptions      A boot option array, created with
                               EfiBootManagerRefreshAllBootOption () and
                               EfiBootManagerGetLoadOptions ().

  @param[in]  BootOptionCount  The number of elements in BootOptions.

  @param[out] ActiveOption     Pointer to the first element in the new array.
                               The caller is responsible for freeing the array
                               with FreePool() after use.

  @param[out] Count            Number of elements in the new array.


  @retval RETURN_SUCCESS           The ActiveOption array has been created.

  @retval RETURN_NOT_FOUND         No active entry has been found in
                                   BootOptions.

  @retval RETURN_OUT_OF_RESOURCES  Memory allocation failed.

**/
STATIC
RETURN_STATUS
CollectActiveOptions (
  IN   CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  IN   UINTN                               BootOptionCount,
  OUT  ACTIVE_OPTION                       **ActiveOption,
  OUT  UINTN                               *Count
  )
{
  UINTN  Index;
  UINTN  ScanMode;

  *ActiveOption = NULL;

  //
  // Scan the list twice:
  // - count active entries,
  // - store links to active entries.
  //
  for (ScanMode = 0; ScanMode < 2; ++ScanMode) {
    *Count = 0;
    for (Index = 0; Index < BootOptionCount; Index++) {
      if ((BootOptions[Index].Attributes & LOAD_OPTION_ACTIVE) != 0) {
        if (ScanMode == 1) {
          (*ActiveOption)[*Count].BootOption = &BootOptions[Index];
          (*ActiveOption)[*Count].Appended   = FALSE;
        }

        ++*Count;
      }
    }

    if (ScanMode == 0) {
      if (*Count == 0) {
        return RETURN_NOT_FOUND;
      }

      *ActiveOption = AllocatePool (*Count * sizeof **ActiveOption);
      if (*ActiveOption == NULL) {
        return RETURN_OUT_OF_RESOURCES;
      }
    }
  }

  return RETURN_SUCCESS;
}

/**
  OpenFirmware device path node
**/
typedef struct {
  SUBSTRING    DriverName;
  SUBSTRING    UnitAddress;
  SUBSTRING    DeviceArguments;
} OFW_NODE;

/**

  Parse an OpenFirmware device path node into the caller-allocated OFW_NODE
  structure, and advance in the input string.

  The node format is mostly parsed after IEEE 1275-1994, 3.2.1.1 "Node names"
  (a leading slash is expected and not returned):

    /driver-name@unit-address[:device-arguments][<LF>]

  A single trailing <LF> character is consumed but not returned. A trailing
  <LF> or NUL character terminates the device path.

  The function relies on ASCII encoding.

  @param[in out] Ptr      Address of the pointer pointing to the start of the
                          node string. After successful parsing *Ptr is set to
                          the byte immediately following the consumed
                          characters. On error it points to the byte that
                          caused the error. The input string is never modified.

  @param[out]    OfwNode  The members of this structure point into the input
                          string, designating components of the node.
                          Separators are never included. If "device-arguments"
                          is missing, then DeviceArguments.Ptr is set to NULL.
                          All components that are present have nonzero length.

                          If the call doesn't succeed, the contents of this
                          structure is indeterminate.

  @param[out]    IsFinal  In case of successful parsing, this parameter signals
                          whether the node just parsed is the final node in the
                          device path. The call after a final node will attempt
                          to start parsing the next path. If the call doesn't
                          succeed, then this parameter is not changed.


  @retval RETURN_SUCCESS            Parsing successful.

  @retval RETURN_NOT_FOUND          Parsing terminated. *Ptr was (and is)
                                    pointing to an empty string.

  @retval RETURN_INVALID_PARAMETER  Parse error.

**/
STATIC
RETURN_STATUS
ParseOfwNode (
  IN OUT  CONST CHAR8  **Ptr,
  OUT     OFW_NODE     *OfwNode,
  OUT     BOOLEAN      *IsFinal
  )
{
  BOOLEAN  AcceptSlash = FALSE;

  //
  // A leading slash is expected. End of string is tolerated.
  //
  switch (**Ptr) {
    case '\0':
      return RETURN_NOT_FOUND;

    case '/':
      ++*Ptr;
      break;

    default:
      return RETURN_INVALID_PARAMETER;
  }

  //
  // driver-name
  //
  OfwNode->DriverName.Ptr = *Ptr;
  OfwNode->DriverName.Len = 0;
  while (OfwNode->DriverName.Len < 32 &&
         (IsAlnum (**Ptr) || IsDriverNamePunct (**Ptr))
         )
  {
    ++*Ptr;
    ++OfwNode->DriverName.Len;
  }

  if ((OfwNode->DriverName.Len == 0) || (OfwNode->DriverName.Len == 32)) {
    return RETURN_INVALID_PARAMETER;
  }

  if (SubstringEq (OfwNode->DriverName, "rom")) {
    //
    // bug compatibility hack
    //
    // qemu passes fw_cfg filenames as rom unit address.
    // The filenames have slashes:
    //      /rom@genroms/linuxboot_dma.bin
    //
    // Alow slashes in the unit address to avoid the parser trip up,
    // so we can successfully parse the following lines (the rom
    // entries themself are ignored).
    //
    AcceptSlash = TRUE;
  }

  //
  // unit-address
  //
  if (**Ptr != '@') {
    return RETURN_INVALID_PARAMETER;
  }

  ++*Ptr;

  OfwNode->UnitAddress.Ptr = *Ptr;
  OfwNode->UnitAddress.Len = 0;
  while (IsPrintNotDelim (**Ptr) || (AcceptSlash && **Ptr == '/')) {
    ++*Ptr;
    ++OfwNode->UnitAddress.Len;
  }

  if (OfwNode->UnitAddress.Len == 0) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // device-arguments, may be omitted
  //
  OfwNode->DeviceArguments.Len = 0;
  if (**Ptr == ':') {
    ++*Ptr;
    OfwNode->DeviceArguments.Ptr = *Ptr;

    while (IsPrintNotDelim (**Ptr)) {
      ++*Ptr;
      ++OfwNode->DeviceArguments.Len;
    }

    if (OfwNode->DeviceArguments.Len == 0) {
      return RETURN_INVALID_PARAMETER;
    }
  } else {
    OfwNode->DeviceArguments.Ptr = NULL;
  }

  switch (**Ptr) {
    case '\n':
      ++*Ptr;
    //
    // fall through
    //

    case '\0':
      *IsFinal = TRUE;
      break;

    case '/':
      *IsFinal = FALSE;
      break;

    default:
      return RETURN_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: DriverName=\"%.*a\" UnitAddress=\"%.*a\" DeviceArguments=\"%.*a\"\n",
    __func__,
    OfwNode->DriverName.Len,
    OfwNode->DriverName.Ptr,
    OfwNode->UnitAddress.Len,
    OfwNode->UnitAddress.Ptr,
    OfwNode->DeviceArguments.Len,
    OfwNode->DeviceArguments.Ptr == NULL ? "" : OfwNode->DeviceArguments.Ptr
    ));
  return RETURN_SUCCESS;
}

/**

  Translate a PCI-like array of OpenFirmware device nodes to a UEFI device path
  fragment.

  @param[in]     OfwNode         Array of OpenFirmware device nodes to
                                 translate, constituting the beginning of an
                                 OpenFirmware device path.

  @param[in]     NumNodes        Number of elements in OfwNode.

  @param[in]     ExtraPciRoots   An EXTRA_ROOT_BUS_MAP object created with
                                 CreateExtraRootBusMap(), to be used for
                                 translating positions of extra root buses to
                                 bus numbers.

  @param[out]    Translated      Destination array receiving the UEFI path
                                 fragment, allocated by the caller. If the
                                 return value differs from RETURN_SUCCESS, its
                                 contents is indeterminate.

  @param[in out] TranslatedSize  On input, the number of CHAR16's in
                                 Translated. On RETURN_SUCCESS this parameter
                                 is assigned the number of non-NUL CHAR16's
                                 written to Translated. In case of other return
                                 values, TranslatedSize is indeterminate.


  @retval RETURN_SUCCESS           Translation successful.

  @retval RETURN_BUFFER_TOO_SMALL  The translation does not fit into the number
                                   of bytes provided.

  @retval RETURN_UNSUPPORTED       The array of OpenFirmware device nodes can't
                                   be translated in the current implementation.

  @retval RETURN_PROTOCOL_ERROR    The initial OpenFirmware node refers to an
                                   extra PCI root bus (by serial number) that
                                   is invalid according to ExtraPciRoots.

**/
STATIC
RETURN_STATUS
TranslatePciOfwNodes (
  IN      CONST OFW_NODE            *OfwNode,
  IN      UINTN                     NumNodes,
  IN      CONST EXTRA_ROOT_BUS_MAP  *ExtraPciRoots,
  OUT     CHAR16                    *Translated,
  IN OUT  UINTN                     *TranslatedSize
  )
{
  UINT32  PciRoot;
  CHAR8   *Comma;
  UINTN   FirstNonBridge;
  CHAR16  Bridges[BRIDGE_TRANSLATION_OUTPUT_SIZE];
  UINTN   BridgesLen;
  UINT64  PciDevFun[2];
  UINTN   NumEntries;
  UINTN   Written;

  //
  // Resolve the PCI root bus number.
  //
  // The initial OFW node for the main root bus (ie. bus number 0) is:
  //
  //   /pci@i0cf8
  //
  // For extra root buses, the initial OFW node is
  //
  //   /pci@i0cf8,4
  //              ^
  //              root bus serial number (not PCI bus number)
  //
  if ((NumNodes < REQUIRED_PCI_OFW_NODES) ||
      !SubstringEq (OfwNode[0].DriverName, "pci")
      )
  {
    return RETURN_UNSUPPORTED;
  }

  PciRoot = 0;
  Comma   = ScanMem8 (
              OfwNode[0].UnitAddress.Ptr,
              OfwNode[0].UnitAddress.Len,
              ','
              );
  if (Comma != NULL) {
    SUBSTRING  PciRootSerialSubString;
    UINT64     PciRootSerial;

    //
    // Parse the root bus serial number from the unit address after the comma.
    //
    PciRootSerialSubString.Ptr = Comma + 1;
    PciRootSerialSubString.Len = OfwNode[0].UnitAddress.Len -
                                 (PciRootSerialSubString.Ptr -
                                  OfwNode[0].UnitAddress.Ptr);
    NumEntries = 1;
    if (RETURN_ERROR (
          ParseUnitAddressHexList (
            PciRootSerialSubString,
            &PciRootSerial,
            &NumEntries
            )
          ))
    {
      return RETURN_UNSUPPORTED;
    }

    //
    // Map the extra root bus's serial number to its actual bus number.
    //
    if (EFI_ERROR (
          MapRootBusPosToBusNr (
            ExtraPciRoots,
            PciRootSerial,
            &PciRoot
            )
          ))
    {
      return RETURN_PROTOCOL_ERROR;
    }
  }

  //
  // Translate a sequence of PCI bridges. For each bridge, the OFW node is:
  //
  //   pci-bridge@1e[,0]
  //              ^   ^
  //              PCI slot & function on the parent, holding the bridge
  //
  // and the UEFI device path node is:
  //
  //   Pci(0x1E,0x0)
  //
  FirstNonBridge = 1;
  Bridges[0]     = L'\0';
  BridgesLen     = 0;
  do {
    UINT64  BridgeDevFun[2];
    UINTN   BridgesFreeBytes;

    if (!SubstringEq (OfwNode[FirstNonBridge].DriverName, "pci-bridge")) {
      break;
    }

    BridgeDevFun[1] = 0;
    NumEntries      = sizeof BridgeDevFun / sizeof BridgeDevFun[0];
    if (ParseUnitAddressHexList (
          OfwNode[FirstNonBridge].UnitAddress,
          BridgeDevFun,
          &NumEntries
          ) != RETURN_SUCCESS)
    {
      return RETURN_UNSUPPORTED;
    }

    BridgesFreeBytes = sizeof Bridges - BridgesLen * sizeof Bridges[0];
    Written          = UnicodeSPrintAsciiFormat (
                         Bridges + BridgesLen,
                         BridgesFreeBytes,
                         "/Pci(0x%Lx,0x%Lx)",
                         BridgeDevFun[0],
                         BridgeDevFun[1]
                         );
    BridgesLen += Written;

    //
    // There's no way to differentiate between "completely used up without
    // truncation" and "truncated", so treat the former as the latter.
    //
    if (BridgesLen + 1 == BRIDGE_TRANSLATION_OUTPUT_SIZE) {
      return RETURN_UNSUPPORTED;
    }

    ++FirstNonBridge;
  } while (FirstNonBridge < NumNodes);

  if (FirstNonBridge == NumNodes) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Parse the OFW nodes starting with the first non-bridge node.
  //
  PciDevFun[1] = 0;
  NumEntries   = ARRAY_SIZE (PciDevFun);
  if (ParseUnitAddressHexList (
        OfwNode[FirstNonBridge].UnitAddress,
        PciDevFun,
        &NumEntries
        ) != RETURN_SUCCESS
      )
  {
    return RETURN_UNSUPPORTED;
  }

  if ((NumNodes >= FirstNonBridge + 3) &&
      SubstringEq (OfwNode[FirstNonBridge + 0].DriverName, "ide") &&
      SubstringEq (OfwNode[FirstNonBridge + 1].DriverName, "drive") &&
      SubstringEq (OfwNode[FirstNonBridge + 2].DriverName, "disk")
      )
  {
    //
    // OpenFirmware device path (IDE disk, IDE CD-ROM):
    //
    //   /pci@i0cf8/ide@1,1/drive@0/disk@0
    //        ^         ^ ^       ^      ^
    //        |         | |       |      master or slave
    //        |         | |       primary or secondary
    //        |         PCI slot & function holding IDE controller
    //        PCI root at system bus port, PIO
    //
    // UEFI device path:
    //
    //   PciRoot(0x0)/Pci(0x1,0x1)/Ata(Primary,Master,0x0)
    //                                                ^
    //                                                fixed LUN
    //
    UINT64  Secondary;
    UINT64  Slave;

    NumEntries = 1;
    if ((ParseUnitAddressHexList (
           OfwNode[FirstNonBridge + 1].UnitAddress,
           &Secondary,
           &NumEntries
           ) != RETURN_SUCCESS) ||
        (Secondary > 1) ||
        (ParseUnitAddressHexList (
           OfwNode[FirstNonBridge + 2].UnitAddress,
           &Slave,
           &NumEntries // reuse after previous single-element call
           ) != RETURN_SUCCESS) ||
        (Slave > 1)
        )
    {
      return RETURN_UNSUPPORTED;
    }

    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)/Ata(%a,%a,0x0)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1],
                Secondary ? "Secondary" : "Primary",
                Slave ? "Slave" : "Master"
                );
  } else if ((NumNodes >= FirstNonBridge + 3) &&
             SubstringEq (OfwNode[FirstNonBridge + 0].DriverName, "pci8086,2922") &&
             SubstringEq (OfwNode[FirstNonBridge + 1].DriverName, "drive") &&
             SubstringEq (OfwNode[FirstNonBridge + 2].DriverName, "disk")
             )
  {
    //
    // OpenFirmware device path (Q35 SATA disk and CD-ROM):
    //
    //   /pci@i0cf8/pci8086,2922@1f,2/drive@1/disk@0
    //        ^                  ^  ^       ^      ^
    //        |                  |  |       |      device number (fixed 0)
    //        |                  |  |       channel (port) number
    //        |                  PCI slot & function holding SATA HBA
    //        PCI root at system bus port, PIO
    //
    // UEFI device path:
    //
    //   PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x1,0xFFFF,0x0)
    //                                   ^   ^      ^
    //                                   |   |      LUN (always 0 on Q35)
    //                                   |   port multiplier port number,
    //                                   |   always 0xFFFF on Q35
    //                                   channel (port) number
    //
    UINT64  Channel;

    NumEntries = 1;
    if (RETURN_ERROR (
          ParseUnitAddressHexList (
            OfwNode[FirstNonBridge + 1].UnitAddress,
            &Channel,
            &NumEntries
            )
          ))
    {
      return RETURN_UNSUPPORTED;
    }

    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)/Sata(0x%Lx,0xFFFF,0x0)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1],
                Channel
                );
  } else if ((NumNodes >= FirstNonBridge + 3) &&
             SubstringEq (OfwNode[FirstNonBridge + 0].DriverName, "isa") &&
             SubstringEq (OfwNode[FirstNonBridge + 1].DriverName, "fdc") &&
             SubstringEq (OfwNode[FirstNonBridge + 2].DriverName, "floppy")
             )
  {
    //
    // OpenFirmware device path (floppy disk):
    //
    //   /pci@i0cf8/isa@1/fdc@03f0/floppy@0
    //        ^         ^     ^           ^
    //        |         |     |           A: or B:
    //        |         |     ISA controller io-port (hex)
    //        |         PCI slot holding ISA controller
    //        PCI root at system bus port, PIO
    //
    // UEFI device path:
    //
    //   PciRoot(0x0)/Pci(0x1,0x0)/Floppy(0x0)
    //                                    ^
    //                                    ACPI UID
    //
    UINT64  AcpiUid;

    NumEntries = 1;
    if ((ParseUnitAddressHexList (
           OfwNode[FirstNonBridge + 2].UnitAddress,
           &AcpiUid,
           &NumEntries
           ) != RETURN_SUCCESS) ||
        (AcpiUid > 1)
        )
    {
      return RETURN_UNSUPPORTED;
    }

    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)/Floppy(0x%Lx)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1],
                AcpiUid
                );
  } else if ((NumNodes >= FirstNonBridge + 2) &&
             SubstringEq (OfwNode[FirstNonBridge + 0].DriverName, "scsi") &&
             SubstringEq (OfwNode[FirstNonBridge + 1].DriverName, "disk")
             )
  {
    //
    // OpenFirmware device path (virtio-blk disk):
    //
    //   /pci@i0cf8/scsi@6[,3]/disk@0,0
    //        ^          ^  ^       ^ ^
    //        |          |  |       fixed
    //        |          |  PCI function corresponding to disk (optional)
    //        |          PCI slot holding disk
    //        PCI root at system bus port, PIO
    //
    // UEFI device path prefix:
    //
    //   PciRoot(0x0)/Pci(0x6,0x0) -- if PCI function is 0 or absent
    //   PciRoot(0x0)/Pci(0x6,0x3) -- if PCI function is present and nonzero
    //
    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1]
                );
  } else if ((NumNodes >= FirstNonBridge + 3) &&
             SubstringEq (OfwNode[FirstNonBridge + 0].DriverName, "scsi") &&
             SubstringEq (OfwNode[FirstNonBridge + 1].DriverName, "channel") &&
             SubstringEq (OfwNode[FirstNonBridge + 2].DriverName, "disk")
             )
  {
    //
    // OpenFirmware device path (virtio-scsi disk):
    //
    //   /pci@i0cf8/scsi@7[,3]/channel@0/disk@2,3
    //        ^          ^             ^      ^ ^
    //        |          |             |      | LUN
    //        |          |             |      target
    //        |          |             channel (unused, fixed 0)
    //        |          PCI slot[, function] holding SCSI controller
    //        PCI root at system bus port, PIO
    //
    // UEFI device path prefix:
    //
    //   PciRoot(0x0)/Pci(0x7,0x0)/Scsi(0x2,0x3)
    //                                        -- if PCI function is 0 or absent
    //   PciRoot(0x0)/Pci(0x7,0x3)/Scsi(0x2,0x3)
    //                                -- if PCI function is present and nonzero
    //
    UINT64  TargetLun[2];

    TargetLun[1] = 0;
    NumEntries   = ARRAY_SIZE (TargetLun);
    if (ParseUnitAddressHexList (
          OfwNode[FirstNonBridge + 2].UnitAddress,
          TargetLun,
          &NumEntries
          ) != RETURN_SUCCESS
        )
    {
      return RETURN_UNSUPPORTED;
    }

    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)/Scsi(0x%Lx,0x%Lx)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1],
                TargetLun[0],
                TargetLun[1]
                );
  } else if ((NumNodes >= FirstNonBridge + 2) &&
             SubstringEq (OfwNode[FirstNonBridge + 0].DriverName, "pci8086,5845") &&
             SubstringEq (OfwNode[FirstNonBridge + 1].DriverName, "namespace")
             )
  {
    //
    // OpenFirmware device path (NVMe device):
    //
    //   /pci@i0cf8/pci8086,5845@6[,1]/namespace@1,0
    //        ^                  ^  ^            ^ ^
    //        |                  |  |            | Extended Unique Identifier
    //        |                  |  |            | (EUI-64), big endian interp.
    //        |                  |  |            namespace ID
    //        |                  PCI slot & function holding NVMe controller
    //        PCI root at system bus port, PIO
    //
    // UEFI device path:
    //
    //   PciRoot(0x0)/Pci(0x6,0x1)/NVMe(0x1,00-00-00-00-00-00-00-00)
    //                                  ^   ^
    //                                  |   octets of the EUI-64
    //                                  |   in address order
    //                                  namespace ID
    //
    UINT64  Namespace[2];
    UINTN   RequiredEntries;
    UINT8   *Eui64;

    RequiredEntries = ARRAY_SIZE (Namespace);
    NumEntries      = RequiredEntries;
    if ((ParseUnitAddressHexList (
           OfwNode[FirstNonBridge + 1].UnitAddress,
           Namespace,
           &NumEntries
           ) != RETURN_SUCCESS) ||
        (NumEntries != RequiredEntries) ||
        (Namespace[0] == 0) ||
        (Namespace[0] >= MAX_UINT32)
        )
    {
      return RETURN_UNSUPPORTED;
    }

    Eui64   = (UINT8 *)&Namespace[1];
    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)/"
                "NVMe(0x%Lx,%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1],
                Namespace[0],
                Eui64[7],
                Eui64[6],
                Eui64[5],
                Eui64[4],
                Eui64[3],
                Eui64[2],
                Eui64[1],
                Eui64[0]
                );
  } else if ((NumNodes >= FirstNonBridge + 2) &&
             SubstringEq (OfwNode[FirstNonBridge + 0].DriverName, "usb") &&
             SubstringEq (OfwNode[FirstNonBridge + 1].DriverName, "storage"))
  {
    //
    // OpenFirmware device path (usb-storage device in XHCI port):
    //
    //   /pci@i0cf8/usb@3[,1]/storage@2/channel@0/disk@0,0
    //        ^         ^  ^          ^         ^      ^ ^
    //        |         |  |          |         fixed  fixed
    //        |         |  |          XHCI port number, 1-based
    //        |         |  PCI function corresponding to XHCI (optional)
    //        |         PCI slot holding XHCI
    //        PCI root at system bus port, PIO
    //
    // UEFI device path prefix:
    //
    //   PciRoot(0x0)/Pci(0x3,0x1)/USB(0x1,0x0)
    //                        ^        ^
    //                        |        XHCI port number in 0-based notation
    //                        0x0 if PCI function is 0, or absent from OFW
    //
    RETURN_STATUS  ParseStatus;
    UINT64         OneBasedXhciPort;

    NumEntries  = 1;
    ParseStatus = ParseUnitAddressHexList (
                    OfwNode[FirstNonBridge + 1].UnitAddress,
                    &OneBasedXhciPort,
                    &NumEntries
                    );
    if (RETURN_ERROR (ParseStatus) || (OneBasedXhciPort == 0)) {
      return RETURN_UNSUPPORTED;
    }

    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)/USB(0x%Lx,0x0)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1],
                OneBasedXhciPort - 1
                );
  } else {
    //
    // Generic OpenFirmware device path for PCI devices:
    //
    //   /pci@i0cf8/ethernet@3[,2]
    //        ^              ^
    //        |              PCI slot[, function] holding Ethernet card
    //        PCI root at system bus port, PIO
    //
    // UEFI device path prefix (dependent on presence of nonzero PCI function):
    //
    //   PciRoot(0x0)/Pci(0x3,0x0)
    //   PciRoot(0x0)/Pci(0x3,0x2)
    //
    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "PciRoot(0x%x)%s/Pci(0x%Lx,0x%Lx)",
                PciRoot,
                Bridges,
                PciDevFun[0],
                PciDevFun[1]
                );
  }

  //
  // There's no way to differentiate between "completely used up without
  // truncation" and "truncated", so treat the former as the latter, and return
  // success only for "some room left unused".
  //
  if (Written + 1 < *TranslatedSize) {
    *TranslatedSize = Written;
    return RETURN_SUCCESS;
  }

  return RETURN_BUFFER_TOO_SMALL;
}

//
// A type providing easy raw access to the base address of a virtio-mmio
// transport.
//
typedef union {
  UINT64    Uint64;
  UINT8     Raw[8];
} VIRTIO_MMIO_BASE_ADDRESS;

/**

  Translate an MMIO-like array of OpenFirmware device nodes to a UEFI device
  path fragment.

  @param[in]     OfwNode         Array of OpenFirmware device nodes to
                                 translate, constituting the beginning of an
                                 OpenFirmware device path.

  @param[in]     NumNodes        Number of elements in OfwNode.

  @param[out]    Translated      Destination array receiving the UEFI path
                                 fragment, allocated by the caller. If the
                                 return value differs from RETURN_SUCCESS, its
                                 contents is indeterminate.

  @param[in out] TranslatedSize  On input, the number of CHAR16's in
                                 Translated. On RETURN_SUCCESS this parameter
                                 is assigned the number of non-NUL CHAR16's
                                 written to Translated. In case of other return
                                 values, TranslatedSize is indeterminate.


  @retval RETURN_SUCCESS           Translation successful.

  @retval RETURN_BUFFER_TOO_SMALL  The translation does not fit into the number
                                   of bytes provided.

  @retval RETURN_UNSUPPORTED       The array of OpenFirmware device nodes can't
                                   be translated in the current implementation.

**/
STATIC
RETURN_STATUS
TranslateMmioOfwNodes (
  IN      CONST OFW_NODE  *OfwNode,
  IN      UINTN           NumNodes,
  OUT     CHAR16          *Translated,
  IN OUT  UINTN           *TranslatedSize
  )
{
  VIRTIO_MMIO_BASE_ADDRESS  VirtioMmioBase;
  CHAR16                    VenHwString[60 + 1];
  UINTN                     NumEntries;
  UINTN                     Written;

  //
  // Get the base address of the virtio-mmio transport.
  //
  if ((NumNodes < REQUIRED_MMIO_OFW_NODES) ||
      !SubstringEq (OfwNode[0].DriverName, "virtio-mmio")
      )
  {
    return RETURN_UNSUPPORTED;
  }

  NumEntries = 1;
  if (ParseUnitAddressHexList (
        OfwNode[0].UnitAddress,
        &VirtioMmioBase.Uint64,
        &NumEntries
        ) != RETURN_SUCCESS
      )
  {
    return RETURN_UNSUPPORTED;
  }

  UnicodeSPrintAsciiFormat (
    VenHwString,
    sizeof VenHwString,
    "VenHw(%g,%02X%02X%02X%02X%02X%02X%02X%02X)",
    &gVirtioMmioTransportGuid,
    VirtioMmioBase.Raw[0],
    VirtioMmioBase.Raw[1],
    VirtioMmioBase.Raw[2],
    VirtioMmioBase.Raw[3],
    VirtioMmioBase.Raw[4],
    VirtioMmioBase.Raw[5],
    VirtioMmioBase.Raw[6],
    VirtioMmioBase.Raw[7]
    );

  if ((NumNodes >= 2) &&
      SubstringEq (OfwNode[1].DriverName, "disk"))
  {
    //
    // OpenFirmware device path (virtio-blk disk):
    //
    //   /virtio-mmio@000000000a003c00/disk@0,0
    //                ^                     ^ ^
    //                |                     fixed
    //                base address of virtio-mmio register block
    //
    // UEFI device path prefix:
    //
    //   <VenHwString>
    //
    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "%s",
                VenHwString
                );
  } else if ((NumNodes >= 3) &&
             SubstringEq (OfwNode[1].DriverName, "channel") &&
             SubstringEq (OfwNode[2].DriverName, "disk"))
  {
    //
    // OpenFirmware device path (virtio-scsi disk):
    //
    //   /virtio-mmio@000000000a003a00/channel@0/disk@2,3
    //                ^                        ^      ^ ^
    //                |                        |      | LUN
    //                |                        |      target
    //                |                        channel (unused, fixed 0)
    //                base address of virtio-mmio register block
    //
    // UEFI device path prefix:
    //
    //   <VenHwString>/Scsi(0x2,0x3)
    //
    UINT64  TargetLun[2];

    TargetLun[1] = 0;
    NumEntries   = ARRAY_SIZE (TargetLun);
    if (ParseUnitAddressHexList (
          OfwNode[2].UnitAddress,
          TargetLun,
          &NumEntries
          ) != RETURN_SUCCESS
        )
    {
      return RETURN_UNSUPPORTED;
    }

    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "%s/Scsi(0x%Lx,0x%Lx)",
                VenHwString,
                TargetLun[0],
                TargetLun[1]
                );
  } else if ((NumNodes >= 2) &&
             SubstringEq (OfwNode[1].DriverName, "ethernet-phy"))
  {
    //
    // OpenFirmware device path (virtio-net NIC):
    //
    //   /virtio-mmio@000000000a003e00/ethernet-phy@0
    //                ^                             ^
    //                |                             fixed
    //                base address of virtio-mmio register block
    //
    // UEFI device path prefix:
    //
    //   <VenHwString>
    //
    Written = UnicodeSPrintAsciiFormat (
                Translated,
                *TranslatedSize * sizeof (*Translated), // BufferSize in bytes
                "%s",
                VenHwString
                );
  } else {
    return RETURN_UNSUPPORTED;
  }

  //
  // There's no way to differentiate between "completely used up without
  // truncation" and "truncated", so treat the former as the latter, and return
  // success only for "some room left unused".
  //
  if (Written + 1 < *TranslatedSize) {
    *TranslatedSize = Written;
    return RETURN_SUCCESS;
  }

  return RETURN_BUFFER_TOO_SMALL;
}

/**

  Translate an array of OpenFirmware device nodes to a UEFI device path
  fragment.

  @param[in]     OfwNode         Array of OpenFirmware device nodes to
                                 translate, constituting the beginning of an
                                 OpenFirmware device path.

  @param[in]     NumNodes        Number of elements in OfwNode.

  @param[in]     ExtraPciRoots   An EXTRA_ROOT_BUS_MAP object created with
                                 CreateExtraRootBusMap(), to be used for
                                 translating positions of extra root buses to
                                 bus numbers.

  @param[out]    Translated      Destination array receiving the UEFI path
                                 fragment, allocated by the caller. If the
                                 return value differs from RETURN_SUCCESS, its
                                 contents is indeterminate.

  @param[in out] TranslatedSize  On input, the number of CHAR16's in
                                 Translated. On RETURN_SUCCESS this parameter
                                 is assigned the number of non-NUL CHAR16's
                                 written to Translated. In case of other return
                                 values, TranslatedSize is indeterminate.


  @retval RETURN_SUCCESS           Translation successful.

  @retval RETURN_BUFFER_TOO_SMALL  The translation does not fit into the number
                                   of bytes provided.

  @retval RETURN_UNSUPPORTED       The array of OpenFirmware device nodes can't
                                   be translated in the current implementation.

  @retval RETURN_PROTOCOL_ERROR    The array of OpenFirmware device nodes has
                                   been (partially) recognized, but it contains
                                   a logic error / doesn't match system state.

**/
STATIC
RETURN_STATUS
TranslateOfwNodes (
  IN      CONST OFW_NODE            *OfwNode,
  IN      UINTN                     NumNodes,
  IN      CONST EXTRA_ROOT_BUS_MAP  *ExtraPciRoots,
  OUT     CHAR16                    *Translated,
  IN OUT  UINTN                     *TranslatedSize
  )
{
  RETURN_STATUS  Status;

  Status = RETURN_UNSUPPORTED;

  if (FeaturePcdGet (PcdQemuBootOrderPciTranslation)) {
    Status = TranslatePciOfwNodes (
               OfwNode,
               NumNodes,
               ExtraPciRoots,
               Translated,
               TranslatedSize
               );
  }

  if ((Status == RETURN_UNSUPPORTED) &&
      FeaturePcdGet (PcdQemuBootOrderMmioTranslation))
  {
    Status = TranslateMmioOfwNodes (
               OfwNode,
               NumNodes,
               Translated,
               TranslatedSize
               );
  }

  return Status;
}

/**

  Translate an OpenFirmware device path fragment to a UEFI device path
  fragment, and advance in the input string.

  @param[in out] Ptr             Address of the pointer pointing to the start
                                 of the path string. After successful
                                 translation (RETURN_SUCCESS) or at least
                                 successful parsing (RETURN_UNSUPPORTED,
                                 RETURN_BUFFER_TOO_SMALL), *Ptr is set to the
                                 byte immediately following the consumed
                                 characters. In other error cases, it points to
                                 the byte that caused the error.

  @param[in]     ExtraPciRoots   An EXTRA_ROOT_BUS_MAP object created with
                                 CreateExtraRootBusMap(), to be used for
                                 translating positions of extra root buses to
                                 bus numbers.

  @param[out]    Translated      Destination array receiving the UEFI path
                                 fragment, allocated by the caller. If the
                                 return value differs from RETURN_SUCCESS, its
                                 contents is indeterminate.

  @param[in out] TranslatedSize  On input, the number of CHAR16's in
                                 Translated. On RETURN_SUCCESS this parameter
                                 is assigned the number of non-NUL CHAR16's
                                 written to Translated. In case of other return
                                 values, TranslatedSize is indeterminate.


  @retval RETURN_SUCCESS            Translation successful.

  @retval RETURN_BUFFER_TOO_SMALL   The OpenFirmware device path was parsed
                                    successfully, but its translation did not
                                    fit into the number of bytes provided.
                                    Further calls to this function are
                                    possible.

  @retval RETURN_UNSUPPORTED        The OpenFirmware device path was parsed
                                    successfully, but it can't be translated in
                                    the current implementation. Further calls
                                    to this function are possible.

  @retval RETURN_PROTOCOL_ERROR     The OpenFirmware device path has been
                                    (partially) recognized, but it contains a
                                    logic error / doesn't match system state.
                                    Further calls to this function are
                                    possible.

  @retval RETURN_NOT_FOUND          Translation terminated. On input, *Ptr was
                                    pointing to the empty string or "HALT". On
                                    output, *Ptr points to the empty string
                                    (ie. "HALT" is consumed transparently when
                                    present).

  @retval RETURN_INVALID_PARAMETER  Parse error. This is a permanent error.

**/
STATIC
RETURN_STATUS
TranslateOfwPath (
  IN OUT  CONST CHAR8               **Ptr,
  IN      CONST EXTRA_ROOT_BUS_MAP  *ExtraPciRoots,
  OUT     CHAR16                    *Translated,
  IN OUT  UINTN                     *TranslatedSize
  )
{
  UINTN          NumNodes;
  RETURN_STATUS  Status;
  OFW_NODE       Node[EXAMINED_OFW_NODES];
  BOOLEAN        IsFinal;
  OFW_NODE       Skip;

  IsFinal  = FALSE;
  NumNodes = 0;
  if (AsciiStrCmp (*Ptr, "HALT") == 0) {
    *Ptr  += 4;
    Status = RETURN_NOT_FOUND;
  } else {
    Status = ParseOfwNode (Ptr, &Node[NumNodes], &IsFinal);
  }

  if (Status == RETURN_NOT_FOUND) {
    DEBUG ((DEBUG_VERBOSE, "%a: no more nodes\n", __func__));
    return RETURN_NOT_FOUND;
  }

  while (Status == RETURN_SUCCESS && !IsFinal) {
    ++NumNodes;
    Status = ParseOfwNode (
               Ptr,
               (NumNodes < EXAMINED_OFW_NODES) ? &Node[NumNodes] : &Skip,
               &IsFinal
               );
  }

  switch (Status) {
    case RETURN_SUCCESS:
      ++NumNodes;
      break;

    case RETURN_INVALID_PARAMETER:
      DEBUG ((DEBUG_VERBOSE, "%a: parse error\n", __func__));
      return RETURN_INVALID_PARAMETER;

    default:
      ASSERT (0);
  }

  Status = TranslateOfwNodes (
             Node,
             NumNodes < EXAMINED_OFW_NODES ? NumNodes : EXAMINED_OFW_NODES,
             ExtraPciRoots,
             Translated,
             TranslatedSize
             );
  switch (Status) {
    case RETURN_SUCCESS:
      DEBUG ((DEBUG_VERBOSE, "%a: success: \"%s\"\n", __func__, Translated));
      break;

    case RETURN_BUFFER_TOO_SMALL:
      DEBUG ((DEBUG_VERBOSE, "%a: buffer too small\n", __func__));
      break;

    case RETURN_UNSUPPORTED:
      DEBUG ((DEBUG_VERBOSE, "%a: unsupported\n", __func__));
      break;

    case RETURN_PROTOCOL_ERROR:
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: logic error / system state mismatch\n",
        __func__
        ));
      break;

    default:
      ASSERT (0);
  }

  return Status;
}

/**
  Connect devices based on the boot order retrieved from QEMU.

  Attempt to retrieve the "bootorder" fw_cfg file from QEMU. Translate the
  OpenFirmware device paths therein to UEFI device path fragments. Connect the
  devices identified by the UEFI devpath prefixes as narrowly as possible, then
  connect all their child devices, recursively.

  If this function fails, then platform BDS should fall back to
  EfiBootManagerConnectAll(), or some other method for connecting any expected
  boot devices.

  @retval RETURN_SUCCESS            The "bootorder" fw_cfg file has been
                                    parsed, and the referenced device-subtrees
                                    have been connected.

  @retval RETURN_UNSUPPORTED        QEMU's fw_cfg is not supported.

  @retval RETURN_NOT_FOUND          Empty or nonexistent "bootorder" fw_cfg
                                    file.

  @retval RETURN_INVALID_PARAMETER  Parse error in the "bootorder" fw_cfg file.

  @retval RETURN_OUT_OF_RESOURCES   Memory allocation failed.

  @return                           Error statuses propagated from underlying
                                    functions.
**/
RETURN_STATUS
EFIAPI
ConnectDevicesFromQemu (
  VOID
  )
{
  RETURN_STATUS         Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  CHAR8                 *FwCfg;
  EFI_STATUS            EfiStatus;
  EXTRA_ROOT_BUS_MAP    *ExtraPciRoots;
  CONST CHAR8           *FwCfgPtr;
  UINTN                 NumConnected;
  UINTN                 TranslatedSize;
  CHAR16                Translated[TRANSLATION_OUTPUT_SIZE];

  Status = QemuFwCfgFindFile ("bootorder", &FwCfgItem, &FwCfgSize);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize == 0) {
    return RETURN_NOT_FOUND;
  }

  FwCfg = AllocatePool (FwCfgSize);
  if (FwCfg == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, FwCfg);
  if (FwCfg[FwCfgSize - 1] != '\0') {
    Status = RETURN_INVALID_PARAMETER;
    goto FreeFwCfg;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: FwCfg:\n", __func__));
  DEBUG ((DEBUG_VERBOSE, "%a\n", FwCfg));
  DEBUG ((DEBUG_VERBOSE, "%a: FwCfg: <end>\n", __func__));

  if (FeaturePcdGet (PcdQemuBootOrderPciTranslation)) {
    EfiStatus = CreateExtraRootBusMap (&ExtraPciRoots);
    if (EFI_ERROR (EfiStatus)) {
      Status = (RETURN_STATUS)EfiStatus;
      goto FreeFwCfg;
    }
  } else {
    ExtraPciRoots = NULL;
  }

  //
  // Translate each OpenFirmware path to a UEFI devpath prefix.
  //
  FwCfgPtr       = FwCfg;
  NumConnected   = 0;
  TranslatedSize = ARRAY_SIZE (Translated);
  Status         = TranslateOfwPath (
                     &FwCfgPtr,
                     ExtraPciRoots,
                     Translated,
                     &TranslatedSize
                     );
  while (!RETURN_ERROR (Status)) {
    EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
    EFI_HANDLE                Controller;

    //
    // Convert the UEFI devpath prefix to binary representation.
    //
    ASSERT (Translated[TranslatedSize] == L'\0');
    DevicePath = ConvertTextToDevicePath (Translated);
    if (DevicePath == NULL) {
      Status = RETURN_OUT_OF_RESOURCES;
      goto FreeExtraPciRoots;
    }

    //
    // Advance along DevicePath, connecting the nodes individually, and asking
    // drivers not to produce sibling nodes. Retrieve the controller handle
    // associated with the full DevicePath -- this is the device that QEMU's
    // OFW devpath refers to.
    //
    EfiStatus = EfiBootManagerConnectDevicePath (DevicePath, &Controller);
    FreePool (DevicePath);
    if (EFI_ERROR (EfiStatus)) {
      Status = (RETURN_STATUS)EfiStatus;
      goto FreeExtraPciRoots;
    }

    //
    // Because QEMU's OFW devpaths have lesser expressive power than UEFI
    // devpaths (i.e., DevicePath is considered a prefix), connect the tree
    // rooted at Controller, recursively. If no children are produced
    // (EFI_NOT_FOUND), that's OK.
    //
    EfiStatus = gBS->ConnectController (Controller, NULL, NULL, TRUE);
    if (EFI_ERROR (EfiStatus) && (EfiStatus != EFI_NOT_FOUND)) {
      Status = (RETURN_STATUS)EfiStatus;
      goto FreeExtraPciRoots;
    }

    ++NumConnected;
    //
    // Move to the next OFW devpath.
    //
    TranslatedSize = ARRAY_SIZE (Translated);
    Status         = TranslateOfwPath (
                       &FwCfgPtr,
                       ExtraPciRoots,
                       Translated,
                       &TranslatedSize
                       );
  }

  if ((Status == RETURN_NOT_FOUND) && (NumConnected > 0)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: %Lu OpenFirmware device path(s) connected\n",
      __func__,
      (UINT64)NumConnected
      ));
    Status = RETURN_SUCCESS;
  }

FreeExtraPciRoots:
  if (ExtraPciRoots != NULL) {
    DestroyExtraRootBusMap (ExtraPciRoots);
  }

FreeFwCfg:
  FreePool (FwCfg);

  return Status;
}

/**
  Write qemu boot order to uefi variables.

  Attempt to retrieve the "bootorder" fw_cfg file from QEMU. Translate
  the OpenFirmware device paths therein to UEFI device path fragments.

  On Success store the device path in VMMBootOrderNNNN variables.
**/
VOID
EFIAPI
StoreQemuBootOrder (
  VOID
  )
{
  RETURN_STATUS         Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  CHAR8                 *FwCfg;
  EFI_STATUS            EfiStatus;
  EXTRA_ROOT_BUS_MAP    *ExtraPciRoots;
  CONST CHAR8           *FwCfgPtr;
  UINTN                 TranslatedSize;
  CHAR16                Translated[TRANSLATION_OUTPUT_SIZE];
  UINTN                 VariableIndex = 0;
  CHAR16                VariableName[20];

  Status = QemuFwCfgFindFile ("bootorder", &FwCfgItem, &FwCfgSize);
  if (RETURN_ERROR (Status)) {
    return;
  }

  if (FwCfgSize == 0) {
    return;
  }

  FwCfg = AllocatePool (FwCfgSize);
  if (FwCfg == NULL) {
    return;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, FwCfg);
  if (FwCfg[FwCfgSize - 1] != '\0') {
    Status = RETURN_INVALID_PARAMETER;
    goto FreeFwCfg;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: FwCfg:\n", __func__));
  DEBUG ((DEBUG_VERBOSE, "%a\n", FwCfg));
  DEBUG ((DEBUG_VERBOSE, "%a: FwCfg: <end>\n", __func__));

  if (FeaturePcdGet (PcdQemuBootOrderPciTranslation)) {
    EfiStatus = CreateExtraRootBusMap (&ExtraPciRoots);
    if (EFI_ERROR (EfiStatus)) {
      Status = (RETURN_STATUS)EfiStatus;
      goto FreeFwCfg;
    }
  } else {
    ExtraPciRoots = NULL;
  }

  //
  // Translate each OpenFirmware path to a UEFI devpath prefix.
  //
  FwCfgPtr       = FwCfg;
  TranslatedSize = ARRAY_SIZE (Translated);
  Status         = TranslateOfwPath (
                     &FwCfgPtr,
                     ExtraPciRoots,
                     Translated,
                     &TranslatedSize
                     );
  while (Status == EFI_SUCCESS ||
         Status == EFI_UNSUPPORTED)
  {
    if (Status == EFI_SUCCESS) {
      EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

      //
      // Convert the UEFI devpath prefix to binary representation.
      //
      ASSERT (Translated[TranslatedSize] == L'\0');
      DevicePath = ConvertTextToDevicePath (Translated);
      if (DevicePath == NULL) {
        Status = RETURN_OUT_OF_RESOURCES;
        goto FreeExtraPciRoots;
      }

      UnicodeSPrint (
        VariableName,
        sizeof (VariableName),
        L"VMMBootOrder%04x",
        VariableIndex++
        );
      DEBUG ((DEBUG_INFO, "%a: %s = %s\n", __func__, VariableName, Translated));
      gRT->SetVariable (
             VariableName,
             &gVMMBootOrderGuid,
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             GetDevicePathSize (DevicePath),
             DevicePath
             );
      FreePool (DevicePath);
    }

    //
    // Move to the next OFW devpath.
    //
    TranslatedSize = ARRAY_SIZE (Translated);
    Status         = TranslateOfwPath (
                       &FwCfgPtr,
                       ExtraPciRoots,
                       Translated,
                       &TranslatedSize
                       );
  }

FreeExtraPciRoots:
  if (ExtraPciRoots != NULL) {
    DestroyExtraRootBusMap (ExtraPciRoots);
  }

FreeFwCfg:
  FreePool (FwCfg);
}

/**

  Convert the UEFI DevicePath to full text representation with DevPathToText,
  then match the UEFI device path fragment in Translated against it.

  @param[in] Translated        UEFI device path fragment, translated from
                               OpenFirmware format, to search for.

  @param[in] TranslatedLength  The length of Translated in CHAR16's.

  @param[in] DevicePath        Boot option device path whose textual rendering
                               to search in.

  @param[in] DevPathToText  Binary-to-text conversion protocol for DevicePath.


  @retval TRUE   If Translated was found at the beginning of DevicePath after
                 converting the latter to text.

  @retval FALSE  If DevicePath was NULL, or it could not be converted, or there
                 was no match.

**/
STATIC
BOOLEAN
Match (
  IN  CONST CHAR16              *Translated,
  IN  UINTN                     TranslatedLength,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16                    *Converted;
  BOOLEAN                   Result;
  VOID                      *FileBuffer;
  UINTN                     FileSize;
  EFI_DEVICE_PATH_PROTOCOL  *AbsDevicePath;
  CHAR16                    *AbsConverted;
  BOOLEAN                   Shortform;
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  Converted = ConvertDevicePathToText (
                DevicePath,
                FALSE, // DisplayOnly
                FALSE  // AllowShortcuts
                );
  if (Converted == NULL) {
    return FALSE;
  }

  Result    = FALSE;
  Shortform = FALSE;
  //
  // Expand the short-form device path to full device path
  //
  if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MEDIA_HARDDRIVE_DP))
  {
    //
    // Harddrive shortform device path
    //
    Shortform = TRUE;
  } else if ((DevicePathType (DevicePath) == MEDIA_DEVICE_PATH) &&
             (DevicePathSubType (DevicePath) == MEDIA_FILEPATH_DP))
  {
    //
    // File-path shortform device path
    //
    Shortform = TRUE;
  } else if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
             (DevicePathSubType (DevicePath) == MSG_URI_DP))
  {
    //
    // URI shortform device path
    //
    Shortform = TRUE;
  } else {
    for ( Node = DevicePath
          ; !IsDevicePathEnd (Node)
          ; Node = NextDevicePathNode (Node)
          )
    {
      if ((DevicePathType (Node) == MESSAGING_DEVICE_PATH) &&
          ((DevicePathSubType (Node) == MSG_USB_CLASS_DP) ||
           (DevicePathSubType (Node) == MSG_USB_WWID_DP)))
      {
        Shortform = TRUE;
        break;
      }
    }
  }

  //
  // Attempt to expand any relative UEFI device path to
  // an absolute device path first.
  //
  if (Shortform) {
    FileBuffer = EfiBootManagerGetLoadOptionBuffer (
                   DevicePath,
                   &AbsDevicePath,
                   &FileSize
                   );
    if (FileBuffer == NULL) {
      goto Exit;
    }

    FreePool (FileBuffer);
    AbsConverted = ConvertDevicePathToText (AbsDevicePath, FALSE, FALSE);
    FreePool (AbsDevicePath);
    if (AbsConverted == NULL) {
      goto Exit;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: expanded relative device path \"%s\" for prefix matching\n",
      __func__,
      Converted
      ));
    FreePool (Converted);
    Converted = AbsConverted;
  }

  //
  // Is Translated a prefix of Converted?
  //
  Result = (BOOLEAN)(StrnCmp (Converted, Translated, TranslatedLength) == 0);
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: against \"%s\": %a\n",
    __func__,
    Converted,
    Result ? "match" : "no match"
    ));
Exit:
  FreePool (Converted);
  return Result;
}

/**
  Append some of the unselected active boot options to the boot order.

  This function should accommodate any further policy changes in "boot option
  survival". Currently we're adding back everything that starts with neither
  PciRoot() nor HD() nor a virtio-mmio VenHw() node.

  @param[in,out] BootOrder     The structure holding the boot order to
                               complete. The caller is responsible for
                               initializing (and potentially populating) it
                               before calling this function.

  @param[in,out] ActiveOption  The array of active boot options to scan.
                               Entries marked as Appended will be skipped.
                               Those of the rest that satisfy the survival
                               policy will be added to BootOrder with
                               BootOrderAppend().

  @param[in]     ActiveCount   Number of elements in ActiveOption.


  @retval RETURN_SUCCESS  BootOrder has been extended with any eligible boot
                          options.

  @return                 Error codes returned by BootOrderAppend().
**/
STATIC
RETURN_STATUS
BootOrderComplete (
  IN OUT  BOOT_ORDER     *BootOrder,
  IN OUT  ACTIVE_OPTION  *ActiveOption,
  IN      UINTN          ActiveCount
  )
{
  RETURN_STATUS  Status;
  UINTN          Idx;

  Status = RETURN_SUCCESS;
  Idx    = 0;
  while (!RETURN_ERROR (Status) && Idx < ActiveCount) {
    if (!ActiveOption[Idx].Appended) {
      CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Current;
      CONST EFI_DEVICE_PATH_PROTOCOL      *FirstNode;

      Current   = ActiveOption[Idx].BootOption;
      FirstNode = Current->FilePath;
      if (FirstNode != NULL) {
        CHAR16         *Converted;
        STATIC CHAR16  ConvFallBack[] = L"<unable to convert>";
        BOOLEAN        Keep;

        Converted = ConvertDevicePathToText (FirstNode, FALSE, FALSE);
        if (Converted == NULL) {
          Converted = ConvFallBack;
        }

        Keep = TRUE;
        if ((DevicePathType (FirstNode) == MEDIA_DEVICE_PATH) &&
            (DevicePathSubType (FirstNode) == MEDIA_HARDDRIVE_DP))
        {
          //
          // drop HD()
          //
          Keep = FALSE;
        } else if ((DevicePathType (FirstNode) == ACPI_DEVICE_PATH) &&
                   (DevicePathSubType (FirstNode) == ACPI_DP))
        {
          ACPI_HID_DEVICE_PATH  *Acpi;

          Acpi = (ACPI_HID_DEVICE_PATH *)FirstNode;
          if (((Acpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) &&
              (EISA_ID_TO_NUM (Acpi->HID) == 0x0a03))
          {
            //
            // drop PciRoot() if we enabled the user to select PCI-like boot
            // options, by providing translation for such OFW device path
            // fragments
            //
            Keep = !FeaturePcdGet (PcdQemuBootOrderPciTranslation);
          }
        } else if ((DevicePathType (FirstNode) == HARDWARE_DEVICE_PATH) &&
                   (DevicePathSubType (FirstNode) == HW_VENDOR_DP))
        {
          VENDOR_DEVICE_PATH  *VenHw;

          VenHw = (VENDOR_DEVICE_PATH *)FirstNode;
          if (CompareGuid (&VenHw->Guid, &gVirtioMmioTransportGuid)) {
            //
            // drop virtio-mmio if we enabled the user to select boot options
            // referencing such device paths
            //
            Keep = !FeaturePcdGet (PcdQemuBootOrderMmioTranslation);
          }
        }

        if (Keep) {
          Status = BootOrderAppend (BootOrder, &ActiveOption[Idx]);
          if (!RETURN_ERROR (Status)) {
            DEBUG ((
              DEBUG_VERBOSE,
              "%a: keeping \"%s\"\n",
              __func__,
              Converted
              ));
          }
        } else {
          DEBUG ((
            DEBUG_VERBOSE,
            "%a: dropping \"%s\"\n",
            __func__,
            Converted
            ));
        }

        if (Converted != ConvFallBack) {
          FreePool (Converted);
        }
      }
    }

    ++Idx;
  }

  return Status;
}

/**
  Delete Boot#### variables that stand for such active boot options that have
  been dropped (ie. have not been selected by either matching or "survival
  policy").

  @param[in]  ActiveOption  The array of active boot options to scan. Each
                            entry not marked as appended will trigger the
                            deletion of the matching Boot#### variable.

  @param[in]  ActiveCount   Number of elements in ActiveOption.
**/
STATIC
VOID
PruneBootVariables (
  IN  CONST ACTIVE_OPTION  *ActiveOption,
  IN  UINTN                ActiveCount
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ActiveCount; ++Idx) {
    if (!ActiveOption[Idx].Appended) {
      CHAR16  VariableName[9];

      UnicodeSPrintAsciiFormat (
        VariableName,
        sizeof VariableName,
        "Boot%04x",
        ActiveOption[Idx].BootOption->OptionNumber
        );

      //
      // "The space consumed by the deleted variable may not be available until
      // the next power cycle", but that's good enough.
      //
      gRT->SetVariable (
             VariableName,
             &gEfiGlobalVariableGuid,
             0,   // Attributes, 0 means deletion
             0,   // DataSize, 0 means deletion
             NULL // Data
             );
    }
  }
}

/**

  Set the boot order based on configuration retrieved from QEMU.

  Attempt to retrieve the "bootorder" fw_cfg file from QEMU. Translate the
  OpenFirmware device paths therein to UEFI device path fragments. Match the
  translated fragments against the current list of boot options, and rewrite
  the BootOrder NvVar so that it corresponds to the order described in fw_cfg.

  Platform BDS should call this function after connecting any expected boot
  devices and calling EfiBootManagerRefreshAllBootOption ().

  @retval RETURN_SUCCESS            BootOrder NvVar rewritten.

  @retval RETURN_UNSUPPORTED        QEMU's fw_cfg is not supported.

  @retval RETURN_NOT_FOUND          Empty or nonexistent "bootorder" fw_cfg
                                    file, or no match found between the
                                    "bootorder" fw_cfg file and BootOptionList.

  @retval RETURN_INVALID_PARAMETER  Parse error in the "bootorder" fw_cfg file.

  @retval RETURN_OUT_OF_RESOURCES   Memory allocation failed.

  @return                           Values returned by gBS->LocateProtocol ()
                                    or gRT->SetVariable ().

**/
RETURN_STATUS
EFIAPI
SetBootOrderFromQemu (
  VOID
  )
{
  RETURN_STATUS         Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  CHAR8                 *FwCfg;
  CONST CHAR8           *FwCfgPtr;

  BOOT_ORDER     BootOrder;
  ACTIVE_OPTION  *ActiveOption;
  UINTN          ActiveCount;

  EXTRA_ROOT_BUS_MAP  *ExtraPciRoots;

  UINTN                         TranslatedSize;
  CHAR16                        Translated[TRANSLATION_OUTPUT_SIZE];
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions;
  UINTN                         BootOptionCount;

  Status = QemuFwCfgFindFile ("bootorder", &FwCfgItem, &FwCfgSize);
  if (Status != RETURN_SUCCESS) {
    return Status;
  }

  if (FwCfgSize == 0) {
    return RETURN_NOT_FOUND;
  }

  FwCfg = AllocatePool (FwCfgSize);
  if (FwCfg == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, FwCfg);
  if (FwCfg[FwCfgSize - 1] != '\0') {
    Status = RETURN_INVALID_PARAMETER;
    goto ErrorFreeFwCfg;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: FwCfg:\n", __func__));
  DEBUG ((DEBUG_VERBOSE, "%a\n", FwCfg));
  DEBUG ((DEBUG_VERBOSE, "%a: FwCfg: <end>\n", __func__));
  FwCfgPtr = FwCfg;

  BootOrder.Produced  = 0;
  BootOrder.Allocated = 1;
  BootOrder.Data      = AllocatePool (
                          BootOrder.Allocated * sizeof (*BootOrder.Data)
                          );
  if (BootOrder.Data == NULL) {
    Status = RETURN_OUT_OF_RESOURCES;
    goto ErrorFreeFwCfg;
  }

  BootOptions = EfiBootManagerGetLoadOptions (
                  &BootOptionCount,
                  LoadOptionTypeBoot
                  );
  if (BootOptions == NULL) {
    Status = RETURN_NOT_FOUND;
    goto ErrorFreeBootOrder;
  }

  Status = CollectActiveOptions (
             BootOptions,
             BootOptionCount,
             &ActiveOption,
             &ActiveCount
             );
  if (RETURN_ERROR (Status)) {
    goto ErrorFreeBootOptions;
  }

  if (FeaturePcdGet (PcdQemuBootOrderPciTranslation)) {
    Status = CreateExtraRootBusMap (&ExtraPciRoots);
    if (EFI_ERROR (Status)) {
      goto ErrorFreeActiveOption;
    }
  } else {
    ExtraPciRoots = NULL;
  }

  //
  // translate each OpenFirmware path
  //
  TranslatedSize = ARRAY_SIZE (Translated);
  Status         = TranslateOfwPath (
                     &FwCfgPtr,
                     ExtraPciRoots,
                     Translated,
                     &TranslatedSize
                     );
  while (Status == RETURN_SUCCESS ||
         Status == RETURN_UNSUPPORTED ||
         Status == RETURN_PROTOCOL_ERROR ||
         Status == RETURN_BUFFER_TOO_SMALL)
  {
    if (Status == RETURN_SUCCESS) {
      UINTN  Idx;

      //
      // match translated OpenFirmware path against all active boot options
      //
      for (Idx = 0; Idx < ActiveCount; ++Idx) {
        if (!ActiveOption[Idx].Appended &&
            Match (
              Translated,
              TranslatedSize, // contains length, not size, in CHAR16's here
              ActiveOption[Idx].BootOption->FilePath
              )
            )
        {
          //
          // match found, store ID and continue with next OpenFirmware path
          //
          Status = BootOrderAppend (&BootOrder, &ActiveOption[Idx]);
          if (Status != RETURN_SUCCESS) {
            goto ErrorFreeExtraPciRoots;
          }
        }
      } // scanned all active boot options
    }   // translation successful

    TranslatedSize = ARRAY_SIZE (Translated);
    Status         = TranslateOfwPath (
                       &FwCfgPtr,
                       ExtraPciRoots,
                       Translated,
                       &TranslatedSize
                       );
  } // scanning of OpenFirmware paths done

  if ((Status == RETURN_NOT_FOUND) && (BootOrder.Produced > 0)) {
    //
    // No more OpenFirmware paths, some matches found: rewrite BootOrder NvVar.
    // Some of the active boot options that have not been selected over fw_cfg
    // should be preserved at the end of the boot order.
    //
    Status = BootOrderComplete (&BootOrder, ActiveOption, ActiveCount);
    if (RETURN_ERROR (Status)) {
      goto ErrorFreeExtraPciRoots;
    }

    //
    // See Table 10 in the UEFI Spec 2.3.1 with Errata C for the required
    // attributes.
    //
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS |
                    EFI_VARIABLE_RUNTIME_ACCESS,
                    BootOrder.Produced * sizeof (*BootOrder.Data),
                    BootOrder.Data
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: setting BootOrder: %r\n",
        __func__,
        Status
        ));
      goto ErrorFreeExtraPciRoots;
    }

    DEBUG ((DEBUG_INFO, "%a: setting BootOrder: success\n", __func__));
    PruneBootVariables (ActiveOption, ActiveCount);
  }

ErrorFreeExtraPciRoots:
  if (ExtraPciRoots != NULL) {
    DestroyExtraRootBusMap (ExtraPciRoots);
  }

ErrorFreeActiveOption:
  FreePool (ActiveOption);

ErrorFreeBootOptions:
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);

ErrorFreeBootOrder:
  FreePool (BootOrder.Data);

ErrorFreeFwCfg:
  FreePool (FwCfg);

  return Status;
}

/**
  Calculate the number of seconds we should be showing the FrontPage progress
  bar for.

  @return  The TimeoutDefault argument for PlatformBdsEnterFrontPage().
**/
UINT16
EFIAPI
GetFrontPageTimeoutFromQemu (
  VOID
  )
{
  FIRMWARE_CONFIG_ITEM  BootMenuWaitItem;
  UINTN                 BootMenuWaitSize;
  UINT16                Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  if (!QemuFwCfgIsAvailable ()) {
    return Timeout;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemBootMenu);
  if (QemuFwCfgRead16 () == 0) {
    //
    // The user specified "-boot menu=off", or didn't specify "-boot
    // menu=(on|off)" at all. Return the platform default.
    //
    return PcdGet16 (PcdPlatformBootTimeOut);
  }

  if (RETURN_ERROR (
        QemuFwCfgFindFile (
          "etc/boot-menu-wait",
          &BootMenuWaitItem,
          &BootMenuWaitSize
          )
        ) ||
      (BootMenuWaitSize != sizeof (UINT16)))
  {
    //
    // "-boot menu=on" was specified without "splash-time=N". In this case,
    // return three seconds if the platform default would cause us to skip the
    // front page, and return the platform default otherwise.
    //
    if (Timeout == 0) {
      Timeout = 3;
    }

    return Timeout;
  }

  //
  // "-boot menu=on,splash-time=N" was specified, where N is in units of
  // milliseconds. The Intel BDS Front Page progress bar only supports whole
  // seconds, round N up.
  //
  QemuFwCfgSelectItem (BootMenuWaitItem);
  return (UINT16)((QemuFwCfgRead16 () + 999) / 1000);
}
