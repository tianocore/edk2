/** @file
  IPF specific debug support functions

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

//
// private header files
//
#include "PlDebugSupport.h"

BOOLEAN  mInHandler = FALSE;

typedef struct {
  UINT64  low;
  UINT64  high;
} BUNDLE;

//
// number of bundles to swap in ivt
//
#define NUM_BUNDLES_IN_STUB 5
#define NUM_IVT_ENTRIES     64

typedef struct {
  BUNDLE  OrigBundles[NUM_BUNDLES_IN_STUB];
  VOID (*RegisteredCallback) ();
} IVT_ENTRY;

/**
  This is the worker function that uninstalls and removes all handlers.

  @param  ExceptionType     Exception Type
  @param  NewBundles        New Boundles
  @param  NewCallback       New Callback

  @retval EFI_ALEADY_STARTED Ivt already hooked.
  @retval others             Indicates the request was not satisfied.
  @retval EFI_SUCCESS        Successfully uninstalled.

**/
EFI_STATUS
ManageIvtEntryTable (
  IN  EFI_EXCEPTION_TYPE    ExceptionType,
  IN  BUNDLE                NewBundles[4],
  IN  VOID                  (*NewCallback) ()
  );

/**
  Saves original IVT contents and inserts a few new bundles which are fixed up
  to store the ExceptionType and then call the common handler.

  @param  ExceptionType      Exception Type
  @param  NewBundles         New Boundles
  @param  NewCallback        New Callback

**/
VOID
HookEntry (
  IN  EFI_EXCEPTION_TYPE    ExceptionType,
  IN  BUNDLE                NewBundles[4],
  IN  VOID                  (*NewCallback) ()
  );

/**
  Restores original IVT contents when unregistering a callback function.

  @param  ExceptionType     Exception Type

**/
VOID
UnhookEntry (
  IN  EFI_EXCEPTION_TYPE    ExceptionType
  );

/**
  Sets up cache flush and calls assembly function to chain external interrupt.

  Records new callback in IvtEntryTable.

  @param  NewCallback     New Callback.

**/
VOID
ChainExternalInterrupt (
  IN  VOID                  (*NewCallback) ()
  );

/**
  Sets up cache flush and calls assembly function to restore external interrupt.
  Removes registered callback from IvtEntryTable.

**/
VOID
UnchainExternalInterrupt (
  VOID
  );

/**
  Given an integer number, return the physical address of the entry point in the IFT.

  @param  HandlerIndex       Index of the Handler 
  @param  EntryPoint         IFT Entrypoint

**/
VOID
GetHandlerEntryPoint (
  UINTN                     HandlerIndex,
  VOID                      **EntryPoint
  );

IVT_ENTRY IvtEntryTable[NUM_IVT_ENTRIES];

//
// IPF context record is overallocated by 512 bytes to guarantee a 512 byte alignment exists
// within the buffer and still have a large enough buffer to hold a whole IPF context record.
//
UINT8     IpfContextBuf[sizeof (EFI_SYSTEM_CONTEXT_IPF) + 512];

//
// The PatchSaveBuffer is used to store the original bundles from the IVT where it is patched
// with the common handler.
//
UINT8     PatchSaveBuffer[0x400];
UINTN     ExternalInterruptCount;

/**
  IPF specific DebugSupport driver initialization. 

  Must be public because it's referenced from DebugSupport.c

  @retval  EFI_SUCCESS     Always.

**/
EFI_STATUS
PlInitializeDebugSupportDriver (
  VOID
  )
{
  ZeroMem (IvtEntryTable, sizeof (IvtEntryTable));
  ExternalInterruptCount = 0;
  return EFI_SUCCESS;
}

/**
  Unload handler that is called during UnloadImage() - deallocates pool memory
  used by the driver.  Must be public because it's referenced from DebugSuport.c

  @param  ImageHandle    The firmware allocated handle for the EFI image.

  @retval EFI_SUCCESS    Always.

**/
EFI_STATUS
EFIAPI
PlUnloadDebugSupportDriver (
  IN EFI_HANDLE       ImageHandle
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;

  for (ExceptionType = 0; ExceptionType < NUM_IVT_ENTRIES; ExceptionType++) {
    ManageIvtEntryTable (ExceptionType, NULL, NULL);
  }

  return EFI_SUCCESS;
}

/**
  C routine that is called for all registered exceptions.  This is the main
  exception dispatcher.  Must be public because it's referenced from AsmFuncs.s.

  @param  ExceptionType        Exception Type
  @param  Context              System Context
**/
VOID
CommonHandler (
  IN EFI_EXCEPTION_TYPE ExceptionType,
  IN EFI_SYSTEM_CONTEXT Context
  )
{
  DEBUG_CODE_BEGIN ();
    if (mInHandler) {
      DEBUG ((EFI_D_INFO, "ERROR: Re-entered debugger!\n"
                                    "       ExceptionType == %X\n"
                                    "       Context       == %X\n"
                                    "       Context.SystemContextIpf->CrIip  == %X\n"
                                    "       Context.SystemContextIpf->CrIpsr == %X\n"
                                    "       mInHandler     == %X\n",
                                    ExceptionType, 
                                    Context, 
                                    Context.SystemContextIpf->CrIip,
                                    Context.SystemContextIpf->CrIpsr,
                                    mInHandler));
    }
  DEBUG_CODE_END ();

  ASSERT (!mInHandler);
  mInHandler = TRUE;
  if (IvtEntryTable[ExceptionType].RegisteredCallback != NULL) {
    if (ExceptionType != EXCEPT_IPF_EXTERNAL_INTERRUPT) {
      IvtEntryTable[ExceptionType].RegisteredCallback (ExceptionType, Context.SystemContextIpf);
    } else {
      IvtEntryTable[ExceptionType].RegisteredCallback (Context.SystemContextIpf);
    }
  } else {
    ASSERT (0);
  }

  mInHandler = FALSE;
}

/**
  Given an integer number, return the physical address of the entry point in the IFT.

  @param  HandlerIndex       Index of the Handler 
  @param  EntryPoint         IFT Entrypoint

**/
VOID
GetHandlerEntryPoint (
  UINTN   HandlerIndex,
  VOID    **EntryPoint
  )
{
  UINT8 *TempPtr;

  //
  // get base address of IVT
  //
  TempPtr = GetIva ();

  if (HandlerIndex < 20) {
    //
    // first 20 provide 64 bundles per vector
    //
    TempPtr += 0x400 * HandlerIndex;
  } else {
    //
    // the rest provide 16 bundles per vector
    //
    TempPtr += 0x5000 + 0x100 * (HandlerIndex - 20);
  }

  *EntryPoint = (VOID *) TempPtr;
}

/**
  This is the worker function that uninstalls and removes all handlers.

  @param  ExceptionType     Exception Type
  @param  NewBundles        New Boundles
  @param  NewCallback       New Callback

  @retval EFI_ALEADY_STARTED Ivt already hooked.
  @retval others             Indicates the request was not satisfied.
  @retval EFI_SUCCESS        Successfully uninstalled.

**/
EFI_STATUS
ManageIvtEntryTable (
  IN  EFI_EXCEPTION_TYPE           ExceptionType,
  IN  BUNDLE                       NewBundles[NUM_BUNDLES_IN_STUB],
  IN  VOID                         (*NewCallback) ()
  )
{
  BUNDLE  *B0Ptr;
  UINT64  InterruptFlags;
  EFI_TPL OldTpl;

  //
  // Get address of bundle 0
  //
  GetHandlerEntryPoint (ExceptionType, (VOID **) &B0Ptr);

  if (IvtEntryTable[ExceptionType].RegisteredCallback != NULL) {
    //
    // we've already installed to this vector
    //
    if (NewCallback != NULL) {
      //
      // if the input handler is non-null, error
      //
      return EFI_ALREADY_STARTED;
    } else {
      //
      // else remove the previously installed handler
      //
      OldTpl          = gBS->RaiseTPL (TPL_HIGH_LEVEL);
      InterruptFlags  = ProgramInterruptFlags (DISABLE_INTERRUPTS);
      if (ExceptionType == EXCEPT_IPF_EXTERNAL_INTERRUPT) {
        UnchainExternalInterrupt ();
      } else {
        UnhookEntry (ExceptionType);
      }

      ProgramInterruptFlags (InterruptFlags);
      gBS->RestoreTPL (OldTpl);
      //
      // re-init IvtEntryTable
      //
      ZeroMem (&IvtEntryTable[ExceptionType], sizeof (IVT_ENTRY));
    }
  } else {
    //
    // no user handler installed on this vector
    //
    if (NewCallback != NULL) {
      OldTpl          = gBS->RaiseTPL (TPL_HIGH_LEVEL);
      InterruptFlags  = ProgramInterruptFlags (DISABLE_INTERRUPTS);
      if (ExceptionType == EXCEPT_IPF_EXTERNAL_INTERRUPT) {
        ChainExternalInterrupt (NewCallback);
      } else {
        HookEntry (ExceptionType, NewBundles, NewCallback);
      }

      ProgramInterruptFlags (InterruptFlags);
      gBS->RestoreTPL (OldTpl);
    }
  }

  return EFI_SUCCESS;
}

/**
  Saves original IVT contents and inserts a few new bundles which are fixed up
  to store the ExceptionType and then call the common handler.

  @param  ExceptionType      Exception Type
  @param  NewBundles         New Boundles
  @param  NewCallback        New Callback

**/
VOID
HookEntry (
  IN  EFI_EXCEPTION_TYPE  ExceptionType,
  IN  BUNDLE              NewBundles[4],
  IN  VOID                (*NewCallback) ()
  )
{
  BUNDLE  *FixupBundle;
  BUNDLE  *B0Ptr;

  //
  // Get address of bundle 0
  //
  GetHandlerEntryPoint (ExceptionType, (VOID **) &B0Ptr);

  //
  // copy original bundles from IVT to IvtEntryTable so we can restore them later
  //
  CopyMem (
    IvtEntryTable[ExceptionType].OrigBundles,
    B0Ptr,
    sizeof (BUNDLE) * NUM_BUNDLES_IN_STUB
    );
  //
  // insert new B0
  //
  CopyMem (B0Ptr, NewBundles, sizeof (BUNDLE) * NUM_BUNDLES_IN_STUB);

  //
  // fixup IVT entry so it stores its index and whether or not to chain...
  //
  FixupBundle = B0Ptr + 2;
  FixupBundle->high |= ExceptionType << 36;

  InstructionCacheFlush (B0Ptr, 5);
  IvtEntryTable[ExceptionType].RegisteredCallback = NewCallback;
}

/**
  Restores original IVT contents when unregistering a callback function.

  @param  ExceptionType     Exception Type

**/
VOID
UnhookEntry (
  IN  EFI_EXCEPTION_TYPE  ExceptionType
  )
{
  BUNDLE  *B0Ptr;

  //
  // Get address of bundle 0
  //
  GetHandlerEntryPoint (ExceptionType, (VOID **) &B0Ptr);
  //
  // restore original bundles in IVT
  //
  CopyMem (
    B0Ptr,
    IvtEntryTable[ExceptionType].OrigBundles,
    sizeof (BUNDLE) * NUM_BUNDLES_IN_STUB
    );
  InstructionCacheFlush (B0Ptr, 5);
}

/**
  Sets up cache flush and calls assembly function to chain external interrupt.

  Records new callback in IvtEntryTable.

  @param  NewCallback     New Callback

**/
VOID
ChainExternalInterrupt (
  IN  VOID  (*NewCallback) ()
  )
{
  VOID  *Start;

  Start = (VOID *) ((UINT8 *) GetIva () + 0x400 * EXCEPT_IPF_EXTERNAL_INTERRUPT + 0x400);
  IvtEntryTable[EXCEPT_IPF_EXTERNAL_INTERRUPT].RegisteredCallback = NewCallback;
  ChainHandler ();
  InstructionCacheFlush (Start, 0x400);
}

/**
  Sets up cache flush and calls assembly function to restore external interrupt.
  Removes registered callback from IvtEntryTable.

**/
VOID
UnchainExternalInterrupt (
  VOID
  )
{
  VOID  *Start;

  Start = (VOID *) ((UINT8 *) GetIva () + 0x400 * EXCEPT_IPF_EXTERNAL_INTERRUPT + 0x400);
  UnchainHandler ();
  InstructionCacheFlush (Start, 0x400);
  IvtEntryTable[EXCEPT_IPF_EXTERNAL_INTERRUPT].RegisteredCallback = NULL;
}

//
// The rest of the functions in this file are all member functions for the
// DebugSupport protocol
//

/**
  This is a DebugSupport protocol member function, hard
  coded to support only 1 processor for now.

  @param  This                The DebugSupport instance
  @param  MaxProcessorIndex   The maximuim supported processor index

  @retval EFI_SUCCESS         Always returned with **MaxProcessorIndex set to 0.

**/
EFI_STATUS
EFIAPI
GetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL    *This,
  OUT UINTN                        *MaxProcessorIndex
  )
{
  *MaxProcessorIndex = 0;
  return (EFI_SUCCESS);
}

/**
  DebugSupport protocol member function.

  @param  This               The DebugSupport instance
  @param  ProcessorIndex     Which processor the callback applies to.
  @param  PeriodicCallback   Callback function

  @retval EFI_SUCCESS        Indicates the callback was registered.
  @retval others             Callback was not registered.

**/
EFI_STATUS
EFIAPI
RegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  IN UINTN                          ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK          PeriodicCallback
  )
{
  return ManageIvtEntryTable (EXCEPT_IPF_EXTERNAL_INTERRUPT, NULL, PeriodicCallback);
}

/**
  DebugSupport protocol member function.

  @param  This              The DebugSupport instance
  @param  ProcessorIndex    Which processor the callback applies to.
  @param  NewCallback       Callback function
  @param  ExceptionType     Which exception to hook

  @retval EFI_SUCCESS        Indicates the callback was registered.
  @retval others             Callback was not registered.

**/
EFI_STATUS
EFIAPI
RegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL    *This,
  IN UINTN                         ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK        NewCallback,
  IN EFI_EXCEPTION_TYPE            ExceptionType
  )
{
  return ManageIvtEntryTable (
          ExceptionType,
          (BUNDLE *) ((EFI_PLABEL *) HookStub)->EntryPoint,
          NewCallback
          );
}

/**
  DebugSupport protocol member function.  Calls assembly routine to flush cache.

  @param  This              The DebugSupport instance
  @param  ProcessorIndex    Which processor the callback applies to.
  @param  Start             Physical base of the memory range to be invalidated
  @param  Length            mininum number of bytes in instruction cache to invalidate

  @retval EFI_SUCCESS       Always returned.

**/
EFI_STATUS
EFIAPI
InvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL    *This,
  IN UINTN                         ProcessorIndex,
  IN VOID                          *Start,
  IN UINTN                         Length
  )
{
  InstructionCacheFlush (Start, Length);
  return EFI_SUCCESS;
}
