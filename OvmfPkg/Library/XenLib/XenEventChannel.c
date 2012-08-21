/**@file
  This file holds the event channel operations used by Xen paravirtualization.

  Copyright (c) 2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/XenHypercallLib.h>
#include <Library/XenLib.h>

#define NR_ENVENTS   1024


//
// TODO Need to change ClearBit and SetBit to the style of edk2
//
#define LOCK_PREFIX ""
#define LOCK ""
#define ADDR (*(long *) addr)
/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static __inline__ void ClearBit(int nr, volatile void * addr)
{
	__asm__ __volatile__( LOCK_PREFIX
		"btrl %1,%0"
		:"=m" (ADDR)
		:"dIr" (nr));
}

/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __inline__ void SetBit(int nr, volatile void * addr)
{
	__asm__ __volatile__( LOCK_PREFIX
		"btsl %1,%0"
		:"=m" (ADDR)
		:"dIr" (nr) : "memory");
}

//
// this represents a event handler. Chaining or sharing is not allowed
//
typedef struct {
  VOID          *Data;
  UINT32        Count;
} EVENT_ACTION;

EVENT_ACTION              EventActions[NR_ENVENTS];
UINTN                     BoundPorts[NR_ENVENTS / (8*sizeof(UINTN))];

/**
**/
EFI_STATUS
EFIAPI
MaskEventChannel (
  IN     UINT32                    Port
  )
{
  return EFI_SUCCESS;
}


/**
**/
EFI_STATUS
EFIAPI
UnmaskEventChannel (
  IN     UINT32                    Port
  )
{
  return EFI_SUCCESS;
}


/**

**/
EFI_EVENT_CHANNEL_PORT
EFIAPI
BindEventChannel (
  IN  OUT  EFI_EVENT_CHANNEL_PORT    Port,
  IN       VOID                      *Data
  )
{
  EventActions[Port].Data = Data;
  MemoryFence ();
  SetBit (Port, BoundPorts);

  return Port;
}


/**
**/
EFI_STATUS
EFIAPI
UnbindEventChannel (
  IN     EFI_EVENT_CHANNEL_PORT    Port
  )
{
  return EFI_SUCCESS;
}


/**
  Create a port available to the pal for exchanging notifications.
  Returns the result of the hypervisor call.

  Unfortunate confusion of terminology: the port is unbound as far
  as Xen is concerned, but we automatically bind a handler to it
  from inside OVMF.

**/
UINTN
EFIAPI
EventChannelAllocateUnbound (
  IN     DOMID                     Pal,
  IN     VOID                      *Data,
  IN     EFI_EVENT_CHANNEL_PORT    *Port
  )
{
  EVENT_CHANNEL_ALLOC_UNBOUND      Unbound;
  UINTN                            ReturnVal;

  Unbound.Dom        = DOMID_SELF;
  Unbound.RemoteDom  = Pal;
  ReturnVal = HypervisorEventChannelOp(EVTCHNOP_ALLOC_UNBOUND, &Unbound);
  if (ReturnVal) {
    DEBUG ((EFI_D_ERROR, "Hypercall Error: Alloc Unbound failed.\n"));
    return ReturnVal;
  }

  *Port = BindEventChannel (Unbound.Port, /*Handler,*/ Data);
  return ReturnVal;
}


/**
TODO
**/
EFI_STATUS
EFIAPI
NotifyRemoteViaEventChannel (
  IN     EFI_EVENT_CHANNEL_PORT    Port
  )
{
  EVENT_CHANNEL_SEND            Send;

  //
  // Tell Xen (Dom0) about the request.
  //
  Send.Port = Port;
  if (HypervisorEventChannelOp(EVTCHNOP_SEND, &Send)) {
    DEBUG ((EFI_D_ERROR, "Hypercall Error: can't notify the remote domain.\n"));
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}


/**
  Initial the event channel for OVMF.
  TODO
**/
EFI_STATUS
EFIAPI
InitEventChannel (
  VOID
  )
{

  return EFI_SUCCESS;
}


/**
  Reset the event channel.
  TODO
**/
EFI_STATUS
EFIAPI
FiniEventChannel (
  VOID
  )
{

  return EFI_SUCCESS;
}

