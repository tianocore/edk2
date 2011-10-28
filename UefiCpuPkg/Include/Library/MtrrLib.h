/** @file
  MTRR setting library

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef  _MTRR_LIB_H_
#define  _MTRR_LIB_H_

//
// According to IA32 SDM, MTRRs number and msr offset are always consistent
// for IA32 processor family
//

//
// The semantics of below macro is MAX_MTRR_NUMBER_OF_VARIABLE_MTRR, the real number can be read out from MTRR_CAP register.
//
#define  MTRR_NUMBER_OF_VARIABLE_MTRR  32
//
// Firmware need reserve 2 MTRR for OS
//
#define  RESERVED_FIRMWARE_VARIABLE_MTRR_NUMBER  2

#define  MTRR_NUMBER_OF_FIXED_MTRR      11
//
// Below macro is deprecated, and should not be used.
//
#define  FIRMWARE_VARIABLE_MTRR_NUMBER  6
#define  MTRR_LIB_IA32_MTRR_CAP                      0x0FE
#define  MTRR_LIB_IA32_MTRR_CAP_VCNT_MASK            0x0FF
#define  MTRR_LIB_IA32_MTRR_FIX64K_00000             0x250
#define  MTRR_LIB_IA32_MTRR_FIX16K_80000             0x258
#define  MTRR_LIB_IA32_MTRR_FIX16K_A0000             0x259
#define  MTRR_LIB_IA32_MTRR_FIX4K_C0000              0x268
#define  MTRR_LIB_IA32_MTRR_FIX4K_C8000              0x269
#define  MTRR_LIB_IA32_MTRR_FIX4K_D0000              0x26A
#define  MTRR_LIB_IA32_MTRR_FIX4K_D8000              0x26B
#define  MTRR_LIB_IA32_MTRR_FIX4K_E0000              0x26C
#define  MTRR_LIB_IA32_MTRR_FIX4K_E8000              0x26D
#define  MTRR_LIB_IA32_MTRR_FIX4K_F0000              0x26E
#define  MTRR_LIB_IA32_MTRR_FIX4K_F8000              0x26F
#define  MTRR_LIB_IA32_VARIABLE_MTRR_BASE            0x200
//
// Below macro is deprecated, and should not be used.
//
#define  MTRR_LIB_IA32_VARIABLE_MTRR_END             0x20F
#define  MTRR_LIB_IA32_MTRR_DEF_TYPE                 0x2FF
#define  MTRR_LIB_MSR_VALID_MASK                     0xFFFFFFFFFULL
#define  MTRR_LIB_CACHE_VALID_ADDRESS                0xFFFFFF000ULL
#define  MTRR_LIB_CACHE_MTRR_ENABLED                 0x800
#define  MTRR_LIB_CACHE_FIXED_MTRR_ENABLED           0x400

//
// Structure to describe a fixed MTRR
//
typedef struct {
  UINT32  Msr;
  UINT32  BaseAddress;
  UINT32  Length;
} FIXED_MTRR;

//
// Structure to describe a variable MTRR
//
typedef struct {
  UINT64  BaseAddress;
  UINT64  Length;
  UINT64  Type;
  UINT32  Msr;
  BOOLEAN Valid;
  BOOLEAN Used;
} VARIABLE_MTRR;

//
// Structure to hold base and mask pair for variable MTRR register
//
typedef struct _MTRR_VARIABLE_SETTING_ {
	UINT64    Base;
	UINT64    Mask;
} MTRR_VARIABLE_SETTING;

//
// Array for variable MTRRs
//
typedef struct _MTRR_VARIABLE_SETTINGS_ {
	MTRR_VARIABLE_SETTING   Mtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
}	MTRR_VARIABLE_SETTINGS;

//
// Array for fixed mtrrs
//
typedef  struct  _MTRR_FIXED_SETTINGS_ {
  UINT64       Mtrr[MTRR_NUMBER_OF_FIXED_MTRR];
} MTRR_FIXED_SETTINGS;

//
// Structure to hold all MTRRs
//
typedef struct _MTRR_SETTINGS_ {
  MTRR_FIXED_SETTINGS       Fixed;
  MTRR_VARIABLE_SETTINGS    Variables;
  UINT64                    MtrrDefType;
} MTRR_SETTINGS;

//
// Memory cache types
//
typedef enum {
	CacheUncacheable    = 0,
	CacheWriteCombining = 1,
	CacheWriteThrough   = 4,
	CacheWriteProtected = 5,
	CacheWriteBack      = 6
} MTRR_MEMORY_CACHE_TYPE;

#define  MTRR_CACHE_UNCACHEABLE      0
#define  MTRR_CACHE_WRITE_COMBINING  1
#define  MTRR_CACHE_WRITE_THROUGH    4
#define  MTRR_CACHE_WRITE_PROTECTED  5
#define  MTRR_CACHE_WRITE_BACK       6
#define  MTRR_CACHE_INVALID_TYPE     7

/**
  Returns the variable MTRR count for the CPU.

  @return Variable MTRR count

**/
UINT32
EFIAPI
GetVariableMtrrCount (
  VOID
  );

/**
  Returns the firmware usable variable MTRR count for the CPU.

  @return Firmware usable variable MTRR count

**/
UINT32
EFIAPI
GetFirmwareVariableMtrrCount (
  VOID
  );

/**
  This function attempts to set the attributes for a memory range.

  @param  BaseAddress            The physical address that is the start address of a memory region.
  @param  Length                 The size in bytes of the memory region.
  @param  Attributes             The bit mask of attributes to set for the memory region.

  @retval RETURN_SUCCESS            The attributes were set for the memory region.
  @retval RETURN_INVALID_PARAMETER  Length is zero.
  @retval RETURN_UNSUPPORTED        The processor does not support one or more bytes of the
                                 memory resource range specified by BaseAddress and Length.
  @retval RETURN_UNSUPPORTED        The bit mask of attributes is not support for the memory resource
                                 range specified by BaseAddress and Length.
  @retval RETURN_ACCESS_DENIED      The attributes for the memory resource range specified by
                                 BaseAddress and Length cannot be modified.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to modify the attributes of
                                 the memory resource range.

**/
RETURN_STATUS
EFIAPI
MtrrSetMemoryAttribute (
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  );


/**
  This function will get the memory cache type of the specific address.
  This function is mainly for debugging purposes.

  @param  Address            The specific address

  @return The memory cache type of the specific address

**/
MTRR_MEMORY_CACHE_TYPE
EFIAPI
MtrrGetMemoryAttribute (
  IN PHYSICAL_ADDRESS   Address
  );


/**
  This function will get the raw value in variable MTRRs

  @param  VariableSettings   A buffer to hold variable MTRRs content.

  @return The buffer point to MTRR_VARIABLE_SETTINGS in which holds the content of the variable mtrr

**/
MTRR_VARIABLE_SETTINGS*
EFIAPI
MtrrGetVariableMtrr (
  OUT MTRR_VARIABLE_SETTINGS         *VariableSettings
  );


/**
  This function sets fixed MTRRs

  @param  VariableSettings   A buffer to hold variable MTRRs content.

  @return The pointer of VariableSettings

**/
MTRR_VARIABLE_SETTINGS*
EFIAPI
MtrrSetVariableMtrr (
  IN MTRR_VARIABLE_SETTINGS         *VariableSettings
  );


/**
  This function gets the content in fixed MTRRs

  @param  FixedSettings      A buffer to hold fixed MTRRs content.

  @return The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS*
EFIAPI
MtrrGetFixedMtrr (
  OUT MTRR_FIXED_SETTINGS         *FixedSettings
  );


/**
  This function sets fixed MTRRs

  @param   FixedSettings      A buffer holding fixed MTRRs content.

  @return  The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS*
EFIAPI
MtrrSetFixedMtrr (
  IN MTRR_FIXED_SETTINGS          *FixedSettings
  );


/**
  This function gets the content in all MTRRs (variable and fixed)

  @param  MtrrSetting   A buffer to hold all MTRRs content.

  @return The pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrGetAllMtrrs (
  OUT MTRR_SETTINGS                *MtrrSetting
  );


/**
  This function sets all MTRRs (variable and fixed)

  @param  MtrrSetting   A buffer to hold all MTRRs content.

  @return The pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrSetAllMtrrs (
  IN MTRR_SETTINGS                *MtrrSetting
  );


/**
  Get the attribute of variable MTRRs.

  This function shadows the content of variable MTRRs into
  an internal array: VariableMtrr

  @param  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param  MtrrValidAddressMask  The valid address mask for MTRR since the base address in
                                MTRR must align to 4K, so valid address mask equal to
                                MtrrValidBitsMask & 0xfffffffffffff000ULL
  @param  VariableMtrr          The array to shadow variable MTRRs content
  @return                       The ruturn value of this paramter indicates the number of
                                MTRRs which has been used.
**/
UINT32
EFIAPI
MtrrGetMemoryAttributeInVariableMtrr (
  IN  UINT64                    MtrrValidBitsMask,
  IN  UINT64                    MtrrValidAddressMask,
  OUT VARIABLE_MTRR             *VariableMtrr
  );


/**
  This function prints all MTRRs for debugging.
**/
VOID
EFIAPI
MtrrDebugPrintAllMtrrs (
  VOID
  );

/**
  Checks if MTRR is supported.

  @retval TRUE  MTRR is supported.
  @retval FALSE MTRR is not supported.

**/
BOOLEAN
EFIAPI
IsMtrrSupported (
  VOID
  );

/**
  Returns the default MTRR cache type for the system.

  @return  The default MTRR cache type.

**/
MTRR_MEMORY_CACHE_TYPE
EFIAPI
MtrrGetDefaultMemoryType (
  VOID
  );

#endif // _MTRR_LIB_H_
