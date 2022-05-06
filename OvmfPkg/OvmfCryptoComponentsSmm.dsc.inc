##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(USE_CRYPTO_DRIVER) == TRUE

    CryptoPkg/Driver/CryptoSmm.inf {
        <LibraryClasses>
          BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
          OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
          TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

        <PcdsFixedAtBuild>
!include CryptoPkg/Driver/CryptoPcdMinDxeSmm.dsc.inc

    }

!endif
