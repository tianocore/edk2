/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueDefinitionChangesBase.h
  
Abstract: 

  Data structure definition changes from EDK to EDKII

--*/

#ifndef __EDKII_GLUE_DEFINITION_CHANGES_BASE_H__
#define __EDKII_GLUE_DEFINITION_CHANGES_BASE_H__

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include "TianoHii.h"
#else
#include "EfiInternalFormRepresentation.h"
#endif
#include "EfiPxe.h"


// ----------------------------------------------------------------------------------
// Data Hub Record GUID Name changes
// ----------------------------------------------------------------------------------
#define gEfiProcessorSubClassGuid   gProcessorSubClassName
#define gEfiCacheSubClassGuid       gCacheSubClassName
#define gEfiMiscSubClassGuid        gMiscSubClassName
#define gEfiProcessorProducerGuid   gProcessorProducerGuid
#define gEfiMemoryProducerGuid      gMemoryProducerGuid
#define gEfiMiscProducerGuid        gMiscProducerGuid


// ----------------------------------------------------------------------------------
// Hob.h: Get the data and data size field of GUID
// ----------------------------------------------------------------------------------
#define GET_GUID_HOB_DATA(GuidHob)      ((VOID *) (((UINT8 *) &((GuidHob)->Name)) + sizeof (EFI_GUID)))
#define GET_GUID_HOB_DATA_SIZE(GuidHob) (((GuidHob)->Header).HobLength - sizeof (EFI_HOB_GUID_TYPE))

// ----------------------------------------------------------------------------------
// InternalFormRepresentation.h:
// ----------------------------------------------------------------------------------
#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
typedef struct {
  EFI_IFR_OP_HEADER Header;
  UINT16            QuestionId; // The ID designating what the question is about...sucked in from a #define, likely in the form of a variable name
  UINT8             Width;      // The Size of the Data being saved
  STRING_REF        Prompt;     // The String Token for the Prompt
  STRING_REF        Help;       // The string Token for the context-help
  UINT8             Flags;      // For now, if non-zero, means that it is the default option, - further definition likely
  UINT16            Key;        // Value to be passed to caller to identify this particular op-code
} EFI_IFR_CHECKBOX;

typedef struct {
  EFI_IFR_OP_HEADER Header;
  UINT8             Flags;
} EFI_IFR_GRAY_OUT;

typedef struct {
  EFI_IFR_OP_HEADER Header;
} EFI_IFR_END_EXPR;
#endif

// ------------------------
// define GUID as EFI_GUID
// ------------------------
typedef EFI_GUID GUID;


// -------------------
// EdkII Names - Edk Names
// -------------------
#define EFI_GLOBAL_VARIABLE                           EFI_GLOBAL_VARIABLE_GUID
#define MPS_TABLE_GUID                                EFI_MPS_TABLE_GUID
#define SAL_SYSTEM_TABLE_GUID                         EFI_SAL_SYSTEM_TABLE_GUID
#define SMBIOS_TABLE_GUID                             EFI_SMBIOS_TABLE_GUID
#define EFI_OPTIONAL_PTR                              EFI_OPTIONAL_POINTER
#define PXE_FRAME_TYPE_FILTERED_MULTICAST             PXE_FRAME_TYPE_MULTICAST
#define IMAGE_FILE_MACHINE_I386                       EFI_IMAGE_MACHINE_IA32
#define IMAGE_FILE_MACHINE_IA64                       EFI_IMAGE_MACHINE_IA64
#define IMAGE_FILE_MACHINE_EBC                        EFI_IMAGE_MACHINE_EBC
#define IMAGE_FILE_MACHINE_X64                        EFI_IMAGE_MACHINE_X64
#define EVENT_TIMER                                   EFI_EVENT_TIMER
#define EVENT_RUNTIME                                 EFI_EVENT_RUNTIME
#define EVENT_RUNTIME_CONTEXT                         EFI_EVENT_RUNTIME_CONTEXT
#define EVENT_NOTIFY_WAIT                             EFI_EVENT_NOTIFY_WAIT
#define EVENT_NOTIFY_SIGNAL                           EFI_EVENT_NOTIFY_SIGNAL
#define EVENT_SIGNAL_EXIT_BOOT_SERVICES               EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES
#define EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE           EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
#define TPL_APPLICATION                               EFI_TPL_APPLICATION
#define TPL_CALLBACK                                  EFI_TPL_CALLBACK
#define TPL_NOTIFY                                    EFI_TPL_NOTIFY
#define TPL_HIGH_LEVEL                                EFI_TPL_HIGH_LEVEL

//
// Typos in EDK
//
#define gEfiHobMemoryAllocModuleGuid        gEfiHobMemeryAllocModuleGuid
#define gEfiHobMemoryAllocStackGuid         gEfiHobMemeryAllocStackGuid
#define gEfiHobMemoryAllocBspStoreGuid      gEfiHobMemeryAllocBspStoreGuid

//
// typedef Edk types - EdkII types
//
typedef PXE_CPB_START                                PXE_CPB_START_30;

#endif
