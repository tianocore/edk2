##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

[LibraryClasses]
!if $(TPM2_ENABLE) == TRUE
!if $(TPM1_ENABLE) == TRUE
  Tpm12CommandLib|SecurityPkg/Library/Tpm12CommandLib/Tpm12CommandLib.inf
!endif
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tpm2DumpLib|SecurityPkg/Library/Tpm2DumpLib/Tpm2DumpLib.inf
  Tcg2PhysicalPresenceLib|OvmfPkg/Library/Tcg2PhysicalPresenceLibQemu/DxeTcg2PhysicalPresenceLib.inf
  Tcg2PpVendorLib|SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
!else
  Tcg2PhysicalPresenceLib|OvmfPkg/Library/Tcg2PhysicalPresenceLibNull/DxeTcg2PhysicalPresenceLib.inf
!endif

!if $(TPM2_ENABLE) == TRUE || $(CC_MEASUREMENT_ENABLE) == TRUE
  #
  # DxeTpmMeasurementLib supports measurement functions for both TPM and Confidential Computing.
  # It should be controlled by TPM2_ENABLE and CC_MEASUREMENT_ENABLE.
  #
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
!else
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
!endif

[LibraryClasses.common.PEIM]
!if $(TPM2_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
!if $(TPM1_ENABLE) == TRUE
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibDTpm/Tpm12DeviceLibDTpm.inf
!endif
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpmSvsm.inf
!endif

!if $(TPM2_ENABLE) == TRUE || $(CC_MEASUREMENT_ENABLE) == TRUE
  #
  # PeiTpmMeasurementLib supports measurement functions for both TPM and Confidential Computing.
  # It should be controlled by TPM2_ENABLE and CC_MEASUREMENT_ENABLE.
  #
  TpmMeasurementLib|SecurityPkg/Library/PeiTpmMeasurementLib/PeiTpmMeasurementLib.inf
!else
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
!endif

[LibraryClasses.common.DXE_DRIVER]
!if $(TPM2_ENABLE) == TRUE
!if $(TPM1_ENABLE) == TRUE
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
!endif
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
!endif
