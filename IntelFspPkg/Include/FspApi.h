/** @file
  Intel FSP API definition from Intel Firmware Support Package External
  Architecture Specification v1.1, April 2015, revision 001.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_API_H_
#define _FSP_API_H_

#define FSP_STATUS EFI_STATUS
#define FSPAPI EFIAPI

/**
  FSP Init continuation function prototype.
  Control will be returned to this callback function after FspInit API call.

  @param[in] Status Status of the FSP INIT API.
  @param[in] HobBufferPtr Pointer to the HOB data structure defined in the PI specification.
**/
typedef
VOID
(* CONTINUATION_PROC) (
  IN EFI_STATUS Status,
  IN VOID       *HobListPtr
  );

#pragma pack(1)

typedef struct {
  ///
  /// Base address of the microcode region.
  ///
  UINT32              MicrocodeRegionBase;
  ///
  /// Length of the microcode region.
  ///
  UINT32              MicrocodeRegionLength;
  ///
  /// Base address of the cacheable flash region.
  ///
  UINT32              CodeRegionBase;
  ///
  /// Length of the cacheable flash region.
  ///
  UINT32              CodeRegionLength;
} FSP_TEMP_RAM_INIT_PARAMS;

typedef struct {
  ///
  /// Non-volatile storage buffer pointer.
  ///
  VOID               *NvsBufferPtr;
  ///
  /// Runtime buffer pointer
  ///
  VOID               *RtBufferPtr;
  ///
  /// Continuation function address
  ///
  CONTINUATION_PROC   ContinuationFunc;
} FSP_INIT_PARAMS;

typedef struct {
  ///
  /// Stack top pointer used by the bootloader.
  /// The new stack frame will be set up at this location after FspInit API call.
  ///
  UINT32             *StackTop;
  ///
  /// Current system boot mode.
  ///
  UINT32              BootMode;
  ///
  /// User platform configuraiton data region pointer.
  ///
  VOID               *UpdDataRgnPtr;
  //
  // Below field is added in FSP EAS v1.1
  //
  ///
  /// The size of memory to be reserved below the top of low usable memory (TOLUM)
  /// for BootLoader usage. This is optional and value can be zero. If non-zero, the
  /// size must be a multiple of 4KB.
  ///
  UINT32              BootLoaderTolumSize;
  ///
  /// Reserved
  ///
  UINT32              Reserved[6];
} FSP_INIT_RT_COMMON_BUFFER;

typedef enum {
  ///
  /// Notification code for post PCI enuermation
  ///
  EnumInitPhaseAfterPciEnumeration = 0x20,
  ///
  /// Notification code before transfering control to the payload
  ///
  EnumInitPhaseReadyToBoot         = 0x40
} FSP_INIT_PHASE;

typedef struct {
  ///
  /// Notification phase used for NotifyPhase API
  ///
  FSP_INIT_PHASE     Phase;
} NOTIFY_PHASE_PARAMS;

typedef struct {
  ///
  /// Non-volatile storage buffer pointer.
  ///
  VOID               *NvsBufferPtr;
  ///
  /// Runtime buffer pointer
  ///
  VOID               *RtBufferPtr;
  ///
  /// Pointer to the HOB data structure defined in the PI specification
  ///
  VOID               **HobListPtr;
} FSP_MEMORY_INIT_PARAMS;

#pragma pack()

/**
  This FSP API is called soon after coming out of reset and before memory and stack is
  available. This FSP API will load the microcode update, enable code caching for the
  region specified by the boot loader and also setup a temporary stack to be used until
  main memory is initialized.

  A hardcoded stack can be set up with the following values, and the "esp" register
  initialized to point to this hardcoded stack.
  1. The return address where the FSP will return control after setting up a temporary
     stack.
  2. A pointer to the input parameter structure

  However, since the stack is in ROM and not writeable, this FSP API cannot be called
  using the "call" instruction, but needs to be jumped to.

  @param[in] TempRaminitParamPtr Address pointer to the FSP_TEMP_RAM_INIT_PARAMS structure.

  @retval EFI_SUCCESS           Temp RAM was initialized successfully.
  @retval EFI_INVALID_PARAMETER Input parameters are invalid..
  @retval EFI_NOT_FOUND         No valid microcode was found in the microcode region.
  @retval EFI_UNSUPPORTED       The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR      Temp RAM initialization failed.

  If this function is successful, the FSP initializes the ECX and EDX registers to point to
  a temporary but writeable memory range available to the boot loader and returns with
  FSP_SUCCESS in register EAX. Register ECX points to the start of this temporary
  memory range and EDX points to the end of the range. Boot loader is free to use the
  whole range described. Typically the boot loader can reload the ESP register to point
  to the end of this returned range so that it can be used as a standard stack.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_TEMP_RAM_INIT) (
  IN FSP_TEMP_RAM_INIT_PARAMS *FspTempRamInitPtr
  );

/**
  This FSP API is called after TempRamInitEntry. This FSP API initializes the memory,
  the CPU and the chipset to enable normal operation of these devices. This FSP API
  accepts a pointer to a data structure that will be platform dependent and defined for
  each FSP binary. This will be documented in the Integration Guide for each FSP
  release.
  The boot loader provides a continuation function as a parameter when calling FspInit.
  After FspInit completes its execution, it does not return to the boot loader from where
  it was called but instead returns control to the boot loader by calling the continuation
  function which is passed to FspInit as an argument.

  @param[in] FspInitParamPtr Address pointer to the FSP_INIT_PARAMS structure.

  @retval EFI_SUCCESS           FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER Input parameters are invalid.
  @retval EFI_UNSUPPORTED       The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR      FSP initialization failed.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_INIT) (
  IN OUT FSP_INIT_PARAMS *FspInitParamPtr
  );

#define FSP_FSP_INIT FSP_INIT

/**
  This FSP API is used to notify the FSP about the different phases in the boot process.
  This allows the FSP to take appropriate actions as needed during different initialization
  phases. The phases will be platform dependent and will be documented with the FSP
  release. The current FSP supports two notify phases:
    Post PCI enumeration
    Ready To Boot

  @param[in] NotifyPhaseParamPtr Address pointer to the NOTIFY_PHASE_PRAMS

  @retval EFI_SUCCESS           The notification was handled successfully.
  @retval EFI_UNSUPPORTED       The notification was not called in the proper order.
  @retval EFI_INVALID_PARAMETER The notification code is invalid.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_NOTIFY_PHASE) (
  IN NOTIFY_PHASE_PARAMS *NotifyPhaseParamPtr
  );

/**
  This FSP API is called after TempRamInit and initializes the memory.
  This FSP API accepts a pointer to a data structure that will be platform dependent
  and defined for each FSP binary. This will be documented in Integration guide with
  each FSP release.
  After FspMemInit completes its execution, it passes the pointer to the HobList and
  returns to the boot loader from where it was called. BootLoader is responsible to 
  migrate it's stack and data to Memory.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to
  complete the silicon initialization and provides bootloader an opportunity to get
  control after system memory is available and before the temporary RAM is torn down.
  These APIs are mutually exclusive to the FspInit API.

  @param[in][out] FspMemoryInitParamPtr Address pointer to the FSP_MEMORY_INIT_PARAMS
                                        structure.

  @retval EFI_SUCCESS           FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER Input parameters are invalid.
  @retval EFI_UNSUPPORTED       The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR      FSP initialization failed.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_MEMORY_INIT) (
  IN OUT FSP_MEMORY_INIT_PARAMS *FspMemoryInitParamPtr
  );


/**
  This FSP API is called after FspMemoryInit API. This FSP API tears down the temporary
  memory setup by TempRamInit API. This FSP API accepts a pointer to a data structure
  that will be platform dependent and defined for each FSP binary. This will be
  documented in Integration Guide.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to
  complete the silicon initialization and provides bootloader an opportunity to get
  control after system memory is available and before the temporary RAM is torn down.
  These APIs are mutually exclusive to the FspInit API.

  @param[in][out] TempRamExitParamPtr Pointer to the Temp Ram Exit parameters structure.
                                      This structure is normally defined in the Integration Guide.
                                      And if it is not defined in the Integration Guide, pass NULL.

  @retval EFI_SUCCESS           FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER Input parameters are invalid.
  @retval EFI_UNSUPPORTED       The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR      FSP initialization failed.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_TEMP_RAM_EXIT) (
  IN OUT VOID *TempRamExitParamPtr
  );


/**
  This FSP API is called after TempRamExit API.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to complete the
  silicon initialization.
  These APIs are mutually exclusive to the FspInit API.

  @param[in][out] FspSiliconInitParamPtr Pointer to the Silicon Init parameters structure.
                                         This structure is normally defined in the Integration Guide.
                                         And if it is not defined in the Integration Guide, pass NULL.

  @retval EFI_SUCCESS           FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER Input parameters are invalid.
  @retval EFI_UNSUPPORTED       The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR      FSP initialization failed.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_SILICON_INIT) (
  IN OUT VOID *FspSiliconInitParamPtr
  );

///
/// FSP API Return Status Code for backward compatibility with v1.0
///@{
#define FSP_SUCCESS              EFI_SUCCESS
#define FSP_INVALID_PARAMETER    EFI_INVALID_PARAMETER
#define FSP_UNSUPPORTED          EFI_UNSUPPORTED
#define FSP_NOT_READY            EFI_NOT_READY
#define FSP_DEVICE_ERROR         EFI_DEVICE_ERROR
#define FSP_OUT_OF_RESOURCES     EFI_OUT_OF_RESOURCES
#define FSP_VOLUME_CORRUPTED     EFI_VOLUME_CORRUPTED
#define FSP_NOT_FOUND            EFI_NOT_FOUND
#define FSP_TIMEOUT              EFI_TIMEOUT
#define FSP_ABORTED              EFI_ABORTED
#define FSP_INCOMPATIBLE_VERSION EFI_INCOMPATIBLE_VERSION
#define FSP_SECURITY_VIOLATION   EFI_SECURITY_VIOLATION
#define FSP_CRC_ERROR            EFI_CRC_ERROR
///@}

#endif
