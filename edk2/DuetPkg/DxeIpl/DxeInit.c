/** @file

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  DxeInit.c

Abstract:

Revision History:

**/

#include "DxeIpl.h"

#include "LegacyTable.h"
#include "HobGeneration.h"
#include "PpisNeededByDxeCore.h"
#include "Debug.h"

/*
--------------------------------------------------------
 Memory Map: (XX=32,64)
--------------------------------------------------------
0x0
        IVT
0x400
        BDA
0x500

0x7C00
        BootSector
0x10000
        EfiLdr (relocate by efiXX.COM)
0x15000
        Efivar.bin (Load by StartXX.COM)
0x20000
        StartXX.COM (E820 table, Temporary GDT, Temporary IDT)
0x21000
        EfiXX.COM (Temporary Interrupt Handler)
0x22000
        EfiLdr.efi + DxeIpl.Z + DxeMain.Z + BFV.Z
0x86000
        MemoryFreeUnder1M (For legacy driver DMA)
0x90000
        Temporary 4G PageTable for X64 (6 page)
0x9F800
        EBDA
0xA0000
        VGA
0xC0000
        OPROM
0xE0000
        FIRMEWARE
0x100000 (1M)
        Temporary Stack (1M)
0x200000

MemoryAbove1MB.PhysicalStart <-----------------------------------------------------+
        ...                                                                        |
        ...                                                                        |
                        <- Phit.EfiMemoryBottom -------------------+               |
        HOB                                                        |               |
                        <- Phit.EfiFreeMemoryBottom                |               |
                                                                   |     MemoryFreeAbove1MB.ResourceLength
                        <- Phit.EfiFreeMemoryTop ------+           |               |
        MemoryDescriptor (For ACPINVS, ACPIReclaim)    |    4M = CONSUMED_MEMORY   |
                                                       |           |               |
        Permament 4G PageTable for IA32 or      MemoryAllocation   |               |
        Permament 64G PageTable for X64                |           |               |
                        <------------------------------+           |               |
        Permament Stack (0x20 Pages = 128K)                        |               |
                        <- Phit.EfiMemoryTop ----------+-----------+---------------+
        DxeCore                                                                    |
                                                                                DxeCore
        DxeIpl                                                                     |
                        <----------------------------------------------------------+
        NvFV + FtwFV                                                               |
                                                                                 MMIO
        BFV                                                                        |
                        <- Top of Free Memory reported by E820 --------------------+
        ACPINVS        or
        ACPIReclaim    or
        Reserved
                        <- Memory Top on RealMemory

0x100000000 (4G)

MemoryFreeAbove4G.Physicalstart <--------------------------------------------------+
                                                                                   |
                                                                                   |
                                                                  MemoryFreeAbove4GB.ResourceLength
                                                                                   |
                                                                                   |
                                <--------------------------------------------------+
*/

VOID
EnterDxeMain (
  IN VOID *StackTop,
  IN VOID *DxeCoreEntryPoint,
  IN VOID *Hob,
  IN VOID *PageTable
  );

VOID
DxeInit (
  IN EFILDRHANDOFF  *Handoff
  )
/*++

  Routine Description:

    This is the entry point after this code has been loaded into memory. 

Arguments:


Returns:

    Calls into EFI Firmware

--*/
{
  VOID                  *StackTop;
  VOID                  *StackBottom;
  VOID                  *PageTableBase;
  VOID                  *MemoryTopOnDescriptor;
  VOID                  *MemoryDescriptor;
  VOID                  *NvStorageBase;
  CHAR8                 PrintBuffer[256];
  
  ClearScreen();
  PrintString("Enter DxeIpl ...\n");
  
/*
  ClearScreen();
  PrintString("handoff:\n");
  PrintString("Handoff.BfvBase = ");   
  PrintValue64((UINT64)(UINTN)Handoff->BfvBase);
  PrintString(", ");   
  PrintString("BfvLength = ");   
  PrintValue64(Handoff->BfvSize);
  PrintString("\n");   
  PrintString("Handoff.DxeIplImageBase = ");   
  PrintValue64((UINT64)(UINTN)Handoff->DxeIplImageBase);
  PrintString(", ");   
  PrintString("DxeIplImageSize = ");   
  PrintValue64(Handoff->DxeIplImageSize);
  PrintString("\n");   
  PrintString("Handoff.DxeCoreImageBase = ");   
  PrintValue64((UINT64)(UINTN)Handoff->DxeCoreImageBase);
  PrintString(", ");   
  PrintString("DxeCoreImageSize = ");   
  PrintValue64(Handoff->DxeCoreImageSize);
  PrintString("\n");   
*/
  //
  // Hob Generation Guild line:
  //   * Don't report FV as physical memory
  //   * MemoryAllocation Hob should only cover physical memory
  //   * Use ResourceDescriptor Hob to report physical memory or Firmware Device and they shouldn't be overlapped
  PrintString("Prepare Cpu HOB information ...\n");
  PrepareHobCpu ();

  //
  // 1. BFV
  //
  PrintString("Prepare BFV HOB information ...\n");
  PrepareHobBfv (Handoff->BfvBase, Handoff->BfvSize);

  //
  // 2. Updates Memory information, and get the top free address under 4GB
  //
  PrintString("Prepare Memory HOB information ...\n");
  MemoryTopOnDescriptor = PrepareHobMemory (Handoff->MemDescCount, Handoff->MemDesc);
  
  //
  // 3. Put [NV], [Stack], [PageTable], [MemDesc], [HOB] just below the [top free address under 4GB]
  //
  
  //   3.1 NV data
  PrintString("Prepare NV Storage information ...\n");
  NvStorageBase = PrepareHobNvStorage (MemoryTopOnDescriptor);
  AsciiSPrint (PrintBuffer, 256, "NV Storage Base=0x%x\n", (UINTN)NvStorageBase);
  PrintString (PrintBuffer);
  
  //   3.2 Stack
  StackTop = NvStorageBase;
  StackBottom = PrepareHobStack (StackTop);
  AsciiSPrint (PrintBuffer, 256, "Stack Top=0x%x, Stack Bottom=0x%x\n", 
              (UINTN)StackTop, (UINTN)StackBottom);
  PrintString (PrintBuffer);
  //   3.3 Page Table
  PageTableBase = PreparePageTable (StackBottom, gHob->Cpu.SizeOfMemorySpace);
  //   3.4 MemDesc (will be used in PlatformBds)
  MemoryDescriptor = PrepareHobMemoryDescriptor (PageTableBase, Handoff->MemDescCount, Handoff->MemDesc);
  //   3.5 Copy the Hob itself to EfiMemoryBottom, and update the PHIT Hob
  PrepareHobPhit (StackTop, MemoryDescriptor);

  //
  // 4. Register the memory occupied by DxeCore and DxeIpl together as DxeCore
  //
  PrintString("Prepare DxeCore memory Hob ...\n");
  PrepareHobDxeCore (
    Handoff->DxeCoreEntryPoint,
    (EFI_PHYSICAL_ADDRESS)(UINTN)Handoff->DxeCoreImageBase,
    (UINTN)Handoff->DxeIplImageBase + (UINTN)Handoff->DxeIplImageSize - (UINTN)Handoff->DxeCoreImageBase
    );

  PrepareHobLegacyTable (gHob);
  
  PreparePpisNeededByDxeCore (gHob);

  CompleteHobGeneration ();

  AsciiSPrint (PrintBuffer, 256, "HobStart=0x%x\n", (UINTN)gHob);
  PrintString (PrintBuffer);

  AsciiSPrint (PrintBuffer, 256, "Memory Top=0x%x, Bottom=0x%x\n", 
              (UINTN)gHob->Phit.EfiMemoryTop, (UINTN)gHob->Phit.EfiMemoryBottom);
  PrintString (PrintBuffer);

  AsciiSPrint (PrintBuffer, 256, "Free Memory Top=0x%x, Bottom=0x%x\n", 
              (UINTN)gHob->Phit.EfiFreeMemoryTop, (UINTN)gHob->Phit.EfiFreeMemoryBottom);
  PrintString (PrintBuffer);

  AsciiSPrint (PrintBuffer, 256, "Nv Base=0x%x, Length=0x%x\n", 
              (UINTN)gHob->NvStorageFvb.FvbInfo.Entries[0].Base, 
              (UINTN)gHob->NvFtwFvb.FvbInfo.Entries[0].Length);
  PrintString (PrintBuffer);
/*
  //
  // Print Hob Info
  //
  ClearScreen();
  PrintString("Hob Info\n");
  PrintString("Phit.EfiMemoryTop = ");   
  PrintValue64(gHob->Phit.EfiMemoryTop);
  PrintString(" Phit.EfiMemoryBottom = ");   
  PrintValue64(gHob->Phit.EfiMemoryBottom);
  PrintString("\n");   
  PrintString("Phit.EfiFreeMemoryTop = ");   
  PrintValue64(gHob->Phit.EfiFreeMemoryTop);
  PrintString(" Phit.EfiFreeMemoryBottom = ");   
  PrintValue64(gHob->Phit.EfiFreeMemoryBottom);
  PrintString("\n");   
  PrintString("Bfv = ");   
  PrintValue64(gHob->Bfv.BaseAddress);
  PrintString(" BfvLength = ");   
  PrintValue64(gHob->Bfv.Length);
  PrintString("\n");
  PrintString("NvStorageFvb = ");
  PrintValue64(gHob->NvStorageFvb.FvbInfo.Entries[0].Base);
  PrintString(" Length = ");
  PrintValue64(gHob->NvStorageFvb.FvbInfo.Entries[0].Length);
  PrintString("\n");
  PrintString("NvFtwFvb = ");
  PrintValue64(gHob->NvFtwFvb.FvbInfo.Entries[0].Base);
  PrintString(" Length = ");
  PrintValue64(gHob->NvFtwFvb.FvbInfo.Entries[0].Length);
  PrintString("\n");
  PrintString("Stack = ");   
  PrintValue64(gHob->Stack.AllocDescriptor.MemoryBaseAddress);
  PrintString(" StackLength = ");   
  PrintValue64(gHob->Stack.AllocDescriptor.MemoryLength);
  PrintString("\n");   
  PrintString("MemoryFreeUnder1MB = ");   
  PrintValue64(gHob->MemoryFreeUnder1MB.PhysicalStart);
  PrintString(" MemoryFreeUnder1MBLength = ");   
  PrintValue64(gHob->MemoryFreeUnder1MB.ResourceLength);
  PrintString("\n");   
  PrintString("MemoryAbove1MB = ");   
  PrintValue64(gHob->MemoryAbove1MB.PhysicalStart);
  PrintString(" MemoryAbove1MBLength = ");   
  PrintValue64(gHob->MemoryAbove1MB.ResourceLength);
  PrintString("\n");   
  PrintString("MemoryAbove4GB = ");   
  PrintValue64(gHob->MemoryAbove4GB.PhysicalStart);
  PrintString(" MemoryAbove4GBLength = ");   
  PrintValue64(gHob->MemoryAbove4GB.ResourceLength);
  PrintString("\n");   
  PrintString("DxeCore = ");   
  PrintValue64(gHob->DxeCore.MemoryAllocationHeader.MemoryBaseAddress);
  PrintString(" DxeCoreLength = ");   
  PrintValue64(gHob->DxeCore.MemoryAllocationHeader.MemoryLength);
  PrintString("\n");   
  PrintString("MemoryAllocation = ");   
  PrintValue64(gHob->MemoryAllocation.AllocDescriptor.MemoryBaseAddress);
  PrintString(" MemoryLength = ");   
  PrintValue64(gHob->MemoryAllocation.AllocDescriptor.MemoryLength);
  PrintString("\n");   
  EFI_DEADLOOP();
*/

  ClearScreen();
  PrintString("\n\n\n\n\n\n\n\n\n\n");
  PrintString("                         WELCOME TO EFI WORLD!\n");
  
  EnterDxeMain (StackTop, Handoff->DxeCoreEntryPoint, gHob, PageTableBase);
 
  PrintString("Fail to enter DXE main!\n");
  //
  // Should never get here
  //
  CpuDeadLoop ();
}

EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFILDRHANDOFF  *Handoff
  )
{
  DxeInit(Handoff);
  return EFI_SUCCESS;
}
