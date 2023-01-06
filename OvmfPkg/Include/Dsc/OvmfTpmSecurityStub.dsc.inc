##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM2_ENABLE) == TRUE
!if $(TPM1_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeTpmMeasureBootLib/DxeTpmMeasureBootLib.inf
!endif
!endif
      #
      # DxeTpm2MeasureBootLib provides security service of TPM2 measure boot and
      # Confidential Computing (CC) measure boot. It should not be controlled by
      # TPM2_ENABLE.
      #
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
