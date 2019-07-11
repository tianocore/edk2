/** @file
  Intel FSP API definition from Intel Firmware Support Package External
  Architecture Specification v2.0.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_API_H_
#define _FSP_API_H_

///
/// FSP Reset Status code
/// These are defined in FSP EAS v2.0 section 11.2.2 - OEM Status Code
/// @{
#define FSP_STATUS_RESET_REQUIRED_COLD         0x40000001
#define FSP_STATUS_RESET_REQUIRED_WARM         0x40000002
#define FSP_STATUS_RESET_REQUIRED_3            0x40000003
#define FSP_STATUS_RESET_REQUIRED_4            0x40000004
#define FSP_STATUS_RESET_REQUIRED_5            0x40000005
#define FSP_STATUS_RESET_REQUIRED_6            0x40000006
#define FSP_STATUS_RESET_REQUIRED_7            0x40000007
#define FSP_STATUS_RESET_REQUIRED_8            0x40000008
/// @}

#pragma pack(1)
///
/// FSP_UPD_HEADER Configuration.
///
typedef struct {
  ///
  /// UPD Region Signature. This signature will be
  /// "XXXXXX_T" for FSP-T
  /// "XXXXXX_M" for FSP-M
  /// "XXXXXX_S" for FSP-S
  /// Where XXXXXX is an unique signature
  ///
  UINT64                      Signature;
  ///
  /// Revision of the Data structure. For FSP v2.0 value is 1.
  ///
  UINT8                       Revision;
  UINT8                       Reserved[23];
} FSP_UPD_HEADER;

///
/// FSPM_ARCH_UPD Configuration.
///
typedef struct {
  ///
  /// Revision of the structure. For FSP v2.0 value is 1.
  ///
  UINT8                       Revision;
  UINT8                       Reserved[3];
  ///
  /// Pointer to the non-volatile storage (NVS) data buffer.
  /// If it is NULL it indicates the NVS data is not available.
  ///
  VOID                        *NvsBufferPtr;
  ///
  /// Pointer to the temporary stack base address to be
  /// consumed inside FspMemoryInit() API.
  ///
  VOID                        *StackBase;
  ///
  /// Temporary stack size to be consumed inside
  /// FspMemoryInit() API.
  ///
  UINT32                      StackSize;
  ///
  /// Size of memory to be reserved by FSP below "top
  /// of low usable memory" for bootloader usage.
  ///
  UINT32                      BootLoaderTolumSize;
  ///
  /// Current boot mode.
  ///
  UINT32                      BootMode;
  UINT8                       Reserved1[8];
} FSPM_ARCH_UPD;

///
/// FSPT_UPD_COMMON Configuration.
///
typedef struct {
  ///
  /// FSP_UPD_HEADER Configuration.
  ///
  FSP_UPD_HEADER              FspUpdHeader;
} FSPT_UPD_COMMON;

///
/// FSPM_UPD_COMMON Configuration.
///
typedef struct {
  ///
  /// FSP_UPD_HEADER Configuration.
  ///
  FSP_UPD_HEADER              FspUpdHeader;
  ///
  /// FSPM_ARCH_UPD Configuration.
  ///
  FSPM_ARCH_UPD               FspmArchUpd;
} FSPM_UPD_COMMON;

///
/// FSPS_UPD_COMMON Configuration.
///
typedef struct {
  ///
  /// FSP_UPD_HEADER Configuration.
  ///
  FSP_UPD_HEADER              FspUpdHeader;
} FSPS_UPD_COMMON;

///
/// Enumeration of FSP_INIT_PHASE for NOTIFY_PHASE.
///
typedef enum {
  ///
  /// This stage is notified when the bootloader completes the
  /// PCI enumeration and the resource allocation for the
  /// PCI devices is complete.
  ///
  EnumInitPhaseAfterPciEnumeration = 0x20,
  ///
  /// This stage is notified just before the bootloader hand-off
  /// to the OS loader.
  ///
  EnumInitPhaseReadyToBoot         = 0x40,
  ///
  /// This stage is notified just before the firmware/Preboot
  /// environment transfers management of all system resources
  /// to the OS or next level execution environment.
  ///
  EnumInitPhaseEndOfFirmware       = 0xF0
} FSP_INIT_PHASE;

///
/// Definition of NOTIFY_PHASE_PARAMS.
///
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

  @param[in] FsptUpdDataPtr     Pointer to the FSPT_UPD data structure.

  @retval EFI_SUCCESS           Temporary RAM was initialized successfully.
  @retval EFI_INVALID_PARAMETER Input parameters are invalid.
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
  IN  VOID    *FsptUpdDataPtr
  );

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

  @param[in]  FspmUpdDataPtr          Pointer to the FSPM_UPD data sructure.
  @param[out] HobListPtr              Pointer to receive the address of the HOB list.

  @retval EFI_SUCCESS                 FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
  @retval EFI_OUT_OF_RESOURCES        Stack range requested by FSP is not met.
  @retval FSP_STATUS_RESET_REQUIREDx  A reset is reuired. These status codes will not be returned during S3.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_MEMORY_INIT) (
  IN  VOID    *FspmUpdDataPtr,
  OUT VOID    **HobListPtr
  );


/**
  This FSP API is called after FspMemoryInit API. This FSP API tears down the temporary
  memory setup by TempRamInit API. This FSP API accepts a pointer to a data structure
  that will be platform dependent and defined for each FSP binary. This will be
  documented in Integration Guide.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to
  complete the silicon initialization and provides bootloader an opportunity to get
  control after system memory is available and before the temporary RAM is torn down.

  @param[in] TempRamExitParamPtr Pointer to the Temp Ram Exit parameters structure.
                                 This structure is normally defined in the Integration Guide.
                                 And if it is not defined in the Integration Guide, pass NULL.

  @retval EFI_SUCCESS            FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER  Input parameters are invalid.
  @retval EFI_UNSUPPORTED        The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR       FSP initialization failed.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_TEMP_RAM_EXIT) (
  IN  VOID    *TempRamExitParamPtr
  );


/**
  This FSP API is called after TempRamExit API.
  FspMemoryInit, TempRamExit and FspSiliconInit APIs provide an alternate method to complete the
  silicon initialization.

  @param[in] FspsUpdDataPtr     Pointer to the FSPS_UPD data structure.
                                If NULL, FSP will use the default parameters.

  @retval EFI_SUCCESS                 FSP execution environment was initialized successfully.
  @retval EFI_INVALID_PARAMETER       Input parameters are invalid.
  @retval EFI_UNSUPPORTED             The FSP calling conditions were not met.
  @retval EFI_DEVICE_ERROR            FSP initialization failed.
  @retval FSP_STATUS_RESET_REQUIREDx  A reset is reuired. These status codes will not be returned during S3.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_SILICON_INIT) (
  IN  VOID    *FspsUpdDataPtr
  );

#endif
