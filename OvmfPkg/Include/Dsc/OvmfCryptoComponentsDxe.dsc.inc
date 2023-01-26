##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# mostly following CryptoPkg/Readme.md recommendations
##

!if $(DXE_USE_CRYPTO_DRIVER) == TRUE

  CryptoPkg/Driver/CryptoDxe.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!if $(NETWORK_TLS_ENABLE) == TRUE
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
!else
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
!endif
    <PcdsFixedAtBuild>
!include CryptoPkg/Include/Dsc/CryptoServicePcd.min_dxe_smm.dsc.inc
  }

!endif
