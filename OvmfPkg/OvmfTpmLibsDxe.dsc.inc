##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM2_ENABLE) == TRUE
!if $(TPM1_ENABLE) == TRUE
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
!endif
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
!endif
