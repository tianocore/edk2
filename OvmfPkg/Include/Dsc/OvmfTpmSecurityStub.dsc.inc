##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM2_ENABLE) == TRUE
!if $(TPM1_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeTpmMeasureBootLib/DxeTpmMeasureBootLib.inf
!endif
!endif

!if $(TPM2_ENABLE) == TRUE || $(TDX_ENABLE) == TRUE
      #
      # DxeTpm2MeasureBootLib provides security service of TPM2 measure boot and
      # Confidential Computing (CC) measure boot. It should be controlled by
      # TPM2_ENABLE and TDX_ENABLE
      #
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
!endif
