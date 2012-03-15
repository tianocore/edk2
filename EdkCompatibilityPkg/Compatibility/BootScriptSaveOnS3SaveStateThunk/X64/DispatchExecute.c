/** @file
  Execute 32-bit code in Long Mode
  Provide a thunk function to transition from long mode to compatibility mode to execute 32-bit code and then transit
  back to long mode.
  
  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "ScriptSave.h"

#pragma pack(1)
typedef union {
  struct {
    UINT32  LimitLow    : 16;
    UINT32  BaseLow     : 16;
    UINT32  BaseMid     : 8;
    UINT32  Type        : 4;
    UINT32  System      : 1;
    UINT32  Dpl         : 2;
    UINT32  Present     : 1;
    UINT32  LimitHigh   : 4;
    UINT32  Software    : 1;
    UINT32  Reserved    : 1;
    UINT32  DefaultSize : 1;
    UINT32  Granularity : 1;
    UINT32  BaseHigh    : 8;
  } Bits;
  UINT64  Uint64;
} IA32_GDT;

///
/// Byte packed structure for an IA-32 Interrupt Gate Descriptor.
///
typedef union {
  struct {
    UINT32  OffsetLow:16;   ///< Offset bits 15..0.
    UINT32  Selector:16;    ///< Selector.
    UINT32  Reserved_0:8;   ///< Reserved.
    UINT32  GateType:8;     ///< Gate Type.  See #defines above.
    UINT32  OffsetHigh:16;  ///< Offset bits 31..16.
  } Bits;
  UINT64  Uint64;
} IA32_IDT_ENTRY;
#pragma pack()

#define COMPATIBILITY_MODE_SELECTOR  8

//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_GDT mGdtEntries[] = {
  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, /* 0x0:  reserve */
  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  0, 1,  1,  0}}, /* 0x8:  compatibility mode */
  {{0xFFFF, 0,  0,  0xB,  1,  0,  1,  0xF,  0,  1, 0,  1,  0}}, /* 0x10: for long mode */
  {{0xFFFF, 0,  0,  0x3,  1,  0,  1,  0xF,  0,  0, 1,  1,  0}}, /* 0x18: data */
  {{0,      0,  0,  0,    0,  0,  0,  0,    0,  0, 0,  0,  0}}, /* 0x20: reserve */
};

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_DESCRIPTOR mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN) mGdtEntries
  };
/**
  Assembly function to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.
  @param  Function     The 32bit code entry to be executed.
  @param  Param1       The first parameter to pass to 32bit code
  @param  Param2       The second parameter to pass to 32bit code
  @param  InternalGdtr The GDT and GDT descriptor used by this library
  
  @retval EFI_SUCCESS  Execute 32bit code successfully.
  @retval other        Something wrong when execute the 32bit code 
**/
EFI_STATUS
AsmExecute32BitCode (
  IN UINT64           Function,
  IN UINT64           Param1,
  IN UINT64           Param2,
  IN IA32_DESCRIPTOR  *InternalGdtr
  );
  
/**
  Wrapper for a thunk  to transition from long mode to compatibility mode to execute 32-bit code and then transit back to
  long mode.
  
  @param  Function     The 32bit code entry to be executed.
  @param  Param1       The first parameter to pass to 32bit code
  @param  Param2       The second parameter to pass to 32bit code
  @retval EFI_SUCCESS  Execute 32bit code successfully.
  @retval other        Something wrong when execute the 32bit code 
              
**/  
EFI_STATUS
Execute32BitCode (
  IN UINT64      Function,
  IN UINT64      Param1,
  IN UINT64      Param2
  )
{
  EFI_STATUS                 Status;
  IA32_DESCRIPTOR            *Ia32Idtr;
  IA32_DESCRIPTOR            X64Idtr;
  UINTN                      Ia32IdtEntryCount;
  UINTN                      Index;
  IA32_IDT_ENTRY             *Ia32IdtEntry;

  //
  // Save x64 IDT Descriptor
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *) &X64Idtr);

  //
  // Get the IA32 IDT Descriptor saved in 16 bytes in front of X64 IDT table.
  //
  Ia32Idtr = (IA32_DESCRIPTOR *) (UINTN) (X64Idtr.Base - 16);
  Ia32IdtEntryCount = (Ia32Idtr->Limit + 1) / sizeof (IA32_IDT_ENTRY);

  Ia32IdtEntry = (IA32_IDT_ENTRY *)(Ia32Idtr->Base);
  for (Index = 0; Index < Ia32IdtEntryCount; Index ++ ) {
    //
    // Use the new Code Selector value
    //
    Ia32IdtEntry[Index].Bits.Selector = COMPATIBILITY_MODE_SELECTOR;
  }

  //
  // Setup IA32 IDT table for 32-bit framework Boot Script code
  //
  AsmWriteIdtr (Ia32Idtr);

  ASSERT (Function != 0);
 
  Status = AsmExecute32BitCode (
             Function,
             Param1,
             Param2,
             &mGdt
             );

  //
  // Restore X64 IDT table
  //
  AsmWriteIdtr ((IA32_DESCRIPTOR *) &X64Idtr);

  return Status;
}

