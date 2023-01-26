##
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(DXE_USE_CRYPTO_DRIVER) == TRUE
INF CryptoPkg/Driver/CryptoDxe.inf
!endif

!if $(SMM_USE_CRYPTO_DRIVER) == TRUE && $(SMM_REQUIRE) == TRUE
INF CryptoPkg/Driver/CryptoSmm.inf
!endif

