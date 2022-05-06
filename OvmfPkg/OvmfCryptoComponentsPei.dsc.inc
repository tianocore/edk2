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

    }

!endif
!endif
