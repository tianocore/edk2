/** @file

  EFI Deferred Procedure Call Protocol.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __DPC_H__
#define __DPC_H__

//
// DPC Protocol GUID value
//
#define EFI_DPC_PROTOCOL_GUID \
    { \
      0x480f8ae9, 0xc46, 0x4aa9, { 0xbc, 0x89, 0xdb, 0x9f, 0xba, 0x61, 0x98, 0x6 } \
    }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_DPC_PROTOCOL  EFI_DPC_PROTOCOL;


/**
  Invoke a Deferred Procedure Call.

  @param  DpcContext           The pointer to the Deferred Procedure Call's context,
                               which is implementation dependent.

**/
typedef
VOID
(EFIAPI *EFI_DPC_PROCEDURE)(
  IN VOID  *DpcContext
  );

/**
  Add a Deferred Procedure Call to the end of the DPC queue.

  @param  This          The protocol instance pointer.
  @param  DpcTpl        The EFI_TPL that the DPC should invoke.
  @param  DpcProcedure  The pointer to the DPC's function.
  @param  DpcContext    The pointer to the DPC's context.  Passed to DpcProcedure
                        when DpcProcedure is invoked.

  @retval EFI_SUCCESS            The DPC was queued.
  @retval EFI_INVALID_PARAMETER  DpcTpl is not a valid EFI_TPL.
  @retval EFI_INVALID_PARAMETER  DpcProcedure is NULL.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to
                                 add the DPC to the queue.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DPC_QUEUE_DPC)(
  IN EFI_DPC_PROTOCOL   *This,
  IN EFI_TPL            DpcTpl,
  IN EFI_DPC_PROCEDURE  DpcProcedure,
  IN VOID               *DpcContext    OPTIONAL
  );

/**
  Dispatch the queue of DPCs.  
  
  DPCs with DpcTpl value greater than the current TPL value are queued, and then DPCs
  with DpcTpl value lower than the current TPL value are queued. All DPCs in the first 
  group (higher DpcTpl values) are invoked before DPCs in the second group (lower DpcTpl values). 

  @param  This  Protocol instance pointer.

  @retval EFI_SUCCESS    One or more DPCs were invoked.
  @retval EFI_NOT_FOUND  No DPCs were invoked.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DPC_DISPATCH_DPC)(
  IN EFI_DPC_PROTOCOL  *This
  );

///
/// DPC Protocol structure.
///
struct _EFI_DPC_PROTOCOL {
  EFI_DPC_QUEUE_DPC     QueueDpc;
  EFI_DPC_DISPATCH_DPC  DispatchDpc;
};

///
/// DPC Protocol GUID variable.
///
extern EFI_GUID gEfiDpcProtocolGuid;

#endif
