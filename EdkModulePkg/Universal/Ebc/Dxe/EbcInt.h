/*++ 

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  EbcInt.h
  
Abstract:

  Main routines for the EBC interpreter.  Includes the initialization and
  main interpreter routines. 
  
--*/

#ifndef _EBC_INT_H_
#define _EBC_INT_H_

typedef INT64   VM_REGISTER;
typedef UINT8   *VMIP;      // instruction pointer for the VM
typedef UINT32  EXCEPTION_FLAGS;

typedef struct {
  VM_REGISTER       R[8];   // General purpose registers.
  UINT64            Flags;  // Flags register:
                            //   0   Set to 1 if the result of the last compare was true
                            //   1  Set to 1 if stepping
                            //   2..63 Reserved.
  VMIP              Ip;                     // Instruction pointer.
  UINTN             LastException;          //
  EXCEPTION_FLAGS   ExceptionFlags;         // to keep track of exceptions
  UINT32            StopFlags;
  UINT32            CompilerVersion;        // via break(6)
  UINTN             HighStackBottom;        // bottom of the upper stack
  UINTN             LowStackTop;            // top of the lower stack
  UINT64            StackRetAddr;           // location of final return address on stack
  UINTN             *StackMagicPtr;         // pointer to magic value on stack to detect corruption
  EFI_HANDLE        ImageHandle;            // for this EBC driver
  EFI_SYSTEM_TABLE  *SystemTable;           // for debugging only
  UINTN             LastAddrConverted;      // for debug
  UINTN             LastAddrConvertedValue; // for debug
  VOID              *FramePtr;
  VOID              *EntryPoint;            // entry point of EBC image
  UINTN             ImageBase;
  VOID              *StackPool;
  VOID              *StackTop;
} VM_CONTEXT;

extern VM_CONTEXT                    *mVmPtr;

//
// Bits of exception flags field of VM context
//
#define EXCEPTION_FLAG_FATAL    0x80000000  // can't continue
#define EXCEPTION_FLAG_ERROR    0x40000000  // bad, but try to continue
#define EXCEPTION_FLAG_WARNING  0x20000000  // harmless problem
#define EXCEPTION_FLAG_NONE     0x00000000  // for normal return
//
// Flags passed to the internal create-thunks function.
//
#define FLAG_THUNK_ENTRY_POINT  0x01  // thunk for an image entry point
#define FLAG_THUNK_PROTOCOL     0x00  // thunk for an EBC protocol service
//
// Put this value at the bottom of the VM's stack gap so we can check it on
// occasion to make sure the stack has not been corrupted.
//
#define VM_STACK_KEY_VALUE  0xDEADBEEF

EFI_STATUS
EbcCreateThunks (
  IN EFI_HANDLE           ImageHandle,
  IN VOID                 *EbcEntryPoint,
  OUT VOID                **Thunk,
  IN UINT32               Flags
  )
;

EFI_STATUS
EbcAddImageThunk (
  IN EFI_HANDLE     ImageHandle,
  IN VOID           *ThunkBuffer,
  IN UINT32         ThunkSize
  )
;

//
// The interpreter calls these when an exception is detected,
// or as a periodic callback.
//
EFI_STATUS
EbcDebugSignalException (
  IN EFI_EXCEPTION_TYPE ExceptionType,
  IN EXCEPTION_FLAGS    ExceptionFlags,
  IN VM_CONTEXT         *VmPtr
  )
;

//
// Define a constant of how often to call the debugger periodic callback
// function.
//
#define EFI_TIMER_UNIT_1MS            (1000 * 10)
#define EBC_VM_PERIODIC_CALLBACK_RATE (1000 * EFI_TIMER_UNIT_1MS)
#define STACK_POOL_SIZE               (1024 * 1020)
#define MAX_STACK_NUM                 4

EFI_STATUS
EbcDebugSignalPeriodic (
  IN VM_CONTEXT   *VmPtr
  )
;

//
// External low level functions that are native-processor dependent
//
UINTN
EbcLLGetEbcEntryPoint (
  VOID
  )
;

UINTN
EbcLLGetStackPointer (
  VOID
  )
;

VOID
EbcLLCALLEXNative (
  IN UINTN        CallAddr,
  IN UINTN        EbcSp,
  IN VOID         *FramePtr
  )
;

VOID
EbcLLCALLEX (
  IN VM_CONTEXT   *VmPtr,
  IN UINTN        CallAddr,
  IN UINTN        EbcSp,
  IN VOID         *FramePtr,
  IN UINT8        Size
  )
;

INT64
EbcLLGetReturnValue (
  VOID
  )
;

EFI_STATUS
GetEBCStack(
  EFI_HANDLE Handle,
  VOID       **StackBuffer,
  UINTN      *BufferIndex
  );

EFI_STATUS
ReturnEBCStack(
  UINTN Index
  );

EFI_STATUS
InitEBCStack (
  VOID
  );

EFI_STATUS
FreeEBCStack(
  VOID
  );

EFI_STATUS
ReturnEBCStackByHandle(
  EFI_HANDLE Handle
  );
//
// Defines for a simple EBC debugger interface
//
typedef struct _EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL;

#define EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL_GUID \
  { \
    0x2a72d11e, 0x7376, 0x40f6, { 0x9c, 0x68, 0x23, 0xfa, 0x2f, 0xe3, 0x63, 0xf1 } \
  }

typedef
EFI_STATUS
(*EBC_DEBUGGER_SIGNAL_EXCEPTION) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           * This,
  IN VM_CONTEXT                                 * VmPtr,
  IN EFI_EXCEPTION_TYPE                         ExceptionType
  );

typedef
VOID
(*EBC_DEBUGGER_DEBUG) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           * This,
  IN VM_CONTEXT                                 * VmPtr
  );

typedef
UINT32
(*EBC_DEBUGGER_DASM) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           * This,
  IN VM_CONTEXT                                 * VmPtr,
  IN UINT16                                     *DasmString OPTIONAL,
  IN UINT32                                     DasmStringSize
  );

//
// This interface allows you to configure the EBC debug support
// driver. For example, turn on or off saving and printing of
// delta VM even if called. Or to even disable the entire interface,
// in which case all functions become no-ops.
//
typedef
EFI_STATUS
(*EBC_DEBUGGER_CONFIGURE) (
  IN EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL           * This,
  IN UINT32                                     ConfigId,
  IN UINTN                                      ConfigValue
  );

//
// Prototype for the actual EBC debug support protocol interface
//
struct _EFI_EBC_SIMPLE_DEBUGGER_PROTOCOL {
  EBC_DEBUGGER_DEBUG            Debugger;
  EBC_DEBUGGER_SIGNAL_EXCEPTION SignalException;
  EBC_DEBUGGER_DASM             Dasm;
  EBC_DEBUGGER_CONFIGURE        Configure;
};

typedef struct {
  EFI_EBC_PROTOCOL  *This;
  VOID              *EntryPoint;
  EFI_HANDLE        ImageHandle;
  VM_CONTEXT        VmContext;
} EFI_EBC_THUNK_DATA;

#define EBC_PROTOCOL_PRIVATE_DATA_SIGNATURE EFI_SIGNATURE_32 ('e', 'b', 'c', 'p')

struct _EBC_PROTOCOL_PRIVATE_DATA {
  UINT32            Signature;
  EFI_EBC_PROTOCOL  EbcProtocol;
  UINTN             StackBase;
  UINTN             StackTop;
  UINTN             StackSize;
} ;

#define EBC_PROTOCOL_PRIVATE_DATA_FROM_THIS(a) \
      CR(a, EBC_PROTOCOL_PRIVATE_DATA, EbcProtocol, EBC_PROTOCOL_PRIVATE_DATA_SIGNATURE)


#endif // #ifndef _EBC_INT_H_
