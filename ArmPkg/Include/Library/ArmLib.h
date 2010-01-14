/** @file

  Copyright (c) 2008-2009 Apple Inc. All rights reserved.<BR>

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_LIB__
#define __ARM_LIB__

typedef enum {
  ARM_CACHE_TYPE_WRITE_BACK,
  ARM_CACHE_TYPE_UNKNOWN
} ARM_CACHE_TYPE;

typedef enum {
  ARM_CACHE_ARCHITECTURE_UNIFIED,
  ARM_CACHE_ARCHITECTURE_SEPARATE,
  ARM_CACHE_ARCHITECTURE_UNKNOWN
} ARM_CACHE_ARCHITECTURE;

typedef struct {
  ARM_CACHE_TYPE          Type;
  ARM_CACHE_ARCHITECTURE  Architecture;
  BOOLEAN                 DataCachePresent;
  UINTN                   DataCacheSize;
  UINTN                   DataCacheAssociativity;
  UINTN                   DataCacheLineLength;
  BOOLEAN                 InstructionCachePresent;
  UINTN                   InstructionCacheSize;
  UINTN                   InstructionCacheAssociativity;
  UINTN                   InstructionCacheLineLength;
} ARM_CACHE_INFO;

typedef enum {
  ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED,
  ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK,
  ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH,
  ARM_MEMORY_REGION_ATTRIBUTE_DEVICE
} ARM_MEMORY_REGION_ATTRIBUTES;

typedef struct {
  UINT32                        PhysicalBase;
  UINT32                        VirtualBase;
  UINT32                        Length;
  ARM_MEMORY_REGION_ATTRIBUTES  Attributes;
} ARM_MEMORY_REGION_DESCRIPTOR;

typedef VOID (*CACHE_OPERATION)(VOID);
typedef VOID (*LINE_OPERATION)(UINTN);

typedef enum {
  ARM_PROCESSOR_MODE_USER       = 0x10,
  ARM_PROCESSOR_MODE_FIQ        = 0x11,
  ARM_PROCESSOR_MODE_IRQ        = 0x12,
  ARM_PROCESSOR_MODE_SUPERVISOR = 0x13,
  ARM_PROCESSOR_MODE_ABORT      = 0x17,
  ARM_PROCESSOR_MODE_UNDEFINED  = 0x1B,
  ARM_PROCESSOR_MODE_SYSTEM     = 0x1F,
  ARM_PROCESSOR_MODE_MASK       = 0x1F
} ARM_PROCESSOR_MODE;

ARM_CACHE_TYPE
EFIAPI
ArmCacheType (
  VOID
  );

ARM_CACHE_ARCHITECTURE
EFIAPI
ArmCacheArchitecture (
  VOID
  );

VOID
EFIAPI
ArmCacheInformation (
  OUT ARM_CACHE_INFO  *CacheInfo
  );

BOOLEAN
EFIAPI
ArmDataCachePresent (
  VOID
  );
  
UINTN
EFIAPI
ArmDataCacheSize (
  VOID
  );
  
UINTN
EFIAPI
ArmDataCacheAssociativity (
  VOID
  );
  
UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  );
  
BOOLEAN
EFIAPI
ArmInstructionCachePresent (
  VOID
  );
  
UINTN
EFIAPI
ArmInstructionCacheSize (
  VOID
  );
  
UINTN
EFIAPI
ArmInstructionCacheAssociativity (
  VOID
  );
  
UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  );
  
UINT32
EFIAPI
Cp15IdCode (
  VOID
  );
  
UINT32
EFIAPI
Cp15CacheInfo (
  VOID
  );

VOID
EFIAPI
ArmInvalidateDataCache (
  VOID
  );


VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  );

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  );

VOID
EFIAPI
ArmInvalidateInstructionCache (
  VOID
  );

VOID
EFIAPI
ArmInvalidateDataCacheEntryByMVA (
  IN  UINTN   Address
  );

VOID
EFIAPI
ArmCleanDataCacheEntryByMVA (
  IN  UINTN   Address
  );

VOID
EFIAPI
ArmCleanInvalidateDataCacheEntryByMVA (
  IN  UINTN   Address
  );

VOID
EFIAPI
ArmEnableDataCache (
  VOID
  );

VOID
EFIAPI
ArmDisableDataCache (
  VOID
  );

VOID
EFIAPI
ArmEnableInstructionCache (
  VOID
  );

VOID
EFIAPI
ArmDisableInstructionCache (
  VOID
  );
  
VOID
EFIAPI
ArmEnableMmu (
  VOID
  );

VOID
EFIAPI
ArmDisableMmu (
  VOID
  );

VOID
EFIAPI
ArmEnableInterrupts (
  VOID
  );

UINTN
EFIAPI
ArmDisableInterrupts (
  VOID
  );
  
BOOLEAN
EFIAPI
ArmGetInterruptState (
  VOID
  );

VOID
EFIAPI
ArmInvalidateTlb (
  VOID
  );
  
VOID
EFIAPI
ArmSetDomainAccessControl (
  IN  UINT32  Domain
  );

VOID
EFIAPI
ArmSetTranslationTableBaseAddress (
  IN  VOID  *TranslationTableBase
  );

VOID *
EFIAPI
ArmGetTranslationTableBaseAddress (
  VOID
  );

VOID
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                          **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize  OPTIONAL
  );
  
BOOLEAN
EFIAPI
ArmMmuEnabled (
  VOID
  );
  
VOID
EFIAPI
ArmSwitchProcessorMode (
  IN ARM_PROCESSOR_MODE Mode
  );

ARM_PROCESSOR_MODE
EFIAPI
ArmProcessorMode (
  VOID
  );
  
VOID
EFIAPI
ArmEnableBranchPrediction (
  VOID
  );

VOID
EFIAPI
ArmDisableBranchPrediction (
  VOID
  );

#endif // __ARM_LIB__
