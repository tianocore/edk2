##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# MorLock support
##

  SecurityPkg/Tcg/MemoryOverwriteControl/TcgMor.inf
!if $(SMM_REQUIRE) == TRUE
  SecurityPkg/Tcg/MemoryOverwriteRequestControlLock/TcgMorLockSmm.inf
!endif
