##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(TPM_ENABLE) == TRUE
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibDTpm/Tpm12DeviceLibDTpm.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
!endif
