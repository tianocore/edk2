/** @file
  Intel FSP API definition from Intel Firmware Support Package External
  Architecture Specification, April 2014, revision 001.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_API_H_
#define _FSP_API_H_

typedef UINT32 FSP_STATUS;
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
  IN FSP_STATUS Status,
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
  ///
  /// Reserved
  ///
  UINT32              Reserved[7];
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

  @retval FSP_SUCCESS           Temp RAM was initialized successfully.
  @retval FSP_INVALID_PARAMETER Input parameters are invalid..
  @retval FSP_NOT_FOUND         No valid microcode was found in the microcode region.
  @retval FSP_UNSUPPORTED       The FSP calling conditions were not met.
  @retval FSP_DEVICE_ERROR      Temp RAM initialization failed.

  If this function is successful, the FSP initializes the ECX and EDX registers to point to
  a temporary but writeable memory range available to the boot loader and returns with
  FSP_SUCCESS in register EAX. Register ECX points to the start of this temporary
  memory range and EDX points to the end of the range. Boot loader is free to use the
  whole range described. Typically the boot loader can reload the ESP register to point
  to the end of this returned range so that it can be used as a standard stack.
**/
typedef
FSP_STATUS
(FSPAPI *FSP_TEMP_RAM_INIT) (
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

  @retval FSP_SUCCESS           FSP execution environment was initialized successfully.
  @retval FSP_INVALID_PARAMETER Input parameters are invalid.
  @retval FSP_UNSUPPORTED       The FSP calling conditions were not met.
  @retval FSP_DEVICE_ERROR      FSP initialization failed.
**/
typedef
FSP_STATUS
(FSPAPI *FSP_FSP_INIT) (
  IN OUT FSP_INIT_PARAMS *FspInitParamPtr
  );

/**
  This FSP API is used to notify the FSP about the different phases in the boot process.
  This allows the FSP to take appropriate actions as needed during different initialization
  phases. The phases will be platform dependent and will be documented with the FSP
  release. The current FSP supports two notify phases:
    Post PCI enumeration
    Ready To Boot

  @param[in] NotifyPhaseParamPtr Address pointer to the NOTIFY_PHASE_PRAMS

  @retval FSP_SUCCESS           The notification was handled successfully.
  @retval FSP_UNSUPPORTED       The notification was not called in the proper order.
  @retval FSP_INVALID_PARAMETER The notification code is invalid.
**/
typedef
FSP_STATUS
(FSPAPI *FSP_NOTIFY_PHASE) (
  IN NOTIFY_PHASE_PARAMS *NotifyPhaseParamPtr
  );

///
/// FSP API Return Status Code
///
#define FSP_SUCCESS              0x00000000
#define FSP_INVALID_PARAMETER    0x80000002
#define FSP_UNSUPPORTED          0x80000003
#define FSP_NOT_READY            0x80000006
#define FSP_DEVICE_ERROR         0x80000007
#define FSP_OUT_OF_RESOURCES     0x80000009
#define FSP_VOLUME_CORRUPTED     0x8000000A
#define FSP_NOT_FOUND            0x8000000E
#define FSP_TIMEOUT              0x80000012
#define FSP_ABORTED              0x80000015
#define FSP_INCOMPATIBLE_VERSION 0x80000010
#define FSP_SECURITY_VIOLATION   0x8000001A
#define FSP_CRC_ERROR            0x8000001B

#endif
