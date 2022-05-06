##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(USE_CRYPTO_DRIVER) == TRUE

INF CryptoPkg/Driver/CryptoDxe.inf
!if $(SMM_REQUIRE) == TRUE
INF CryptoPkg/Driver/CryptoSmm.inf
!endif

!endif
