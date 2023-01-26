##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# mostly following CryptoPkg/Readme.md recommendations
##

!if $(PEI_USE_CRYPTO_DRIVER) == TRUE

  CryptoPkg/Driver/CryptoPei.inf {
    <LibraryClasses>
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
    <PcdsFixedAtBuild>
#!include CryptoPkg/Include/Dsc/CryptoServicePcd.min_pei.dsc.inc
!include CryptoPkg/Include/Dsc/CryptoServicePcd.hash_only.dsc.inc
  }

!endif
