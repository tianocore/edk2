/** @file
  This file declares EFI Smm Periodic Timer Smi Child Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmPeriodicTimerDispatch.h

  @par Revision Reference:
  This Protocol is defined in Framework of EFI SMM Core Interface Spec
  Version 0.9.

**/

#ifndef _EFI_SMM_PERIODIC_TIMER_DISPATCH_H_
#define _EFI_SMM_PERIODIC_TIMER_DISPATCH_H_

//
// Global ID for the Periodic Timer SMI Protocol
//
#define EFI_SMM_PERIODIC_TIMER_DISPATCH_PROTOCOL_GUID \
  { \
    0x9cca03fc, 0x4c9e, 0x4a19, {0x9b, 0x6, 0xed, 0x7b, 0x47, 0x9b, 0xde, 0x55 } \
  }

typedef struct _EFI_SMM_PERIODIC_TIMER_DISPATCH_PROTOCOL  EFI_SMM_PERIODIC_TIMER_DISPATCH_PROTOCOL;

//
// Related Definitions
//
//
// Period is the minimum period of time in 100 nanosecond units that child gets called.
// The child will be called back after a time greater than the time Period.
//
// SmiTickInterval is the period of time interval between SMIs.  Children of this interface
// should use this field when registering for periodic timer intervals when a finer
// granularity periodic SMI is desired.  Valid values for this field are those returned
// by GetNextInterval.  A value of 0 indicates the parent is allowed to use any SMI
// interval period to satisfy the requested period.
//    Example: A chipset supports periodic SMIs on every 64ms or 2 seconds.
//      A child wishes schedule a period SMI to fire on a period of 3 seconds, there
//      are several ways to approach the problem:
//      1. The child may accept a 4 second periodic rate, in which case it registers with
//           Period = 40000
//           SmiTickInterval = 20000
//         The resulting SMI will occur every 2 seconds with the child called back on
//         every 2nd SMI.
//         NOTE: the same result would occur if the child set SmiTickInterval = 0.
//      2. The child may choose the finer granularity SMI (64ms):
//           Period = 30000
//           SmiTickInterval = 640
//         The resulting SMI will occur every 64ms with the child called back on
//         every 47th SMI.
//         NOTE: the child driver should be aware that this will result in more
//           SMIs occuring during system runtime which can negatively impact system
//           performance.
//
// ElapsedTime is the actual time in 100 nanosecond units elapsed since last called, a
// value of 0 indicates an unknown amount of time.
//
typedef struct {
  UINT64  Period;
  UINT64  SmiTickInterval;
  UINT64  ElapsedTime;
} EFI_SMM_PERIODIC_TIMER_DISPATCH_CONTEXT;

//
// Member functions
//
/**
  Dispatch function for a Periodic Timer SMI handler.

  @param  DispatchHandle Handle of this dispatch function.
  @param  DispatchContext Pointer to the dispatch function's context.
  The DispatchContext fields are filled in
  by the dispatching driver prior to
  invoking this dispatch function.

  Nothing

**/
typedef
VOID
(EFIAPI *EFI_SMM_PERIODIC_TIMER_DISPATCH) (
  IN  EFI_HANDLE                                DispatchHandle,
  IN  EFI_SMM_PERIODIC_TIMER_DISPATCH_CONTEXT   *DispatchContext
  );

/**
  Returns the next SMI tick period supported by the chipset.  The order
  returned is from longest to shortest interval period.

  @param  This Protocol instance pointer.
  @param  SmiTickInterval Pointer to pointer of next shorter SMI interval
  period supported by the child.  This parameter works as a get-first, 
  get-next field.The first time this function is called, *SmiTickInterval 
  should be set to NULL to get the longest SMI interval.The returned 
  *SmiTickInterval should be passed in on subsequent calls to get the 
  next shorter interval period until *SmiTickInterval = NULL.

  @retval EFI_SUCCESS The service returned successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_PERIODIC_TIMER_INTERVAL) (
  IN EFI_SMM_PERIODIC_TIMER_DISPATCH_PROTOCOL           *This,
  IN OUT UINT64                                         **SmiTickInterval
  );

/**
  Register a child SMI source dispatch function with a parent SMM driver

  @param  This Protocol instance pointer.
  @param  DispatchFunction Pointer to dispatch function to be invoked for
  this SMI source
  @param  DispatchContext Pointer to the dispatch function's context.
  The caller fills this context in before calling
  the register function to indicate to the register
  function the period at which the dispatch function
  should be invoked.
  @param  DispatchHandle Handle of dispatch function, for when interfacing
  with the parent Sx state SMM driver.

  @retval  EFI_SUCCESS The dispatch function has been successfully
  registered and the SMI source has been enabled.
  @retval  EFI_DEVICE_ERROR The driver was unable to enable the SMI source.
  @retval  EFI_OUT_OF_RESOURCES Not enough memory (system or SMM) to manage this
  child.
  @retval  EFI_INVALID_PARAMETER DispatchContext is invalid. The period input value
  is not within valid range.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_PERIODIC_TIMER_REGISTER) (
  IN EFI_SMM_PERIODIC_TIMER_DISPATCH_PROTOCOL           *This,
  IN  EFI_SMM_PERIODIC_TIMER_DISPATCH                   DispatchFunction,
  IN  EFI_SMM_PERIODIC_TIMER_DISPATCH_CONTEXT           *DispatchContext,
  OUT EFI_HANDLE                                        *DispatchHandle
  );

/**
  Unregister a child SMI source dispatch function with a parent SMM driver

  @param  This Protocol instance pointer.
  @param  DispatchHandle Handle of dispatch function to deregister.

  @retval  EFI_SUCCESS The dispatch function has been successfully
  unregistered and the SMI source has been disabled
  if there are no other registered child dispatch
  functions for this SMI source.
  @retval  EFI_INVALID_PARAMETER Handle is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SMM_PERIODIC_TIMER_UNREGISTER) (
  IN EFI_SMM_PERIODIC_TIMER_DISPATCH_PROTOCOL           *This,
  IN  EFI_HANDLE                                        DispatchHandle
  );

//
// Interface structure for the SMM Periodic Timer Dispatch Protocol
//
/**
  @par Protocol Description:
  Provides the parent dispatch service for the periodical timer SMI source generator.

  @param Register
  Installs a child service to be dispatched by this protocol.

  @param UnRegister
  Removes a child service dispatched by this protocol.

  @param GetNextShorterInterval
  Returns the next SMI tick period that is supported by the chipset. 

**/
struct _EFI_SMM_PERIODIC_TIMER_DISPATCH_PROTOCOL {
  EFI_SMM_PERIODIC_TIMER_REGISTER   Register;
  EFI_SMM_PERIODIC_TIMER_UNREGISTER UnRegister;
  EFI_SMM_PERIODIC_TIMER_INTERVAL   GetNextShorterInterval;
};

extern EFI_GUID gEfiSmmPeriodicTimerDispatchProtocolGuid;

#endif
