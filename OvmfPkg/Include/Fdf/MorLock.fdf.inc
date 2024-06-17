##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# MorLock support
##

INF SecurityPkg/Tcg/MemoryOverwriteControl/TcgMor.inf
!if $(SMM_REQUIRE) == TRUE
INF SecurityPkg/Tcg/MemoryOverwriteRequestControlLock/TcgMorLockSmm.inf
!endif
