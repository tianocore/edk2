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
    }

!endif
