##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(USE_CRYPTO_DRIVER) == TRUE

    CryptoPkg/Driver/CryptoDxe.inf {
        <LibraryClasses>
          BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
          OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
          TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf

        <PcdsFixedAtBuild>
!include CryptoPkg/Driver/CryptoPcdMinDxeSmm.dsc.inc
!if $(TPM2_ENABLE) == TRUE
          gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family     | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
          gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family     | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
!endif

    }

!endif
