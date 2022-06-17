##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(USE_CRYPTO_DRIVER) == TRUE
!if $(TPM2_ENABLE) == TRUE

    CryptoPkg/Driver/CryptoPei.inf {
        <LibraryClasses>
          BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
          OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
          TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

        <PcdsFixedAtBuild>
!include CryptoPkg/Driver/CryptoPcdMinPei.dsc.inc
          gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family     | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
          gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family     | PCD_CRYPTO_SERVICE_ENABLE_FAMILY

    }

!endif
!endif
