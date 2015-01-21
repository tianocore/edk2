/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  EfiRegTableLib.c

Abstract:

  Lib function for table driven register initialization.

Revision History

--*/

#include <Library/EfiRegTableLib.h>
#include <Library/S3BootScriptLib.h>

//
// Local Functions
//

/**
  Local worker function to process PCI_WRITE table entries.  Performs write and
  may also call BootScriptSave protocol if indicated in the Entry flags

  @param Entry            A pointer to the PCI_WRITE entry to process

  @param PciRootBridgeIo  A pointer to the instance of PciRootBridgeIo that is used
                          when processing the entry.

  @retval Nothing.

**/
STATIC
VOID
PciWrite (
  EFI_REG_TABLE_PCI_WRITE             *Entry,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL     *PciRootBridgeIo
  )
{
  EFI_STATUS  Status;

  Status = PciRootBridgeIo->Pci.Write (
                                  PciRootBridgeIo,
                                  (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
                                  (UINT64) Entry->PciAddress,
                                  1,
                                  &Entry->Data
                                  );
  ASSERT_EFI_ERROR (Status);

  if (OPCODE_FLAGS (Entry->OpCode) & OPCODE_FLAG_S3SAVE) {
    Status = S3BootScriptSavePciCfgWrite (
              (EFI_BOOT_SCRIPT_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
              (UINT64) Entry->PciAddress,
              1,
              &Entry->Data
              );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Local worker function to process PCI_READ_MODIFY_WRITE table entries.
  Performs RMW write and may also call BootScriptSave protocol if indicated in
  the Entry flags.

  @param Entry            A pointer to the PCI_READ_MODIFY_WRITE entry to process.

  @param PciRootBridgeIo  A pointer to the instance of PciRootBridgeIo that is used
                          when processing the entry.

  @retval Nothing.

**/
STATIC
VOID
PciReadModifyWrite (
  EFI_REG_TABLE_PCI_READ_MODIFY_WRITE *Entry,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL     *PciRootBridgeIo
  )
{
  EFI_STATUS  Status;
  UINT32      TempData;

  Status = PciRootBridgeIo->Pci.Read (
                                  PciRootBridgeIo,
                                  (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
                                  (UINT64) Entry->PciAddress,
                                  1,
                                  &TempData
                                  );
  ASSERT_EFI_ERROR (Status);

  Entry->OrMask &= Entry->AndMask;
  TempData &= ~Entry->AndMask;
  TempData |= Entry->OrMask;

  Status = PciRootBridgeIo->Pci.Write (
                                  PciRootBridgeIo,
                                  (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
                                  (UINT64) Entry->PciAddress,
                                  1,
                                  &TempData
                                  );
  ASSERT_EFI_ERROR (Status);

  if (OPCODE_FLAGS (Entry->OpCode) & OPCODE_FLAG_S3SAVE) {
    Status = S3BootScriptSavePciCfgReadWrite (
              (EFI_BOOT_SCRIPT_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
              (UINT64) Entry->PciAddress,
              &Entry->OrMask,
              &Entry->AndMask
              );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Local worker function to process MEM_READ_MODIFY_WRITE table entries.
  Performs RMW write and may also call BootScriptSave protocol if indicated in
  the Entry flags.

  @param Entry            A pointer to the MEM_READ_MODIFY_WRITE entry to process.

  @param PciRootBridgeIo  A pointer to the instance of PciRootBridgeIo that is used
                          when processing the entry.

  @retval Nothing.

**/
STATIC
VOID
MemReadModifyWrite (
  EFI_REG_TABLE_MEM_READ_MODIFY_WRITE *Entry,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL     *PciRootBridgeIo
  )
{
  EFI_STATUS  Status;
  UINT32      TempData;

  Status = PciRootBridgeIo->Mem.Read (
                                  PciRootBridgeIo,
                                  (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
                                  (UINT64) Entry->MemAddress,
                                  1,
                                  &TempData
                                  );
  ASSERT_EFI_ERROR (Status);

  Entry->OrMask &= Entry->AndMask;
  TempData &= ~Entry->AndMask;
  TempData |= Entry->OrMask;

  Status = PciRootBridgeIo->Mem.Write (
                                  PciRootBridgeIo,
                                  (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
                                  (UINT64) Entry->MemAddress,
                                  1,
                                  &TempData
                                  );
  ASSERT_EFI_ERROR (Status);

  if (OPCODE_FLAGS (Entry->OpCode) & OPCODE_FLAG_S3SAVE) {
    Status = S3BootScriptSaveMemReadWrite (
              (EFI_BOOT_SCRIPT_WIDTH) (OPCODE_EXTRA_DATA (Entry->OpCode)),
              Entry->MemAddress,
              &Entry->OrMask,
              &Entry->AndMask
              );
    ASSERT_EFI_ERROR (Status);
  }
}

//
// Exported functions
//

/**
  Processes register table assuming which may contain PCI, IO, MEM, and STALL
  entries.

  No parameter checking is done so the caller must be careful about omitting
  values for PciRootBridgeIo or CpuIo parameters.  If the regtable does
  not contain any PCI accesses, it is safe to omit the PciRootBridgeIo (supply
  NULL).  If the regtable does not contain any IO or Mem entries, it is safe to
  omit the CpuIo (supply NULL).

  The RegTableEntry parameter is not checked, but is required.

  gBS is assumed to have been defined and is used when processing stalls.

  The function processes each entry sequentially until an OP_TERMINATE_TABLE
  entry is encountered.

  @param RegTableEntry    A pointer to the register table to process

  @param PciRootBridgeIo  A pointer to the instance of PciRootBridgeIo that is used
                          when processing PCI table entries

  @param CpuIo            A pointer to the instance of CpuIo that is used when processing IO and
                          MEM table entries

  @retval Nothing.

**/
VOID
ProcessRegTablePci (
  EFI_REG_TABLE                       *RegTableEntry,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL     *PciRootBridgeIo,
  EFI_CPU_IO_PROTOCOL                 *CpuIo
  )
{
  while (OPCODE_BASE (RegTableEntry->Generic.OpCode) != OP_TERMINATE_TABLE) {
    switch (OPCODE_BASE (RegTableEntry->Generic.OpCode)) {
    case OP_PCI_WRITE:
      PciWrite ((EFI_REG_TABLE_PCI_WRITE *) RegTableEntry, PciRootBridgeIo);
      break;

    case OP_PCI_READ_MODIFY_WRITE:
      PciReadModifyWrite ((EFI_REG_TABLE_PCI_READ_MODIFY_WRITE *) RegTableEntry, PciRootBridgeIo);
      break;

    case OP_MEM_READ_MODIFY_WRITE:
      MemReadModifyWrite ((EFI_REG_TABLE_MEM_READ_MODIFY_WRITE *) RegTableEntry, PciRootBridgeIo);
      break;

    default:
      DEBUG ((EFI_D_ERROR, "RegTable ERROR: Unknown RegTable OpCode (%x)\n", OPCODE_BASE (RegTableEntry->Generic.OpCode)));
      ASSERT (0);
      break;
    }

    RegTableEntry++;
  }
}

/**
  Processes register table assuming which may contain IO, MEM, and STALL
  entries, but must NOT contain any PCI entries.  Any PCI entries cause an
  ASSERT in a DEBUG build and are skipped in a free build.

  No parameter checking is done.  Both RegTableEntry and CpuIo parameters are
  required.

  gBS is assumed to have been defined and is used when processing stalls.

  The function processes each entry sequentially until an OP_TERMINATE_TABLE
  entry is encountered.

  @param  RegTableEntry   A pointer to the register table to process

  @param  CpuIo           A pointer to the instance of CpuIo that is used when processing IO and
                          MEM table entries

  @retval Nothing.

**/
VOID
ProcessRegTableCpu (
  EFI_REG_TABLE                       *RegTableEntry,
  EFI_CPU_IO_PROTOCOL                 *CpuIo
  )
{
  while (OPCODE_BASE (RegTableEntry->Generic.OpCode) != OP_TERMINATE_TABLE) {
    switch (OPCODE_BASE (RegTableEntry->Generic.OpCode)) {
    default:
      DEBUG ((EFI_D_ERROR, "RegTable ERROR: Unknown RegTable OpCode (%x)\n", OPCODE_BASE (RegTableEntry->Generic.OpCode)));
      ASSERT (0);
      break;
    }

    RegTableEntry++;
  }
}
