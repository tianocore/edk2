##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM2_ENABLE) == TRUE
!if $(TPM1_ENABLE) == TRUE
  Tpm12CommandLib|SecurityPkg/Library/Tpm12CommandLib/Tpm12CommandLib.inf
!endif
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tcg2PhysicalPresenceLib|OvmfPkg/Library/Tcg2PhysicalPresenceLibQemu/DxeTcg2PhysicalPresenceLib.inf
  Tcg2PpVendorLib|SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
!else
  Tcg2PhysicalPresenceLib|OvmfPkg/Library/Tcg2PhysicalPresenceLibNull/DxeTcg2PhysicalPresenceLib.inf
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
!endif
