##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeTpmMeasureBootLib/DxeTpmMeasureBootLib.inf
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
!endif
