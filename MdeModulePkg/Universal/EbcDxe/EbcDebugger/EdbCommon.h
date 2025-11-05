/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_EDB_COMMON_H_
#define _EFI_EDB_COMMON_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Ebc.h>
#include <Protocol/EbcVmTest.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DebuggerConfiguration.h>
#include <Guid/FileInfo.h>
#include <Guid/DebugImageInfoTable.h>

typedef UINTN EFI_DEBUG_STATUS;

typedef struct _EFI_DEBUGGER_PRIVATE_DATA EFI_DEBUGGER_PRIVATE_DATA;

//
// Definition for Debugger Command
//
typedef
EFI_DEBUG_STATUS
(*EFI_DEBUGGER_COMMAND) (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  );

typedef struct {
  CHAR16                  *CommandName;
  CHAR16                  *CommandTitle;
  CHAR16                  *CommandHelp;
  CHAR16                  *CommandSyntax;
  CHAR16                  *ClassName;
  EFI_INPUT_KEY           CommandKey;
  EFI_DEBUGGER_COMMAND    CommandFunc;
} EFI_DEBUGGER_COMMAND_SET;

//
// Definition for Debugger Symbol
//
#define EFI_DEBUGGER_SYMBOL_NAME_MAX    256
#define EFI_DEBUGGER_SYMBOL_ENTRY_MAX   512
#define EFI_DEBUGGER_SYMBOL_OBJECT_MAX  32

//
// We have following SYMBOL data structure:
//
// SYMBOL_CONTEXT -> SYMBOL_OBJECT -> SYMBOL_ENTRY (FuncXXX, 0xXXX)
//                                    SYMBOL_ENTRY (VarYYY,  0xYYY)
//                                    SYMBOL_ENTRY
//
//                   SYMBOL_OBJECT -> SYMBOL_ENTRY
//                                    SYMBOL_ENTRY
//
//                   SYMBOL_OBJECT -> SYMBOL_ENTRY
//                                    SYMBOL_ENTRY
//

typedef enum {
  EfiDebuggerSymbolFunction,
  EfiDebuggerSymbolStaticFunction,
  EfiDebuggerSymbolGlobalVariable,
  EfiDebuggerSymbolStaticVariable,
  EfiDebuggerSymbolTypeMax,
} EFI_DEBUGGER_SYMBOL_TYPE;

typedef struct {
  CHAR8                       Name[EFI_DEBUGGER_SYMBOL_NAME_MAX];
  UINTN                       Rva;
  EFI_DEBUGGER_SYMBOL_TYPE    Type;
  CHAR8                       ObjName[EFI_DEBUGGER_SYMBOL_NAME_MAX];
  CHAR8                       *CodBuffer;
  UINTN                       CodBufferSize;
  UINTN                       FuncOffsetBase;
  CHAR8                       *SourceBuffer;
} EFI_DEBUGGER_SYMBOL_ENTRY;

typedef struct {
  CHAR16                       Name[EFI_DEBUGGER_SYMBOL_NAME_MAX];
  UINTN                        EntryCount;
  UINTN                        MaxEntryCount;
  UINTN                        BaseAddress;
  UINTN                        StartEntrypointRVA;
  UINTN                        MainEntrypointRVA;
  EFI_DEBUGGER_SYMBOL_ENTRY    *Entry;
  VOID                         **SourceBuffer;
} EFI_DEBUGGER_SYMBOL_OBJECT;

typedef struct {
  UINTN                         ObjectCount;
  UINTN                         MaxObjectCount;
  EFI_DEBUGGER_SYMBOL_OBJECT    *Object;
  BOOLEAN                       DisplaySymbol;
  BOOLEAN                       DisplayCodeOnly;
} EFI_DEBUGGER_SYMBOL_CONTEXT;

//
// Definition for Debugger Breakpoint
//
#define EFI_DEBUGGER_BREAKPOINT_MAX  0x10

typedef struct {
  EFI_PHYSICAL_ADDRESS    BreakpointAddress;
  UINT64                  OldInstruction;      // UINT64 is enough for an instruction
  BOOLEAN                 State;
} EFI_DEBUGGER_BREAKPOINT_CONTEXT;

//
// Definition for Debugger Call-Stack
//
#define EFI_DEBUGGER_CALLSTACK_MAX  0x10

typedef enum {
  EfiDebuggerBranchTypeEbcCall,
  EfiDebuggerBranchTypeEbcCallEx,
  EfiDebuggerBranchTypeEbcRet,
  EfiDebuggerBranchTypeEbcJmp,
  EfiDebuggerBranchTypeEbcJmp8,
  EfiDebuggerBranchTypeEbcMax,
} EFI_DEBUGGER_BRANCH_TYPE;

#define EFI_DEBUGGER_CALL_MAX_PARAMETER      0x16
#define EFI_DEBUGGER_CALL_DEFAULT_PARAMETER  0x8

typedef struct {
  EFI_PHYSICAL_ADDRESS        SourceAddress;
  EFI_PHYSICAL_ADDRESS        DestAddress;
  //
  // We save all parameter here, because code may update the parameter as local variable.
  //
  UINTN                       ParameterAddr;
  UINTN                       Parameter[EFI_DEBUGGER_CALL_MAX_PARAMETER];
  EFI_DEBUGGER_BRANCH_TYPE    Type;
} EFI_DEBUGGER_CALLSTACK_CONTEXT;

//
// Definition for Debugger Trace
//
#define EFI_DEBUGGER_TRACE_MAX  0x10

typedef struct {
  EFI_PHYSICAL_ADDRESS        SourceAddress;
  EFI_PHYSICAL_ADDRESS        DestAddress;
  EFI_DEBUGGER_BRANCH_TYPE    Type;
} EFI_DEBUGGER_TRACE_CONTEXT;

//
// Definition for Debugger Step
//
typedef struct {
  EFI_PHYSICAL_ADDRESS    BreakAddress;
  EFI_PHYSICAL_ADDRESS    FramePointer;
} EFI_DEBUGGER_STEP_CONTEXT;

//
// Definition for Debugger GoTil
//
typedef struct {
  EFI_PHYSICAL_ADDRESS    BreakAddress;
} EFI_DEBUGGER_GOTIL_CONTEXT;

//
// Definition for Debugger private data structure
//
#define EFI_DEBUGGER_SIGNATURE  SIGNATURE_32 ('e', 'd', 'b', '!')

#define EFI_DEBUG_DEFAULT_INSTRUCTION_NUMBER  5

#define EFI_DEBUG_BREAK_TIMER_INTERVAL  10000000         // 1 second

#define EFI_DEBUG_FLAG_EBC             0x80000000
#define EFI_DEBUG_FLAG_EBC_B_BOC       0x1
#define EFI_DEBUG_FLAG_EBC_B_BOCX      0x2
#define EFI_DEBUG_FLAG_EBC_B_BOR       0x4
#define EFI_DEBUG_FLAG_EBC_B_BOE       0x8
#define EFI_DEBUG_FLAG_EBC_B_BOT       0x10
#define EFI_DEBUG_FLAG_EBC_B_STEPOVER  0x20
#define EFI_DEBUG_FLAG_EBC_B_STEPOUT   0x40
#define EFI_DEBUG_FLAG_EBC_B_BP        0x80
#define EFI_DEBUG_FLAG_EBC_B_GT        0x100
#define EFI_DEBUG_FLAG_EBC_B_BOK       0x200
#define EFI_DEBUG_FLAG_EBC_BOC         (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_BOC)
#define EFI_DEBUG_FLAG_EBC_BOCX        (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_BOCX)
#define EFI_DEBUG_FLAG_EBC_BOR         (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_BOR)
#define EFI_DEBUG_FLAG_EBC_BOE         (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_BOE)
#define EFI_DEBUG_FLAG_EBC_BOT         (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_BOT)
#define EFI_DEBUG_FLAG_EBC_STEPOVER    (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_STEPOVER)
#define EFI_DEBUG_FLAG_EBC_STEPOUT     (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_STEPOUT)
#define EFI_DEBUG_FLAG_EBC_BP          (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_BP)
#define EFI_DEBUG_FLAG_EBC_GT          (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_GT)
#define EFI_DEBUG_FLAG_EBC_BOK         (EFI_DEBUG_FLAG_EBC | EFI_DEBUG_FLAG_EBC_B_BOK)

//
// Debugger private data structure
//
typedef struct _EFI_DEBUGGER_PRIVATE_DATA {
  UINT32                                 Signature;
  EFI_INSTRUCTION_SET_ARCHITECTURE       Isa;
  UINT32                                 EfiDebuggerRevision;
  UINT32                                 EbcVmRevision;
  EFI_DEBUGGER_CONFIGURATION_PROTOCOL    DebuggerConfiguration;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER      *DebugImageInfoTableHeader;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL        *Vol;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *PciRootBridgeIo;
  EFI_DEBUGGER_COMMAND_SET               *DebuggerCommandSet;
  EFI_DEBUGGER_SYMBOL_CONTEXT            DebuggerSymbolContext;
  UINTN                                  DebuggerBreakpointCount;
  EFI_DEBUGGER_BREAKPOINT_CONTEXT        DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX + 1];
  UINTN                                  CallStackEntryCount;
  EFI_DEBUGGER_CALLSTACK_CONTEXT         CallStackEntry[EFI_DEBUGGER_CALLSTACK_MAX + 1];
  UINTN                                  TraceEntryCount;
  EFI_DEBUGGER_TRACE_CONTEXT             TraceEntry[EFI_DEBUGGER_TRACE_MAX + 1];
  EFI_DEBUGGER_STEP_CONTEXT              StepContext;
  EFI_DEBUGGER_GOTIL_CONTEXT             GoTilContext;
  EFI_PHYSICAL_ADDRESS                   InstructionScope;
  UINTN                                  InstructionNumber;
  UINT32                                 FeatureFlags;
  UINT32                                 StatusFlags;
  BOOLEAN                                EnablePageBreak;
  EFI_EVENT                              BreakEvent;
} EFI_DEBUGGER_PRIVATE_DATA;

#endif
