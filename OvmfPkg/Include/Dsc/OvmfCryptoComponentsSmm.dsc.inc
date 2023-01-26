##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# mostly following CryptoPkg/Readme.md recommendations
##

!if $(SMM_USE_CRYPTO_DRIVER) == TRUE && $(SMM_REQUIRE) == TRUE

  CryptoPkg/Driver/CryptoSmm.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
    <PcdsFixedAtBuild>
!include CryptoPkg/Include/Dsc/CryptoServicePcd.min_dxe_smm.dsc.inc
  }

!endif
