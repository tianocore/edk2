/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ExtendedSalBootService.h

Abstract:

--*/

#ifndef _EXTENDED_SAL_PROTOCOL_H_
#define _EXTENDED_SAL_PROTOCOL_H_

#include "SalApi.h"
#include "LinkedList.h"
#include EFI_PROTOCOL_CONSUMER (CpuIo)


#define EXTENDED_SAL_BOOT_SERVICE_PROTOCOL_GUID   \
  {0xde0ee9a4,0x3c7a,0x44f2,{0xb7,0x8b,0xe3,0xcc,0xd6,0x9c,0x3a,0xf7}}

#define EXTENDED_SAL_SIGNATURE  EFI_SIGNATURE_32('e', 's', 'a', 'l')

#define SAL_MIN_STATE_SIZE    0x400 * 1
#define PAL_SCARTCH_SIZE      0x400 * 3
#define ALIGN_MINSTATE_SIZE   512
#define MAX_SAL_RECORD_SIZE   8*1024

#define SAL_RUNTIMESERVICE

typedef UINT16    EFI_SAL_PROCESSOR_ID;

EFI_FORWARD_DECLARATION (EXTENDED_SAL_BOOT_SERVICE_PROTOCOL);

typedef
SAL_RUNTIMESERVICE
SAL_RETURN_REGS
(EFIAPI *SAL_EXTENDED_SAL_PROC) (
  IN  EFI_GUID                                    *ClassGuid,
  IN   UINT64                                      FunctionId,
  IN  UINT64                                      Arg2,
  IN  UINT64                                      Arg3,
  IN  UINT64                                      Arg4,
  IN  UINT64                                      Arg5,
  IN  UINT64                                      Arg6,
  IN  UINT64                                      Arg7,
  IN  UINT64                                      Arg8
  );

typedef
SAL_RUNTIMESERVICE
SAL_RETURN_REGS
(EFIAPI *SAL_INTERNAL_EXTENDED_SAL_PROC) (
  IN  UINT64                                      FunctionId,
  IN  UINT64                                      Arg2,
  IN  UINT64                                      Arg3,
  IN  UINT64                                      Arg4,
  IN  UINT64                                      Arg5,
  IN  UINT64                                      Arg6,
  IN  UINT64                                      Arg7,
  IN  UINT64                                      Arg8,
  IN  SAL_EXTENDED_SAL_PROC                       ExtendedSalProc,
  IN   BOOLEAN                                     VirtualMode,
  IN  VOID                                        *ModuleGlobal
  ); 

typedef
EFI_STATUS
(EFIAPI *EXTENDED_SAL_ADD_SST_INFO) (
  IN EXTENDED_SAL_BOOT_SERVICE_PROTOCOL  *This,
  IN  UINT16                                      SalAVersion,
  IN   UINT16                                      SalBVersion,
  IN  CHAR8                                        *OemId,
  IN  CHAR8                                        *ProductId
  );

typedef
EFI_STATUS
(EFIAPI *EXTENDED_SAL_ADD_SST_ENTRY) (
  IN EXTENDED_SAL_BOOT_SERVICE_PROTOCOL  *This,
  IN  UINT8                                        EntryType,
  IN   UINT8                                        *TableEntry,
  IN  UINTN                                        EntrySize
  );

typedef
EFI_STATUS
(EFIAPI *EXTENDED_SAL_REGISTER_INTERNAL_PROC) (
  IN EXTENDED_SAL_BOOT_SERVICE_PROTOCOL  *This,
  IN  EFI_GUID                                    *ClassGuid,
  IN   UINT64                                      FunctionId,
  IN  SAL_INTERNAL_EXTENDED_SAL_PROC              InternalSalProc,
  IN  VOID                                        *PhysicalModuleGlobal
  );

//
// Extended Sal Boot Service Protocol Interface
//
struct _EXTENDED_SAL_BOOT_SERVICE_PROTOCOL {
  EXTENDED_SAL_ADD_SST_INFO                       AddSalSystemTableInfo;
  EXTENDED_SAL_ADD_SST_ENTRY                      AddSalSystemTableEntry;
  EXTENDED_SAL_REGISTER_INTERNAL_PROC             AddExtendedSalProc;   
  SAL_EXTENDED_SAL_PROC                           ExtendedSalProc;
  SAL_PROC                                        SalProc;
};

extern EFI_GUID   gEfiExtendedSalBootServiceProtocolGuid;

#endif
