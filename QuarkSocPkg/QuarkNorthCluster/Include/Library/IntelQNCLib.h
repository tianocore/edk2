/** @file
Library that provides QNC specific library services in PEI phase

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __INTEL_QNC_LIB_H__
#define __INTEL_QNC_LIB_H__

/**
  This function initializes the QNC register before MRC.
  It sets RCBA, PMBASE, disable Watchdog timer and initialize QNC GPIO.
  If the function cannot complete it'll ASSERT().
**/
VOID
EFIAPI
PeiQNCPreMemInit (
  VOID
  );


/**
  Used to check SCH if it's S3 state.  Clear the register state after query.

  @retval TRUE if it's S3 state.
  @retval FALSE if it's not S3 state.

**/
BOOLEAN
EFIAPI
QNCCheckS3AndClearState (
   VOID
  );

/**
  Used to check SCH if system wakes up from power on reset. Clear the register state after query.

  @retval TRUE  if system wakes up from power on reset
  @retval FALSE if system does not wake up from power on reset

**/
BOOLEAN
EFIAPI
QNCCheckPowerOnResetAndClearState (
   VOID
  );

/**
  This function is used to clear SMI and wake status.

**/
VOID
EFIAPI
QNCClearSmiAndWake (
  VOID
  );

/**
  Used to initialize the QNC register after MRC.

**/
VOID
EFIAPI
PeiQNCPostMemInit (
  VOID
  );

/** Send DRAM Ready opcode.

  @param[in]       OpcodeParam  Parameter to DRAM ready opcode.

  @retval          VOID
**/
VOID
EFIAPI
QNCSendOpcodeDramReady (
  IN UINT32   OpcodeParam
  );

/**

  Relocate RMU Main binary to memory after MRC to improve performance.

  @param[in]  DestBaseAddress  - Specify the new memory address for the RMU Main binary.
  @param[in]  SrcBaseAddress   - Specify the current memory address for the RMU Main binary.
  @param[in]  Size             - Specify size of the RMU Main binary.

  @retval     VOID

**/
VOID
EFIAPI
RmuMainRelocation (
  IN CONST UINT32   DestBaseAddress,
  IN CONST UINT32   SrcBaseAddress,
  IN CONST UINTN    Size
  );

/**
  Get the total memory size

**/
UINT32
EFIAPI
QNCGetTotalMemorysize (
  VOID
  );

/**
  Get the memory range of TSEG.
  The TSEG's memory is below TOLM.

  @param[out] BaseAddress The base address of TSEG's memory range
  @param[out] MemorySize  The size of TSEG's memory range

**/
VOID
EFIAPI
QNCGetTSEGMemoryRange (
  OUT UINT64  *BaseAddress,
  OUT UINT64  *MemorySize
  );

/**
  Updates the PAM registers in the MCH for the requested range and mode.

  @param   Start        The start address of the memory region
  @param   Length       The length, in bytes, of the memory region
  @param   ReadEnable   Pointer to the boolean variable on whether to enable read for legacy memory section.
                        If NULL, then read attribute will not be touched by this call.
  @param   ReadEnable   Pointer to the boolean variable on whether to enable write for legacy memory section.
                        If NULL, then write attribute will not be touched by this call.
  @param   Granularity  A pointer to granularity, in bytes, that the PAM registers support

  @retval  RETURN_SUCCESS            The PAM registers in the MCH were updated
  @retval  RETURN_INVALID_PARAMETER  The memory range is not valid in legacy region.

**/
RETURN_STATUS
EFIAPI
QNCLegacyRegionManipulation (
  IN  UINT32                  Start,
  IN  UINT32                  Length,
  IN  BOOLEAN                 *ReadEnable,
  IN  BOOLEAN                 *WriteEnable,
  OUT UINT32                  *Granularity
  );

/**
  Do early init of pci express rootports on Soc.

**/
VOID
EFIAPI
PciExpressEarlyInit (
  VOID
  );

/**
  Complete initialization of all the pci express rootports on Soc.
**/
EFI_STATUS
EFIAPI
PciExpressInit (
  );

/**
  Determine if QNC is supported.

  @retval FALSE  QNC is not supported.
  @retval TRUE   QNC is supported.
**/
BOOLEAN
EFIAPI
IsQncSupported (
  VOID
  );

/**
  Get the DeviceId of the SoC

  @retval PCI DeviceId of the SoC
**/
UINT16
EFIAPI
QncGetSocDeviceId (
  VOID
  );

/**
  Enable SMI detection of legacy flash access violations.
**/
VOID
EFIAPI
QncEnableLegacyFlashAccessViolationSmi (
  VOID
  );

/**
  Setup RMU Thermal sensor registers for Vref mode.
**/
VOID
EFIAPI
QNCThermalSensorSetVRefMode (
  VOID
  );

/**
  Setup RMU Thermal sensor registers for Ratiometric mode.
**/
VOID
EFIAPI
QNCThermalSensorSetRatiometricMode (
  VOID
  );

/**
  Setup RMU Thermal sensor trip point values.

  @param[in]  CatastrophicTripOnDegreesCelsius  - Catastrophic set trip point threshold.
  @param[in]  HotTripOnDegreesCelsius           - Hot set trip point threshold.
  @param[in]  HotTripOffDegreesCelsius          - Hot clear trip point threshold.

  @retval     VOID
**/
EFI_STATUS
EFIAPI
QNCThermalSensorSetTripValues (
  IN  CONST UINTN             CatastrophicTripOnDegreesCelsius,
  IN  CONST UINTN             HotTripOnDegreesCelsius,
  IN  CONST UINTN             HotTripOffDegreesCelsius
  );

/**
  Enable RMU Thermal sensor with a Catastrophic Trip point.

  @retval  EFI_SUCCESS            Trip points setup.
  @retval  EFI_INVALID_PARAMETER  Invalid trip point value.

**/
EFI_STATUS
EFIAPI
QNCThermalSensorEnableWithCatastrophicTrip (
  IN  CONST UINTN             CatastrophicTripOnDegreesCelsius
  );

/**
  Lock all RMU Thermal sensor control & trip point registers.

**/
VOID
EFIAPI
QNCThermalSensorLockAllRegisters (
  VOID
  );

/**
  Set chipset policy for double bit ECC error.

  @param[in]       PolicyValue  Policy to config on double bit ECC error.

**/
VOID
EFIAPI
QNCPolicyDblEccBitErr (
  IN  CONST UINT32                        PolicyValue
  );

/**
  Determine if running on secure Quark hardware Sku.

  @retval FALSE  Base Quark Sku or unprovisioned Secure Sku running.
  @retval TRUE   Provisioned SecureSku hardware running.
**/
BOOLEAN
EFIAPI
QncIsSecureProvisionedSku (
  VOID
  );
#endif

