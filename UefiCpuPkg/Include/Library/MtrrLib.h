/** @file
  MTRR setting library

  Copyright (c) 2008 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef  _MTRR_LIB_H_
#define  _MTRR_LIB_H_

//
// According to IA32 SDM, MTRRs number and MSR offset are always consistent
// for IA32 processor family
//

//
// The semantics of below macro is MAX_MTRR_NUMBER_OF_VARIABLE_MTRR, the real number can be read out from MTRR_CAP register.
//
#define  MTRR_NUMBER_OF_VARIABLE_MTRR  32
//
// Firmware need reserve 2 MTRR for OS
// Note: It is replaced by PCD PcdCpuNumberOfReservedVariableMtrrs
//
#define  RESERVED_FIRMWARE_VARIABLE_MTRR_NUMBER  2

#define  MTRR_NUMBER_OF_FIXED_MTRR  11

//
// Structure to describe a fixed MTRR
//
typedef struct {
  UINT32    Msr;
  UINT32    BaseAddress;
  UINT32    Length;
} FIXED_MTRR;

//
// Structure to describe a variable MTRR
//
typedef struct {
  UINT64     BaseAddress;
  UINT64     Length;
  UINT64     Type;
  UINT32     Msr;
  BOOLEAN    Valid;
  BOOLEAN    Used;
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
  MTRR_VARIABLE_SETTING    Mtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
} MTRR_VARIABLE_SETTINGS;

//
// Array for fixed MTRRs
//
typedef  struct  _MTRR_FIXED_SETTINGS_ {
  UINT64    Mtrr[MTRR_NUMBER_OF_FIXED_MTRR];
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
  CacheWriteBack      = 6,
  CacheInvalid        = 7
} MTRR_MEMORY_CACHE_TYPE;

#define  MTRR_CACHE_UNCACHEABLE      0
#define  MTRR_CACHE_WRITE_COMBINING  1
#define  MTRR_CACHE_WRITE_THROUGH    4
#define  MTRR_CACHE_WRITE_PROTECTED  5
#define  MTRR_CACHE_WRITE_BACK       6
#define  MTRR_CACHE_INVALID_TYPE     7

typedef struct {
  UINT64                    BaseAddress;
  UINT64                    Length;
  MTRR_MEMORY_CACHE_TYPE    Type;
} MTRR_MEMORY_RANGE;

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

  @param[in]       BaseAddress       The physical address that is the start
                                     address of a memory region.
  @param[in]       Length            The size in bytes of the memory region.
  @param[in]       Attribute         The bit mask of attributes to set for the
                                     memory region.

  @retval RETURN_SUCCESS            The attributes were set for the memory
                                    region.
  @retval RETURN_INVALID_PARAMETER  Length is zero.
  @retval RETURN_UNSUPPORTED        The processor does not support one or
                                    more bytes of the memory resource range
                                    specified by BaseAddress and Length.
  @retval RETURN_UNSUPPORTED        The bit mask of attributes is not support
                                    for the memory resource range specified
                                    by BaseAddress and Length.
  @retval RETURN_ACCESS_DENIED      The attributes for the memory resource
                                    range specified by BaseAddress and Length
                                    cannot be modified.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to
                                    modify the attributes of the memory
                                    resource range.
                                    Multiple memory range attributes setting by calling this API multiple
                                    times may fail with status RETURN_OUT_OF_RESOURCES. It may not mean
                                    the number of CPU MTRRs are too small to set such memory attributes.
                                    Pass the multiple memory range attributes to one call of
                                    MtrrSetMemoryAttributesInMtrrSettings() may succeed.
  @retval RETURN_BUFFER_TOO_SMALL   The fixed internal scratch buffer is too small for MTRR calculation.
                                    Caller should use MtrrSetMemoryAttributesInMtrrSettings() to specify
                                    external scratch buffer.
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

  @param[in]  Address            The specific address

  @return The memory cache type of the specific address

**/
MTRR_MEMORY_CACHE_TYPE
EFIAPI
MtrrGetMemoryAttribute (
  IN PHYSICAL_ADDRESS  Address
  );

/**
  This function gets the content in fixed MTRRs

  @param[out]  FixedSettings      A buffer to hold fixed MTRRs content.

  @return The pointer of FixedSettings

**/
MTRR_FIXED_SETTINGS *
EFIAPI
MtrrGetFixedMtrr (
  OUT MTRR_FIXED_SETTINGS  *FixedSettings
  );

/**
  This function gets the content in all MTRRs (variable and fixed)

  @param[out]  MtrrSetting   A buffer to hold all MTRRs content.

  @return The pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrGetAllMtrrs (
  OUT MTRR_SETTINGS  *MtrrSetting
  );

/**
  This function sets all MTRRs (variable and fixed)

  @param[in]  MtrrSetting   A buffer to hold all MTRRs content.

  @return The pointer of MtrrSetting

**/
MTRR_SETTINGS *
EFIAPI
MtrrSetAllMtrrs (
  IN MTRR_SETTINGS  *MtrrSetting
  );

/**
  Get the attribute of variable MTRRs.

  This function shadows the content of variable MTRRs into
  an internal array: VariableMtrr

  @param[in]   MtrrValidBitsMask    The mask for the valid bit of the MTRR
  @param[in]   MtrrValidAddressMask The valid address mask for MTRR since the base address in
                                    MTRR must align to 4K, so valid address mask equal to
                                    MtrrValidBitsMask & 0xfffffffffffff000ULL
  @param[out]  VariableMtrr         The array to shadow variable MTRRs content

  @return                       The return value of this parameter indicates the number of
                                MTRRs which has been used.
**/
UINT32
EFIAPI
MtrrGetMemoryAttributeInVariableMtrr (
  IN  UINT64         MtrrValidBitsMask,
  IN  UINT64         MtrrValidAddressMask,
  OUT VARIABLE_MTRR  *VariableMtrr
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

/**
  This function attempts to set the attributes into MTRR setting buffer for a memory range.

  @param[in, out]  MtrrSetting  MTRR setting buffer to be set.
  @param[in]       BaseAddress  The physical address that is the start address
                                of a memory region.
  @param[in]       Length       The size in bytes of the memory region.
  @param[in]       Attribute    The bit mask of attributes to set for the
                                memory region.

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
                                    Multiple memory range attributes setting by calling this API multiple
                                    times may fail with status RETURN_OUT_OF_RESOURCES. It may not mean
                                    the number of CPU MTRRs are too small to set such memory attributes.
                                    Pass the multiple memory range attributes to one call of
                                    MtrrSetMemoryAttributesInMtrrSettings() may succeed.
  @retval RETURN_BUFFER_TOO_SMALL   The fixed internal scratch buffer is too small for MTRR calculation.
                                    Caller should use MtrrSetMemoryAttributesInMtrrSettings() to specify
                                    external scratch buffer.
**/
RETURN_STATUS
EFIAPI
MtrrSetMemoryAttributeInMtrrSettings (
  IN OUT MTRR_SETTINGS       *MtrrSetting,
  IN PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                  Length,
  IN MTRR_MEMORY_CACHE_TYPE  Attribute
  );

/**
  This function attempts to set the attributes into MTRR setting buffer for multiple memory ranges.

  @param[in, out]  MtrrSetting  MTRR setting buffer to be set.
  @param[in]       Scratch      A temporary scratch buffer that is used to perform the calculation.
  @param[in, out]  ScratchSize  Pointer to the size in bytes of the scratch buffer.
                                It may be updated to the actual required size when the calculation
                                needs more scratch buffer.
  @param[in]       Ranges       Pointer to an array of MTRR_MEMORY_RANGE.
                                When range overlap happens, the last one takes higher priority.
                                When the function returns, either all the attributes are set successfully,
                                or none of them is set.
  @param[in]      RangeCount    Count of MTRR_MEMORY_RANGE.

  @retval RETURN_SUCCESS            The attributes were set for all the memory ranges.
  @retval RETURN_INVALID_PARAMETER  Length in any range is zero.
  @retval RETURN_UNSUPPORTED        The processor does not support one or more bytes of the
                                    memory resource range specified by BaseAddress and Length in any range.
  @retval RETURN_UNSUPPORTED        The bit mask of attributes is not support for the memory resource
                                    range specified by BaseAddress and Length in any range.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to modify the attributes of
                                    the memory resource ranges.
  @retval RETURN_ACCESS_DENIED      The attributes for the memory resource range specified by
                                    BaseAddress and Length cannot be modified.
  @retval RETURN_BUFFER_TOO_SMALL   The scratch buffer is too small for MTRR calculation.
**/
RETURN_STATUS
EFIAPI
MtrrSetMemoryAttributesInMtrrSettings (
  IN OUT MTRR_SETTINGS            *MtrrSetting,
  IN     VOID                     *Scratch,
  IN OUT UINTN                    *ScratchSize,
  IN     CONST MTRR_MEMORY_RANGE  *Ranges,
  IN     UINTN                    RangeCount
  );

#endif // _MTRR_LIB_H_
